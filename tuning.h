#pragma once
#ifdef TUNING
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

const double Lambda = 0; //1e-6;


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
  const double k;

public:
  explicit MSE(double k0 = Tunes::K100) : k(k0) {}

  double f(double y, double s) const override
  {
    const double diff = y - s;
    return diff * diff;
  }

  double df(double y, double s) const override
  {
    const double df_ds = 2 * (s - y);     // ((y - s)^2)' = -2(y - s)
    const double ds_dv = k * s * (1 - s); // s'(kv) = k * s * (1 - s)
    return df_ds * ds_dv; // chain rule (don't forget dE/dv itself)
  }

  string name() const override { return "MSE"; }
};


// Binary Cross-Entropy - may be more expressed

class BCE : public Loss
{
  const double k, eps;

public:
  explicit BCE(double k0 = Tunes::K100, double eps = 1e-15)
    : k(k0), eps(eps) {}

  double f(double y, double s) const override
  {
    double q = std::clamp(s, eps, 1. - eps);
    return      -y  * std::log(q)
         - (1. - y) * std::log(1. - q);
  }

  double df(double y, double s) const override
  {
    double q = std::clamp(s, eps, 1. - eps);
    // df/ds  = -y / s - (1 - y)/(1 - s)
    // s'(kv) = k * s * (1 - s)
    // , so
    // 
    // -k * (y(1 - s) - (1 - y)s) = k(s - y)

    return k * (q - y); // don't forget dE/dv itself
  }

  string name() const override { return "BCE"; }
};


// --------------------------------------------------------------------
//  Data providers
// --------------------------------------------------------------------

struct PosResult
{
  string fen;
  int result;
};

class DataProvider
{
  vector<PosResult> & poss;

public:
  DataProvider(vector<PosResult> & poss) : poss(poss) {}
  bool open(string file);

private:
  bool open_csv(string file);
  bool open_epd(string file);
  bool open_book(string file);
};


struct PSQ { Piece p; SQ sq; };

struct PSTMatResult
{
  vector<PSQ> psq;
  Val eval;
  int phase;
  int result;
};

class PSTMatConverter
{
  vector<PSTMatResult> & data;
  const vector<PosResult> & poss;
  mutable Board B;

public:
  PSTMatConverter(const vector<PosResult> & poss, vector<PSTMatResult> & data)
    : poss(poss), data(data)
  {}

  void convert() const;

private:
  PSTMatResult convert(const PosResult & pr) const;
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
  virtual Score  score(const Tune & v, double k0 = 0.) = 0;
  virtual void   next_iter() = 0;
  virtual Bounds get_bounds() const = 0;
  virtual Tune   get_init() const = 0;
  virtual string to_string(const Tune & v) = 0;
  virtual bool   open(string file) = 0;
  virtual size_t batch_n() const = 0;
  virtual size_t size() const = 0;
};


// Static tuner - fits tune to position-result dataset

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

  Score  score(const Tune & v, double k0 = 0.) override;
  void   next_iter() override { index = (index + batch_sz) % size(); }
  Bounds get_bounds() const override { return E->bounds(); }
  Tune   get_init() const override { return E->to_tune(); }
  string to_string(const Tune & v) override { return Eval(v).to_string(); }
  bool   open(string file) override { return DataProvider(poss).open(file); }
  size_t batch_n() const { return batch_sz ? batch_sz : size(); }
  size_t size() const { return poss.size(); }

private:
  Tune calc_dE(const Trace & trace) const;
};


// Cached tuner - fits tune to position-result dataset
// Shrinking traces removing params that are not activated in position

// amount: [ idx val ] [ idx val ] [ idx val ] [ idx val ] [ idx val ] ...
// posidx:  ^- [ 0 3 rho phi rest wdl ] ...     ^- [ 3 7 rho phi rest wdl ] ...

struct PosIndex
{
  int offset, size;
  float rho, phi;
  float rest, wdl;
};

struct Amount
{
  int index;
  float val;
};

class TunerCached : public Tuner
{
  Board B;
  unique_ptr<Loss> L;
  vector<PosIndex> posis;
  vector<Amount> amounts;

public:
  TunerCached(unique_ptr<Loss> loss_fn) : L(move(loss_fn)) {}

  Score  score(const Tune & v, double k0 = 0.) override;
  void   next_iter() override {}
  Bounds get_bounds() const override { return E->bounds(); }
  Tune   get_init() const override { return E->to_tune(); }
  string to_string(const Tune & v) override { return Eval(v).to_string(); }
  bool   open(string file) override;
  size_t batch_n() const { return size(); }
  size_t size() const { return posis.size(); }

private:
  double calc_eval(const Tune & v, int pos_idx) const;
  Tune   calc_dE(const Tune & v, int pos_idx) const;
};


// Piece-Square Tables tuner

class TunerPST : public Tuner
{
  unique_ptr<Loss> L;
  vector<PSTMatResult> data;
  int batch_sz, index = 0;

public:
  TunerPST(unique_ptr<Loss> loss_fn, int batch_size = 10'000)
    : L(move(loss_fn)), batch_sz(batch_size)
  {}

  Score  score(const Tune & v, double k0 = 0.) override;
  void   next_iter() override {}
  Bounds get_bounds() const override { return Eval{}.bounds(); }
  Tune   get_init() const override { return Tune{}; }
  string to_string(const Tune & v) override { return "[too lazy]"; }
  size_t batch_n() const { return batch_sz; }
  size_t size() const { return data.size(); }
  bool   open(string file) override
  {
    vector<PosResult> poss;
    bool success = DataProvider(poss).open(file);
    if (!success) return false;

    PSTMatConverter converter(poss, data);
    converter.convert();
    return true;
  }
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


// Adaptive Gradient optimizer (AdaGrad)

class AdaGrad
{
  unique_ptr<Tuner> tuner;
  double lrate, lambda, eps;
  int iters;

public:
  AdaGrad(unique_ptr<Tuner> tuner,
          double learning_rate = 0.01,
          double eps = 1e-8);

  void start();
};

// --------------------------------------------------------------------
//  Utilities
// --------------------------------------------------------------------

extern double find_k(unique_ptr<Tuner> tuner, Tune v, double a, double b, double eps = 1e-6);

static Tune operator * (double val, const Tune & v)
{
  Tune r;
  for (int i = 0; i < Param_N; i++)
  {
    r.param[i][0] = val * v.param[i][0];
    r.param[i][1] = val * v.param[i][1];
  }
  return r;
}

static Tune operator / (const Tune & v, double val)
{
  Tune r;
  for (int i = 0; i < Param_N; i++)
  {
    r.param[i][0] = val / v.param[i][0];
    r.param[i][1] = val / v.param[i][1];
  }
  return r;
}

static Tune operator / (double val, const Tune & v)
{
  Tune r;
  for (int i = 0; i < Param_N; i++)
  {
    r.param[i][0] = v.param[i][0] / val;
    r.param[i][1] = v.param[i][1] / val;
  }
  return r;
}

static Tune operator * (const Tune & v, const Tune & w)
{
  Tune r;
  for (int i = 0; i < Param_N; i++)
  {
    r.param[i][0] = v.param[i][0] * w.param[i][0];
    r.param[i][1] = v.param[i][1] * w.param[i][1];
  }
  return r;
}

static Tune operator + (const Tune & v, const Tune & w)
{
  Tune r;
  for (int i = 0; i < Param_N; i++)
  {
    r.param[i][0] = v.param[i][0] + w.param[i][0];
    r.param[i][1] = v.param[i][1] + w.param[i][1];
  }
  return r;
}

static Tune operator - (const Tune & v, const Tune & w)
{
  Tune r;
  for (int i = 0; i < Param_N; i++)
  {
    r.param[i][0] = v.param[i][0] - w.param[i][0];
    r.param[i][1] = v.param[i][1] - w.param[i][1];
  }
  return r;
}

static Tune operator + (const Tune & v, double val)
{
  Tune r;
  for (int i = 0; i < Param_N; i++)
  {
    r.param[i][0] = v.param[i][0] + val;
    r.param[i][1] = v.param[i][1] + val;
  }
  return r;
}

static Tune operator - (const Tune & v, double val)
{
  Tune r;
  for (int i = 0; i < Param_N; i++)
  {
    r.param[i][0] = v.param[i][0] - val;
    r.param[i][1] = v.param[i][1] - val;
  }
  return r;
}

static Tune & operator += (Tune & v, const Tune & w)
{
  for (int i = 0; i < Param_N; i++)
  {
    v.param[i][0] += w.param[i][0];
    v.param[i][1] += w.param[i][1];
  }
  return v;
}

static Tune & operator -= (Tune & v, const Tune & w)
{
  for (int i = 0; i < Param_N; i++)
  {
    v.param[i][0] -= w.param[i][0];
    v.param[i][1] -= w.param[i][1];
  }
  return v;
}

static Tune & operator *= (Tune & v, double val)
{
  for (int i = 0; i < Param_N; i++)
  {
    v.param[i][0] *= val;
    v.param[i][1] *= val;
  }
  return v;
}

static Tune & operator /= (Tune & v, double val)
{
  for (int i = 0; i < Param_N; i++)
  {
    v.param[i][0] /= val;
    v.param[i][1] /= val;
  }
  return v;
}

static double scalar_mult(const Tune & v, const Tune & w)
{
  double val = 0.;
  for (int i = 0; i < Param_N; i++)
  {
    val += v.param[i][0] * w.param[i][0];
    val += v.param[i][1] * w.param[i][1];
  }
  return val;
}

static Tune adagrad_update(f64 lrate, f64 eps, const Tune & g, const Tune & grad)
{
  Tune r;
  for (int i = 0; i < Param_N; i++)
  {
    r.param[i][0] = lrate * grad.param[i][0] / (std::sqrt(g.param[i][0] + eps));
    r.param[i][1] = lrate * grad.param[i][1] / (std::sqrt(g.param[i][1] + eps));
  }
  return r;
}

}

template<>
struct std::formatter<eia::Tune> : std::formatter<std::string>
{
  auto format(const eia::Tune & v, std::format_context & ctx) const
  {
    std::string str;

    str = "[";
    for (int i = 0; i < eia::Param_N; i++)
    {
      str += "{"  + std::format("{:+.4f}", v.param[i][0])
          +  ", " + std::format("{:+.4f}", v.param[i][1]) + "}, ";
    }
    str = str.substr(0, str.size() - 2) + "]";

    return std::formatter<std::string>::format(str, ctx);
  }
};
#endif
