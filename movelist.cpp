#include "movelist.h"

Move MoveList::get_best(i64 lower_bound)
{
  curr = first;
  for (MoveVal * ptr = first + 1; ptr != last; ++ptr)
  {
    if (*ptr > *curr) curr = ptr;
  }

  if (*curr >= lower_bound) return move(*curr);

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


