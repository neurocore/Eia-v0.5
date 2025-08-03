#include <random>
#include "zobrist.h"

using namespace std;

namespace eia::Zobrist {

u64 key[SQ_N][Piece_N];
u64 castle[Castling_N];
u64 ep[SQ_N + 1];
u64 turn = [&]() -> u64
{
  mt19937_64 gen(0xC0FFEE);
  uniform_int_distribution<u64> distr;

  for (Piece p = BP; p < Piece_N; ++p)
    for (SQ sq = A1; sq < SQ_N; ++sq)
      key[sq][p] = distr(gen);

  for (int i = 0; i < 16; i++)
    castle[i] = i ? distr(gen) : Empty;

  for (SQ sq = A1; sq < SQ_N; ++sq)
  {
    int y = rank(sq);
    ep[sq] = (y == 2 || y == 5) ? distr(gen) : Empty;
  }

  return distr(gen);
}();

}
