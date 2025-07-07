#include <utility>
#include "board.h"
#include "magics.h"
#include "piece.h"

void Board::clear()
{
  for (Piece p = BP; p < Piece_N; ++p) piece[p] = Empty;
  for (SQ sq = A1; sq < SQ_N; ++sq) square[sq] = NOP;

  color = White;
  occ[0] = occ[1] = Empty;
  state = State{SQ_N, NOP, Castling::ALL, 0, Empty, Empty, Empty};
  //threefold.clear();
}

int Board::phase() const
{
  int phase = Phase::Total
            - Phase::Queen * popcnt(queens())
            - Phase::Rook  * popcnt(rooks())
            - Phase::Light * popcnt(lights());

  return std::max(phase, 0);
}

bool Board::is_draw() const
{
  return false; // TODO
}

bool Board::is_attacked(SQ king, u64 o, int opp) const
{
  const Color c = color ^ opp;

  if (atts[BN ^ c][king] & piece[WN ^ c]) return true; // Knights
  if (atts[BP ^ c][king] & piece[WP ^ c]) return true; // Pawns
  if (atts[BK ^ c][king] & piece[WK ^ c]) return true; // King

  const u64 bq = piece[WB ^ c] | piece[WQ ^ c]; // Diagonal
  const u64 rq = piece[WR ^ c] | piece[WQ ^ c]; // Orthogonal

  if (b_att(o, king) & bq) return true;
  if (r_att(o, king) & rq) return true;

  return false;
}

u64 Board::get_attacks(u64 o, SQ sq) const
{
  u64 att = Empty;
  att |= b_att(o, sq) & diags();
  att |= r_att(o, sq) & ortho();
  att |= atts[WP][sq] & piece[BP];
  att |= atts[BP][sq] & piece[WP];
  att |= atts[BN][sq] & knights();
  att |= atts[BK][sq] & kings();
  return att;
}

bool Board::in_check(int opp) const
{
  const Piece p = to_piece(King, color ^ opp);
  const SQ king = bitscan(piece[p]);
  return is_attacked(king, occupied(), opp);
}

bool Board::make(Move move, Undo *& undo)
{
  assert(!is_empty(move));

  const SQ from = get_from(move);
  const SQ to   = get_to(move);
  const MT mt   = get_mt(move);
  const Piece p = square[from];
  bool irreversible = true;

  (undo++)->state = state;

  state.castling &= uncastle[from] & uncastle[to];    
  state.cap = square[to];
  state.ep = SQ_N;
  state.fifty++;

  switch (mt)
  {
    case Cap:
    {
      state.fifty = 0;
      remove(from);
      remove(to);
      place(to, p);
      break;
    }

    case KCastle:
    {
      remove(from);
      remove(to + 1);
      place(to, p);
      place(to - 1, to_piece(Rook, color));
      break;
    }

    case QCastle:
    {
      remove(from);
      remove(to - 2);
      place(to, p);
      place(to + 1, to_piece(Rook, color));
      break;
    }

    case NProm: case BProm: case RProm: case QProm:
    {
      state.fifty = 0;
      const Piece prom = promoted(move, color);

      remove(from);
      place(to, prom);
      break;
    }

    case NCapProm: case BCapProm: case RCapProm: case QCapProm:
    {
      state.fifty = 0;
      const Piece prom = promoted(move, color);

      remove(from);
      remove(to);
      place(to, prom);
      break;
    }

    case Ep:
    {
      state.fifty = 0;
      const SQ cap = to_sq(file(to), rank(from));

      remove(cap);
      remove(from);
      place(to, p);
      break;
    }

    case Pawn2: // ---------- FALL THROUGH! ------------
    {
      state.ep = middle(from, to);
    }

    default:
    {
      assert(square[to] == NOP);
      remove(from);
      place(to, p);

      if (is<Pawn>(p))
        state.fifty = 0;
      else
        irreversible = false;
    }
  }

  color = ~color;
  //state.bhash ^= hash_wtm[0];
  //threefold ~= Key(state.hash, irreversible);

  if (in_check(1))
  {
    unmake(move, undo);
    return false;
  }

  switch (mt)
  {
    case KCastle: case QCastle:
    {
      const u64 o = occupied();
      const SQ mid = middle(from, to);

      if (is_attacked(from, o, 1)
      ||  is_attacked(mid, o, 1)
      ||  is_attacked(to, o, 1))
      {
        unmake(move, undo);
        return false;
      }
      break;
    }
    default: break;
  }

  undo->curr = move;
  return true;
}

void Board::unmake(Move move, Undo *& undo)
{
  assert(!is_empty(move));

  const SQ from = get_from(move);
  const SQ to   = get_to(move);
  const MT mt   = get_mt(move);
  const Piece p = square[from];

  //assert(threefold.length > 0);
  //threefold.popBack();

  color = ~color;

  switch (mt)
  {
    case Cap:
    {
      assert(state.cap != NOP);
      remove<false>(to);
      place<false>(from, p);
      place<false>(to, state.cap);
      break;
    }

    case KCastle:
    {
      remove<false>(to - 1);
      remove<false>(to);
      place<false>(to + 1, to_piece(Rook, color));
      place<false>(from, p);
      break;
    }

    case QCastle:
    {
      remove<false>(to + 1);
      remove<false>(to);
      place<false>(to - 2, to_piece(Rook, color));
      place<false>(from, p);
      break;
    }

    case NProm: case BProm: case RProm: case QProm:
    {
      remove<false>(to);
      place<false>(from, to_piece(Pawn, color));
      break;
    }

    case NCapProm: case BCapProm: case RCapProm: case QCapProm:
    {
      remove<false>(to);
      place<false>(from, to_piece(Pawn, color));
      place<false>(to, state.cap);
      break;
    }

    case Ep:
    {
      const SQ cap = to_sq(file(to), rank(from));

      remove<false>(to);
      place<false>(cap, opp(p));
      place<false>(from, p);
      break;
    }

    default:
    {
      remove<false>(to);
      place<false>(from, p);
    }
  }

  state = (--undo)->state;
}






