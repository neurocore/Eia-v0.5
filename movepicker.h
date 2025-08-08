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
  GenChecks, Checks,
  Done
};

template<bool QS>
struct MovePicker
{
  Stage stage;
  Board * B;
  History * H;
  MoveList ml;
  Move hash_mv, killer[2], counter;

  Move get_next(bool checks = true);
};

using MovePickerPVS = MovePicker<false>;
using MovePickerQS = MovePicker<true>;


template<bool QS>
Move MovePicker<QS>::get_next(bool checks)
{
  switch (stage)
  {
    case Stage::Hash:

      log("Stage::Hash\n");
      stage = Stage::GenCaps;
      if (!is_empty(hash_mv)) return hash_mv;

      [[fallthrough]];

    case Stage::GenCaps:

      log("Stage::GenCaps\n");
      stage = Stage::GoodCaps;
      if (B->color) B->generate_attacks<White, QS>(ml);
      else          B->generate_attacks<Black, QS>(ml);

      ml.remove_move(hash_mv);
      ml.value_attacks(B);

      [[fallthrough]];

    case Stage::GoodCaps:

      log("Stage::GoodCaps\n");
      if (!ml.empty())
      {
        Move mv = ml.get_best(O_EqCap);
        if (!is_empty(mv)) return mv;
      }
      ml.put_to_pocket();

      if constexpr (QS)
      {
        stage = Stage::GenChecks;
        return get_next(checks);
      }
      stage = Stage::Killer1;

      [[fallthrough]];

    case Stage::Killer1:

      log("Stage::Killer1\n");
      stage = Stage::Killer2;
      if (killer[0] != hash_mv
      &&  B->pseudolegal(killer[0])) return killer[0];

      [[fallthrough]];

    case Stage::Killer2:

      log("Stage::Killer2\n");
      stage = Stage::CounterMove;
      if (killer[1] != hash_mv
      &&  B->pseudolegal(killer[1])) return killer[1];

      [[fallthrough]];

    case Stage::CounterMove:

      log("Stage::CounterMove\n");
      stage = Stage::GenQuiets;
      if (counter != hash_mv
      &&  counter != killer[0]
      &&  counter != killer[1]
      &&  B->pseudolegal(counter)) return counter;

      [[fallthrough]];

    case Stage::GenQuiets:

      log("Stage::GenQuiets\n");
      stage = Stage::Quiets;

      if (B->color) B->generate_quiets<White>(ml);
      else          B->generate_quiets<Black>(ml);
      
      ml.remove_move(hash_mv);
      ml.remove_move(killer[0]);
      ml.remove_move(killer[1]);
      ml.remove_move(counter);
      ml.value_quiets(B, *H);

      [[fallthrough]];

    case Stage::Quiets:

      log("Stage::Quiets\n");
      if (!ml.empty())
      {
        Move mv = ml.get_best(O_Quiet);
        if (!is_empty(mv)) return mv;
      }
      ml.reveal_pocket();
      stage = Stage::BadCaps;

      [[fallthrough]];

    case Stage::BadCaps:

      log("Stage::BadCaps\n");
      if (!ml.empty())
      {
        Move mv = ml.get_best();
        if (!is_empty(mv)) return mv;
      }
      break;

    case Stage::GenChecks:

      log("Stage::GenChecks\n");
      if (!B->state.checkers) break;
      stage = Stage::Checks;

      if (B->color) B->generate_checks<White>(ml);
      else          B->generate_checks<Black>(ml);

      [[fallthrough]];

    case Stage::Checks:

      log("Stage::Checks\n");
      if (!ml.empty())
      {
        Move mv = ml.get_next();
        if (!is_empty(mv)) return mv;
      }
      break;

    default:

      log("Stage::Done\n");
      return Move::None;
  };
  
  stage = Stage::Done;
  return Move::None;
}

}
