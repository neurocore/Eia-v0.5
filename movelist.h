#pragma once
#include <cassert>
#include "moves.h"

namespace eia {
                     //   32     16     16
using MoveVal = u64; // value | 0000 | move

INLINE int value(MoveVal mv) { return mv >> 32; } // compiler be smart
INLINE Move move(MoveVal mv) { return static_cast<Move>(mv & 0xFFFF); }

// only queen in QS; remove bishop in PVS
enum class PromMode { QS, PVS, ALL };

struct Board;

// TODO: make cyclic?
// to hold and pick moves
class MoveList
{
  MoveVal moves[256];
  MoveVal * first, * last, * curr;

public:
  MoveList()    { clear(); }
  void clear()  { first = last = curr = &moves[0]; }
  void rewind() { curr = &moves[0]; }

  bool   empty() const { return last == first; }
  size_t count() const { return last -  first; }

  Move   get_next()    { return move(*(curr++)); }
  bool   is_empty()    { return curr >= last; }

  void remove_curr()   { remove(curr); }
  void reveal_pocket() { first = &moves[0]; }

  Move get_best(u64 lower_bound = 0ull);
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

  void value_attacks(const Board * B);
  void value_quiets(const Board * B, const History & history);

private:
  // TODO: rid of mixing
  void remove(MoveVal * ptr) { *ptr = *(--last); }
};

enum Order : int
{
  O_Hash    = 0x50000000,
  O_WinCap  = 0x40000000,
  O_EqCap   = 0x30000000,
  O_Killer1 = 0x20000001,
  O_Killer2 = 0x20000000,
  O_Quiet   = 0x10000000,
  O_BadCap  = 0x00000000
};

}
