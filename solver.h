#pragma once
#include "board.h"

namespace eia {

struct Board;
class Engine;

struct Undo
{
  // Duo pst;
  int eval;
  Move curr, best;
  Move killer[2];
};

struct SearchCfg
{
  MS time[2] = { Time::Def, Time::Def };
  MS inc[2]  = { Time::Inc, Time::Inc };
  bool infinite = false;
  int depth = Val::Inf;

  INLINE MS full_time(Color c) const { return time[c] + inc[c]; }

  // not supported by eia
  Move searchmoves = Move::None;
  bool ponder = false;
  int movestogo = Val::Inf;
  int nodes = Val::Inf, mate = Val::Inf;
  MS movetime = limits<i64>::max();
};

class Solver
{
protected:
  mutable bool thinking = false;
  mutable bool infinite = false;
  mutable bool verbose = true;

public:
  Solver() {}

  virtual bool is_solver() { return 0; }
  virtual void new_game() {}
  virtual Move get_move(const SearchCfg & cfg) = 0;
  virtual void make(Move move) = 0;
  virtual void set(const Board & board) = 0;
  virtual u64 perft(int depth) { return 0; }
  virtual int plegt() { return 0; }
  virtual int eval() { return 0; }
  void stop() { thinking = false; }
  void set_analysis(bool val) { infinite = val; }
  void set_verbosity(bool val) { verbose = val; }
};

class Reader : public Solver
{
public:
  Reader() {}
  Move get_move(const SearchCfg & cfg) override { return Move(); }
  void make(Move move) override {}
  void set(const Board & board) override {}
};

}
