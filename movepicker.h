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

struct MovePicker
{
  Stage stage;
  Board * B;
  History * H;
  MoveList ml;
  Move hash_mv, killer[2], counter;

  Move get_next(int skip_quiets = false);
};

}
