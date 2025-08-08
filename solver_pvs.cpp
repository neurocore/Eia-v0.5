#include <format>
#include <iostream>
#include "solver_pvs.h"
#include "hash.h"

using namespace std;

namespace eia {

// from GreKo 2021.12
const int Futility_Margin[] = { 0, 50, 350, 550 };

SolverPVS::SolverPVS(Engine * engine, Eval * eval) : Solver(engine), E(eval)
{
  B = new Board;
  H = new Hash::Table(HashTables::Size);
  undo = &undos[0];
}

SolverPVS::~SolverPVS()
{
  delete H;
  delete E;
  delete B;
}

void SolverPVS::set(const Board & board)
{
  *B = board;
}

Move SolverPVS::get_move(MS time)
{
  timer.start();
  to_think = time;
  max_ply = 0;
  nodes = 0ull;

  Move best = Move::None;
  for (int depth = 1; depth < Limits::Plies; ++depth)
  {
    int val = pvs<Root>(-Val::Inf, Val::Inf, depth);
    if (!thinking) break;

    best = is_empty(undos[0].best) ? best : undos[0].best;

    say<1>("info depth {} seldepth {} score cp {} nodes {} time {} pv {}\n",
        depth, max_ply, val, nodes, timer.getms(), best);

    if (val > Val::Mate || val < -Val::Mate) break;
  }

  say<1>("bestmove {}\n", best);
  
  thinking = false;
  return best;
}

u64 SolverPVS::perft(int depth)
{
  u64 count = 0ull;

  say("-- Perft {}\n", depth);
  B->print();

  timer.start();

  /*MoveList ml;
  B->generate_all(ml);

  while (!ml.is_empty())
  {
    Move move = ml.get_next();
    if (is_empty(move)) break;
  */

  MovePickerPVS mp; // must be correct
  set_movepicker(mp, Move::None);

  Move move;
  while (!is_empty(move = mp.get_next()))
  {
    if (!B->make(move, undo)) continue;

    say("{}", move);

    u64 cnt = perft_inner(depth - 1);
    count += cnt;

    say(" - {}\n", cnt);

    B->unmake(move, undo);
  }

  i64 time = timer.getms();
  double knps = static_cast<double>(count) / (time + 1);

  say("\nCount: {}\n", count);
  say("Time: {} ms\n", time);
  say("Speed: {:.2f} knps\n\n", knps);

  return count;
}

u64 SolverPVS::perft_inner(int depth)
{
  if (depth <= 0) return 1;

  u64 count = 0ull;

 /* MoveList ml;
  B->generate_all(ml);

  while (!ml.is_empty())
  {
    Move move = ml.get_next();
    if (is_empty(move)) break;*/

  MovePickerPVS mp; // must be correct
  set_movepicker(mp, Move::None);

  Move move;
  while (!is_empty(move = mp.get_next()))
  {
    if (!B->make(move, undo)) continue;

    count += depth > 1 ? perft_inner(depth - 1) : 1;

    B->unmake(move, undo);
  }
  return count;
}

int SolverPVS::plegt()
{
  const u16 count = 0xFFFF;
  int false_pos = 0;
  int false_neg = 0;

  MoveList pseudo, legal;
  B->generate_all(pseudo);
  B->generate_legal(legal);
  B->print();

  say("Pseudo moves: {}\n", pseudo.count());
  say("Legal moves: {}\n\n", legal.count());

  string report;

  for (u16 i = 0u; i < count; i++)
  {
    const Move move = static_cast<Move>(i);
    const bool passed = B->pseudolegal(move);
    const Piece p = B->square[get_from(move)];

    if (passed && !pseudo.contains(move)) // pseudolegality is enough
    {
      false_pos++;
      report += format("!pos - 0x{:04X} : {} | {}\n", i, p, detailed(move));
    }

    if (!passed && legal.contains(move)) // a narrower check 
    {
      false_neg++;
      report += format("!neg - 0x{:04X} : {} | {}\n", i, p, detailed(move));
    }
  }

  say("{}", report);
  say("\nCount: {}\n", count);
  say("False pos: {}\n", false_pos);
  say("False neg: {}\n", false_neg);

  return false_pos + false_neg;
}

bool SolverPVS::abort() const
{
  if ((nodes & 8191) == 0 && Input.available())
  {
    std::string str;
    getline(cin, str);

    if (str == "isready") say<1>("readyok");
    if (str == "stop" || str == "quit")
    {
      thinking = false;
      return true;
    }
  }

  if (!thinking) return true;
  if (infinite) return false;

  const MS time_to_move = to_think / 30;
  if (timer.getms() > time_to_move)
  {
    thinking = false;
    return true;
  }
  return false;
}

void SolverPVS::update_moves_stats(int depth)
{
  // History table

  const Move move = undo->curr;
  const SQ from = get_from(move);
  const SQ to = get_to(move);

  const bool leave = B->state.threats & bit(from);
  const bool enter = B->state.threats & bit(to);

  history[B->color][leave][enter][from][to] += depth * depth;

  // Counter move

  if (ply() > 0)
  {
    Move prev = (undo - 1)->curr;
    counter[~B->color][get_from(prev)][get_to(prev)] = move;
  }

  // Killers

  if (undo->killer[0] != move)
  {
    undo->killer[1] = undo->killer[0];
    undo->killer[0] = move;
  }
}

template<NodeType NT>
int SolverPVS::pvs(int alpha, int beta, int depth, bool is_null)
{
  using namespace Hash;
  if constexpr (NT == Root) thinking = true;
  const bool in_check = !!B->state.checkers;
  const int old_alpha = alpha;
  int val, best = ply() - Val::Inf;
  undo->best = Move::None;
  nodes++;

  if (!in_check && depth <= 0) return qs(alpha, beta, depth + 2);

  if constexpr (NT != Root) // +70 (1+1 h2h-10)
  {
    if (B->is_draw()) return 0; // contempt();
  }

  int legal = 0;

  // 0. Mate pruning

  //if (ply > 0)
  //{
  //  alpha = max(-Val.Inf + ply, alpha);
  //  beta = min(Val.Inf - (ply + 1), beta);
  //  if (alpha >= beta) return alpha;
  //}

  // 1. Retrieving hash move

  Entry entry;
  const bool tt_hit = H->probe(B->hash(), ply(), entry);
  Move hash_move = tt_hit ? entry.move : Move::None;

  if constexpr (NT == NonPV) // +100 elo (1+1 h2h-10)
  {
    if (tt_hit
    &&  entry.depth >= depth)
    {
      if (entry.type == Type::Exact
      || (entry.type == Type::Lower && entry.val >= beta)
      || (entry.type == Type::Upper && entry.val <= alpha))
      {
        return entry.val;
      }
    }
  }

  int static_eval = tt_hit ? entry.val
                  : (NT == NonPV ? E->eval(B, alpha, beta) : 0);

  if constexpr (NT == NonPV) // +70 elo (1+1 h2h-16)
  {
    // 2. Futility Pruning

    if (!in_check
    &&  !is_null
    &&  depth >= 1
    &&  depth <= 3)
    {
      if (static_eval <= alpha - Futility_Margin[depth])
        return qs(alpha, beta, depth + 2);
      if (static_eval >= beta + Futility_Margin[depth])
        return beta;
    }
  }

  if constexpr (NT == NonPV) // +100 elo (10s+.1 h2h-20)
  {
    // 3. Null Move Pruning

    if (!in_check
    &&  !is_null
    &&  B->has_pieces(B->color)
    &&  static_eval >= beta
    &&  beta > -Val::Mate
    &&  depth >= 2)
    {
      int R = 3 + depth / 4;

      B->make_null(undo);
      int v = -pvs<NonPV>(-beta, -beta + 1, depth - R, true);
      B->unmake_null(undo);

      if (v >= beta)
      {
        return v > Val::Mate ? beta : v;
      }
    }
  }

  /*

  // 4. Internal Iterative Deepening

  if constexpr (NT == PV) // -50 elo (10s+.1 h2h-20)
  {
    if (depth >= 3
    &&  hash_move == Move::None)
    {
      int new_depth = depth - 2;

      int v = pvs<PV>(alpha, beta, new_depth, is_null);
      if (v <= alpha)
        v = pvs<PV>(-Val::Inf, beta, new_depth, is_null);

      if (!is_empty(undo->best))
        if (B->pseudolegal(undo->best))
          hash_move = undo->best;
    }
  }
  */

  // Looking all legal moves

  MovePickerPVS mp;
  set_movepicker(mp, hash_move);

  Move move;
  while (!is_empty(move = mp.get_next()))
  {
    if (!B->make(move, undo)) continue;

    undo->curr = move;
    legal++;
    int new_depth = depth - 1;
    int reduction = 0;

    // LMR

    if constexpr (NT == NonPV)
    {
      if (!is_null
      &&  !in_check
      &&  depth >= 4
      &&  !B->in_check()
      &&  !is_attack(move))
      {
        // from Fruit Reloaded
        reduction = static_cast<int>(sqrt(depth - 1) + sqrt(legal - 1));
      }
    }

    if (legal == 1)
      val = -pvs<PV>(-beta, -alpha, new_depth, is_null);
    else
    {
      val = -pvs<NonPV>(-alpha - 1, -alpha, new_depth - reduction, is_null);
      if (val > alpha && reduction > 0)
        val = -pvs<NonPV>(-alpha - 1, -alpha, new_depth, is_null);
      if (val > alpha && val < beta)
        val = -pvs<PV>(-beta, -alpha, new_depth, is_null);
    }

    B->unmake(move, undo);

    if (abort()) return alpha;

    if (val > best)
    {
      best = val;
      undo->best = move;

      if (val > alpha)
      {
        alpha = val;

        if (val >= beta)
        {
          if (!is_attack(undo->curr))
          {
            update_moves_stats(depth);
          }
          break;
        }
      }
    }
  }

  if (!legal)
  {
    return in_check ? best : 0; // contempt();
  }

  if (!abort())
  {
    Hash::Type type = best >= beta ? Lower
                    : best > old_alpha ? Exact : Upper;
    Move best_mv = type == Upper ? Move::None : undo->best;
    H->store(B->hash(), ply(), best_mv, best, depth, type);
  }

  return best;
}

template<bool InCheck>
int SolverPVS::qs(int alpha, int beta, int max_checks_depth)
{
  max_ply = std::max(max_ply, ply());

  if (ply() >= Limits::Plies) return E->eval(B, alpha, beta);
  if (B->is_draw()) return 0; // contempt();

  int val, best = ply() - Val::Inf;
  const int old_alpha = alpha;
  int legal = 0;

  // 1. Retrieving hash eval

  Entry entry;
  const bool tt_hit = H->probe(B->hash(), ply(), entry);

  if (tt_hit) // +0 elo (1+1 h2h-10) - not storing
  {
    if (entry.type == Type::Exact
    || (entry.type == Type::Lower && entry.val >= beta)
    || (entry.type == Type::Upper && entry.val <= alpha))
    {
      return entry.val;
    }
  }

  // 2. Calculating stand pat

  if constexpr (!InCheck)
  {
    best = E->eval(B, alpha, beta);
    //H->store(B->hash(), ply(), Move::None, best, 0, Type::None);

    if (best > alpha) alpha = best;
    if (alpha >= beta) return best;

    // 3. Delta pruning (+123 elo 10s+.1 h2h-30)

    if (std::max(143, B->best_cap_value()) < alpha - best)
    {
      return best;
    }
  }

  MovePicker<!InCheck> mp;
  set_movepicker(mp, Move::None);

  Move move;
  while (!is_empty(move = mp.get_next(/*max_checks_depth > 0*/)))
  {
    if (!B->make(move, undo)) continue;

    // SEE pruning (+70 elo 10s+.1 h2h-30)
    if (!InCheck
    &&  !is_prom(move)
    &&  B->see(move) < 0) continue; 

    legal++;
    nodes++;
    undo->curr = move;

    val = -qs(-beta, -alpha, max_checks_depth - 1);

    // -50 elo (10s+.1 h2h-30)
    /*val = B->state.checkers
        ? -qs<1>(-beta, -alpha, max_checks_depth - 1)
        : -qs<0>(-beta, -alpha, max_checks_depth - 1);*/

    B->unmake(move, undo);

    if (abort()) return alpha;

    if (val > best)
    {
      best = val;
      undo->best = move;

      if (val > alpha) alpha = val;
      if (val >= beta) break;
    }
  }

  return best;
}

}
