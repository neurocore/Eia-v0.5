#pragma once
#include <format>
#include "bitboard.h"
#include "utils.h"

namespace eia {

enum Color     : int { Black, White, Color_N };
enum Piece     : u8  { BP, WP, BN, WN, BB, WB, BR, WR, BQ, WQ, BK, WK, Piece_N, NOP };
enum PieceType : u8  { Pawn, Knight, Bishop, Rook, Queen, King, PieceType_N };

INLINE Color operator ~ (Color c) { return static_cast<Color>(!c); }
INLINE Color operator ^ (Color c, int i) { return static_cast<Color>(+c ^ i); }

INLINE char to_char(Color c) { return "bw"[c]; }
INLINE char to_char(Piece p) { return "pPnNbBrRqQkK.."[p]; }
INLINE char to_char(PieceType p) { return "pnbrqk."[p]; }

INLINE Color to_color(const char c)
{
  size_t i = index_of("bw", c);
  if (i < 0) i = Color_N;
  return static_cast<Color>(i);
}

INLINE Piece to_piece(const char c)
{
  auto i = index_of("pPnNbBrRqQkK.", c);
  if (i < 0) return NOP;
  return static_cast<Piece>(i);
}

INLINE PieceType to_pt(const char c)
{
  auto i = index_of("pnbrqk", c);
  if (i < 0) return PieceType_N;
  return static_cast<PieceType>(i);
}

INLINE Piece to_piece(PieceType pt, Color c) { return static_cast<Piece>(2 * pt + c); }
INLINE Color col(Piece p)    { return static_cast<Color>(p & 1); }
INLINE Piece opp(Piece p)    { return static_cast<Piece>(p ^ 1); }
INLINE PieceType pt(Piece p) { return static_cast<PieceType>(p >> 1); }

INLINE Piece operator + (Piece p, int val) { return static_cast<Piece>(+p + val); }
INLINE Piece operator - (Piece p, int val) { return static_cast<Piece>(+p - val); }
INLINE Piece operator ^ (Piece p, Color c) { return static_cast<Piece>(+p ^ +c); }

INLINE Piece & operator ++ (Piece & a) { a = static_cast<Piece>(a + 1); return a; }
INLINE Piece & operator -- (Piece & a) { a = static_cast<Piece>(a - 1); return a; }

INLINE bool is_pawn(Piece p) { return p < BN; }
INLINE bool is_king(Piece p) { return p == BK || p == WK; }
INLINE bool is_slider(Piece p) { return p > WN && p < BK; }

template<PieceType pt>
INLINE bool is(Piece p) { return (p >> 1) == pt; }

}

template<>
struct std::formatter<eia::Piece> : std::formatter<std::string>
{
  auto format(const eia::Piece & p, std::format_context & ctx) const
  {
    std::string str = std::string() + to_char(p);
    return std::formatter<std::string>::format(str, ctx);
  }
};
