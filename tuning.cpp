#ifdef TUNING
#include <fstream>
#include <utility>
#include <algorithm>
#include <omp.h>
#include "epd.h"
#include "tuning.h"
#include "engine.h"

using namespace std;

namespace eia {

// --------------------------------------------------------------------
//  Data providers
// --------------------------------------------------------------------

bool DataProvider::open(string file)
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

  if (poss.size() < 1)
  {
    log("There is no any position in dataset\n");
    return false;
  }

  return true;
}

bool DataProvider::open_csv(string file)
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

bool DataProvider::open_epd(string file)
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

bool DataProvider::open_book(string file)
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

void PSTMatConverter::convert() const
{
  for (auto pos : poss)
    data.push_back(convert(pos));
}

PSTMatResult PSTMatConverter::convert(const PosResult & pr) const
{
  vector<PSQ> psq;
  B.set(pr.fen);
  Val val = E->eval(&B, -Val::Inf, Val::Inf);

  for (int col = 0; col < 2; col++)
  {
    for (PieceType pt = Pawn; pt < PieceType_N; ++pt)
    {
      const Piece p = to_piece(pt, (Color)col);

      for (u64 bb = B.piece[p]; bb; bb = rlsb(bb))
      {
        SQ sq = bitscan(bb);
        psq.push_back(PSQ(p, sq));
      }
    }
  }

  return PSTMatResult
  {
    .psq = psq,
    .eval = B.color ? val : -val, // from white's perspective
    .phase = B.phase(),
    .result = pr.result // already right perspective (Ethereal)
  };
}


// --------------------------------------------------------------------
//  Tuners
// --------------------------------------------------------------------

// TunerStatic /////////////////////////////////

Score TunerStatic::score(const Tune & v, double k0)
{
  const double k = k0 ? k0 : Tunes::K100;
  const size_t total = size();
  const size_t N = batch_n();

  Tune grad{};
  double loss = 0.0;

  E->set(v);

  for (int i = 0; i < N; i++)
  {
    const auto & pos = poss[(index + i) % total];
    B.set(pos.fen);
    Val val = E->eval(&B, -Val::Inf, Val::Inf, false);
    const Trace & trace = E->get_trace();

    const double r = B.color ? pos.result : -pos.result;
    const double y = (r + 1) / 2.0;
    const double s = sigmoid(dry_double(val), k);

    loss += L->f(y, s);
    grad += L->df(y, s) * calc_dE(trace);
  }

  loss /= N;
  grad /= N;

  return Score(loss, grad);
}

Tune TunerStatic::calc_dE(const Trace & T) const
{
  Tune dE;
  for (int i = 0; i < Param_N; i++)
  {
    dE.param[i][0] = T.factor[0] * T.amount[i];
    dE.param[i][1] = T.factor[1] * T.amount[i];
  }
  return dE;
}


// TunerCached /////////////////////////////////

Score TunerCached::score(const Tune & v, double k0)
{
  const double k = k0 ? k0 : Tunes::K100;
  const size_t N = size();

  Tune grad{};
  double loss = 0.0;

  for (int i = 0; i < N; i++)
  {
    const double val = calc_eval(v, i);

    const double y = posis[i].wdl;
    const double s = sigmoid(val, k);

    loss += L->f(y, s);
    grad += L->df(y, s) * calc_dE(v, i);
  }

  loss /= N;
  grad /= N;

  // regularization
  loss += scalar_mult(v, v) * Lambda / 2;
  grad += Lambda * v;

  return Score(loss, grad);
}

bool TunerCached::open(string file)
{
  const int show_n = (1 << 15) - 1;

  vector<PosResult> poss;
  if (!DataProvider(poss).open(file)) return false;

  log("Total: {} positions\n\n", poss.size());
  log("Collecting eval traces...\n");

  posis.resize(poss.size());
  const Tune v = E->to_tune();
  //log("used tune: {}\n", v);

  int offset = 0;
  for (int j = 0; j < poss.size(); j++)
  {
    const auto & pos = poss[j];
    if (!(j & show_n)) log("{}", progress(1. * j / poss.size()));

    B.set(pos.fen);
    Val val = E->eval(&B, -Val::Inf, Val::Inf, false);
    double y = B.color ? dry_double(val) : -dry_double(val);
    const float wdl = (pos.result + 1) / 2.f;
    const Trace T = E->get_trace();
    posis[j] = { offset, 0, B.color, T.factor[0], T.factor[1], wdl, 0.f };

    int size = 0;
    for (int i = 0; i < Param_N; i++)
    {
      if (abs(T.amount[i]) > 1e-6)
      {
        amounts.push_back({i, T.amount[i]});
        size++;
      }
    }

    posis[j].size = size;
    posis[j].rest = y - calc_eval(v, j);

    offset += size;
  }
  log("{}\n\n", progress(1.));

  // Testing eval linearity

  //return true; // ---------------------------------------------------- !!!

  log("Testing eval linearity...\n");

  Tune w = v;
  //E->set(w);

  double max_err = 0;
  for (int j = 0; j < poss.size(); j++)
  {
    const auto & pos = poss[j];
    if (!(j & show_n)) log("{}", progress(1. * j / poss.size()));

    B.set(pos.fen);
    Val val = E->eval(&B, -Val::Inf, Val::Inf, false);
    const double y = B.color ? dry_double(val) : -dry_double(val);
    const double t = calc_eval(w, j);
    const double diff = abs(y - t);

    max_err = std::max(max_err, diff);

    if (diff > 1.)
    {
      const auto & P = posis[j];

      log("\n\n");
      log("{}\n", B.to_string());
      log("{}\n", pos.fen);
      log("Incorrect eval!\n");
      log("Original: {}\n", y);
      log("Calculated: {}\n", t);
      log("\n");
      log("f_op: {}\n", P.factor[0]);
      log("f_eg: {}\n", P.factor[1]);
      log("rest: {}\n", P.rest);
      log("wdl: {}\n", P.wdl);
      log("\n");

      //__debugbreak();
      //const double tt = calc_eval(w, j, true);

      return false;
    }
  }
  log("{}\n\n", progress(1.));
  log("Max error: {}\n\n", max_err);

  E->set(g_tune);

  return true;
}

// White's perspective position eval
double TunerCached::calc_eval(const Tune & v, int pos_idx, bool verbose) const
{
  const auto & P = posis[pos_idx];
  double op = 0., eg = 0.;

  for (int i = P.offset; i < P.offset + P.size; i++)
  {
    const int j = amounts[i].index;
    const double amount = amounts[i].val;
    op += v.param[j][0] * amount;
    eg += v.param[j][1] * amount;

    if (verbose)
    {
      log("{}: {} - {} / {}\n", j, amount, v.param[j][0], v.param[j][1]);
    }
  }

  if (verbose)
  {
    log("\nop = {}\n", op);
    log("eg = {}\n\n", eg);
  }

  const double mixed = op * P.factor[OP] + eg * P.factor[EG] + P.rest;

  return mixed + (P.color ? dry_double(Tempo) : -dry_double(Tempo));
}

Tune TunerCached::calc_dE(const Tune & v, int pos_idx) const
{
  const auto & P = posis[pos_idx];
  Tune dE{};

  for (int i = P.offset; i < P.offset + P.size; i++)
  {
    const int j = amounts[i].index;
    const double amount = amounts[i].val;
    dE.param[j][0] = (double)P.factor[0] * amount;
    dE.param[j][1] = (double)P.factor[1] * amount;
  }
  return dE;
}


// TunerPST ////////////////////////////////////

Score TunerPST::score(const Tune & v, double k0)
{
  const double k = k0 ? k0 : Tunes::K100;

  Duo pst[12][64];
  for (PieceType pt = Pawn; pt < PieceType_N; ++pt)
  {
    for (SQ sq = A1; sq < SQ_N; ++sq)
    {
      const Piece p = to_piece(pt, White);
      const int index = pt * 64 + sq;

      pst[p][sq].op = 100_cp * v.param[index][0];
      pst[p][sq].eg = 100_cp * v.param[index][1];

      pst[opp(p)][opp(sq)].op = -100_cp * v.param[index][0];
      pst[opp(p)][opp(sq)].eg = -100_cp * v.param[index][1];
    }
  }

  const size_t total = size();
  double loss = 0.0;
  int debug = 0;

  #pragma omp parallel for reduction(+:loss)
  for (int i = 0; i < total; i++)
  {
    const auto & pos = data[i];

    Duo duo{};

    for (auto const & [p, sq] : pos.psq)
      duo += pst[p][sq];

    Val val = pos.eval + duo.tapered(pos.phase);

    const double y = (pos.result + 1) / 2.0;
    const double s = sigmoid(dry_double(val), k);

    loss += L->f(y, s);
  }

  loss /= total;

  return Score(loss, {});
}


// --------------------------------------------------------------------
//  Optimizers
// --------------------------------------------------------------------

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

  log("-- Starting eval tuning (SPSA)\n\n");
  log("Total parameters: {}\n", Param_N);

  // 0. Init starting point

  Tune u, u1, u2, delta;
  //u = tuner->get_init();
  u = {0.074506,-0.030899,-0.120039,0.005563,0.980563,1.147090,1.068486,1.055019,1.029963,0.206775,0.500036,0.023737,0.210753,0.164393,0.343717,0.742357,0.301160,-0.418598,0.161018,0.233051,0.439673,0.858514,0.700427,0.467164,-0.432802,0.867044,0.199762,0.444517,0.752622,0.342763,0.724683,1.340209,1.119933,0.015337,0.100196,0.007525,-0.286378,0.494122,1.227117,0.920667,0.543169,0.383593,0.274269,0.200473,-1.412706,0.921979,0.007484,0.131884,0.478227,0.393790,0.256431,0.409250,-0.071533,0.134280,0.090487,0.210104,0.141156};
  //u = {0.037074,-0.088436,-0.174262,-0.074960,0.785410,1.007010,0.904238,0.872242,0.861856,-0.018162,0.496033,1.135750,0.273255,-0.568132,0.145085,0.636661,-0.045077,0.704088,-0.467398,0.057071,0.162683,-0.027680,0.063607,0.440003,-0.400178,0.388717,-0.005833,0.426431,0.060174,0.197362,0.301646,0.206722,0.660011,0.228416,-0.019413,-0.106902,0.234184,0.477068,0.312123,0.691386,-0.996932,0.367877,-0.466317,0.168993,0.435962,0.552339,0.510864,0.464921,0.590594,1.073799,-0.015324,0.242125,-0.006888,-0.271511,0.113451,1.163061,0.852508,0.521721,0.449050,0.332933,0.112919,-0.998922,0.648018,0.037484,0.153743,0.460426,0.383879,0.233696,0.387489,0.044967,0.190420,0.402252,0.216965,0.090511};
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

    for (int i = 0; i < Param_N; i++)
    {
      delta.param[i][0] = rand_rademacher();
      delta.param[i][1] = rand_rademacher();
    }

    //if (report) log("delta = "); print(delta); log("\n");

    u1 = u + ck * delta;
    u2 = u - ck * delta;

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

    u -= ak * grad / delta;
  }
}


AdaGrad::AdaGrad(std::unique_ptr<Tuner> tuner,
                 double learning_rate, double eps)
  : tuner(move(tuner)), lrate(learning_rate), eps(eps)
{}

void AdaGrad::start()
{
  const int iters = (int)tuner->size();
  const int batches = iters / (int)tuner->batch_n();

  log("-- Starting eval tuning (AdaGrad)\n\n");
  log("Total parameters: {}\n", Param_N);

  // 0. Init starting points

  Tune g{}, x = E->to_tune();

  // 1. Iterate

  for (int epoch = 0; epoch < iters; epoch++)
  {
    //if (!(epoch % 100))
    {
      log("\n -- Epoch #{} --\n\n", epoch);
      log("x = {}\n\n", x);
    }

    for (int batch = 0; batch < batches; batch++)
    {
      Score s = tuner->score(x);

      for (int i = 0; i < Param_N; i++)
      {
        if (!(i % 40)) log("\n");
        log("{}", "- +"[1 + sgn(s.grad.param[i][0])]);
        log("{}", "- +"[1 + sgn(s.grad.param[i][1])]);
      }
      log("\n\n");

      //if (!(epoch % 100))
      if (!(batch % 100))
      {
        log("Batch {}/{} | Loss = {:+.4f}\n", batch + 1, batches, s.loss);
      }

      g += s.grad * s.grad;
      x -= adagrad_update(lrate, eps, g, s.grad);

      //x.param[0][0] = +86.3783; // anchor pawn op material

      tuner->next_iter();
    }
    //log("g = {}\n\n", g);
  }
}

// Golden Section Search for optimal K that minimizes loss

double find_k(unique_ptr<Tuner> tuner, Tune v, double a, double b, double eps)
{
  static const double INV_PHI = .5 * (std::sqrt(5.) - 1.); // 1/phi
  double k1, k2;
  if (a > b) std::swap(a, b);

  k1 = b - INV_PHI * (b - a);
  k2 = a + INV_PHI * (b - a);

  double f1 = tuner->score(v, k1).loss;
  double f2 = tuner->score(v, k2).loss;

  log("k = {} | loss = {}\n", k1, f1);
  log("k = {} | loss = {}\n", k2, f2);

  while (std::abs(b - a) > eps)
  {
    if (f1 < f2)
    {
      b  = k2;
      k2 = k1;
      f2 = f1;
      k1 = b - INV_PHI * (b - a);
      f1 = tuner->score(v, k1).loss;
      log("k = {} | loss = {}\n", k1, f1);
    }
    else
    {
      a  = k1;
      k1 = k2;
      f1 = f2;
      k2 = a + INV_PHI * (b - a);
      f2 = tuner->score(v, k2).loss;
      log("k = {} | loss = {}\n", k2, f2);
    }
  }
  return (a + b) / 2.0;
}

}
#endif
