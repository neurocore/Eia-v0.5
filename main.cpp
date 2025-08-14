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

  Epd epd;
  epd.read("C:\\neurocore\\chess\\datasets\\_epd\\STS1-STS15_LAN_v4.epd");

  Engine * E = new Engine;
  E->start();

  return 0;
}
