#pragma once
#include <cassert>
#include "moves.h"

namespace eia {
                     //   32     16     16
using MoveVal = i64; // value | 0000 | move

INLINE int value(MoveVal mv) { return mv >> 32; } // compiler be smart
INLINE Move move(MoveVal mv) { return static_cast<Move>(mv & 0xFFFF); }

// only queen in QS; remove bishop in PVS
enum class PromMode { QS, PVS, ALL };

// to hold and pick moves
class MoveList
{
  MoveVal moves[256];
  MoveVal * first, * last, * curr;

public:
  MoveList()   { clear(); }
  void clear() { first = last = curr = &moves[0]; }

  bool   empty() const { return last == first; }
  size_t count() const { return last -  first; }
  //void   pop_front()   { remove_curr(); }
  Move   front()       { return move(*first); }

  //void remove_curr()   { remove(curr); }
  void reveal_pocket() { first = &moves[0]; }

  Move get_best(i64 lower_bound = I64_MIN);
  void remove_move(Move move);

  void add(Move move)
  {
    assert(count() < 256);
    *(last++) = move;
  }

  void add_move(SQ from, SQ to, MT mt = MT::Quiet)
  {
    add(to_move(from, to, mt));
  }

  template<bool QS>
  void add_prom(SQ from, SQ to)
  {
    add(to_move(from, to, QProm));

    if constexpr (!QS)
    {
      add(to_move(from, to, RProm));
      add(to_move(from, to, NProm));
      add(to_move(from, to, BProm));
    }
  }

  template<bool QS>
  void add_capprom(SQ from, SQ to)
  {
    add(to_move(from, to, QCapProm));

    if constexpr (!QS)
    {
      add(to_move(from, to, RCapProm));
      add(to_move(from, to, NCapProm));
      add(to_move(from, to, BCapProm));
    }
  }

private:
  void remove(MoveVal * ptr) { *ptr = *(--last); }
};

}
