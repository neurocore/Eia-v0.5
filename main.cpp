#include <iostream>
#include "bitboard.h"
#include "magics.h"
#include "piece.h"
#include "moves.h"

using namespace std;

int main()
{
  /*for (int i = 0; i < 8; i++)
  {
    cout << BitBoard{ file_bb[i] } << endl;
    cout << bitscan(file_bb[i]) << endl;
  }

  cout << "here" << endl;

  SQ sq = D4;
  cout << sq << endl;
  cout << BitBoard{ att[WP][sq] } << endl;
  cout << BitBoard{ att_span[White][sq] } << endl;
  cout << BitBoard{ adj_files[sq] } << endl;*/

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

  return 0;
}
