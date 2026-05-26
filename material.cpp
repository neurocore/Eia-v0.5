#include "material.h"
#include "piece.h"

namespace eia {

std::pair<MatKey, MatInfo> get_matinfo(std::vector<int> cnts)
{
  const int cost[] = { -1, 1, -3, 3, -3, 3, -5, 5, -9, 9 };
  const int pawns = cnts[BP] + cnts[WP];
  int cnt = 0, val = 0, heavy = 0;
  MatKey mk = MatKey::Init;
  MatInfo mi{ .scale = 256, .val = 0, .eg = NoEG };

  for (int col = 0; col < 2; col++)
  {
    for (PieceType pt = Pawn; pt < King; ++pt)
    {
      const Piece p = to_piece(pt, (Color)col);
      assert(p < Piece_N);
      cnt += cnts[p];
      val += cost[p];
      mk  += cnts[p] * matkey[p];
      heavy += (p >= BR) * cnts[p];
    }
  }

  mi.val = val; // speculative! (need eval and duo)

  if (!pawns && !heavy && abs(val) < 4) // drawish endgame
  {
    mi.scale = cnt * 8; // [1/16; 1/4]
  }

  // TODO: what else?

  return std::make_pair(mk, mi);
}

}
