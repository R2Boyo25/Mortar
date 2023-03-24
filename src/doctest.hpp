// doctest.hpp - 2023
//
// proxy file for doctest.

#pragma once
#include <doctest/doctest.h>
#ifndef DOCTEST_CONFIG_DISABLE
#include <sstream>
#include <vector>

namespace doctest {
template <typename T> struct StringMaker<std::vector<std::vector<T>>> {
  static String convert(const std::vector<std::vector<T>> &in) {
    std::ostringstream oss;

    oss << "{";
    for (auto &it : in) {
      oss << "{";
      for (auto &it2 : it)
        oss << it2 << ", ";
      oss << "}";
    }
    oss << "}";

    return oss.str().c_str();
  }
};

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
