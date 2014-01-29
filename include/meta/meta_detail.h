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

#ifndef META_DETAIL_H_
#define META_DETAIL_H_

#include <sstream>

namespace meta {

namespace detail {

// MetaConverter<>

template <typename T>
struct MetaConverter {
  static bool ToString(const T& inValue, std::string* outValue) {
    assert(outValue);
    std::stringstream iss;
    iss << inValue;
    *outValue = iss.str();
    return true;
  }

  static bool FromString(const std::string& inValue, T* outValue) {
    assert(outValue);
    std::stringstream oss(inValue);
    oss >> *outValue;
    return true;
  }
};

// We give bool it's own MetaConverter so we can convert "true" and "false" into
// boolean values.
template <>
struct MetaConverter<bool> {
  static bool ToString(bool inValue, std::string* outValue) {
    *outValue = inValue ? "true" : "false";
    return true;
  }

  static bool FromString(const std::string& inValue, bool* outValue) {
    *outValue = (inValue == "true" || inValue == "1");
    return true;
  }
};

// MetaPropertyTraits<>

template <typename C, typename T>
struct MetaPropertyTraits {
  typedef const T& (C::*GetterType)() const;
  typedef void (C::*SetterType)(const T&);
};

template <typename C>
struct MetaPropertyTraits<C, bool> {
  typedef bool (C::*GetterType)() const;
  typedef void (C::*SetterType)(bool);
};

template <typename C>
struct MetaPropertyTraits<C, int> {
  typedef int (C::*GetterType)() const;
  typedef void (C::*SetterType)(int);
};

template <typename C>
struct MetaPropertyTraits<C, double> {
  typedef double (C::*GetterType)() const;
  typedef void (C::*SetterType)(double);
};

}  // namespace detail

}  // namespace meta

#endif  // META_DETAIL_H_
