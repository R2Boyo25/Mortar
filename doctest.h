#pragma once
// proxy file for doctest.
#include <doctest/doctest.h>
#ifndef DOCTEST_CONFIG_DISABLE
#include <sstream>
#include <vector>

namespace doctest {
template <typename T> struct StringMaker<std::vector<T>> {
  static String convert(const std::vector<T> &in) {
    std::ostringstream oss;

    oss << "{";
    for (typename std::vector<T>::const_iterator it = in.begin();
         it != in.end(); ++it)
      oss << *it << ", ";
    oss << "}";

    return oss.str().c_str();
  }
};
} // namespace doctest
#endif