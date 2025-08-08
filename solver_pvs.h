#pragma once
#include "types.h"
#include "consts.h"
#include "engine.h"
#include "board.h"
#include "timer.h"
#include "solver.h"
#include "eval.h"
#include "hash.h"

namespace eia {

using namespace Hash;

enum NodeType { PV, NonPV, Root };

class SolverPVS : public Solver
{
  Undo undos[Limits::Plies];
  Undo * undo;
  Board * B;
  Eval * E;
  Table * H;
  Timer timer;
  Counter counter;
  History history;

  int max_ply;
  MS to_think;
  u64 nodes;

public:
  SolverPVS(Engine * engine, Eval * eval);
  ~SolverPVS();
  bool is_solver() { return true; }
  void new_game() { H->clear(); }
  void set(const Board & board) override;
  Move get_move(MS time) override;

  u64 perft(int depth);
  u64 perft_inner(int depth);

  // checks pseudolegal test correctness
  int plegt();

  bool abort() const;
  int ply() const { return static_cast<int>(undo - undos); }

  template<MPType Type>
  void set_movepicker(MovePicker<Type> & mp, Move hash);
  void update_moves_stats(int depth);

  template<NodeType NT>
  int pvs(int alpha, int beta, int depth, bool is_null = false);

  int qs(int alpha, int beta);

  template<MPType Type>
  friend struct MovePicker;
};


template<MPType Type>
void SolverPVS::set_movepicker(MovePicker<Type> & mp, Move hash)
{
  const bool hash_correct = B->pseudolegal(hash);

  mp.ml.clear();

  mp.B = B;
  mp.H = &history;
  mp.killer[0] = undo->killer[0];
  mp.killer[1] = undo->killer[1];

  mp.hash_mv = hash_correct ? hash : Move::None;

  if constexpr (Type == MPType::Evade)
  {
    mp.stage = Stage::GenCaps;
  }
  else
  {
    mp.stage = hash_correct ? Stage::Hash : Stage::GenCaps;
  }

  if (!ply()) return;

  const Move prev = (undo - 1)->curr;
  mp.counter = is_empty(prev) ? Move::None
             : counter[~B->color][get_from(prev)][get_to(prev)];
}


// explicit instantiate for use in cpp leads to better compile time
template int SolverPVS::pvs<PV>(int alpha, int beta, int depth, bool is_null);
template int SolverPVS::pvs<NonPV>(int alpha, int beta, int depth, bool is_null);
template int SolverPVS::pvs<Root>(int alpha, int beta, int depth, bool is_null);

}
