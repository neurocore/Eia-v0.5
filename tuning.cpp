#include <algorithm>
#include "tuning.h"
#include "engine.h"

using namespace std;

namespace eia {

Tuner::Tuner(TunerCfg cfg) : cfg(cfg)
{
  E[0] = new Eval;
  E[1] = new Eval;
  S[0] = new SolverPVS(E[0]);
  S[1] = new SolverPVS(E[1]);
  S[0]->set_verbosity(false);
  S[1]->set_verbosity(false);
}

Tuner::~Tuner()
{
  delete S[1];
  delete S[0];
}

void Tuner::init(std::string book_pgn)
{
  book.clear();
  BookReader(&book).read_pgn(book_pgn);
}

double Tuner::score(const Eval & eval)
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
int Tuner::play_game(int side)
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

}
