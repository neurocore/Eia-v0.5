#include "movelist.h"
#include "board.h"

namespace eia {

bool MoveList::contains(Move move) const
{
  for (MoveVal * ptr = first; ptr < last; ++ptr)
  {
    if (*ptr == move) return true;
  }
  return false;
}

Move MoveList::get_best(u64 lower_bound)
{
  assert(first < last);

  MoveVal * best = first;
  for (MoveVal * ptr = first + 1; ptr < last; ++ptr)
  {
    if (value(*ptr) > value(*best)) best = ptr;
  }

  if (*best >= lower_bound)
  {
    Move mv = move(*best);
    remove(best);
    return mv;
  }

  first = last; // putting all moves into pocket
  return Move::None;
}

void MoveList::remove_move(Move move)
{
  for (MoveVal * ptr = first; ptr < last; ++ptr)
  {
    if (*ptr == move)
    {
      remove(ptr);
      break;
    }
  }
}

u64 MoveList::value_attack(Move mv, const Board * B)
{
  static const int cost[] = {1, 1, 3, 3, 3, 3, 5, 5, 9, 9, 200, 200, 0, 0};
  static const int prom[] = {0, cost[WN], cost[WB], cost[WR], cost[WQ], 0};

  if (is_ep(mv)) return O_EqCap;

  const SQ from = get_from(move(mv));
  const SQ to   = get_to(move(mv));
  const MT mt   = get_mt(move(mv));

  const int a = cost[B->square[from]]; // attacker
  const int v = cost[B->square[to]];  // victim

  if (is_prom(mt))
  {
    int p = prom[promoted(mt)];
    return O_WinCap + 100 * (p + v) - a;
  }
  else if (is_cap(mt))
  {
    int score = 10000 * B->see(mv);
    u64 order = compare(score, 0, O_BadCap, O_EqCap, O_WinCap);
    score = order == O_EqCap ? 100 * v - a : score;
    return order + score;
  }
  return O_BadCap;
}

void MoveList::value_attacks(const Board * B)
{
  for (MoveVal * ptr = first; ptr != last; ptr++)
  {
    const Move mv = move(*ptr);
    *ptr += value_attack(mv, B) << 32;
  }
}

void MoveList::value_quiets(const Board * B, const History & history)
{
  for (MoveVal * ptr = first; ptr != last; ptr++)
  {
    const Move mv = move(*ptr);
    const SQ from = get_from(move(mv));
    const SQ to   = get_to(move(mv));

    const bool leave_threat = B->state.threats & bit(from);
    const bool enter_threat = B->state.threats & bit(to);
    
    u64 val = history[B->color][leave_threat][enter_threat][from][to];
    *ptr += (O_Quiet + val) << 32;
  }
}

}
