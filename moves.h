#pragma once
#include <iostream>
#include <string>
#include <array>
#include "piece.h"
#include "utils.h"

enum MT : u16 // Move type
{
  Quiet,     // PC12 (Promotion, Capture, Flag_1, Flag_2)
  Pawn2,
  KCastle,   // 0010
  QCastle,   // 0011
  Cap,       // 0100
  Ep,        // 0101
  NProm = 8, // 1000
  BProm,     // 1001
  RProm,     // 1010
  QProm,     // 1011
  NCapProm,  // 1100
  BCapProm,  // 1101
  RCapProm,  // 1110
  QCapProm,  // 1111
  size       //   ^^ = (promoted - 1)
};

// | 0..5 | 6..11 | 12..15 | = 16 bits
// | FROM |  TO   |   MT   |


enum Move : u16;
INLINE Move to_move(SQ from, SQ to, MT mt = Quiet)
{
  return static_cast<Move> (from | (to << 6) | (mt << 12));
}

enum Move : u16 { None = to_move(A1, A1), Null = to_move(B1, B1) };

INLINE bool is_empty(Move move) { return move == None || move == Null; }

INLINE SQ from(Move move) { return static_cast<SQ>(move & 63); } 
INLINE SQ to(Move move) { return static_cast<SQ>((move >> 6) & 63); } 
INLINE MT mt(Move move) { return static_cast<MT>(move >> 12); }

INLINE bool is_cap(MT mt) { return !!(mt & Cap); }
INLINE bool is_prom(MT mt) { return !!(mt & NProm); }
INLINE bool is_attack(MT mt) { return !!(mt & NCapProm); }

INLINE bool is_ep(MT mt)     { return mt == Ep; }
INLINE bool is_pawn2(MT mt)  { return mt == Pawn2; }
INLINE bool is_castle(MT mt) { return mt == KCastle || mt == QCastle; }

INLINE bool is_cap(Move move) { return !!(move & (Cap << 12)); }
INLINE bool is_prom(Move move) { return !!(move & (NProm << 12)); }
INLINE bool is_attack(Move move) { return !!(move & (NCapProm << 12)); }

INLINE bool is_ep(Move move)     { return is_ep(mt(move)); }
INLINE bool is_pawn2(Move move)  { return is_pawn2(mt(move)); }
INLINE bool is_castle(Move move) { return is_castle(mt(move)); }

INLINE PieceType promoted(MT mt) { return static_cast<PieceType>(1 + (mt & 3)); }
INLINE PieceType promoted(Move move) { return promoted(mt(move)); }
INLINE Piece promoted(MT mt, Color col) { return to_piece(promoted(mt), col); }
INLINE Piece promoted(Move move, Color col) { return promoted(mt(move), col); }

inline std::ostream & operator << (std::ostream & os, Move move)
{
  os << to_string(from(move)) << to_string(to(move));
  if (is_prom(move)) os << to_char(promoted(move));
  return os;
}

enum class Castling : u8
{
  NO = 0,
  BK = 1 << 0,
  WK = 1 << 1,
  BQ = 1 << 2,
  WQ = 1 << 3,
  ALL = BK | BQ | WK | WQ
};

INLINE Castling operator | (Castling a, Castling b)
{
  return static_cast<Castling>(static_cast<u8>(a) | static_cast<u8>(b));
}

INLINE Castling operator & (Castling a, Castling b)
{
  return static_cast<Castling>(static_cast<u8>(a) & static_cast<u8>(b));
}

INLINE Castling operator ^ (Castling a, Castling b)
{
  return static_cast<Castling>(static_cast<u8>(a) ^ static_cast<u8>(b));
}

INLINE Castling & operator |= (Castling & a, Castling b)
{
  a = a | b;
  return a;
}

INLINE Castling & operator &= (Castling & a, Castling b)
{
  a = a & b;
  return a;
}

INLINE Castling & operator ^= (Castling & a, Castling b)
{
  a = a ^ b;
  return a;
}

template<Castling C>
INLINE bool has(Castling castling)
{
  return static_cast<bool>(castling & C);
}

const u64 Span_BK = bit(F8) | bit(G8);
const u64 Span_WK = bit(F1) | bit(G1);
const u64 Span_BQ = bit(B8) | bit(C8) | bit(D8);
const u64 Span_WQ = bit(B1) | bit(C1) | bit(D1);

INLINE Castling to_castling(const char c)
{
  size_t i = index_of("kKqQ", c);
  return static_cast<Castling>( i < 0 ? 0 : (1 << i) );
}

INLINE MT parse_promotee(const char c)
{
  size_t i = index_of("nbrq", c);
  return static_cast<MT>( i < 0 ? 0 : (MT::NProm + i) );
}

INLINE std::string to_string(Castling castling, std::string fill = "")
{
  std::string str;
  str += has<Castling::WK>(castling) ? "K" : fill;
  str += has<Castling::WQ>(castling) ? "Q" : fill;
  str += has<Castling::BK>(castling) ? "k" : fill;
  str += has<Castling::BQ>(castling) ? "q" : fill;
  return str;
}

const std::array<Castling, 64> uncastle = []
{
  std::array<Castling, 64> result;
  for (SQ sq = A1; sq < SQ_N; ++sq)
    result[sq] = Castling::ALL;

  result[A1] ^= Castling::WQ;
  result[E1] ^= Castling::WQ | Castling::WK;
  result[H1] ^= Castling::WK;
  result[A8] ^= Castling::BQ;
  result[E8] ^= Castling::BQ | Castling::BK;
  result[H8] ^= Castling::BK;

  return result;
}();
