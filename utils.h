#pragma once
#include <string_view>
#include <string>
#include <charconv>
#include <algorithm>
#include "types.h"

namespace eia {

template<typename T> INLINE int sgn(T val)
{
  return (T(0) < val) - (val < T(0));
}

INLINE ptrdiff_t index_of(std::string_view str, char ch)
{
  auto it = std::find(str.begin(), str.end(), ch);
  return it != str.end() ? it - str.begin() : -1;
}

INLINE std::string cut(std::string & str, char delim = ' ')
{
  auto pos = str.find(delim);
  std::string part = str.substr(0, pos);
  str = str.substr(pos + 1, str.length());
  return part;
}

inline int parse_int(const std::string_view str)
{
  int result = 0;
  std::from_chars(str.data(), str.data() + str.size(), result);
  return result;
}

}
