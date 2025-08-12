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

using namespace std;
using namespace eia;

int main()
{
  if (Input.is_console())
    cout << format("Chess engine {} v{} by {} (c) 2025\n", Name, Vers, Auth);

  Book book;
  book.read_pgn("C:\\neurocore\\chess\\datasets\\Perfect_2011.pgn");
  auto moves = book.get_random_line();
  book.print_some(3);

  for (Move move : moves)
    say("{} ", move);
  say("\n\n");

  Engine * E = new Engine;
  E->start();

  return 0;
}
