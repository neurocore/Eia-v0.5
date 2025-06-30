#include <iostream>
#include "bitboard.h"
#include "piece.h"

using namespace std;

int main()
{
  for (int i = 0; i < 8; i++)
  {
    cout << BitBoard{ file_bb[i] } << endl;
    cout << bitscan(file_bb[i]) << endl;
  }

  cout << "here" << endl;

  SQ sq = D4;
  cout << sq << endl;
  cout << BitBoard{ att[WP][sq] } << endl;
  cout << BitBoard{ att_span[White][sq] } << endl;
  cout << BitBoard{ adj_files[sq] } << endl;

  return 0;
}
