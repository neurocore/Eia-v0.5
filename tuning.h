#pragma once
#include <vector>
#include <random>
#include "solver_pvs.h"
#include "eval.h"

using ProbVec = std::vector<double>;

namespace eia {

class Tuning
{
  SolverPVS * S[2];
  Eval * E[2];
  ProbVec prob;
  int bits;
  int iters;
  int pop_n;
  int games;
  int depth = 3;
  int adj_cnt = 4;
  int adj_val = 800;
  double lrate = 0.1;
  double nrate = 0.075;
  double mut_prob  = 0.02;
  double mut_shift = 0.05;

public:
  Tuning(int bits, int iters = 1000, int pop_n = 100, int games = 20);
  ~Tuning();
  void start();

private:
  Board B;
  Genome gen_genome();
  inline double rand() { return distr(gen); }
  inline double rand_bool()
  {
    return static_cast<double>(rand() > .5);
  }

  double score(const Genome & genome);
  int play_game(const Genome & genome, int side);

  std::mt19937 gen;
  std::uniform_real_distribution<double> distr;
};

}
