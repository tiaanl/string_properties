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

#include <utility>

#include "meta/meta.h"

class Obj : public meta::MetaObject {
  DECLARE_META_OBJECT(Obj);

public:
  explicit Obj(std::string name) : _name(std::move(name)), _count(0) {}

  const std::string &GetName() const { return _name; }

  void SetCount(int value) { _count = value; }
  int GetCount() const { return _count; }

private:
  std::string _name;
  int _count;
};

DEFINE_META_OBJECT(Obj)
    .AddProperty<Obj, std::string>("name", "name description",
                                   meta::PROPERTY_EDITOR_STRING, &Obj::GetName)
    .AddProperty<Obj, int>("count", "count description",
                           meta::PROPERTY_EDITOR_INTEGER, &Obj::GetCount,
                           &Obj::SetCount);

class AnotherObj : public Obj {
  DECLARE_META_OBJECT(AnotherObj);

public:
  explicit AnotherObj(const std::string &name) : Obj(name), _visible(false) {}

  ~AnotherObj() override = default;

  bool IsVisible() const { return _visible; }
  void SetVisible(bool visible) { _visible = visible; }

private:
  bool _visible;
};

DEFINE_META_OBJECT(AnotherObj)
    .AddBase(Obj::GetStaticMetaBuilder())
    .AddProperty<AnotherObj, bool>("visible", "visible description",
                                   meta::PROPERTY_EDITOR_STRING,
                                   &AnotherObj::IsVisible,
                                   &AnotherObj::SetVisible);

int main() {
  Obj obj("obj1");

  std::string testValue;

  assert(obj.Get("name", &testValue));
  assert(std::string("obj1") == testValue);

  assert(!obj.Set("name", "new name"));

  assert(obj.Set("count", "50"));
  assert(50 == obj.GetCount());

  assert(obj.Get("count", &testValue));
  assert(std::string("50") == testValue);

  AnotherObj anotherObj("anotherObj1");

  assert(anotherObj.Get("name", &testValue));
  assert(std::string("anotherObj1") == testValue);

  assert(!anotherObj.Set("name", "new another obj name"));

  assert(anotherObj.Set("visible", "false"));
  assert(!anotherObj.IsVisible());

  assert(anotherObj.Get("visible", &testValue));
  assert(std::string("false") == testValue);

  return 0;
}
