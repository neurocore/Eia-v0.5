#include <vector>
#include "tables.h"
#include "utils.h"

namespace eia {

using Ray = std::pair<int, int>;
using Rays = std::vector<Ray>;

const Rays wp_offset = { {-1, 1}, {1, 1} };
const Rays bp_offset = { {-1,-1}, {1,-1} };

const Rays n_offset =
{
  {1, 2}, {1,-2}, {-1, 2}, {-1,-2},
  {2, 1}, {2,-1}, {-2, 1}, {-2,-1}
};

const Rays k_offset =
{
  {-1, 1}, {0, 1}, {1, 1},
  {-1, 0},         {1, 0},
  {-1,-1}, {0,-1}, {1,-1}
};

const Rays diag_offset = { {-1,-1}, {-1, 1}, {1,-1}, {1, 1} };
const Rays rook_offset = { {-1, 0}, {0, 1}, {1, 0}, {0,-1} };

constexpr SQ_BB init_piece(Piece piece, Rays rays, bool slider = false)
{
  SQ_BB result{};
  for (SQ sq = A1; sq < SQ_N; ++sq)
  {
    for (auto ray : rays)
    {
      int x = file(sq);
      int y = rank(sq);

      do
      {
        x += ray.first;
        y += ray.second;

        if (x < 0 || x > 7 || y < 0 || y > 7) break;

        result[sq] |= Bit << to_sq(x, y);
      } while (slider);
    }
  }
  return result;
}

const std::array<SQ_BB, Piece_N> atts = []
{
  std::array<SQ_BB, Piece_N> result{};

  result[WP] = init_piece(WP, wp_offset);
  result[BP] = init_piece(BP, bp_offset);
  result[WN] = init_piece(WN, n_offset);
  result[BN] = init_piece(BN, n_offset);
  result[WK] = init_piece(WK, k_offset);
  result[BK] = init_piece(BK, k_offset);
  result[WB] = init_piece(WB, diag_offset, true);
  result[BB] = init_piece(BB, diag_offset, true);
  result[WQ] = init_piece(WQ, diag_offset, true);
  result[BQ] = init_piece(BQ, diag_offset, true);
  result[WR] = init_piece(WR, rook_offset, true);
  result[BR] = init_piece(BR, rook_offset, true);
  result[WQ] = init_piece(WQ, rook_offset, true);
  result[BQ] = init_piece(BQ, rook_offset, true);

  return result;
}();

constexpr int on_line(SQ i, SQ j)
{
  int dx = file(j) - file(i);
  int dy = rank(j) - rank(i);

  return !dx || !dy          // orthogonal or
      || abs(dx) == abs(dy)   // diagonal then
       ? sgn(dx) + 8 * sgn(dy) // return shift
       : 0;
}

const std::array<SQ_Val, SQ_N + 1> dir = []
{
  std::array<SQ_Val, SQ_N + 1> result{};
  for (SQ i = A1; i < SQ_N; ++i)
  {
    for (SQ j = A1; j < SQ_N; ++j)
    {
      if (int dt = on_line(i, j); dt)
        result[i][j] = dt;
    }
  }
  return result;
}();

const std::array<SQ_BB, SQ_N + 1> between = []
{
  std::array<SQ_BB, SQ_N + 1> result{};
  for (SQ i = A1; i < SQ_N; ++i)
  {
    for (SQ j = A1; j < SQ_N; ++j)
    {
      if (int dt = on_line(i, j); dt)
        for (int k = i + dt; k < j; k += dt)
          result[i][j] |= Bit << k;
    }
  }
  return result;
}();

const std::array<SQ_BB, Color_N> front_one = []
{
  std::array<SQ_BB, Color_N> result{};
  for (SQ sq = A1; sq < SQ_N; ++sq)
  {
    if (rank(sq) < 7) result[0][sq] = (Bit << sq) >> 8;
    if (rank(sq) > 0) result[1][sq] = (Bit << sq) << 8;
  }
  return result;
}();

const std::array<SQ_BB, Color_N> front = []
{
  std::array<SQ_BB, Color_N> result{};
  for (SQ sq = A1; sq < SQ_N; ++sq)
  {
    for (u64 bb = (Bit << sq) >> 8; bb; bb >>= 8) result[0][sq] |= bb;
    for (u64 bb = (Bit << sq) << 8; bb; bb <<= 8) result[1][sq] |= bb;
  }
  return result;
}();

const std::array<SQ_BB, Color_N> att_span = []
{
  std::array<SQ_BB, Color_N> result{};
  for (SQ sq = A1; sq < SQ_N; ++sq)
  {
    result[0][sq]  = file(sq) > 0 ? front[0][sq - 1] : Empty;
    result[0][sq] |= file(sq) < 7 ? front[0][sq + 1] : Empty;

    result[1][sq]  = file(sq) > 0 ? front[1][sq - 1] : Empty;
    result[1][sq] |= file(sq) < 7 ? front[1][sq + 1] : Empty;
  }
  return result;
}();

const SQ_BB adj_files = []
{
  SQ_BB result{};
  for (SQ sq = A1; sq < SQ_N; ++sq)
  {
    result[sq]  = file(sq) > 0 ? file_bb[file(sq) - 1] : Empty;
    result[sq] |= file(sq) < 7 ? file_bb[file(sq) + 1] : Empty;
  }
  return result;
}();

}
