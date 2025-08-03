#pragma once
#include <array>
#include "moves.h"

namespace eia::Zobrist {

extern u64 key[SQ_N][Piece_N];
extern u64 castle[Castling_N];
extern u64 ep[SQ_N + 1];
extern u64 turn;

}
