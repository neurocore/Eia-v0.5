#pragma once
#include <cassert>
#include <format>
#include "utils.h"

namespace eia {

  enum Val : int
  {
    Draw  = 0,
    Grain = 1,
    CP    = 10000,
    Inf   = 32767 * CP,
    Mate  = 32000 * CP,
  };

  static INLINE Val operator ""_cp(u64 val) { return static_cast<Val>(val * CP); }
  static INLINE Val operator ""_cp(long double val) { return static_cast<Val>(+CP * static_cast<double>(val)); }
  static INLINE Val cp(int v)    { return static_cast<Val>(+CP * v); }
  static INLINE Val cp(double v) { return static_cast<Val>(+CP * v); }
  static INLINE int dry(Val val) { return static_cast<int>(val) / CP; }
  static INLINE float  dry_float(Val val)  { return static_cast<float>(val) / +CP; }
  static INLINE double dry_double(Val val) { return static_cast<double>(val) / +CP; }
  static INLINE Val to_val(int v){ return static_cast<Val>(v); }
  static INLINE Val undraw(Val v){ return v == Draw ? Grain : v; }

  static INLINE Val operator + (Val a, Val b) { return static_cast<Val>(+a + +b); }
  static INLINE Val operator - (Val a, Val b) { return static_cast<Val>(+a - +b); }
  static INLINE int operator / (Val a, Val b) { return static_cast<int>(+a / +b); }
  static INLINE Val operator * (Val a, int k) { return static_cast<Val>(+a * k); }
  static INLINE Val operator / (Val a, int k) { return static_cast<Val>(+a / k); }
  static INLINE Val operator + (Val a, int k) { return static_cast<Val>(+a + k); }
  static INLINE Val operator - (Val a, int k) { return static_cast<Val>(+a - k); }
  static INLINE Val operator * (int k, Val a) { return static_cast<Val>(+a * k); }
  static INLINE Val operator + (int k, Val a) { return static_cast<Val>(+a + k); }
  static INLINE Val operator - (int k, Val a) { return static_cast<Val>(k - +a); }
  static INLINE Val operator * (Val a, double k) { return static_cast<Val>(+a * k); }
  static INLINE Val operator / (Val a, double k) { return static_cast<Val>(+a / k); }
  static INLINE Val operator * (double k, Val a) { return static_cast<Val>(+a * k); }
  
  static INLINE Val operator * (Val a, Val b)
  {
    assert(false);     // usually we don't like that, but trying
    return a * dry(b); //  to resolve it in most gracious way
  }

  static INLINE int operator + (Val a) { return static_cast<int>(a); }
  static INLINE Val operator - (Val a) { return static_cast<Val>(-(+a)); }

  static INLINE Val & operator += (Val & a, Val b)
  {
    a = a + b;
    return a;
  }

  static INLINE Val & operator -= (Val & a, Val b)
  {
    a = a - b;
    return a;
  }

  static INLINE Val & operator *= (Val & a, int k)
  {
    a = a * k;
    return a;
  }

  static INLINE Val & operator /= (Val & a, int k)
  {
    a = a * k;
    return a;
  }

  static INLINE Val & operator *= (Val & a, double k)
  {
    a = a * k;
    return a;
  }

  static INLINE Val & operator /= (Val & a, double k)
  {
    a = a * k;
    return a;
  }
}

template<>
struct std::formatter<eia::Val> : std::formatter<std::string>
{
  bool output = false;

  constexpr auto parse(std::format_parse_context& ctx)
  {
    auto it = ctx.begin();
    if (it != ctx.end() && *it == 'o')
    {
      output = true;
      ++it;
    }
    return it;
  }

  auto format(const eia::Val & val, std::format_context & ctx) const
  {
    using namespace eia;
    int v = dry(val);
    int absv = eia::abs(v);
    int sign = eia::sgn(v);
    std::string str;

    if (output)
    {
      if (absv < dry(Val::Mate))
      {
        str += std::format("cp {}", v);
      }
      else
      {
        const int ply = (dry(Val::Inf) - absv) / 2 + 1;
        str += "mate " + to_string(sign * ply);
      }
    }
    else
    {
      if (absv < dry(Val::Mate))
      {
        str += std::format("{:+.2f}", v / 100.0);
      }
      else
      {
        const int ply = (dry(Val::Inf) - absv) / 2 + 1;
        str += "#" + to_string(sign * ply);
      }
    }

    return std::formatter<std::string>::format(str, ctx);
  }
};
