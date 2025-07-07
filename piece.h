#pragma once
#include <array>
#include <vector>
#include "bitboard.h"

using SQ_BB = std::array<u64, SQ_N>;
using SQ_Val = std::array<int, SQ_N>;

enum Color     : int { Black, White, Color_N };
enum Piece     : u8  { BP, WP, BN, WN, BB, WB, BR, WR, BQ, WQ, BK, WK, Piece_N, NOP };
enum PieceType : u8  { Pawn, Knight, Bishop, Rook, Queen, King, PieceType_N };

INLINE Color operator ~ (Color c) { return static_cast<Color>(!c); }
INLINE Color operator ^ (Color c, int i) { return static_cast<Color>(+c ^ i); }

INLINE char to_char(Color c) { return "bw"[c]; }
INLINE char to_char(Piece p) { return "pPnNbBrRqQkK.."[p]; }
INLINE char to_char(PieceType p) { return "pnbrqk."[p]; }

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

template<PieceType pt>
INLINE bool is(Piece p) { return (p >> 1) == pt; }

extern const std::array<SQ_BB, Piece_N> atts;
extern const std::array<SQ_Val, SQ_N> dir;
extern const std::array<SQ_BB, SQ_N> between;
extern const std::array<SQ_BB, Color_N> front_one;
extern const std::array<SQ_BB, Color_N> front;
extern const std::array<SQ_BB, Color_N> att_span;
extern const SQ_BB adj_files;
