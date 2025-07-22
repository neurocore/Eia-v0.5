#include <format>
#include <iostream>
#include "solver_pvs.h"

using namespace std;

namespace eia {

SolverPVS::SolverPVS(Engine * engine) : Solver(engine)
{
  B = new Board;
  E = new Eval;
  undo = &undos[0];
}

SolverPVS::~SolverPVS()
{
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
  thinking = true;
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

  MovePicker mp; // must be correct
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

  MovePicker mp; // must be correct
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

void SolverPVS::set_movepicker(MovePicker & mp, Move hash)
{
  mp.ml.clear();

  mp.B = B;
  mp.H = &history;
  mp.hash_mv = hash;
  mp.killer[0] = Move::None; // undo->killer[0];
  mp.killer[1] = Move::None; // undo->killer[1];

  mp.stage = B->pseudolegal(hash) ? Stage::Hash : Stage::GenCaps;

  if (!ply()) return;

  const Move prev = (undo - 1)->curr;
  mp.counter = is_empty(prev) ? Move::None
             : counter[~B->color][get_from(prev)][get_to(prev)];
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
int SolverPVS::pvs(int alpha, int beta, int depth)
{
  const bool in_check = !!B->state.king_atts;
  //HashType hash_type = HashType::Alpha;
  int val = ply() - Val::Inf;
  undo->best = Move::None;
  nodes++;

  if (!in_check && depth <= 0) return qs(alpha, beta);
  if (ply() > 0 && B->is_draw()) return 0; // contempt();

  int legal = 0;

  /*MoveList ml;
  B->generate_all(ml);*/

  MovePicker mp;
  set_movepicker(mp, Move::None);

  Move move;
  while (!is_empty(move = mp.get_next()))
  {
    if (!B->make(move, undo)) continue;

    /*for (int i = 0; i < ply(); i++)
      log("  ");
    log("{}\n", move);*/

    undo->curr = move;
    legal++;
    int new_depth = depth - 1;
    int reduction = 0;

    if (legal == 1)
      val = -pvs<PV>(-beta, -alpha, new_depth);
    else
    {
      val = -pvs<NonPV>(-alpha - 1, -alpha, new_depth - reduction);
      if (val > alpha && reduction > 0)
        val = -pvs<NonPV>(-alpha - 1, -alpha, new_depth);
      if (val > alpha && val < beta)
        val = -pvs<PV>(-beta, -alpha, new_depth);
    }

    B->unmake(move, undo);

    if (abort()) return alpha;

    if (val > alpha)
    {
      alpha = val;
      //hash_type = HashType::Exact;
      undo->best = move;

      if (val >= beta)
      {
        if (!is_attack(move) && !in_check)
        {
          update_moves_stats(depth);
        }

        //hash_type = HashType::Beta;
        break;
      }
    }

    if (!legal)
    {
      return in_check ? val : 0; // contempt();
    }
  }

  // TODO: if not abort store in tt

  return alpha;
}

int SolverPVS::qs(int alpha, int beta)
{
  max_ply = std::max(max_ply, ply());
  int stand_pat = E->eval(B, alpha, beta);
  if (stand_pat >= beta) return beta;
  if (stand_pat > alpha) alpha = stand_pat;

  if (ply() >= Limits::Plies) return stand_pat;

  MoveList ml;
  ml.clear();

  if (B->color) B->generate_attacks<White, true>(ml);
  else          B->generate_attacks<Black, true>(ml);

  while (!ml.empty())
  {
    Move move = ml.get_next();
    if (is_empty(move)) break;
    if (!B->make(move, undo)) continue;

    nodes++;
    undo->curr = move;

    int val = -qs(-beta, -alpha);

    B->unmake(move, undo);

    if (abort()) return alpha;

    if (val >= beta) return beta;
    if (val > alpha) alpha = val;
  }

  return alpha;
}

}
