#pragma once
#include <iostream>
#include <format>
#include <string>
#include "types.h"
#include "consts.h"

namespace eia {

struct Duo
{
  int op, eg;

  constexpr Duo(int op = 0, int eg = 0) : op(op), eg(eg) {}
  static constexpr Duo both(int x)  { return Duo(x, x); }
  static constexpr Duo as_op(int x) { return Duo(x, 0); }
  static constexpr Duo as_eg(int x) { return Duo(0, x); }

  INLINE void clear() { op = eg = 0; }
  INLINE int tapered(int phase) const
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
