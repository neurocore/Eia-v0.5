#include "movepicker.h"

namespace eia {

void MovePicker::init(Board * board, History * history, Move hash)
{
  B = board;
  H = history;
  hash_mv = hash;
  killer[0] = B->state.killer[0];
  killer[1] = B->state.killer[1];
  counter   = B->state.counter;

  stage = B->pseudolegal(hash) ? Stage::GenCaps : Stage::Hash;
}

Move MovePicker::get_next(int skip_quiets)
{
  switch (stage)
  {
    case Stage::Hash:
      log("Stage::Hash\n");
      stage = Stage::GenCaps;
      if (!is_empty(hash_mv)) return hash_mv;

    case Stage::GenCaps:
      log("Stage::GenCaps\n");
      stage = Stage::GoodCaps;
      if (B->color) B->generate_attacks<White>(ml);
      else          B->generate_attacks<Black>(ml);

      ml.remove_move(hash_mv);
      ml.value_attacks(B);

    case Stage::GoodCaps:
      log("Stage::GoodCaps\n");
      if (!ml.empty())
      {
        Move mv = ml.get_best(O_EqCap);
        if (!is_empty(mv)) return mv;
      }
      stage = Stage::Killer1;

    case Stage::Killer1:
      log("Stage::Killer1\n");
      stage = Stage::Killer2;
      if (B->pseudolegal(killer[0])) return killer[0];

    case Stage::Killer2:
      log("Stage::Killer2\n");
      stage = Stage::GenQuiets;
      if (B->pseudolegal(killer[1])) return killer[1];

    case Stage::GenQuiets:
      log("Stage::GenQuiets\n");
      stage = Stage::Quiets;
      if (B->color) B->generate_quiets<White>(ml);
      else          B->generate_quiets<Black>(ml);

      ml.remove_move(hash_mv);
      ml.remove_move(killer[0]);
      ml.remove_move(killer[1]);
      ml.value_quiets(B, *H);

    case Stage::Quiets:
      log("Stage::Quiets\n");
      if (!ml.empty())
      {
        Move mv = ml.get_best(O_Quiet);
        if (!is_empty(mv)) return mv;
      }
      ml.reveal_pocket();
      stage = Stage::BadCaps;

    case Stage::BadCaps:
      log("Stage::BadCaps\n");
      if (!ml.empty())
      {
        Move mv = ml.get_best();
        if (!is_empty(mv)) return mv;
      }
      stage = Stage::Done;

    default:
      log("Stage::Done\n");      
      return Move::None;
  }
  
  return Move::None;
 }

void MovePicker::pop_curr()
{
  ml.remove_curr();
}

}
