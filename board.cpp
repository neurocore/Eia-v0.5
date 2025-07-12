#include <utility>
#include <format>
#include "board.h"
#include "tables.h"
#include "magics.h"
#include "timer.h"
#include "search.h"

using namespace std;

namespace eia {

Board::Board(const Board & board)
{
  for (Piece p = BP; p < Piece_N; ++p) piece[p] = board.piece[p];
  for (SQ sq = A1; sq < SQ_N; ++sq) square[sq] = board.square[sq];

  occ[0] = board.occ[0];
  occ[1] = board.occ[1];
  color  = board.color;
  state  = board.state;
}

void Board::clear()
{
  for (Piece p = BP; p < Piece_N; ++p) piece[p] = Empty;
  for (SQ sq = A1; sq < SQ_N; ++sq) square[sq] = NOP;

  color = White;
  occ[0] = occ[1] = Empty;
  //threefold.clear();
}

INLINE int Board::phase() const
{
  int phase = Phase::Total
            - Phase::Queen * popcnt(queens())
            - Phase::Rook  * popcnt(rooks())
            - Phase::Light * popcnt(lights());

  return max(phase, 0);
}

INLINE bool Board::is_draw() const
{
  return false; // TODO
}

bool Board::set(string fen)
{
  SQ sq = A8;
  clear();

  string fen_board = cut(fen); // parsing main part
  for (char ch : fen_board)
  {
    if (isdigit(ch)) sq += ch - '0';
    else
    {
      Piece p = to_piece(ch);
      if (p == NOP) continue;

      place(sq, p);
      ++sq;
    }

    if (!(sq & 7)) // row wrap
    {
      sq -= 16;
      if (sq < 0) break;
    }
  }

  string fen_color = cut(fen); // parsing color
  for (char ch : fen_color)
    color = to_color(ch);

  string fen_castling = cut(fen); // parsing castling
  state.castling = Castling::NO;
  for (char ch : fen_castling)
  {
    state.castling |= to_castling(ch);
  }

  string fen_ep = cut(fen); // parsing en passant
  state.ep = to_sq(fen_ep);

  string fen_fifty = cut(fen); // fifty move counter
  state.fifty = parse_int(fen_fifty);

  // full move counter - no need

  //state.bhash ^= hash_wtm[color];
  //threefold ~= Key(state.hash, true);

  return true;
}

std::string Board::to_fen()
{
  std::string fen; // TODO
  return fen;
}

bool Board::is_correct(std::string & details) const
{
  // piece[] -> square[] mapping
  for (Piece p = BP; p < Piece_N; ++p)
  {
    for (u64 bb = piece[p]; bb; bb = rlsb(bb))
    {
      SQ sq = bitscan(bb);
      if (square[sq] != p)
      {
        details += format("square[{}] = {} != {}\n", sq, square[sq], p);
      }
    }
  }

  // square[] -> piece[] mapping
  for (SQ sq = A1; sq < SQ_N; ++sq)
  {
    Piece p = square[sq];
    if (p < NOP && !(piece[p] & bit(sq)))
    {
      details += format("piece[{}] not have bit({})\n", p, sq);
    }
  }

  return details.empty();
}

bool Board::operator == (const Board & board) const
{
  if (color  != board.color)  return false;
  if (occ[0] != board.occ[0]) return false;
  if (occ[1] != board.occ[1]) return false;

  for (Piece p = BP; p < Piece_N; ++p)
    if (piece[p] != board.piece[p]) return false;

  for (SQ sq = A1; sq < SQ_N; ++sq)
    if (square[sq] != board.square[sq]) return false;

  state;

  return true;
}

void Board::print() const
{
  for (int y = 7; y >= 0; --y)
  {
    cout << ' ' << (y + 1) << " | ";

    for (int x = 0; x < 8; ++x)
    {
      SQ sq = to_sq(x, y);
      Piece p = square[sq];
      
      cout << to_char(p) << ' ';
    }
    cout << "\n";
  }
  cout << "   +----------------  ";
  cout << (color ? "<W>" : "<B>") << " ";
  cout << to_string(state.castling) << "\n";
  cout << "     a b c d e f g h   \n\n";
}

void Board::print(Move move) const
{
  cout << move; // TODO: make prettifier
}

INLINE bool Board::is_attacked(SQ sq, u64 o, int opp) const
{
  const Color c = color ^ opp;

  if (atts[BN ^ c][sq] & piece[WN ^ c]) return true; // Knights
  if (atts[BP ^ c][sq] & piece[WP ^ c]) return true; // Pawns
  if (atts[BK ^ c][sq] & piece[WK ^ c]) return true; // King

  const u64 bq = piece[WB ^ c] | piece[WQ ^ c]; // Diagonal
  const u64 rq = piece[WR ^ c] | piece[WQ ^ c]; // Orthogonal

  if (b_att(o, sq) & bq) return true;
  if (r_att(o, sq) & rq) return true;

  return false;
}

INLINE u64 Board::get_attacks(u64 o, SQ sq) const
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

INLINE bool Board::in_check(int opp) const
{
  const Piece p = to_piece(King, color ^ opp);
  const SQ king = bitscan(piece[p]);
  return is_attacked(king, occupied(), opp);
}

INLINE bool Board::castling_attacked(SQ from, SQ to) const
{
  const u64 o = occupied();
  const SQ mid = middle(from, to);

  return is_attacked(from, o)
      || is_attacked(mid, o)
      || is_attacked(to, o);
}

INLINE void Board::generate_all(MoveList & ml) const
{
  if (color)
  {
    generate_attacks<White>(ml);
    generate_quiets<White>(ml);
  }
  else
  {
    generate_attacks<Black>(ml);
    generate_quiets<Black>(ml);
  }
}

Move Board::recognize(Move candidate)
{
  MoveList ml;
  Undo undos[2];
  Undo * undo = &undos[0];

  generate_all(ml);

  while (!ml.is_empty())
  {
    Move move = ml.get_next();
    bool success = make(move, undo);
    unmake(move, undo);

    if (success && similar(move, candidate))
    {
      //cout << "recognized " << move << " as " << candidate << "\n";
      return move;
    }
  }
  return Move::None;
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

    case PawnMove:
    {
      state.fifty = 0;
      remove(from);
      place(to, p);
      state.ep = ep_square[from][to];
      break;
    }

    default:
    {
      assert(square[to] == NOP);
      irreversible = false;
      remove(from);
      place(to, p);
      break;
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

  undo->curr = move;
  return true;
}

void Board::unmake(Move move, Undo *& undo)
{
  assert(!is_empty(move));

  const SQ from = get_from(move);
  const SQ to   = get_to(move);
  const MT mt   = get_mt(move);
  const Piece p = square[to];

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

}




