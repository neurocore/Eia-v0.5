#pragma once
#include <vector>
#include <random>
#include <memory>
#include "solver_pvs.h"
#include "eval.h"
#include "book.h"

namespace eia {

using std::unique_ptr;
using std::string;
using std::vector;


// --------------------------------------------------------------------
//  Estimators
// --------------------------------------------------------------------

class Loss
{
public:
  virtual ~Loss() = default;
  virtual double f(double target, double predict) const = 0;
  virtual double df(double target, double predict) const = 0;
  virtual string name() const = 0;
};


// Mean Squared Error - classical loss function

class MSE : public Loss
{
  const double k = std::log(10.) / 400;

public:
  double f(double y, double s) const override
  {
    const double diff = y - s;
    return diff * diff;
  }

  double df(double y, double s) const override
  {
    const double df_ds = -2 * (y - s);    // ((y - s)^2)' = -2(y - s)
    const double ds_dv = k * s * (1 - s); // s'(kv) = k * s * (1 - s)
    return df_ds * ds_dv; // chain rule
  }

  string name() const override { return "MSE"; }
};


// Binary Cross-Entropy - may be more expressed

class BCE : public Loss
{
  double eps;

public:
  explicit BCE(double eps = 1e-15) : eps(eps) {}

  double f(double y, double s) const override
  {
    double q = std::clamp(s, eps, 1. - eps);
    return       -y  * std::log(q)
          - (1. - y) * std::log(1. - q);
  }

  double df(double y, double s) const override
  {
    double q = std::clamp(s, eps, 1. - eps);
    return (q - y) / (q * (1. - q));
  }

  string name() const override { return "BCE"; }
};


// --------------------------------------------------------------------
//  Tuners
// --------------------------------------------------------------------

struct Score
{
  double loss;
  Tune grad;
};

class Tuner
{
public:
  virtual ~Tuner() {}
  virtual Score  score(Tune v) = 0;
  virtual void   next_iter() = 0;
  virtual Bounds get_bounds() const = 0;
  virtual string to_string(Tune v) = 0;
};


// Static tuner - fits tune to position-result dataset

struct PosResult
{
  string fen;
  int result;
};

class TunerStatic : public Tuner
{
  Board B;
  unique_ptr<Loss> L;
  vector<PosResult> poss;
  int batch_sz, index = 0;

public:
  TunerStatic(unique_ptr<Loss> loss_fn, int batch_size = 10'000)
    : L(move(loss_fn)), batch_sz(batch_size)
  {}

  Score  score(Tune v) override;
  void   next_iter() override { index = (index + batch_sz) % poss.size(); }
  Bounds get_bounds() const override { return Eval{}.bounds(); }
  string to_string(Tune v) override { return Eval(v).to_string(); }

  size_t size() const { return poss.size(); }
  bool open(string file);

private:
  bool open_csv(string file);
  bool open_epd(string file);
  bool open_book(string file);
};


// --------------------------------------------------------------------
//  Optimizers
// --------------------------------------------------------------------

// Simultaneous Perturbation Stochastic Approximation (SPSA)

class SPSA
{
  const double alpha = .602; // power of learning rate
  const double gamma = .101; // power of perturbation

  unique_ptr<Tuner> tuner;
  double a, c, A0;
  int iters;

public:
  SPSA(unique_ptr<Tuner> tuner,
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


// Adaptive Moment Estimation optimizer (Adam)

class Adam
{
  unique_ptr<Tuner> tuner;
  double alpha, beta1, beta2, eps;
  int iters, t;

public:
  Adam(unique_ptr<Tuner> tuner,
       int max_iters = 100'000,
       double alpha = 0.001,
       double beta1 = 0.9,
       double beta2 = 0.999,
       double eps   = 1e-8,
       int t = 0);

  void start();
};

}
