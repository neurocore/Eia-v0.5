#include <utility>
#include <format>
#include "board.h"
#include "tables.h"
#include "magics.h"
#include "timer.h"
#include "solver.h"

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
  state = State();
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

  state.bhash ^= color ? Empty : Zobrist::turn;
  //threefold ~= Key(state.hash, true);

  state.king_atts = king_attackers();
  state.threats = opp_attacks();

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

// used in SEE
INLINE u64 Board::get_all_attackers(u64 o, SQ sq) const
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

INLINE u64 Board::king_attackers(int opp) const
{
  return color ^ opp ? king_attrs<White>() : king_attrs<Black>();
}

INLINE u64 Board::opp_attacks() const
{
  return color ? opp_atts<White>() : opp_atts<Black>();
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

void Board::generate_legal(MoveList & ml)
{
  Undo undos[2];
  Undo * undo = &undos[0];
  MoveList pseudo;

  generate_all(pseudo);

  while (!pseudo.empty())
  {
    Move move = pseudo.get_next();

    if (make(move, undo))
    {
      unmake(move, undo);
      ml.add(move);
    }
  }
}

int Board::see(Move move) const
{
  auto least_valuable_piece = [&](u64 attadef, Color col, Piece & p) -> u64
  {
    for (p = BP ^ col; p <= (BK ^ col); p = p + 2)
    {
      const u64 subset = attadef & piece[p];
      if (subset) return lsb(subset);
    }
    return Empty;
  };

  auto consider_xrays = [&](u64 o, SQ sq) -> u64
  {
    u64 att = Empty;
    att |= o & diags() & b_att(o, sq);
    att |= o & ortho() & r_att(o, sq);
    return att;
  };

  const SQ from = get_from(move);
  const SQ to = get_to(move);

  const int value[] = { 100, 100, 325, 325, 325, 325, 500, 500, 1000, 1000, 20000, 20000, 0, 0 };
  int gain[32];
  int d = 0;
  Piece p = square[from];

  u64 o       = occ[0] | occ[1];
  u64 xrayers = o ^ (knights() | kings());
  u64 from_bb = bit(from);
  u64 attadef = get_all_attackers(o, to) | from_bb;
  gain[d]     = value[square[to]];

  do
  {
    //writeln(attadef.to_bitboard);
    //writeln(o.to_bitboard);
    //writefln("gain[%d] = %d", d, gain[d]);

    d++;
    gain[d]  = value[p] - gain[d - 1]; // speculative store, if defended
    attadef ^= from_bb; // reset bit in set to traverse
    o       ^= from_bb; // reset bit in temporary occupancy (for x-Rays)

    if (from_bb & xrayers) attadef |= consider_xrays(o, to);
    from_bb = least_valuable_piece(attadef, ~col(p), p);
  }
  while (from_bb);

  while (--d) gain[d - 1] = -max(-gain[d - 1], gain[d]);
  return gain[0];
}

Move Board::recognize(Move candidate)
{
  MoveList ml;
  Undo undos[2];
  Undo * undo = &undos[0];

  generate_all(ml);

  while (!ml.empty())
  {
    Move move = ml.get_next();
    bool success = make(move, undo);
    if (success) unmake(move, undo);

    if (success && similar(move, candidate))
    {
      return move;
    }
  }
  return Move::None;
}

// TODO: test on every generated move
bool Board::pseudolegal(Move move) const
{
  if (is_empty(move)) return false;

  const SQ from  = get_from(move);
  const SQ to    = get_to(move);
  const MT mt    = get_mt(move);
  const Piece p  = square[from];
  const Piece d  = square[to];
  const bool cap = is_cap(mt);
  const bool ep  = is_ep(mt);
  const u64 o    = occupied();

  if (p == NOP) return false;         // moving piece must exist
  if (col(p) != color) return false;    // turn must correspond
  if (is_garbage(mt)) return false;       // garbage move types (6 & 7)
  if (between[from][to] & o) return false;  // no obstruction for move
  if (!ep && cap ^ (d < NOP)) return false;   // cap flag match with dest
  if (d < NOP && col(d) == color) return false; // can't capture own piece

  const u64 mask = !only_one(state.king_atts) ? ~o : check_ray();
  const u64 att = atts[p][from] & (is_king(p) ? ~o : mask);

  if ((mt == Quiet) || cap) // simple move (quiet or capture)
  {
    if (!(att & bit(to))) return false; // piece can move that way
  }

  if (is_pawn(p))
  {
    if (several(state.king_atts)) return false;
    if (mt == Ep) return state.ep < NOP && to == state.ep;

    const u64 PromRank = color ? Rank7 : Rank2;

    if (mt == Cap || is_capprom(mt))
    {
      if (!(att & bit(to))) return false; // piece can move that way
      if (is_prom(mt)) return PromRank & bit(from); // correct promotion
      return true;
    }

    if (mt == PawnMove || is_prom(mt))
    {
      const u64 mov = pmov[color][from] & mask;
      if (!(mov & bit(to))) return false; // piece can move that way
      if (is_prom(mt)) return PromRank & bit(from); // correct promotion
      return true;
    }

    return false;
  }
  else if (is_king(p))
  {
    if (mt == KCastle)
    {
      if (state.king_atts) return false;

      if (color)
      {
        return !!(state.castling & Castling::WK)
             && !(o & Span_WK) && (to == G1)
             && !(state.threats & Path_WK);
      }
      else
      {
        return !!(state.castling & Castling::BK)
             && !(o & Span_BK) && (to == G8)
             && !(state.threats & Path_BK);
      }
    }
    else if (mt == QCastle)
    {
      if (state.king_atts) return false;

      if (color)
      {
        return !!(state.castling & Castling::WQ)
             && !(o & Span_WQ) && (to == C1)
             && !(state.threats & Path_WQ);
      }
      else
      {
        return !!(state.castling & Castling::BQ)
             && !(o & Span_BQ) && (to == C8)
             && !(state.threats & Path_BQ);
      }
    }
    else return is_castle(mt) || mt == Quiet;
  }
  
  return !is_castle(mt) && !is_prom(mt) && !is_pawn(mt) && !is_ep(mt)
      && !several(state.king_atts);
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
  state.bhash ^= Zobrist::turn;
  //threefold ~= Key(state.hash, irreversible);

  if (in_check(1))
  {
    unmake(move, undo);
    return false;
  }

  state.king_atts = king_attackers();
  state.threats = opp_attacks();

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

void Board::make_null(Undo *& undo)
{
  undo->state = state;
  undo++;

  color = ~color;
  state.ep = SQ_N;
  state.bhash ^= Zobrist::turn;

  //threefold ~= Key(state.hash, true);
}

void Board::unmake_null(Undo *& undo)
{
  color = ~color;
    
  undo--;
  state = undo->state;

  /*assert(threefold.length > 0);
  threefold.popBack();*/
}

}




