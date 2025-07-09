#include <iostream>
#include "search.h"

using namespace std;

namespace eia {

Search::Search(Engine * engine)
{
  B = new Board;
  undo = &undos[0];

  B->set(/*Pos::Kiwi*/); // TODO: move to engine
}

Search::~Search()
{
  delete B;
}

Move Search::get_move(MS time)
{
  return Move();
}

u64 Search::perft(int depth)
{
  u64 count = 0ull;

  cout << "-- Perft " << depth << "\n";
  B->print();

  timer.start();

  MoveList ml;
  if (B->color)
  {
    B->generate_attacks<White>(ml);
    B->generate_quiets<White>(ml);
  }
  else
  {
    B->generate_attacks<Black>(ml);
    B->generate_quiets<Black>(ml);
  }

  while (!ml.is_empty())
  {
    Move move = ml.get_next();
    if (is_empty(move)) break;
    if (!B->make(move, undo)) continue;

    cout << move;

    u64 cnt = perft_inner(depth - 1);
    count += cnt;

    cout << " - " << cnt << "\n";

    B->unmake(move, undo);
  }

  i64 time = timer.getms();
  double knps = static_cast<double>(count) / (time + 1);

  cout << "\nCount: " << count << "\n";
  cout << "Time: " << time << " ms\n";
  cout << "Speed: " << knps << " knps\n";
  cout << "\n";

  return count;
}

u64 Search::perft_inner(int depth)
{
  if (depth <= 0) return 1;

  MoveList ml;
  if (B->color)
  {
    B->generate_attacks<White>(ml);
    B->generate_quiets<White>(ml);
  }
  else
  {
    B->generate_attacks<Black>(ml);
    B->generate_quiets<Black>(ml);
  }

  u64 count = 0ull;
  while (!ml.is_empty())
  {
    Move move = ml.get_next();
    if (is_empty(move)) break;
    if (!B->make(move, undo)) continue;

    count += depth > 1 ? perft_inner(depth - 1) : 1;

    B->unmake(move, undo);
  }
  return count;
}

void Search::shift_killers()
{
}

bool Search::time_lack() const
{
  return false;
}

void Search::check_input() const
{
}

int Search::pvs(int alpha, int beta, int depth)
{
  return 0;
}

int Search::qs(int alpha, int beta)
{
  return 0;
}

}
