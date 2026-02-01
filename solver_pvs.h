#pragma once
#include "movepicker.h"
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
  int LMR[64][256];
  Undo undos[Limits::Plies];
  Board * B;
  Eval * E;
  Table * H;
  Timer timer;
  Counter counter;
  History history;

  int max_ply;
  MS to_think;
  u64 nodes;
  Val best_val;

public:
  SolverPVS(Eval * eval);
  ~SolverPVS();
  void init();
  bool is_solver() { return true; }
  void new_game() { H->clear(); }
  void set(const Board & board) override;
  Move get_move(const SearchCfg & cfg) override;
  int  get_best_val() const { return best_val; }

  u64 get_hash() const { return B->state.bhash; }
  void make(Move move) override
  {
    B->revert_states();
    B->make(move);
  }

  u64 perft(int depth);
  u64 perft_inner(int depth);

  // checks pseudolegal test correctness
  int plegt();

  bool abort() const;
  int ply() const { return B->ply(); }
  Val contempt() const { return 0_cp; }

  template<bool QS>
  void set_movepicker(MovePicker<QS> & mp, Move hash);
  void update_moves_stats(int depth);

  template<NodeType NT>
  Val pvs(Val alpha, Val beta, int depth, bool is_null = false);
  Val qs(Val alpha, Val beta);

  template<bool QS>
  friend struct MovePicker;
};


template<bool QS>
void SolverPVS::set_movepicker(MovePicker<QS> & mp, Move hash)
{
  const Undo & undo = undos[ply()];
  const bool hash_correct = B->pseudolegal(hash);

  mp.ml.clear();

  mp.B = B;
  mp.H = &history;
  mp.killer[0] = undo.killer[0];
  mp.killer[1] = undo.killer[1];

  mp.hash_mv = hash_correct ? hash : Move::None;
  mp.stage = hash_correct ? Stage::Hash : Stage::GenCaps;

  if (!ply()) return;

  const Move prev = undos[ply() - 1].curr;
  mp.counter = is_empty(prev) ? Move::None
             : counter[~B->color][get_from(prev)][get_to(prev)];
}


// explicit instantiate for use in cpp leads to better compile time
template Val SolverPVS::pvs<PV>(Val alpha, Val beta, int depth, bool is_null);
template Val SolverPVS::pvs<NonPV>(Val alpha, Val beta, int depth, bool is_null);
template Val SolverPVS::pvs<Root>(Val alpha, Val beta, int depth, bool is_null);

//template Val SolverPVS::qs<0>(Val alpha, Val beta, int max_checks_depth);
//template Val SolverPVS::qs<1>(Val alpha, Val beta, int max_checks_depth);

}
