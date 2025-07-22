#include "movepicker.h"

namespace eia {

Move MovePicker::get_next(int skip_quiets)
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
      if (B->color) B->generate_attacks<White>(ml);
      else          B->generate_attacks<Black>(ml);

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

      if (skip_quiets)
      {
        stage = Stage::BadCaps;
        return get_next(skip_quiets);
      }
      //stage = Stage::Killer1;
      stage = Stage::GenQuiets;

      [[fallthrough]];

    //case Stage::Killer1:

    //  log("Stage::Killer1\n");
    //  stage = Stage::Killer2;
    //  if (!skip_quiets
    //  &&  killer[0] != hash_mv
    //  &&  B->pseudolegal(killer[0])) return killer[0];

    //  [[fallthrough]];

    //case Stage::Killer2:

    //  log("Stage::Killer2\n");
    //  //stage = Stage::CounterMove;
    //  stage = Stage::GenQuiets;
    //  if (!skip_quiets
    //  &&  killer[1] != hash_mv
    //  &&  B->pseudolegal(killer[1])) return killer[1];

    //  [[fallthrough]];

    /*case Stage::CounterMove:

      log("Stage::CounterMove\n");
      stage = Stage::GenQuiets;
      if (!skip_quiets
      &&  counter != hash_mv
      &&  counter != killer[0]
      &&  counter != killer[1]
      &&  B->pseudolegal(counter)) return counter;

      [[fallthrough]];*/

    case Stage::GenQuiets:

      log("Stage::GenQuiets\n");
      stage = Stage::Quiets;
      if (!skip_quiets)
      {
        if (B->color) B->generate_quiets<White>(ml);
        else          B->generate_quiets<Black>(ml);

        ml.remove_move(hash_mv);
        ml.remove_move(killer[0]);
        ml.remove_move(killer[1]);
        ml.remove_move(counter);
        ml.value_quiets(B, *H);
      }

      [[fallthrough]];

    case Stage::Quiets:

      log("Stage::Quiets\n");
      if (!skip_quiets && !ml.empty())
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
      stage = Stage::Done;

      [[fallthrough]];

    default:

      log("Stage::Done\n");
      return Move::None;
  };
  
  return Move::None;
 }

}
