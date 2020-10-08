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

#include <iostream>
#include <utility>

#include "meta/meta.h"

class Obj : public meta::MetaObject {
  DECLARE_META_OBJECT(Obj);

public:
  explicit Obj(std::string name) : m_name(std::move(name)), m_count(0) {}

  const std::string& getName() const {
    return m_name;
  }

  void setCount(int value) {
    m_count = value;
  }
  int getCount() const {
    return m_count;
  }

private:
  std::string m_name;
  int m_count;
};

DEFINE_META_OBJECT(Obj)
    .addProperty<Obj, std::string>("name", "name description", meta::PropertyEditorType::String,
                                   &Obj::getName)
    .addProperty<Obj, int>("count", "count description", meta::PropertyEditorType::Integer,
                           &Obj::getCount, &Obj::setCount);

class AnotherObj : public Obj {
  DECLARE_META_OBJECT(AnotherObj);

public:
  explicit AnotherObj(const std::string& name) : Obj(name), m_visible(false) {}

  ~AnotherObj() override = default;

  bool isVisible() const {
    return m_visible;
  }
  void setVisible(bool visible) {
    m_visible = visible;
  }

private:
  bool m_visible;
};

DEFINE_META_OBJECT(AnotherObj)
    .addBase(Obj::GetStaticMetaBuilder())
    .addProperty<AnotherObj, bool>("visible", "visible description",
                                   meta::PropertyEditorType::String, &AnotherObj::isVisible,
                                   &AnotherObj::setVisible);

int main() {
  Obj obj("obj1");

  std::string testValue;

  assert(obj.get("name", &testValue));
  assert(std::string("obj1") == testValue);

  assert(!obj.set("name", "new name"));

  assert(obj.set("count", "50"));
  assert(50 == obj.getCount());

  assert(obj.get("count", &testValue));
  assert(std::string("50") == testValue);

  AnotherObj anotherObj("anotherObj1");

  assert(anotherObj.get("name", &testValue));
  assert(std::string("anotherObj1") == testValue);

  assert(!anotherObj.set("name", "new another obj name"));

  assert(anotherObj.set("visible", "false"));
  assert(!anotherObj.isVisible());

  assert(anotherObj.get("visible", &testValue));
  assert(std::string("false") == testValue);

  return 0;
}
