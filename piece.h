#pragma once
#include <array>
#include <vector>
#include "bitboard.h"

using SQ_BB = std::array<u64, SQ_N>;
using SQ_Val = std::array<int, SQ_N>;

enum Color     : int { Black, White, Color_N };
enum Piece     : u8  { BP, WP, BN, WN, BB, WB, BR, WR, BQ, WQ, BK, WK, Piece_N, NOP };
enum PieceType : u8  { Pawn, Knight, Bishop, Rook, Queen, King, PieceType_N };

inline constexpr Color opp(Color c) { return static_cast<Color>(!c); }

inline constexpr char to_char(Color c) { return "bw"[c]; }
inline constexpr char to_char(Piece p) { return "pPnNbBrRqQkK.."[p]; }
inline constexpr char to_char(PieceType p) { return "pnbrqk."[p]; }

inline constexpr Piece to_piece(PieceType pt, Color c) { return static_cast<Piece>(2 * pt + c); }

extern const std::array<SQ_BB, Piece_N> att;
extern const std::array<SQ_Val, SQ_N> dir;
extern const std::array<SQ_BB, SQ_N> between;
extern const std::array<SQ_BB, Color_N> front_one;
extern const std::array<SQ_BB, Color_N> front;
extern const std::array<SQ_BB, Color_N> att_span;
extern const SQ_BB adj_files;
