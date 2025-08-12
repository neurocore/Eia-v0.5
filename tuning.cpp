#include <algorithm>
#include "tuning.h"
#include "engine.h"

using namespace std;

namespace eia {

Tuning::Tuning(int bits, int iters, int pop_n, int games)
  : bits(bits), iters(iters), pop_n(pop_n), games(games)
  , gen(random_device{}()), distr(0, 1)
{
  prob.resize(bits);
  E[0] = new Eval;
  E[1] = new Eval;
  S[0] = new SolverPVS(E[0]);
  S[1] = new SolverPVS(E[1]);
  S[0]->set_verbosity(false);
  S[1]->set_verbosity(false);
}

Tuning::~Tuning()
{
  delete S[1];
  delete S[0];
}

void Tuning::start()
{
  vector<Genome> colony;
  vector<double> cost;

  // 0. Build base probabilities vector

  for (int k = 0; k < bits; ++k)
    prob[k] = .5;

  Genome best;
  double best_cost = -100 * games;

  // 1. Generate few species

  for (int i = 1; i <= iters; ++i)
  {
    say("\nIteration #{}\n\n", i);

    cost.clear();
    colony.clear();
    for (int j = 0; j < pop_n; ++j)
      colony.push_back(gen_genome());

    // 2. Score population

    for (int j = 0; j < pop_n; ++j)
    {
      say("{:4d} ", j + 1);
      auto val = score(colony[j]);
      cost.push_back(val);
    }

    // 3. Find min/max species

    auto it = minmax_element(cost.begin(), cost.end());
    int min = static_cast<int>(it.first  - cost.begin());
    int max = static_cast<int>(it.second - cost.begin());
    const Genome & min_gene = colony[min];
    const Genome & max_gene = colony[max];

    // 4. Compare with previous best

    if (cost[max] > best_cost)
    {
      best_cost = cost[max];
      best = colony[max];
    }

    // 5. Update prob vec with min/max

    auto shift = [](double from, double to, double rate)
    {
      return from * (1 - rate) + to * rate;
    };

    for (int k = 0; k < bits; k++)
    {
      double rate = lrate + nrate * (min_gene[k] != max_gene[k]);
      prob[k] = shift(prob[k], max_gene[k], rate);
    }

    // 6. Mutate prob vec

    for (int k = 0; k < bits; k++)
    {
      if (rand() < mut_prob)
      {
        prob[k] = shift(prob[k], rand_bool(), mut_shift);
      }
    }

    // 7. Output

    Eval E;
    E.set(best);
    E.init();
    say("\nbest: {}\n", E.to_string());
  }
}

Genome Tuning::gen_genome()
{
  Genome genome;
  for (auto v : prob) genome.push_back(rand() < v);
  return genome;
}

double Tuning::score(const Genome & genome)
{
  int played = 0;
  double total = 0.0;

  for (int i = 0; i < games; ++i)
  {
    const int side = i & 1;
    int result = play_game(genome, side);

#ifdef _DEBUG
    say("\n");
    B.print();
#endif

    if (side) result = -result;
    total += result;

    say("{}", "-=+"[result + 1]);
  }

  if (played) total /= played;

  say(" - {}\n", total);

  return total;
}

// TODO: add random openings to make score more informative
// Returns -1 for black win, +1 for white win, 0 for draw
int Tuning::play_game(const Genome & genome, int side)
{
  vector<int> vals[2];
  SearchCfg cfg;
  cfg.depth = depth;

  E[0]->set(genome);
  E[0]->init();

  B.set();
  S[0]->set(B);
  S[1]->set(B);

  S[0]->new_game();
  S[1]->new_game();

  for (int stm = side;; stm = 1 - stm)
  {
    assert(B.state.bhash == S[stm]->get_hash());

    Move move = S[stm]->get_move(cfg);
    int val = S[stm]->get_best_val();

    // Direct mate on board

    if (val == -Val::Inf) return -1;
    if (val ==  Val::Inf) return  1;

    vals[stm].push_back(val);
    B.make(move);
    S[0]->make(move);
    S[1]->make(move);

    // Finishing game by draw

    if (B.is_draw()) return 0;

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

    // Finishing game by adjucation

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

}
