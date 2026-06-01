#include <format>
#include <iostream>
#include "solver_pvs.h"
#include "history.h"
#include "hash.h"
#include "eval.h"

using namespace std;

namespace eia {

const bool USE_SINGULAR_MOVE = true;

// from GreKo 2021.12
const Val Futility_Margin[] = { 0_cp, 50_cp, 350_cp, 550_cp };

// from Ethereal
const int LMP_Depth = 8;

SolverPVS::SolverPVS()
{
  B = new Board;
  H = new Hash::Table(HashTables::Size);
  init();
}

SolverPVS::~SolverPVS()
{
  delete H;
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
    LMP_Counts[0][depth] = (int)(2.0767 + 0.3743 * depth * depth);
    LMP_Counts[1][depth] = (int)(3.8733 + 0.7124 * depth * depth);
  }
}

void SolverPVS::new_game()
{
  best_val = 0_cp;
  max_ply = 0;
  nodes = 0ull;
  g_depth = 0;

  H->clear();
  undos[0].killer[0] = Move::None;
  undos[0].killer[1] = Move::None;

  for (int col = 0; col < 2; col++)
  {
    for (SQ i = A1; i < SQ_N; ++i)
    {
      for (SQ j = A1; j < SQ_N; ++j)
      {
        history[col][0][0][i][j] = 0;
        history[col][0][1][i][j] = 0;
        history[col][1][0][i][j] = 0;
        history[col][1][1][i][j] = 0;
      }
    }
  }

  for (Piece p = BP; p < Piece_N; ++p)
  {
    for (SQ sq = A1; sq < SQ_N; ++sq)
    {
      for (PieceType q = Pawn; q < King; ++q)
      {
        caphist[p][0][0][sq][q] = 0;
        caphist[p][0][1][sq][q] = 0;
        caphist[p][1][0][sq][q] = 0;
        caphist[p][1][1][sq][q] = 0;
      }
    }
  }

  for (int i = 0; i < Limits::Plies; i++)
    undos[i].excluded = Move::None;
}

void SolverPVS::set(const Board & board)
{
  *B = board;
}

void SolverPVS::set_time(const SearchCfg & cfg)
{
  Color we = B->to_move(); // late EG needs more time
  int moves_base = 50 - popcnt(B->occupied()) / 3;
  int moves_left = std::max(25, moves_base - B->moves_cnt / 2);
  MS time = cfg.time[we] - 50;
  MS to_think = time / moves_left + cfg.inc[we] / 2;

  hard_bound = std::min((MS)(1.2 * to_think), time - 50);
  soft_bound = std::min((MS)(0.7 * to_think), hard_bound);

  log("  time: {} + {}\n", time, cfg.inc[we]);
  log(" whole: {}\n", to_think);
  log("bounds: {} / {}\n\n", soft_bound, hard_bound);
}

Move SolverPVS::get_move(Timestamp move_start, const SearchCfg & cfg)
{
  start = move_start;
  infinite = cfg.infinite;
  set_time(cfg);  
  max_ply = 0;
  nodes = 0ull;
  g_depth = 0;
  best_val = 0_cp;

  const int iters_soft = 6;
  Move bests[Limits::Plies + 1];
  Val  vals[Limits::Plies + 1];

  for (int i = 0; i < Limits::Plies; i++)
    undos[i].excluded = Move::None;

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

  for (g_depth = 1; g_depth <= (std::min)(+Limits::Plies, cfg.depth); ++g_depth)
  {
    Val val = best_val = pvs<Root>(-Val::Inf, Val::Inf, g_depth);
    if (!thinking) break;

    best = is_empty(undos[0].best) ? best : undos[0].best;

    bests[g_depth] = best;
    vals[g_depth] = val;

    if (verbose)
    say<1>("info depth {} seldepth {} score {:o} nodes {} time {} pv {} hashfull {}\n",
            g_depth, max_ply, val, nodes, elapsed(start), best, H->hashfull());

    if (val > Val::Mate || val < -Val::Mate) break;

    // checking soft bound

    if (elapsed(start) > soft_bound
    &&  g_depth > iters_soft)
    {
      bool stable_bests = true;
      Val max_range = 0_cp;

      for (int i = 0; i < iters_soft; i++)
      {
        if (best != bests[g_depth - i]) stable_bests = false;
        Val range = abs(val - vals[g_depth - i]);
        if (range > max_range) max_range = range;
      }

      bool stable_vals = max_range < 20_cp;
      if (stable_bests && stable_vals) break;
    }
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

  start = Clock::now();

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

  i64 time = elapsed(start);
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
  if (!thinking) return true;
  if (infinite) return false;

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

  if (g_depth > 2 && elapsed(start) > hard_bound)
  {
    thinking = false;
    return true;
  }
  return false;
}

void SolverPVS::update_moves_stats(Move * moves, int cnt, int depth)
{
  Undo & undo = undos[ply()];
  Move best = moves[cnt - 1];

  // Counter move

  if (ply() > 0)
  {
    Move prev = undos[ply() - 1].curr;
    if (!is_empty(prev)) // null move may be
      counter[~B->color][get_from(prev)][get_to(prev)] = best;
  }

  // Killers

  if (undo.killer[0] != best)
  {
    undo.killer[1] = undo.killer[0];
    undo.killer[0] = best;
  }

  // History table

  if (!depth // found a low-depth cutoff too easily
  || (cnt == 1 && depth <= 3))
    return; 

  for (int i = 0; i < cnt; i++)
  {
    int & hist = underlying_history(B, moves[i], history);
    update_history(hist, depth, moves[i] == best);
  }
}

void SolverPVS::update_captures_stats(Move best, Move * moves, int cnt, int depth)
{
  for (int i = 0; i < cnt; i++)
  {
    int & hist = underlying_caphist(B, moves[i], caphist);
    update_history(hist, depth, moves[i] == best);
  }
}

template<NodeType NT>
Val SolverPVS::pvs(Val alpha, Val beta, int depth, bool is_null, bool is_singular)
{
  using namespace Hash;
  if constexpr (NT == Root) thinking = true;

  if (ply() >= Limits::Plies) return E->eval(B, alpha, beta);

  const bool in_check = !!B->state.checkers;
  int quiets_cnt = 0, captures_cnt = 0;
  Move quiets_tried[Limits::M0ves];
  Move captures_tried[Limits::M0ves];
  Undo & undo = undos[ply()];
  Val val = cp(ply()) - Val::Inf;
  Val best = -Val::Inf;
  Val alpha_ = alpha;
  undo.best = Move::None;
  nodes++;

  if (!in_check && depth <= 0) return qs(alpha, beta);

  if constexpr (NT != Root) // +70 elo (1+1 h2h-10)
  {
    if (B->is_draw()) return contempt();
  }

  // 0. Mate pruning ?? -35 elo (20s+.2 h2h-40)

  /*if constexpr (NT != Root)
  {
    Val r_alpha = std::max(-Val::Inf + cp(ply()), alpha);
    Val r_beta  = std::min( Val::Inf - cp(ply() + 1), beta);
    if (r_alpha >= r_beta) return r_alpha;
  }*/

  // 1. Retrieving hash move

  Entry entry{};
  bool tt_hit = false;
  Move hash_move = Move::None;
  Val hash_val = 0_cp;

  if (is_empty(undo.excluded))
  {
    tt_hit = H->probe(B->hash(), ply(), entry);
    hash_move = tt_hit ? entry.move : Move::None;
    hash_val = tt_hit ? cp(entry.val) : 0_cp;

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
    &&  is_empty(undo.excluded)
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
  //  &&  is_empty(undo.excluded)
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
    &&  is_empty(undo.excluded)
    &&  B->has_pieces(B->color)
    &&  eval >= beta
    &&  beta > -Val::Mate
    &&  depth >= 2)
    {
      int R = 3 + depth / 4;

      B->make_null();
      Val v = -pvs<NonPV>(-beta, -beta + 1, depth - R, true, is_singular);
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
    if (move == undo.excluded) continue;
    if (!B->make(move)) continue;

    /*if constexpr (NT == PV)
    {
      log("{}\n", move);
    }*/

    undo.curr = move;
    legal++;
    bool gives_check = B->in_check();
    bool is_tactical = is_attack(move);

    if (!is_tactical) quiets_tried[quiets_cnt++] = move;
             else captures_tried[captures_cnt++] = move;

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

    // Singular move extension | +50 elo (20s+.2 h2h-20)

    if constexpr (USE_SINGULAR_MOVE && NT != Root)
    {
      if (!is_singular
      &&  undo.extensions <= 8
      &&  ply() < 2 * (g_depth + 1)
      &&  depth >= 6
      &&  move == hash_move
      &&  is_empty(undo.excluded)
      &&  entry.depth >= depth - 3
      &&  entry.type == Hash::Type::Lower
      &&  abs(hash_val) < Val::Mate)
      {
        B->unmake(hash_move);
        Undo & undo = undos[ply()]; // numbers from SF
        Val margin = cp((1.0926 + 1.4259 * (NT == NonPV)) * depth);
        Val s_beta = hash_val - margin * 1.5;

        undo.excluded = hash_move;
        Val val = pvs<NonPV>(s_beta - 1, s_beta, depth / 2, false, true);
        undo.excluded = Move::None;

        B->make(hash_move);

        if (abort()) return alpha;

        if (val < s_beta)
        {
          extend = 1;
        }

        // Multi-cut pruning
        else if (val >= beta && abs(val) < Val::Mate)
        {
          B->unmake(hash_move);
          return val;
        }

        // Hash move isn't singular
        // so give chance to others
        else if (hash_val >= beta)
        {
          extend = -3;
        }
      }
    }
    
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

    undo.extensions = NT == Root ? 0
        : undos[ply() - 1].extensions + extend;

    if (legal == 1)
      val = -pvs<PV>(-beta, -alpha, new_depth, is_null, is_singular);
    else
    {
      val = -pvs<NonPV>(-alpha - 1, -alpha, new_depth - reduce, is_null, is_singular);
      if (val > alpha && reduce > 0)
        val = -pvs<NonPV>(-alpha - 1, -alpha, new_depth, is_null, is_singular);
      if (val > alpha && val < beta)
        val = -pvs<PV>(-beta, -alpha, new_depth, is_null, is_singular);
    }

    B->unmake(move);

    if (abort()) return alpha;

    if (val > best)
    {
      best = val;
      undo.best = move;

      if (val > alpha)
      {
        alpha = val;

        if (val >= beta) break;
      }
    }
  }

  // Histories | +0 elo (20s+.2 h2h-20) - requires fine tuning?

  if (best >= beta && !is_attack(undo.best))
    update_moves_stats(quiets_tried, quiets_cnt, depth);

  if (best >= beta)
    update_captures_stats(undo.best, captures_tried, captures_cnt, depth);

  if (is_empty(undo.excluded))
  {
    if (!legal)
    {
      return in_check ? val : contempt();
    }

    if (!abort())
    {
      auto bound = best >= beta  ? Hash::Lower
                 : best > alpha_ ? Hash::Exact : Hash::Upper;
      Move bm = bound == Upper ? Move::None : undo.best;
      H->store(B->hash(), ply(), bm, best, depth, bound);
    }
  }

  return best;
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

  Val eval = in_check ? cp(ply()) - Val::Inf : E->eval(B, alpha, beta);
  //H->store(B->hash(), ply(), Move::None, eval, 0, Type::None);

  if (eval >= beta) return beta;
  if (eval > alpha) alpha = eval;

  // 3. Delta pruning (+123 elo 10s+.1 h2h-30)

  if (eval > -Val::Mate
  &&  (std::max)(143, B->best_cap_value()) < dry(alpha - eval))
  {
    return eval;
  }

  MovePickerQS mp;
  set_movepicker(mp, Move::None);

  Move move;
  while (!is_empty(move = mp.get_next(false)))
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
