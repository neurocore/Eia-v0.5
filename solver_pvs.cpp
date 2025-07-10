#include <iostream>
#include "solver_pvs.h"

using namespace std;

namespace eia {

SolverPVS::SolverPVS(Engine * engine) : Solver(engine)
{
  B = new Board;
  undo = &undos[0];

  B->set(Pos::Init); // TODO: move to engine
}

SolverPVS::~SolverPVS()
{
  delete B;
}

void SolverPVS::set(const Board & board)
{
  *B = board;
}

Move SolverPVS::get_move(MS time)
{
  return Move();
}

u64 SolverPVS::perft(int depth)
{
  u64 count = 0ull;

  cout << "-- Perft " << depth << "\n";
  B->print();

  timer.start();

  MoveList ml;
  B->generate_all(ml);

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

u64 SolverPVS::perft_inner(int depth)
{
  if (depth <= 0) return 1;

  MoveList ml;
  B->generate_all(ml);

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

void SolverPVS::shift_killers()
{
}

bool SolverPVS::time_lack() const
{
  return false;
}

void SolverPVS::check_input() const
{
}

int SolverPVS::pvs(int alpha, int beta, int depth)
{
  return 0;
}

int SolverPVS::qs(int alpha, int beta)
{
  return 0;
}

}
