#pragma once
#include <array>
#include "piece.h"

namespace eia {

using SQ_BB = std::array<u64, SQ_N>;
using SQ_Val = std::array<int, SQ_N>;

extern const std::array<SQ_BB, Piece_N> atts;
extern const std::array<SQ_Val, SQ_N> dir;
extern const std::array<SQ_BB, SQ_N> between;
extern const std::array<SQ_BB, Color_N> front_one;
extern const std::array<SQ_BB, Color_N> front;
extern const std::array<SQ_BB, Color_N> att_span;
extern const SQ_BB adj_files;

}
