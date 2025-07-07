#pragma once
#include "board.h"
#include "movelist.h"

enum class Stage
{
  Hash,
  GenCaps, WinCaps, EqCaps,
  GenKillers, Killers,
  GenQuiets, Quiets, BadCaps,
  Done
};

class MovePicker
{
  Stage stage;
  Board * B;
  MoveList ml;
  Move hash_mv, killer[2], counter;

  MovePicker(Board * board)
  {
    B = board;
    hash_mv = Move::None;
  }
};

