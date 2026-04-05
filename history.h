#pragma once
#include <utility>
#include "board.h"
#include "utils.h"

namespace eia {

static const int HistoryMax = 16384;

static int stat_bonus(int depth) // from Stockfish
{
  return depth > 13 ? 32 : 16 * depth * depth + 128 * (std::max)(depth - 1, 0);
}

static void update_history(int & curr, int depth, bool good)
{
  const int delta = good ? stat_bonus(depth) : -stat_bonus(depth);
  curr += delta - curr * abs(delta) / HistoryMax; // T-norm
}

static PieceType get_captured(const Board * B, Move move)
{
  return is_cap(move) ? pt(B->state.cap) : Pawn;
}

static int & underlying_history(const Board * B, Move move, History & H)
{
  const SQ from = get_from(move);
  const SQ   to = get_to(move);

  const bool leave = B->state.threats & bit(from);
  const bool enter = B->state.threats & bit(to);

  return H[B->color][leave][enter][from][to];
}

static int & underlying_caphist(const Board * B, Move move, CapHist & C)
{
  const SQ from = get_from(move);
  const SQ   to = get_to(move);
  const auto cap = get_captured(B, move);
  const Piece p = B->square[from];

  const bool leave = B->state.threats & bit(from);
  const bool enter = B->state.threats & bit(to);

  return C[p][leave][enter][from][cap];
}

}
