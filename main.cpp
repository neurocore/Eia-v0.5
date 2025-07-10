#include <iostream>
#include "bitboard.h"
#include "magics.h"
#include "piece.h"
#include "moves.h"
#include "board.h"
#include "engine.h"
#include "solver_pvs.h"

using namespace std;
using namespace eia;

int main()
{
  Engine * E = new Engine;
  E->start();

  return 0;
}
