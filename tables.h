#pragma once
#include <array>
#include "piece.h"

namespace eia {

using SQ_BB  = std::array<u64, SQ_N + 1>;
using SQ_SQ  = std::array<SQ,  SQ_N + 1>;
using SQ_Val = std::array<int, SQ_N + 1>;

// front_one  front  att_span  att_rear  isolator  psupport  kingzone
//
//   ---       -x-     x-x       ---       x-x       ---       xxx
//   -x-       -x-     x-x       ---       x-x       ---       xxx
//   -O-       -O-     -O-       xOx       xOx       xOx       xOx
//   ---       ---     ---       x-x       x-x       x-x       xxx
//   ---       ---     ---       x-x       x-x       ---       ---

// isolator = att_span | att_rear
// psupport = (rank | backrank) & isolator

extern const std::array<SQ_BB, Piece_N> atts;
extern const std::array<SQ_BB, Color_N> pmov;
extern const std::array<SQ_Val, SQ_N + 1> dir;
extern const std::array<SQ_BB, SQ_N + 1> between;
extern const std::array<SQ_BB, Color_N> front_one;
extern const std::array<SQ_BB, Color_N> front;
extern const std::array<SQ_BB, Color_N> att_span;
extern const std::array<SQ_BB, Color_N> att_rear;
extern const std::array<SQ_BB, Color_N> psupport;
extern const std::array<SQ_BB, Color_N> kingzone;
extern const std::array<SQ_SQ, SQ_N + 1> ep_square;
extern const SQ_BB isolator;

}
