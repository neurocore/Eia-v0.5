#pragma once

template <typename T> constexpr int sgn(T val)
{
  return (T(0) < val) - (val < T(0));
}
