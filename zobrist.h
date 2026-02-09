#pragma once
#include <array>
#include "moves.h"

namespace eia::Zobrist {

extern u64 key[Piece_N][SQ_N];
extern u64 castle[Castling_N];
extern u64 ep[SQ_N + 1];
extern u64 turn;

extern u64 pk_key[Piece_N][SQ_N];

}
