// Copyright (c) 2014 Tiaan Louw
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
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "meta/meta_detail.h"

namespace meta {

class MetaBuilder;

class MetaObject {
public:
  virtual ~MetaObject() {}

  virtual bool Get(const std::string& name, std::string* outValue) = 0;
  virtual bool Set(const std::string& name, const std::string& value) = 0;
  virtual const MetaBuilder* GetMetaBuilder() const = 0;
};

struct PropertyBase {
  // Generic pointer type we use to store function pointers that can be cast
  // back to their original type before use.
  typedef void (*FuncStorage)();

  virtual ~PropertyBase() {}

  virtual bool Get(MetaObject* obj, std::string* outValue) { return false; }
  virtual bool Set(MetaObject* obj, const std::string& value) { return false; }
  virtual bool IsReadOnly() const { return false; }

  FuncStorage invokerGet;
  FuncStorage invokerSet;
};

// Interface to incoke get and set functions of a property on a given object.
template <typename PropertyType>
struct Invoker {
  typedef typename PropertyType::ClassType ClassType;
  typedef typename PropertyType::Type Type;

  typedef bool (*SetType)(PropertyBase*, ClassType*, const std::string&);
  typedef bool (*GetType)(PropertyBase*, ClassType*, std::string*);

  static bool Get(PropertyBase* p, ClassType* obj, std::string* outValue) {
    PropertyType* prop = static_cast<PropertyType*>(p);

    // No need to check if a getter is set to nullptr, because our system
    // doesn't allow a nullptr getter.

    Type x((obj->*(prop->getter))());
    return detail::MetaConverter<Type>::ToString(x, outValue);
  }

  static bool Set(PropertyBase* p, ClassType* obj, const std::string& value) {
    PropertyType* prop = static_cast<PropertyType*>(p);
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
  typedef C ClassType;
  typedef T Type;

  typedef typename detail::MetaPropertyTraits<C, T>::GetterType GetterType;
  typedef typename detail::MetaPropertyTraits<C, T>::SetterType SetterType;

  typedef Invoker<Property<ClassType, Type> > InvokerType;
  typedef typename InvokerType::GetType InvokerGetType;
  typedef typename InvokerType::SetType InvokerSetType;

  Property(GetterType getter, SetterType setter)
    : getter(getter), setter(setter) {
    invokerGet = reinterpret_cast<FuncStorage>(&InvokerType::Get);
    invokerSet = reinterpret_cast<FuncStorage>(&InvokerType::Set);
  }

  virtual ~Property() {}

  virtual bool Get(MetaObject* obj, std::string* outValue) override {
    InvokerGetType getter = reinterpret_cast<InvokerGetType>(invokerGet);
    return getter(this, static_cast<ClassType*>(obj), outValue);
  }

  virtual bool Set(MetaObject* obj, const std::string& value) override {
    InvokerSetType setter = reinterpret_cast<InvokerSetType>(invokerSet);
    return setter(this, static_cast<ClassType*>(obj), value);
  }

  virtual bool IsReadOnly() const { return !setter; }

  GetterType getter;
  SetterType setter;
};

template <typename C, typename T>
Property<C, T> MakeProperty(const T& (C::*getter)() const,
                            void (C::*setter)(const T&)) {
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

  MetaEntry(const std::string& name, const std::string& description,
            PropertyEditorType editorType,
            const std::shared_ptr<PropertyBase>& prop)
    : name(name), description(description), editorType(editorType), prop(prop) {
  }
};

// Utility class to build properties for a specified class.
class MetaBuilder {
public:
  MetaBuilder& AddBase(const MetaBuilder* metaBuilder) {
    _bases.push_back(metaBuilder);
    return *this;
  }

  template <typename C, typename T>
  MetaBuilder& AddProperty(
      const std::string& name, const std::string& description,
      PropertyEditorType editorType,
      typename detail::MetaPropertyTraits<C, T>::GetterType getter) {
    _properties.insert(PropertiesType::value_type(
        name, MetaEntry(name, description, editorType,
                        std::make_shared<Property<C, T> >(getter, nullptr))));
    return *this;
  }

  template <typename C, typename T>
  MetaBuilder& AddProperty(
      const std::string& name, const std::string& description,
      PropertyEditorType editorType,
      typename detail::MetaPropertyTraits<C, T>::GetterType getter,
      typename detail::MetaPropertyTraits<C, T>::SetterType setter) {
    _properties.insert(PropertiesType::value_type(
        name, MetaEntry(name, description, editorType,
                        std::make_shared<Property<C, T> >(getter, setter))));
    return *this;
  }

  const MetaEntry* GetProperty(const std::string& name) const {
    PropertiesType::const_iterator it = _properties.find(name);
    if (it != _properties.end())
      return &it->second;

    // The property wasn't found in this class, so let's check the base classes.
    for (BasesType::const_iterator it(_bases.begin()), eit(_bases.end());
         it != eit; ++it) {
      const MetaEntry* entry = (*it)->GetProperty(name);
      if (entry)
        return entry;
    }

    return nullptr;
  }

  void GetListOfProperties(std::set<std::string>* outNames) const {
    assert(outNames);
    for (const auto& entry : _properties)
      outNames->insert(entry.first);

    for (const auto& base : _bases)
      base->GetListOfProperties(outNames);
  }

private:
  typedef std::map<std::string, MetaEntry> PropertiesType;
  PropertiesType _properties;

  typedef std::vector<const MetaBuilder*> BasesType;
  BasesType _bases;
};

}  // namespace meta

#define DECLARE_META_OBJECT(ClassName)                                         \
  \
private:                                                                       \
  static const meta::MetaBuilder _##ClassName##_properties;                    \
  \
public:                                                                        \
  static const meta::MetaBuilder* GetStaticMetaBuilder() {                     \
    return &_##ClassName##_properties;                                         \
  }                                                                            \
  virtual bool Get(const std::string&, std::string*) override;                 \
  virtual bool Set(const std::string&, const std::string&) override;           \
  virtual const meta::MetaBuilder* GetMetaBuilder() const

#define DEFINE_META_OBJECT(ClassName)                                          \
  bool ClassName::Get(const std::string& name, std::string* outValue) {        \
    const meta::MetaEntry* entry =                                             \
        _##ClassName##_properties.GetProperty(name);                           \
    if (!entry)                                                                \
      return false;                                                            \
    return entry->prop->Get(this, outValue);                                   \
  }                                                                            \
  bool ClassName::Set(const std::string& name, const std::string& value) {     \
    const meta::MetaEntry* entry =                                             \
        _##ClassName##_properties.GetProperty(name);                           \
    if (!entry)                                                                \
      return false;                                                            \
    return entry->prop->Set(this, value);                                      \
  \
}                                                                         \
  const meta::MetaBuilder* ClassName::GetMetaBuilder() const {                 \
    return &_##ClassName##_properties;                                         \
  }                                                                            \
  const meta::MetaBuilder ClassName::_##ClassName##_properties =               \
      meta::MetaBuilder()

#endif  // META_H_
