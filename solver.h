#pragma once
#include "moves.h"
#include "movepicker.h"

namespace eia {

struct Board;
class Engine;

struct Undo
{
  State state;
  MovePicker mp;
  // Vals pst;
  Move curr, best;
};

class Solver
{
protected:
  Engine * engine = nullptr;
  mutable bool thinking = false;
  mutable bool infinite = false;

public:
  Solver(Engine * engine) : engine(engine) {}

  virtual Move get_move(MS time) = 0;
  virtual void set(const Board & board) = 0;
  virtual u64 perft(int depth) { return 0; }
  void stop() { thinking = false; }
  void set_analysis(bool val) { infinite = val; }
};

class Reader : public Solver
{
public:
  Reader(Engine * engine) : Solver(engine) {}
  Move get_move(MS time) override { return Move(); }
  void set(const Board & board) override {}
};

}
