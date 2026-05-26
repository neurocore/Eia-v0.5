#pragma once
#include <array>
#include <vector>
#include <utility>
#include "piece.h"

using u64 = unsigned long long;

namespace eia {

//         [overflow counters]             | [Rybka-matkey]
//  BN   WN   BB   WB   BR   WR   BQ   WQ  |
// 0001|0001|0001|0001|0001|0001|0010|0010 |  (2^2 * 3^10)
//                                         |
// 1100|1100|1100|1100|1100|1100|1100|1100 | overflow mask

enum class MatKey : u64
{
  WQ = 1,
  BQ = WQ * 2,
  WR = BQ * 2,
  BR = WR * 3,
  WB = BR * 3,
  BB = WB * 3,
  WN = BB * 3,
  BN = WN * 3,
  WP = BN * 3,
  BP = WP * 9,
  Total = BP * 9,
  Mask = (1ull << 18) - 1,

  WQ_1 = 0b0001ull << 18, // Singular elements for
  BQ_1 = 0b0001ull << 22, // every overflow counter
  WR_1 = 0b0001ull << 26, // for certain piece
  BR_1 = 0b0001ull << 30,
  WB_1 = 0b0001ull << 34, // Used for fast test
  BB_1 = 0b0001ull << 38, // on pieces overflow
  WN_1 = 0b0001ull << 42,
  BN_1 = 0b0001ull << 46,

  Init = (WQ_1 << 1)
       | (BQ_1 << 1)  // Q: 2 (init) + 1 (board) < 4
       | WR_1 | BR_1  // R: 1 (init) + 2 (board) < 4
       | WB_1 | BB_1  // B: 1 (init) + 2 (board) < 4
       | WN_1 | BN_1, // N: 1 (init) + 2 (board) < 4

  WQ_O = 0b1100ull << 18,
  BQ_O = 0b1100ull << 22,
  WR_O = 0b1100ull << 26,
  BR_O = 0b1100ull << 30,
  WB_O = 0b1100ull << 34,
  BB_O = 0b1100ull << 38,
  WN_O = 0b1100ull << 42,
  BN_O = 0b1100ull << 46,

  Overflow = WQ_O | BQ_O | WR_O | BR_O
           | WB_O | BB_O | WN_O | BN_O,
};

static_assert(
  MatKey::Total == (MatKey)236'196,
  "Incorrect material key"
);

INLINE u64 operator + (const MatKey & a)
{
  return static_cast<u64>(a);
}

INLINE MatKey operator + (const MatKey & a, const MatKey & b)
{
  return static_cast<MatKey>(+a + +b);
}

INLINE MatKey operator - (const MatKey & a, const MatKey & b)
{
  return static_cast<MatKey>(+a - +b);
}

INLINE MatKey & operator += (MatKey & a, const MatKey & b) { a = a + b; return a; }
INLINE MatKey & operator -= (MatKey & a, const MatKey & b) { a = a - b; return a; }

INLINE MatKey operator * (const MatKey & a, int k)
{
  return static_cast<MatKey>(+a * k);
}

INLINE MatKey operator * (int k, const MatKey & a)
{
  return static_cast<MatKey>(+a * k);
}

INLINE u64 operator & (const MatKey & a, const MatKey & b)
{
  return +a & +b;
}

INLINE bool is_correct(const MatKey & mk)
{
  return !(mk & MatKey::Overflow);
}

INLINE int get_index(const MatKey & mk)
{
  return static_cast<int>(mk & MatKey::Mask);
}

static const MatKey matkey[12] =
{
  MatKey::BP,
  MatKey::WP,
  MatKey::BN + MatKey::BN_1,
  MatKey::WN + MatKey::WN_1,
  MatKey::BB + MatKey::BB_1,
  MatKey::WB + MatKey::WB_1,
  MatKey::BR + MatKey::BR_1,
  MatKey::WR + MatKey::WR_1,
  MatKey::BQ + MatKey::BQ_1,
  MatKey::WQ + MatKey::WQ_1,
};

enum Endgame
{
  NoEG, KPk, KRk, KQk, KNBk
};

struct MatInfo
{
  int scale, val;
  Endgame eg;
};

extern std::pair<MatKey, MatInfo> get_matinfo(std::vector<int> cnts);

}
