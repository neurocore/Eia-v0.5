#pragma once
#include <vector>
#include <random>
#include <memory>
#include "solver_pvs.h"
#include "eval.h"
#include "book.h"

namespace eia {

using std::string;
using std::vector;

// Abstract Tuner (estimates tunes)

class Tuner
{
public:
  virtual double score(Tune v1, Tune v2) = 0;
  virtual Bounds get_bounds() const = 0;
  virtual double max_score() const = 0;
  virtual string to_string(Tune v) = 0;
};


// This tuner fits tune to position-result dataset

struct PosResult
{
  string fen;
  int result;
};

enum Loss
{
  MSE, // Mean Squared Error - classical loss function
  BCE  // Binary Cross-Entropy - may be more expressed
};

class TunerStatic : public Tuner
{
  Board B;
  vector<PosResult> poss;
  int batch_sz, index = 0;
  Loss loss_type;

public:
  TunerStatic(Loss loss_type = MSE, int batch_size = 10'000)
    : loss_type(loss_type), batch_sz(batch_size)
  {}

  double score(Tune v1, Tune v2) override;
  Bounds get_bounds() const override { return Eval{}.bounds(); }
  string to_string(Tune v) override { return Eval(v).to_string(); }
  double max_score() const override { return 100.0 * size(); }

  size_t size() const { return poss.size(); }
  int open_csv(string file);
  int open_epd(string file, int result_cn = 9);
  int open_book(string file);
  double score(string str);

private:
  double score(Eval & E, bool shift_batch = true);
  double mse(Tune result, Tune predicted);
  double bce(Tune result, Tune predicted);
};


// This tuner plays matches to score tune

struct TunerCfg
{
  bool verbose = true;
  int games = 20;
  int depth = 3;
  int adj_cnt = 4;
  int adj_val = 800;
};

class TunerDynamic : public Tuner
{
  Board B;
  Book book;
  Moves opening;
  SolverPVS * S[2];
  Eval * E[2];

public:
  double score(Tune v1, Tune v2) override;
  Bounds get_bounds() const override { return Eval{}.bounds(); }
  string to_string(Tune v) override { return Eval(v).to_string(); }
  double max_score() const override { return 100.0 * cfg.games; }

  TunerCfg cfg;
  TunerDynamic(TunerCfg cfg = TunerCfg());
  ~TunerDynamic();
  void init(std::string book_pgn = Tunes::Book);
  double score(const Eval & eval);

private:
  int play_game(int side);
};


// Simultaneous Perturbation Stochastic Approximation (SPSA)

class SPSA
{
  const double alpha = .602; // power of learning rate
  const double gamma = .101; // power of perturbation

  std::unique_ptr<Tuner> tuner;
  double a, c, A0;
  int iters;

public:
  SPSA(std::unique_ptr<Tuner> tuner,
       int    max_iters = 1000,
       double lrate_init = .1,
       double perturb_init = .1,
       double stability_const = 100);

  void start();

private:
  inline double rand() { return distr(gen); }
  inline double rand_rademacher() { return rand() > .5 ? +1. : -1.; }

  std::uniform_real_distribution<double> distr;
  std::mt19937 gen;
};

}
