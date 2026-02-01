#pragma once
#include <vector>
#include <random>
#include <memory>
#include "solver_pvs.h"
#include "eval.h"
#include "book.h"

namespace eia {

struct TunerCfg
{
  bool verbose = true;
  int games = 20;
  int depth = 3;
  int adj_cnt = 4;
  int adj_val = 800;
};

class Tuner
{
  Board B;
  Book book;
  Moves opening;
  SolverPVS * S[2];
  Eval * E[2];

public:
  TunerCfg cfg;
  Tuner(TunerCfg cfg = TunerCfg());
  ~Tuner();
  void init(std::string book_pgn = Tunes::Book);
  double score(const Eval & eval);

private:
  int play_game(int side);
};

}
