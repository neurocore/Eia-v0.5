#pragma once
#include "board.h"
#include "movelist.h"

namespace eia {

enum class Stage
{
  Hash,
  GenCaps, GoodCaps,
  Killer1, Killer2, CounterMove,
  GenQuiets, Quiets, BadCaps,
  Done
};

class MovePicker
{
  Stage stage;
  Board * B;
  History * H;
  MoveList ml;
  Move hash_mv, killer[2], counter;

public:
  void init(Board * B, History * history, Move hash = Move::None);
  Move get_next(int skip_quiets = false);
  void pop_curr();
};

}
