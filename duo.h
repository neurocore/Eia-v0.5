#pragma once
#include <iostream>
#include <format>
#include <string>
#include "types.h"
#include "value.h"
#include "consts.h"

namespace eia {

struct Duo
{
  Val op, eg;

  constexpr Duo(Val op = 0_cp, Val eg = 0_cp) : op(op), eg(eg) {}
  static constexpr Duo both(Val x)  { return Duo(x, x); }
  static constexpr Duo as_op(Val x) { return Duo(x, 0_cp); }
  static constexpr Duo as_eg(Val x) { return Duo(0_cp, x); }

  INLINE void clear() { op = eg = 0_cp; }
  INLINE Val tapered(int phase) const
  {
    return ((op * (Phase::Total - phase)) + eg * phase) / Phase::Total;
  }

  INLINE Duo operator * (int k) const { return Duo(op * k, eg * k); }
  INLINE Duo operator / (int k) const { return Duo(op / k, eg / k); }

  INLINE Duo & operator += (const Duo & v) { op += v.op; eg += v.eg; return *this; }
  INLINE Duo & operator -= (const Duo & v) { op -= v.op; eg -= v.eg; return *this; }

  INLINE Duo & operator *= (int k) { op *= k; eg *= k; return *this; }
  INLINE Duo & operator /= (int k) { op /= k; eg /= k; return *this; }

  inline std::string to_string() const { return std::format("({}, {})", op, eg); }
  friend std::ostream & operator << (std::ostream & os, const Duo & v)
  {
    os << v.to_string();
    return os;
  }

  INLINE bool operator == (const Duo & v) const { return op == v.op && eg == v.eg; }
  INLINE bool operator != (const Duo & v) const { return op != v.op || eg != v.eg; }

  INLINE Duo operator + (const Duo & v) const { return Duo(op + v.op, eg + v.eg); }
  INLINE Duo operator - (const Duo & v) const { return Duo(op - v.op, eg - v.eg); }
  INLINE Duo operator - ()              const { return Duo(-op, -eg); }
};

}

template<>
struct std::formatter<eia::Duo> : std::formatter<std::string>
{
  auto format(const eia::Duo & duo, std::format_context & ctx) const
  {
    std::string str = duo.to_string();
    return std::formatter<std::string>::format(str, ctx);
  }
};
