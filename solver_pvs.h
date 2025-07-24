#pragma once
#include "types.h"
#include "consts.h"
#include "engine.h"
#include "board.h"
#include "timer.h"
#include "solver.h"
#include "eval.h"

namespace eia {

enum NodeType { PV, NonPV, Root };

class SolverPVS : public Solver
{
  Undo undos[Limits::Plies];
  Undo * undo;
  Board * B;
  Eval * E;
  Timer timer;
  Counter counter;
  History history;

  int max_ply;
  MS to_think;
  u64 nodes;

public:
  SolverPVS(Engine * engine);
  ~SolverPVS();
  bool is_solver() { return true; }
  void set(const Board & board) override;
  Move get_move(MS time) override;

  u64 perft(int depth);
  u64 perft_inner(int depth);

  // checks pseudolegal test correctness
  int plegt();

  int eval() { return E->eval<true>(B, -Val::Inf, Val::Inf); }
  const EvalDetails & eval_details() { return E->get_details(); }

  bool abort() const;
  int ply() const { return static_cast<int>(undo - undos); }
  void set_movepicker(MovePicker & mp, Move hash);
  void update_moves_stats(int depth);

  template<NodeType NT>
  int pvs(int alpha, int beta, int depth);

  int qs(int alpha, int beta);
};

// explicit instantiate for use in cpp leads to better compile time
template int SolverPVS::pvs<PV>(int alpha, int beta, int depth);
template int SolverPVS::pvs<NonPV>(int alpha, int beta, int depth);
template int SolverPVS::pvs<Root>(int alpha, int beta, int depth);

}
