#include <format>
#include <iostream>
#include "solver_pvs.h"
#include "hash.h"

using namespace std;

namespace eia {

// from GreKo 2021.12
const Val Futility_Margin[] = { 0_cp, 50_cp, 350_cp, 550_cp };

// from Ethereal
const int LMP_Depth = 8;

SolverPVS::SolverPVS(Eval * eval) : E(eval)
{
  B = new Board;
  H = new Hash::Table(HashTables::Size);
  init();
}

SolverPVS::~SolverPVS()
{
  delete H;
  delete E;
  delete B;
}

void SolverPVS::init()
{
  // from Ethereal

  for (int depth = 1; depth < 64; depth++)
  {
    for (int moves = 1; moves < 256; moves++)
    {
       double ldll = std::log(depth) * std::log(moves);
       double r = 0.7844 + ldll / 2.4696;
       LMR[depth][moves] = static_cast<int>(r);
    }
  }

  for (int depth = 1; depth <= 10; depth++)
  {
    LMP_Counts[0][depth] = 2.0767 + 0.3743 * depth * depth;
    LMP_Counts[1][depth] = 3.8733 + 0.7124 * depth * depth;
  }
}

void SolverPVS::set(const Board & board)
{
  *B = board;
}

Move SolverPVS::get_move(const SearchCfg & cfg)
{
  timer.start();
  to_think = cfg.full_time(B->to_move());
  infinite = cfg.infinite;
  max_ply = 0;
  nodes = 0ull;
  best_val = 0_cp;

  B->revert_states();

  MoveList ml;
  B->generate_legal(ml);
  Move best = Move::None;

  if (ml.count() == 0)
  {
    best_val = !B->state.checkers ? 0_cp
             : B->to_move() ? -Val::Inf : Val::Inf;
    best = Move::None;
  }

  else if (ml.count() == 1) best = ml.get_next();

  else

  for (int depth = 1; depth <= (std::min)(+Limits::Plies, cfg.depth); ++depth)
  {
    Val val = best_val = pvs<Root>(-Val::Inf, Val::Inf, depth);
    if (!thinking) break;

    best = is_empty(undos[0].best) ? best : undos[0].best;

    if (verbose)
    say<1>("info depth {} seldepth {} score {:o} nodes {} time {} pv {}\n",
            depth, max_ply, val, nodes, timer.getms(), best);

    if (val > Val::Mate || val < -Val::Mate) break;
  }

  if (verbose) say<1>("bestmove {}\n", best);
  
  thinking = false;
  return is_empty(best) ? ml.get_next() : best;
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
    if (!B->make(move)) continue;

    say("{}", move);

    u64 cnt = perft_inner(depth - 1);
    count += cnt;

    say(" - {}\n", cnt);

    B->unmake(move);
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
    if (!B->make(move)) continue;

    count += depth > 1 ? perft_inner(depth - 1) : 1;

    B->unmake(move);
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
  Undo & undo = undos[ply()];

  // History table

  const Move move = undo.curr;
  const SQ from = get_from(move);
  const SQ to = get_to(move);

  const bool leave = B->state.threats & bit(from);
  const bool enter = B->state.threats & bit(to);

  history[B->color][leave][enter][from][to] += depth * depth;

  // Counter move

  if (ply() > 0)
  {
    Move prev = undos[ply() - 1].curr;
    counter[~B->color][get_from(prev)][get_to(prev)] = move;
  }

  // Killers

  if (undo.killer[0] != move)
  {
    undo.killer[1] = undo.killer[0];
    undo.killer[0] = move;
  }
}

template<NodeType NT>
Val SolverPVS::pvs(Val alpha, Val beta, int depth, bool is_null)
{
  using namespace Hash;
  if constexpr (NT == Root) thinking = true;

  if (ply() >= Limits::Plies) return E->eval(B, alpha, beta);

  const bool in_check = !!B->state.checkers;
  Type hash_type = Type::Upper;
  Val val = cp(ply()) - Val::Inf;
  Undo & undo = undos[ply()];
  undo.best = Move::None;
  nodes++;

  if (!in_check && depth <= 0) return qs(alpha, beta);

  if constexpr (NT != Root) // +70 elo (1+1 h2h-10)
  {
    if (B->is_draw()) return contempt();
  }

  // 0. Mate pruning ?? -35 elo (20s+.2 h2h-20)

  //if (ply > 0)
  //{
  //  Val r_alpha = max(-Val.Inf + cp(ply), alpha);
  //  Val r_beta = min(Val.Inf - cp(ply + 1), beta);
  //  if (r_alpha >= r_beta) return r_alpha;
  //}

  // 1. Retrieving hash move

  Entry entry;
  const bool tt_hit = H->probe(B->hash(), ply(), entry);
  Move hash_move = tt_hit ? entry.move : Move::None;
  Val hash_val = tt_hit ? cp(entry.val) : 0_cp;

  if constexpr (NT == NonPV) // +100 elo (1+1 h2h-10)
  {
    if (tt_hit
    &&  entry.depth >= depth)
    {
      if (entry.type == Type::Exact
      || (entry.type == Type::Lower && hash_val >= beta)
      || (entry.type == Type::Upper && hash_val <= alpha))
      {
        return hash_val;
      }
    }
  }

  Val eval = tt_hit ? hash_val
           : (NT == NonPV ? E->eval(B, alpha, beta) : 0_cp);

  undo.eval = eval;
  const bool improving = !in_check && ply() > 2
                      && eval > undos[ply() - 2].eval;

  if constexpr (NT == NonPV) // +70 elo (1+1 h2h-16)
  {
    // 2. Futility Pruning

    if (!in_check
    &&  !is_null
    &&  depth >= 1
    &&  depth <= 3)
    {
      if (eval <= alpha - Futility_Margin[depth])
        return qs(alpha, beta);
      if (eval >= beta + Futility_Margin[depth])
        return beta;
    }
  }

  //if constexpr (NT == NonPV) // 
  //{
  //  // 2.1. Reversed Futility Pruning

  //  if (!in_check
  //  &&  !is_null
  //  &&  depth <= 6)
  //  {
  //    if (eval - 65 * std::max(0, (depth - improving)) >= beta)
  //      return eval;
  //  }
  //}

  if constexpr (NT == NonPV) // +100 elo (10s+.1 h2h-20)
  {
    // 3. Null Move Pruning

    if (!in_check
    &&  !is_null
    &&  B->has_pieces(B->color)
    &&  eval >= beta
    &&  beta > -Val::Mate
    &&  depth >= 2)
    {
      int R = 3 + depth / 4;

      B->make_null();
      Val v = -pvs<NonPV>(-beta, -beta + 1, depth - R, true);
      B->unmake_null();

      if (v >= beta)
      {
        return v > Val::Mate ? beta : v;
      }
    }
  }

  // 4. Internal Iterative Deepening (Ethereal approach)

  //if constexpr (NT != Root) // +20 elo (20s+.2 h2h-20)
  //{
  //  // predicted
  //  const bool cutnode = NT == NonPV && eval >= beta;
  //  const bool is_pv = NT == PV;

  //  if (depth >= 7
  //  && (is_pv || cutnode)
  //  && (!tt_hit || entry.depth + 4 < depth))
  //  {
  //    depth--;
  //  }
  //}

  // Looking all legal moves

  int legal = 0;
  bool do_quiets = true;
  MovePickerPVS mp;
  set_movepicker(mp, hash_move);

  Move move;
  while (!is_empty(move = mp.get_next(do_quiets)))
  {
    if (!B->make(move)) continue;

    undo.curr = move;
    legal++;
    bool gives_check = B->in_check();
    bool is_tactical = is_attack(move);
    int reduce = 0, extend = 0;

    // Late Move Pruning | +0 elo (20s+.2 h2h-20)
    // 
    //  losing opportunities, sometimes vital

    /*if (val > -Val::Mate
    &&  depth <= LMP_Depth
    &&  legal >= LMP_Counts[improving][depth])
    {
      do_quiets = false;
    }*/

    // Check extension | +50 elo (20s+.2 h2h-20)
    
    if (in_check) extend++;

    // Pawn push extension | -100 elo (20s+.2 h2h-20) ??

    //if (is_prom(move))
    //{
    //  extend++;
    //}
    //else if (is_pawn(move))
    //{
    //  const SQ to = get_to(move);

    //  // move is done
    //  if ((~B->color && rank(to) == 6)
    //  ||  ( B->color && rank(to) == 1))
    //  {
    //    extend++;
    //  }
    //}
    
    // LMR | +70 elo (20s+.2 h2h-20)

    if (!is_null
    &&  !in_check
    &&  depth >= 3
    &&  extend <= 0
    &&  !is_tactical)
    {
      // From Ethereal
      reduce += LMR[std::min(depth, 63)][std::min(legal, 255)];

      // Non-PV nodes and not imporing reduced
      reduce += (NT == NonPV) + !improving;

      // Prevent reducing killers and countermove
      reduce -= (mp.stage < Stage::GenQuiets);
    }

    // Don't extend or drop into QS
    reduce = std::min(depth - 1, (std::max)(reduce, 1));
    int new_depth = depth - 1 + extend;

    if (legal == 1)
      val = -pvs<PV>(-beta, -alpha, new_depth, is_null);
    else
    {
      val = -pvs<NonPV>(-alpha - 1, -alpha, new_depth - reduce, is_null);
      if (val > alpha && reduce > 0)
        val = -pvs<NonPV>(-alpha - 1, -alpha, new_depth, is_null);
      if (val > alpha && val < beta)
        val = -pvs<PV>(-beta, -alpha, new_depth, is_null);
    }

    B->unmake(move);

    if (abort()) return alpha;

    if (val > alpha)
    {
      alpha = val;
      hash_type = Type::Exact;
      undo.best = move;

      if (val >= beta)
      {
        if (!is_attack(move)/* && !in_check*/)
        {
          update_moves_stats(depth);
        }

        hash_type = Type::Lower;
        break;
      }
    }
  }

  if (!legal)
  {
    return in_check ? val : contempt();
  }

  if (!abort())
  {
    Move best = hash_type == Upper ? Move::None : undo.best;
    H->store(B->hash(), ply(), best, alpha, depth, hash_type);
  }

  return alpha;
}

Val SolverPVS::qs(Val alpha, Val beta)
{
  const bool in_check = !!B->state.checkers;
  max_ply = (std::max)(max_ply, ply());

  if (ply() >= Limits::Plies) return E->eval(B, alpha, beta);
  if (B->is_draw()) return contempt();

  Undo & undo = undos[ply()];

  // 1. Retrieving hash eval

  Entry entry;
  const bool tt_hit = H->probe(B->hash(), ply(), entry);
  Move hash_move = tt_hit ? entry.move : Move::None;
  Val hash_val = tt_hit ? cp(entry.val) : 0_cp;

  if (tt_hit)
  {
    if (entry.type == Type::Exact
    || (entry.type == Type::Lower && hash_val >= beta)
    || (entry.type == Type::Upper && hash_val <= alpha))
    {
      return hash_val;
    }
  }

  // 2. Calculating stand pat

  Val eval = E->eval(B, alpha, beta);
  //H->store(B->hash(), ply(), Move::None, eval, 0, Type::None);

  if (eval >= beta) return beta;
  if (eval > alpha) alpha = eval;

  // 3. Delta pruning (+123 elo 10s+.1 h2h-30)

  if ((std::max)(143, B->best_cap_value()) < dry(alpha - eval))
  {
    return eval;
  }

  MovePickerQS mp;
  set_movepicker(mp, Move::None);

  Move move;
  while (!is_empty(move = mp.get_next()))
  {
    if (!B->make(move)) continue;

    // SEE pruning (+70 elo 10s+.1 h2h-30)
    if (!in_check
    &&  !is_prom(move)
    &&  B->see(move) < 0) continue; 

    nodes++;
    undo.curr = move;

    Val val = -qs(-beta, -alpha);

    B->unmake(move);

    if (abort()) return alpha;

    if (val >= beta) return beta;
    if (val > alpha) alpha = val;
  }

  return alpha;
}

}
