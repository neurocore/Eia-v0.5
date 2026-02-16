#include <fstream>
#include <utility>
#include <algorithm>
#include "epd.h"
#include "tuning.h"
#include "engine.h"

using namespace std;

#ifdef PY_BINDINGS
// Engine implements python3 bindings
//  for automated evaluation tuning
//  (such as CMA-ES, PSO or any other)
//  via interop library 'pybind11'
// 
// Note that in this case we building
//  shared library to use it in python
//
// Don't forget to include paths to
//  pybind11 itself and python.h
//  and path to python.lib
//
// Also, resulting eia_tuner.dll
//  must be renamed to pyd extension
//  and should be put in python/dlls

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

unique_ptr<eia::TunerStatic> g_tuner;

eia::Tune load(string file, int batch_sz)
{
  auto loss = make_unique<eia::MSE>();
  g_tuner = make_unique<eia::TunerStatic>(std::move(loss), batch_sz);
  g_tuner->open(file);

  eia::log("Positions: {}\n", g_tuner->size());

  return g_tuner->get_init();
}

double score(eia::Tune v)
{
  return g_tuner->score(v).loss;
}

PYBIND11_MODULE(eia_tuner, m)
{
  m.doc() = "pybind11 plugin for Eia v0.5 chess engine eval tuning";
  m.def("load", &load, "Load dataset with given batch size");
  m.def("score", &score, "Returns a loss score for tune");
}
#endif


namespace eia {

Score TunerStatic::score(Tune v)
{
  static Eval E;
  E.set(v);

  const int total = poss.size();
  double loss = 0.0;
  double grad = 0.0;
  int debug = 0;

  for (int i = 0; i < batch_sz; i++)
  {
    const auto & pos = poss[(index + i) % total];
    B.set(pos.fen);
    Val val = E.eval(&B, -Val::Inf, Val::Inf, false);

    const double r = B.color ? pos.result : -pos.result;
    const double y = (r + 1) / 2.0;
    const double s = sigmoid(dry_double(val), 1.0);

    loss += L->f(y, s);
    grad += L->df(y, s);

    if (debug --> 0)
    {
      log("val = {}, result = {}\n", val, y);
    }
  }

  loss /= batch_sz;
  grad /= batch_sz;

  return Score(loss, grad * v);
}

bool TunerStatic::open(string file)
{
  auto parts = split(file, ".");
  if (parts.size() < 2)
  {
    log("Allowed only 'csv' and 'epd' extensions\n");
    return false;
  }

  std::string ext = parts[parts.size() - 1];

  if (ext == "csv")
  {
    log("Reading csv...\n");
    if (!open_csv(file))
    {
      log("Error in reading file\n");
      return false;
    }
  }
  else if (ext == "epd")
  {
    log("Reading epd...\n");
    if (!open_epd(file))
    {
      log("Error in reading file\n");
      return false;
    }
  }
  else if (ext == "book")
  {
    log("Reading book...\n");
    if (!open_book(file))
    {
      log("Error in reading file\n");
      return false;
    }
  }

  if (size() < 1)
  {
    log("There is no any position in dataset\n");
    return false;
  }

  return true;
}

bool TunerStatic::open_csv(string file)
{
  ifstream f_csv(file);
  if (!f_csv.is_open()) return false;

  string line;
  while (f_csv)
  {
    getline(f_csv, line);
    string fen = cut(line, ";");
    string eval = cut(line, ";");
    int result = parse_int(line); // eval?

    poss.push_back({ fen, result });
  }
  return true;
}

bool TunerStatic::open_epd(string file)
{
  Epd epd;
  if (!epd.read(file)) return false;

  const auto & problems = epd.get_problems();
  for (const auto & p : problems)
  {
    if (!p.valid) continue;

    const string & result = p.comment[9];
    const int r = result.starts_with("1-0")
                - result.starts_with("0-1");

    poss.push_back({ p.fen, r });
  }
  return true;
}

bool TunerStatic::open_book(string file)
{
  ifstream f_book(file);
  if (!f_book.is_open()) return false;

  string line;
  while (f_book)
  {
    getline(f_book, line);
    string fen = cut(line, "[");
    string eval = cut(line, "]");
    int result = eval.starts_with("1.0")
               - eval.starts_with("0.0");

    if (fen.empty()) continue;

    poss.push_back({ fen, result });
  }
  return true;
}

/////////////////////////////////////

SPSA::SPSA(std::unique_ptr<Tuner> tuner,
           int    max_iters,
           double lrate_init,
           double perturb_init,
           double stability_const)
  : tuner(std::move(tuner)), gen(random_device{}()), distr(0, 1)
  , iters(max_iters), a(lrate_init), c(perturb_init), A0(stability_const)
{}

void SPSA::start()
{
  auto bounds = tuner->get_bounds();
  size_t N = bounds.size();

  log("-- Starting eval tuning (SPSA)\n\n");
  log("Total parameters: {}\n", N);

  // 0. Init starting point

  Tune u(N), u1(N), u2(N), delta(N);
  u = tuner->get_init();;
  /*for (int i = 0; i < N; i++)
    u[i] = .5;*/

  // 1. Iterate

  for (int k = 0; k < iters; k++)
  {
    bool report = (k % 10) == 0;
    double ak = a / pow(k + 1 + A0, alpha);
    double ck = c / pow(k + 1, gamma);

    if (report)
    {
      log("\nIteration #{}\n\n", k);
      log("ak = {:.4f}, ck = {:.4f}\n", ak, ck);
      log("u = {}\n", u);
      log("{}\n", tuner->to_string(u));
    }

    for (int i = 0; i < N; i++)
      delta[i] = rand_rademacher();

    //if (report) log("delta = "); print(delta); log("\n");

    for (int i = 0; i < N; i++)
    {
      u1[i] = u[i] + ck * delta[i];
      u2[i] = u[i] - ck * delta[i];
    }

    Score s1 = tuner->score(u1);
    Score s2 = tuner->score(u2);
    tuner->next_iter();

    double score = s1.loss - s2.loss;
    double grad = score / ck / 2;

    if (report)
    {
      log("J(u + ck * delta) = {:+.4f}\n", s1.loss);
      log("J(u - ck * delta) = {:+.4f}\n", s2.loss);
      log("ak * grad = {:+f}\n", ak * grad);
    }

    for (int i = 0; i < N; i++)
      u[i] -= ak * grad / delta[i];
  }
}

/////////////////////////////////////

DiffEvo::DiffEvo(unique_ptr<Tuner> tuner, int num, double cr, double f)
  : tuner(std::move(tuner)), num(num), cr(cr), f(f)
{}



/////////////////////////////////////

Adam::Adam(std::unique_ptr<Tuner> tuner, int max_iters,
           double alpha, double beta1, double beta2,
           double eps, int t)
  : tuner(move(tuner)), iters(max_iters), alpha(alpha)
  , beta1(beta1), beta2(beta2), eps(eps), t(t)
{}

void Adam::start()
{
  auto bounds = tuner->get_bounds();
  size_t N = bounds.size();

  log("-- Starting eval tuning (Adam)\n\n");
  log("Total parameters: {}\n", N);

  // 0. Init starting points

  Tune w(N), m(N), v(N);
  w = Eval{}.to_tune();
  m.resize(N, 0.);
  v.resize(N, 0.);

  // 1. Iterate

  for (int k = 0; k < iters; k++)
  {
    t++;
    bool report = (k % 100) == 0;

    Score s = tuner->score(w);
    tuner->next_iter();

    if (report)
    {
      log("\nIteration #{}\n\n", k);
      log("w = {}\n", w);
      log("{}\n", tuner->to_string(w));
      log("Loss = {:+.4f}\n", s.loss);
    }

    for (size_t i = 0; i < N; i++)
    {
      m[i] = beta1 * m[i] + (1. - beta1) * s.grad[i];
      v[i] = beta2 * v[i] + (1. - beta2) * s.grad[i] * s.grad[i];

      double m_hat = m[i] / (1. - std::pow(beta1, t));
      double v_hat = v[i] / (1. - std::pow(beta2, t));

      w[i] -= alpha * m_hat / (std::sqrt(v_hat) + eps);
    }
  }
}

}
