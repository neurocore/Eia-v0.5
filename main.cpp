#include <iostream>
#include <format>
#include "bitboard.h"
#include "magics.h"
#include "piece.h"
#include "moves.h"
#include "board.h"
#include "engine.h"
#include "solver_pvs.h"
#include "book.h"
#include "epd.h"

using namespace std;
using namespace eia;

int main()
{
  if (Input.is_console())
    cout << format("Chess engine {} v{} by {} (c) 2025\n", Name, Vers, Auth);

  Engine * E = new Engine;
  E->start();

  return 0;
}
