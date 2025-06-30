#pragma once
#include <string>
#include <iostream>
#include <algorithm>
#include "types.h"

enum SQ : u8
{
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8, SQ_N
};

INLINE int rank(SQ x) { return x >> 3; }
INLINE int file(SQ x) { return x & 7; }

INLINE SQ to_sq(int f, int r)
{
  return static_cast<SQ>((r << 3) + f);
}

INLINE SQ to_sq(std::string s)
{
  return s.length() > 1 ? to_sq(s[0] - 'a', s[1] - '1') : SQ_N;
}

INLINE SQ opp(SQ sq)
{
  return to_sq(file(sq), 7 - rank(sq));
}

INLINE std::string to_string(SQ sq)
{
  char fileChar = 'a' + file(sq);
  char rankChar = '1' + rank(sq);
  return (std::string() + fileChar) + rankChar;
}

inline std::ostream & operator << (std::ostream & os, const SQ & sq)
{
  os << static_cast<char>('a' + file(sq))
     << static_cast<char>('1' + rank(sq)) << std::endl;
  return os;
}

INLINE SQ operator + (SQ a, SQ b) { return static_cast<SQ>(a + b); }
INLINE SQ operator - (SQ a, SQ b) { return static_cast<SQ>(a - b); }
INLINE SQ & operator ++(SQ & a) { a = static_cast<SQ>(a + 1); return a; }
INLINE SQ & operator --(SQ & a) { a = static_cast<SQ>(a - 1); return a; }

INLINE int k_dist(SQ a, SQ b) // Chebyshev or king distance
{
  return std::max(abs(rank(a) - rank(b)), abs(file(a) - file(b)));
}

