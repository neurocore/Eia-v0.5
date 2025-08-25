#pragma once
#include <vector>
#include <random>
#include <memory>
#include "solver_pvs.h"
#include "eval.h"
#include "book.h"

using ProbVec = std::vector<double>;

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
  void init(std::string book_pgn = Tune::Book);
  double score(const Eval & eval);

private:
  int play_game(int side);
};


class PBIL
{
  std::unique_ptr<Tuner> tuner;
  ProbVec prob;
  int bits;
  int iters;
  int pop_n;
  double lrate = 0.1;
  double nrate = 0.075;
  double mut_prob  = 0.02;
  double mut_shift = 0.05;

public:
  PBIL(std::unique_ptr<Tuner> tuner, int bits, int iters = 1000, int pop_n = 100);
  void start();

private:
  Genome gen_genome();
  double score(const Genome & genome);

  inline double rand()      { return distr(gen); }
  inline double rand_bool() { return rand() > .5 ? 1. : 0.; }

  std::mt19937 gen;
  std::uniform_real_distribution<double> distr;
};

}
