#pragma once
#include <bit>
#include <array>
#include <cassert>
#include <concepts>
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

const u64 Seventh = Rank2 | Rank7;

const u64 QWing = FileA | FileB | FileC | FileD;
const u64 KWing = FileE | FileF | FileG | FileH;

INLINE u64 bit(SQ sq) { return Bit << static_cast<int>(sq); }
INLINE u64 lsb(u64 bb) { return bb & (Empty - bb); }
INLINE u64 rlsb(u64 bb) { return bb & (bb - Bit); }

INLINE bool only_one(u64 bb) { return bb && !rlsb(bb); }
INLINE bool several(u64 bb)  { return !!rlsb(bb); }

INLINE bool contains(u64 bb, u64 part) { return (bb & part) == part; }

template<typename... Squares>
constexpr uint64_t bits(Squares... sq)
{
  return ( bit(sq) | ... );
}

template<std::integral T>
INLINE T msb(T bb)
{
  unsigned n = 0;

  while (bb >>= 1) n++;
  return static_cast<T>(1) << n;
}

const u64 file_bb[] = { FileA, FileB, FileC, FileD, FileE, FileF, FileG, FileH };
const u64 rank_bb[] = { Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8 };

INLINE int popcnt(u64 bb)
{
  return std::popcount(bb);
}

INLINE SQ bitscan(u64 bb)
{
  assert(bb);
  return static_cast<SQ>(std::countr_zero(bb));
}

INLINE SQ bitscan_r(u64 bb)
{
  assert(bb);
  return static_cast<SQ>(std::countl_zero(bb) ^ 63);
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

const u64 sA1 = bit(A1);
const u64 sA2 = bit(A2);
const u64 sA3 = bit(A3);
const u64 sA4 = bit(A4);
const u64 sA5 = bit(A5);
const u64 sA6 = bit(A6);
const u64 sA7 = bit(A7);
const u64 sA8 = bit(A8);

const u64 sB1 = bit(B1);
const u64 sB2 = bit(B2);
const u64 sB3 = bit(B3);
const u64 sB4 = bit(B4);
const u64 sB5 = bit(B5);
const u64 sB6 = bit(B6);
const u64 sB7 = bit(B7);
const u64 sB8 = bit(B8);

const u64 sC1 = bit(C1);
const u64 sC2 = bit(C2);
const u64 sC3 = bit(C3);
const u64 sC4 = bit(C4);
const u64 sC5 = bit(C5);
const u64 sC6 = bit(C6);
const u64 sC7 = bit(C7);
const u64 sC8 = bit(C8);

const u64 sD1 = bit(D1);
const u64 sD2 = bit(D2);
const u64 sD3 = bit(D3);
const u64 sD4 = bit(D4);
const u64 sD5 = bit(D5);
const u64 sD6 = bit(D6);
const u64 sD7 = bit(D7);
const u64 sD8 = bit(D8);

const u64 sE1 = bit(E1);
const u64 sE2 = bit(E2);
const u64 sE3 = bit(E3);
const u64 sE4 = bit(E4);
const u64 sE5 = bit(E5);
const u64 sE6 = bit(E6);
const u64 sE7 = bit(E7);
const u64 sE8 = bit(E8);

const u64 sF1 = bit(F1);
const u64 sF2 = bit(F2);
const u64 sF3 = bit(F3);
const u64 sF4 = bit(F4);
const u64 sF5 = bit(F5);
const u64 sF6 = bit(F6);
const u64 sF7 = bit(F7);
const u64 sF8 = bit(F8);

const u64 sG1 = bit(G1);
const u64 sG2 = bit(G2);
const u64 sG3 = bit(G3);
const u64 sG4 = bit(G4);
const u64 sG5 = bit(G5);
const u64 sG6 = bit(G6);
const u64 sG7 = bit(G7);
const u64 sG8 = bit(G8);

const u64 sH1 = bit(H1);
const u64 sH2 = bit(H2);
const u64 sH3 = bit(H3);
const u64 sH4 = bit(H4);
const u64 sH5 = bit(H5);
const u64 sH6 = bit(H6);
const u64 sH7 = bit(H7);
const u64 sH8 = bit(H8);

}
