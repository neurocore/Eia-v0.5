#pragma once
#include <array>
#include <iostream>
#include "square.h"

namespace eia {

const u64 Empty = 0x0000000000000000ull;
const u64 Full  = 0xFFFFFFFFFFFFFFFFull;
const u64 Bit   = 0x0000000000000001ull;
const u64 Light = 0xaa55aa55aa55aa55ull;
const u64 Dark  = 0x55aa55aa55aa55aaull;

const u64 FileA = 0x0101010101010101ull;
const u64 FileB = 0x0202020202020202ull;
const u64 FileC = 0x0404040404040404ull;
const u64 FileD = 0x0808080808080808ull;
const u64 FileE = 0x1010101010101010ull;
const u64 FileF = 0x2020202020202020ull;
const u64 FileG = 0x4040404040404040ull;
const u64 FileH = 0x8080808080808080ull;

const u64 Rank1 = 0x00000000000000ffull;
const u64 Rank2 = 0x000000000000ff00ull;
const u64 Rank3 = 0x0000000000ff0000ull;
const u64 Rank4 = 0x00000000ff000000ull;
const u64 Rank5 = 0x000000ff00000000ull;
const u64 Rank6 = 0x0000ff0000000000ull;
const u64 Rank7 = 0x00ff000000000000ull;
const u64 Rank8 = 0xff00000000000000ull;

const u64 Debruijn = 0x03f79d71b4cb0a89ull;

INLINE u64 bit(SQ sq) { return Bit << static_cast<int>(sq); }
INLINE u64 lsb(u64 bb) { return bb & (Empty - bb); }
INLINE u64 rlsb(u64 bb) { return bb & (bb - Bit); }

INLINE bool only_one(u64 bb) { return bb && !rlsb(bb); }
INLINE bool several(u64 bb)  { return !!rlsb(bb); }

INLINE u64 msb(u64 bb)
{
  unsigned n = 0;

  while (bb >>= 1) n++;
  return Bit << n;
}

const std::array<int, 65536> lut = []
{
  std::array<int, 65536> arr{};
  for (int i = 0; i < 65536; ++i)
  {
    int cnt = 0;
    for (u64 j = +i; j; j = rlsb(j)) ++cnt;
    arr[i] = cnt;
  }
  return arr;
}();

const int btscn[64] =
{
   0,  1, 48,  2, 57, 49, 28,  3,
  61, 58, 50, 42, 38, 29, 17,  4,
  62, 55, 59, 36, 53, 51, 43, 22,
  45, 39, 33, 30, 24, 18, 12,  5,
  63, 47, 56, 27, 60, 41, 37, 16,
  54, 35, 52, 21, 44, 32, 23, 11,
  46, 26, 40, 15, 34, 20, 31, 10,
  25, 14, 19,  9, 13,  8,  7,  6
};

const u64 file_bb[] = { FileA, FileB, FileC, FileD, FileE, FileF, FileG, FileH };
const u64 rank_bb[] = { Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8 };

INLINE int popcnt(u64 bb)
{
  const int a =  bb >> 48;
  const int b = (bb >> 32) & 0xFFFF;
  const int c = (bb >> 16) & 0xFFFF;
  const int d =  bb        & 0xFFFF;
  return lut[a] + lut[b] + lut[c] + lut[d];
}

INLINE SQ bitscan(u64 bb)
{
  return static_cast<SQ>(btscn[(lsb(bb) * Debruijn) >> 58]);
}

struct BitBoard { u64 val; }; // adapter
std::ostream & operator << (std::ostream & os, const BitBoard & bb);
extern void print64(u64 bb);

INLINE u64 shift_u(u64 bb) { return bb << 8; }
INLINE u64 shift_d(u64 bb) { return bb >> 8; }
INLINE u64 shift_l(u64 bb) { return (bb & ~FileA) >> 1; }
INLINE u64 shift_r(u64 bb) { return (bb & ~FileH) << 1; }

INLINE u64 shift_ul(u64 bb) { return (bb & ~FileA) << 7; }
INLINE u64 shift_ur(u64 bb) { return (bb & ~FileH) << 9; }
INLINE u64 shift_dl(u64 bb) { return (bb & ~FileA) >> 9; }
INLINE u64 shift_dr(u64 bb) { return (bb & ~FileH) >> 7; }

enum class Dir {U, D, L, R, UL, UR, DL, DR};
extern constexpr u64 shift(u64 bb, Dir dir);

}
