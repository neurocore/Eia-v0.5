#include <fstream>
#include <utility>
#include <algorithm>
#include "epd.h"
#include "tuning.h"
#include "engine.h"

using namespace std;

namespace eia {

TunerDynamic::TunerDynamic(TunerCfg cfg) : cfg(cfg)
{
  E[0] = new Eval;
  E[1] = new Eval;
  S[0] = new SolverPVS(E[0]);
  S[1] = new SolverPVS(E[1]);
  S[0]->set_verbosity(false);
  S[1]->set_verbosity(false);
}

TunerDynamic::~TunerDynamic()
{
  delete S[1];
  delete S[0];
}

void TunerDynamic::init(std::string book_pgn)
{
  book.clear();
  BookReader(&book).read_pgn(book_pgn);
}

double TunerDynamic::score(const Eval & eval)
{
  int played = 0;
  double total = 0.0;

  E[0]->set(eval);
  E[0]->init();

  for (int i = 0; i < cfg.games; ++i)
  {
    const int side = i & 1;
    int result = play_game(side);

#ifdef _DEBUG
    /*if (verbose)*/ { say("\n"); B.print(); }
#endif

    if (side) result = -result;
    total += result;

    if (cfg.verbose) say("{}", "-=+"[result + 1]);
  }

  total /= played ? played : 1;

  return total;
}

// Returns -1 for black win, +1 for white win, 0 for draw
int TunerDynamic::play_game(int side)
{
  const auto s_cfg = SearchCfg{.depth = cfg.depth};
  vector<int> vals[2];

  B.set();
  S[0]->set(B);
  S[1]->set(B);

  S[0]->new_game();
  S[1]->new_game();

  // Playing random opening (symmetrical)

  if (!side || opening.empty()) 
    opening = book.get_random_line();

  for (Move move : opening)
  {
    B.make(move);
    B.revert_states();
    S[0]->make(move);
    S[1]->make(move);
    side = 1 - side;
  }

  // Actual playing a game

  for (int stm = side;; stm = 1 - stm)
  {
    assert(B.state.bhash == S[stm]->get_hash());

    Move move = S[stm]->get_move(s_cfg);
    int val = S[stm]->get_best_val();

    // Direct mate on board

    if (val == -Val::Inf) return -1;
    if (val ==  Val::Inf) return  1;

    //B.print();

    vals[stm].push_back(val);
    B.make(move);
    B.revert_states();
    S[0]->make(move);
    S[1]->make(move);

    // Finishing game by draw

    if (B.is_draw()) return 0; // seems like 3-fold not working?
    if (vals[0].size() > 256) return 0;

    // Finishing game by mate

    if (vals[0].size() > 0
    &&  vals[1].size() > 0)
    {
      int last0 = vals[0][vals[0].size() - 1];
      int last1 = vals[1][vals[1].size() - 1];

      if (last0 < -Val::Mate && last1 < -Val::Mate)
      {
        return -1;
      }
      if (last0 >  Val::Mate && last1 >  Val::Mate)
      {
        return  1;
      }
    }

    // Finishing game by adjucation (color bug)

    /*if (vals[0].size() > adj_cnt
    &&  vals[1].size() > adj_cnt)
    {
      bool adj_w = true;
      bool adj_b = true;

      for (int k = 1; k <= adj_cnt; ++k)
      {
        int last0 = vals[0][vals[0].size() - k];
        int last1 = vals[1][vals[1].size() - k];

        if (last0 <  adj_val || last1 <  adj_val) adj_w = false;
        if (last0 > -adj_val || last1 > -adj_val) adj_b = false;
      }

      if (adj_w)
      {
        return  1;
      }
      if (adj_b)
      {
        return -1;
      }
    }*/
  }
  return 0;
}

/////////////////////////////////////

Vals TunerStatic::score(Tune v1, Tune v2)
{
  static Eval E1, E2;
  E1.set(v1);
  E2.set(v2);
  double s1 = score(E1);
  double s2 = score(E2);
  return make_pair(s1, s2);
}

int TunerStatic::open_csv(string file)
{
  ifstream f_csv(file);
  if (!f_csv.is_open()) return 0;

  string line;
  while (f_csv)
  {
    getline(f_csv, line);
    string fen = cut(line, ";");
    string eval = cut(line, ";");
    int result = parse_int(line);

    poss.push_back({ fen, result });
  }
  return 1;
}

int TunerStatic::open_epd(string file, int result_cn)
{
  Epd epd;
  if (!epd.read(file)) return 0;

  const auto & problems = epd.get_problems();
  for (const auto & p : problems)
  {
    if (!p.valid) continue;

    const string & result = p.comment[result_cn];
    int r = result.starts_with("1-0")
          - result.starts_with("0-1");

    poss.push_back({ p.fen, r });
  }
  return 1;
}

double TunerStatic::score(Eval & E)
{
  const int total = poss.size();
  const int batch = (std::min)(10'000, total);
  int debug = 0;
  double loss = 0.0;

  for (int i = 0; i < batch; i++)
  {
    const auto & pos = poss[(index + i) % total];
    B.set(pos.fen);
    Val val = E.eval(&B, -Val::Inf, Val::Inf);

    double v = dry_double(val);
    double r = B.color ? pos.result : -pos.result;
    double result = (r + 1) / 2.0; // [0; 1]
    double diff = sigmoid(v, 1.1) - result;
    loss += diff * diff;

    if (debug --> 0)
    {
      log("val = {}, result = {}, diff = {}\n", val, result, diff);
    }
  }
  index += batch;
  return sqrt(loss / batch);
}

double TunerStatic::score(string str)
{
  log("{}\n", str);
  Eval E;
  E.set_raw(str);
  return score(E);
}

/////////////////////////////////////

SPSA::SPSA(std::unique_ptr<Tuner> tuner,
           int    max_iters,
           double lrate_init,
           double perturb_init,
           double stability_const)
  : tuner(std::move(tuner)), gen(random_device{}()), distr(0, 1)
  , iters(max_iters), a(lrate_init), c(perturb_init), A0(stability_const)
{
}

void print(const Tune & T)
{
  log("[");
  for (int i = 0; i < T.size(); i++)
    log("{:.4f}, ", T[i]);
  log("\b\b]");
}

void SPSA::start()
{
  auto bounds = tuner->get_bounds();
  size_t N = bounds.size();

  log("-- Starting eval tuning\n\n");
  log("Total parameters: {}\n", N);

  // 0. Init starting point

  Tune u(N), u1(N), u2(N), delta(N);
  for (int i = 0; i < N; i++)
    u[i] = .5;

  // 1. Iterate

  for (int k = 0; k < iters; k++)
  {
    bool report = (k % 200) == 0;
    double ak = a / pow(k + 1 + A0, alpha);
    double ck = c / pow(k + 1, gamma);

    if (report)
    {
      log("\nIteration #{}\n\n", k);
      log("ak = {:.4f}, ck = {:.4f}\n", ak, ck);
      log("u = "); print(u); log("\n");
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

    auto [s1, s2] = tuner->score(u1, u2);
    double grad = (s1 - s2) / ck / 2;

    if (report)
    {
      log("J(u + ck * delta) = {:.4f}\n", s1);
      log("J(u - ck * delta) = {:.4f}\n", s2);
      log("ak * grad = {:f}\n", ak * grad);
    }

    for (int i = 0; i < N; i++)
      u[i] -= ak * grad / delta[i];
  }
}

}
