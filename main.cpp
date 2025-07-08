#include <iostream>
#include "bitboard.h"
#include "magics.h"
#include "piece.h"
#include "moves.h"
#include "board.h"

using namespace std;
using namespace eia;

int main()
{
  for (SQ sq = A1; sq < SQ_N; ++sq)
  {
    if (sq != bitscan(Bit << sq))
      cout << sq << endl;
  }

  cout << BitBoard{ get_mask<1>(E4) } << endl;
  cout << BitBoard{ get_att<1>(E4, Empty) } << endl;

  cout << BitBoard{ r_att(Empty, E4) } << endl;
  cout << BitBoard{ b_att(Empty, E4) } << endl;
  cout << BitBoard{ q_att(Empty, E4) } << endl;

  MoveList ml;
  Board B;
  B.set();

  B.generate_attacks<White>(ml);
  B.generate_quiets<White>(ml);

  cout << ml.count() << endl;

  return 0;
}
