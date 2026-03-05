#include "movepicker.h"

namespace eia {

template<>
Move MovePickerPVS::get_next(bool do_quiets)
{
  switch (stage)
  {
    case Stage::Hash:

      stage = Stage::GenCaps;
      if (!is_empty(hash_mv)) return hash_mv;

      [[fallthrough]];

    case Stage::GenCaps:

      stage = Stage::GoodCaps;
      if (B->color) B->generate_attacks<White, false>(ml);
      else          B->generate_attacks<Black, false>(ml);

      ml.remove_move(hash_mv);
      ml.value_attacks(B);

      [[fallthrough]];

    case Stage::GoodCaps:

      if (!ml.empty())
      {
        Move mv = ml.get_best(O_EqCap);
        if (!is_empty(mv)) return mv;
      }
      ml.put_to_pocket();

      if (do_quiets)
      {
        stage = Stage::Killer1;
      }
      else
      {
        stage = Stage::BadCaps;
        return get_next(do_quiets);
      }

      [[fallthrough]];

    case Stage::Killer1:

      stage = Stage::Killer2;
      if (do_quiets
      &&  killer[0] != hash_mv
      &&  B->pseudolegal(killer[0])) return killer[0];

      [[fallthrough]];

    case Stage::Killer2:

      stage = Stage::CounterMove;
      if (do_quiets
      &&  killer[1] != hash_mv
      &&  B->pseudolegal(killer[1])) return killer[1];

      [[fallthrough]];

    case Stage::CounterMove:

      stage = Stage::GenQuiets;
      if (do_quiets
      &&  counter != hash_mv
      &&  counter != killer[0]
      &&  counter != killer[1]
      &&  B->pseudolegal(counter)) return counter;

      [[fallthrough]];

    case Stage::GenQuiets:

      stage = Stage::Quiets;

      if (do_quiets)
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

      if (do_quiets
      && !ml.empty())
      {
        Move mv = ml.get_best(O_Quiet);
        if (!is_empty(mv)) return mv;
      }
      ml.reveal_pocket();
      stage = Stage::BadCaps;

      [[fallthrough]];

    case Stage::BadCaps:

      if (!ml.empty())
      {
        Move mv = ml.get_best();
        if (!is_empty(mv)) return mv;
      }
      break;

    default:

      return Move::None;
  };
  
  stage = Stage::Done;
  return Move::None;
}


template<>
Move MovePickerQS::get_next(bool do_quiets)
{
  switch (stage)
  {
    case Stage::Hash:

      stage = Stage::GenCaps;
      if (!is_empty(hash_mv)) return hash_mv;

      [[fallthrough]];

    case Stage::GenCaps:

      stage = Stage::GoodCaps;
      if (B->color) B->generate_attacks<White, true>(ml);
      else          B->generate_attacks<Black, true>(ml);

      ml.remove_move(hash_mv);
      ml.value_attacks(B);

      [[fallthrough]];

    case Stage::GoodCaps:

      if (!ml.empty())
      {
        Move mv = ml.get_best(O_EqCap);
        if (!is_empty(mv)) return mv;
      }
      ml.put_to_pocket();

      stage = Stage::BadCaps;
      
      [[fallthrough]];

    case Stage::GenChecks:
      
      if (do_quiets)
      {
        stage = Stage::Checks;
        if (B->color) B->generate_checks<White>(ml);
        else          B->generate_checks<Black>(ml);

        ml.remove_move(hash_mv);
        //ml.remove_move(mthreat);
        //ml.value_checks(B);
      }

      [[fallthrough]];

    case Stage::Checks:

      if (do_quiets)
      {
        if (!ml.empty())
        {
          Move mv = ml.get_best();
          if (!is_empty(mv)) return mv;
        }
        stage = Stage::BadCaps;
      }
      ml.reveal_pocket();

      [[fallthrough]];

    case Stage::BadCaps:

      if (!ml.empty())
      {
        Move mv = ml.get_best();
        if (!is_empty(mv)) return mv;
      }
      break;

    case Stage::GenEvasions:

      stage = Stage::Evasions; // only legal here
      if (B->color) B->generate_evasions<White>(ml);
      else          B->generate_evasions<Black>(ml);

      evasions_cnt = static_cast<int>(ml.count());

      [[fallthrough]];
      
    case Stage::Evasions:

      if (!ml.empty())
      {
        Move mv = ml.get_best();
        if (!is_empty(mv)) return mv;
      }
      break;


    default:

      return Move::None;
  };
  
  stage = Stage::Done;
  return Move::None;
}

}