#include <iostream>
#include "bitboard.h"
#include "magics.h"
#include "piece.h"
#include "moves.h"
#include "board.h"
#include "search.h"

using namespace std;
using namespace eia;

int main()
{
  Search S(new Engine);
  S.perft(6);

  return 0;
}
