#pragma once
#include "types.h"
#include "consts.h"
#include "engine.h"
#include "board.h"
#include "timer.h"
#include "solver.h"

namespace eia {

struct CommandGo
{
  MS wtime_ = Time::Def;
  MS btime_ = Time::Def;
  MS winc_  = Time::Inc;
  MS binc_  = Time::Inc;
  bool infinite_ = false;
};

class SolverPVS : public Solver
{
  Undo undos[Limits::Plies];
  Undo * undo;
  Board * B;
  Timer timer;

  int max_ply;
  MS to_think;
  u64 nodes;

public:
  SolverPVS(Engine * engine);
  ~SolverPVS();
  void set(const Board & board) override;
  Move get_move(MS time) override;

  u64 perft(int depth);
  u64 perft_inner(int depth);

  void shift_killers();
  bool time_lack() const;
  void check_input() const;

  int ply() const { return static_cast<int>(undo - undos); }
  int pvs(int alpha, int beta, int depth);
  int qs(int alpha, int beta);
};

}
