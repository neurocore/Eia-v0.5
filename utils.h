#pragma once
#include <string_view>
#include <algorithm>
#include "types.h"

template<typename T> INLINE int sgn(T val)
{
  return (T(0) < val) - (val < T(0));
}

INLINE ptrdiff_t index_of(std::string_view str, char ch)
{
  auto it = std::find(str.begin(), str.end(), ch);
  return it != str.end() ? it - str.begin() : -1;
}
