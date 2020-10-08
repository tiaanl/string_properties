// Copyright (c) 2020 Tiaan Louw
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef META_H_
#define META_H_

#include <cassert>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "meta/meta_detail.h"

namespace meta {

class MetaBuilder;

class MetaObject {
public:
  virtual ~MetaObject();

  virtual bool Get(const std::string &name, std::string *outValue) = 0;
  virtual bool Set(const std::string &name, const std::string &value) = 0;
  virtual const MetaBuilder *GetMetaBuilder() const = 0;
};

struct PropertyBase {
  // Generic pointer type we use to store function pointers that can be cast
  // back to their original type before use.
  using FuncStorage = void (*)();

  virtual ~PropertyBase();

  virtual bool Get(MetaObject *, std::string *) {
    return false;
  }

  virtual bool Set(MetaObject *, const std::string &) {
    return false;
  }

  virtual bool IsReadOnly() const {
    return false;
  }

  FuncStorage invokerGet;
  FuncStorage invokerSet;
};

// Interface to invoke the get and set functions of a property for a given
// object.
template <typename PropertyType> struct Invoker {
  using ClassType = typename PropertyType::ClassType;
  using Type = typename PropertyType::Type;

  using SetType = bool (*)(PropertyBase *, ClassType *, const std::string &);
  using GetType = bool (*)(PropertyBase *, ClassType *, std::string *);

  static bool Get(PropertyBase *p, ClassType *obj, std::string *outValue) {
    auto *prop = static_cast<PropertyType *>(p);

    // No need to check if a getter is set to nullptr, because our system
    // doesn't allow a nullptr getter.

    Type x((obj->*(prop->getter))());
    return detail::MetaConverter<Type>::ToString(x, outValue);
  }

  static bool Set(PropertyBase *p, ClassType *obj, const std::string &value) {
    auto *prop = static_cast<PropertyType *>(p);
    if (!prop->setter)
      return false;

    Type x;
    if (detail::MetaConverter<Type>::FromString(value, &x)) {
      (obj->*(prop->setter))(x);
      return true;
    }

    return false;
  }
};

// A property typed on it's class type and property type.
template <typename C, typename T>
// C is the type of the class we are a property of.
// T is the type of property we represent.
struct Property : public PropertyBase {
  using ClassType = C;
  using Type = T;

  using GetterType = typename detail::MetaPropertyTraits<C, T>::GetterType;
  using SetterType = typename detail::MetaPropertyTraits<C, T>::SetterType;

  using InvokerType = Invoker<Property<ClassType, Type>>;
  using InvokerGetType = typename InvokerType::GetType;
  using InvokerSetType = typename InvokerType::SetType;

  Property(GetterType getter, SetterType setter) : getter(getter), setter(setter) {
    invokerGet = reinterpret_cast<FuncStorage>(&InvokerType::Get);
    invokerSet = reinterpret_cast<FuncStorage>(&InvokerType::Set);
  }

  ~Property() override = default;

  bool Get(MetaObject *obj, std::string *outValue) override {
    auto func = reinterpret_cast<InvokerGetType>(invokerGet);
    return func(this, static_cast<ClassType *>(obj), outValue);
  }

  bool Set(MetaObject *obj, const std::string &value) override {
    auto func = reinterpret_cast<InvokerSetType>(invokerSet);
    return func(this, static_cast<ClassType *>(obj), value);
  }

  bool IsReadOnly() const override {
    return !setter;
  }

  GetterType getter;
  SetterType setter;
};

template <typename C, typename T>
Property<C, T> MakeProperty(const T &(C::*getter)() const, void (C::*setter)(const T &)) {
  return Property<C, T>(getter, setter);
}

enum PropertyEditorType {
  PROPERTY_EDITOR_STRING,
  PROPERTY_EDITOR_INTEGER,
  PROPERTY_EDITOR_BOOL,
};

struct MetaEntry {
public:
  std::string name;
  std::string description;
  PropertyEditorType editorType;
  std::shared_ptr<PropertyBase> prop;

  MetaEntry(std::string name, std::string description, PropertyEditorType editorType,
            std::shared_ptr<PropertyBase> prop)
      : name(std::move(name)), description(std::move(description)), editorType(editorType),
        prop(std::move(prop)) {}
};

// Utility class to build properties for a specified class.
class MetaBuilder {
public:
  MetaBuilder();

  MetaBuilder &AddBase(const MetaBuilder *metaBuilder) {
    _bases.push_back(metaBuilder);
    return *this;
  }

  template <typename C, typename T>
  MetaBuilder &AddProperty(const std::string &name, const std::string &description,
                           PropertyEditorType editorType,
                           typename detail::MetaPropertyTraits<C, T>::GetterType getter) {
    _properties.insert(PropertiesType::value_type(
        name, MetaEntry(name, description, editorType,
                        std::make_shared<Property<C, T>>(getter, nullptr))));
    return *this;
  }

  template <typename C, typename T>
  MetaBuilder &AddProperty(const std::string &name, const std::string &description,
                           PropertyEditorType editorType,
                           typename detail::MetaPropertyTraits<C, T>::GetterType getter,
                           typename detail::MetaPropertyTraits<C, T>::SetterType setter) {
    _properties.insert(PropertiesType::value_type(
        name, MetaEntry(name, description, editorType,
                        std::make_shared<Property<C, T>>(getter, setter))));
    return *this;
  }

  const MetaEntry *GetProperty(const std::string &name) const {
    auto it = _properties.find(name);
    if (it != _properties.end())
      return &it->second;

    // The property wasn't found in this class, so let's check the base classes.
    for (auto _base : _bases) {
      const MetaEntry *entry = _base->GetProperty(name);
      if (entry)
        return entry;
    }

    return nullptr;
  }

  void GetListOfProperties(std::set<std::string> *outNames) const {
    assert(outNames);
    for (const auto &entry : _properties)
      outNames->insert(entry.first);

    for (const auto &base : _bases)
      base->GetListOfProperties(outNames);
  }

private:
  using PropertiesType = std::unordered_map<std::string, MetaEntry>;
  using BasesType = std::vector<const MetaBuilder *>;

  PropertiesType _properties;

  BasesType _bases;
};

} // namespace meta

#define DECLARE_META_OBJECT(ClassName)                                                             \
private:                                                                                           \
  static const meta::MetaBuilder _##ClassName##_properties;                                        \
                                                                                                   \
public:                                                                                            \
  static const meta::MetaBuilder *GetStaticMetaBuilder() {                                         \
    return &_##ClassName##_properties;                                                             \
  }                                                                                                \
  bool Get(const std::string &, std::string *) override;                                           \
  bool Set(const std::string &, const std::string &) override;                                     \
  const meta::MetaBuilder *GetMetaBuilder() const override

#define DEFINE_META_OBJECT(ClassName)                                                              \
  bool ClassName::Get(const std::string &name, std::string *outValue) {                            \
    const meta::MetaEntry *entry = _##ClassName##_properties.GetProperty(name);                    \
    if (!entry)                                                                                    \
      return false;                                                                                \
    return entry->prop->Get(this, outValue);                                                       \
  }                                                                                                \
  bool ClassName::Set(const std::string &name, const std::string &value) {                         \
    const meta::MetaEntry *entry = _##ClassName##_properties.GetProperty(name);                    \
    if (!entry)                                                                                    \
      return false;                                                                                \
    return entry->prop->Set(this, value);                                                          \
  }                                                                                                \
  const meta::MetaBuilder *ClassName::GetMetaBuilder() const {                                     \
    return &_##ClassName##_properties;                                                             \
  }                                                                                                \
  const meta::MetaBuilder ClassName::_##ClassName##_properties = meta::MetaBuilder()

#endif // META_H_
