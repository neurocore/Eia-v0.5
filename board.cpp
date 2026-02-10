#include <utility>
#include <format>
#include "board.h"
#include "tables.h"
#include "magics.h"
#include "timer.h"
#include "solver.h"
#include "moves.h"

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
  moves_cnt = board.moves_cnt;
}

void Board::clear()
{
  for (Piece p = BP; p < Piece_N; ++p) piece[p] = Empty;
  for (SQ sq = A1; sq < SQ_N; ++sq) square[sq] = NOP;

  color = White;
  occ[0] = occ[1] = Empty;
  state = State();
  state_ptr = &states[0];

  moves_cnt = 0;
  for (int i = 0; i < 8192; ++i)
    threefold[i] = 0ull;
}

void Board::revert_states()
{
  state_ptr = &states[0];
}

int Board::phase() const
{
  int phase = Phase::Total
            - Phase::Queen * popcnt(queens())
            - Phase::Rook  * popcnt(rooks())
            - Phase::Light * popcnt(lights());

  return (std::max)(phase, 0);
}

bool Board::is_draw() const
{
  if (state.fifty > 99 || is_repetition()) return true;

  // Insufficient material

  const int total = popcnt(occupied());

  if (total == 2)
  {
    return true; // KK
  }
  else if (total == 3)
  {
    if (lights()) return true; // KLK
  }
  else if (total == 4)
  {
    if (several(knights())) return true; // KNNK, KNKN

    if (only_one(lights<White>())
    &&  only_one(lights<Black>()))
      return true; // KLKL
  }
  return false;
}

bool Board::is_repetition() const
{
  const u64 key = hash();

  // Going until first irreversible move
  // Position must repeat twice

  for (int i = moves_cnt - 5; i >= 0; i -= 2)
  {
    if (i < moves_cnt - state.fifty) break;
    if (threefold[i] == key) return true;
  }
  return false;
}

bool Board::is_simply_mated() const
{
  if (!state.checkers) return false;

  const u64 o = occ[color] | state.threats;
  const SQ  ksq = bitscan(piece[BK ^ color]);
  const u64 katt = atts[BK][ksq];
  const u64 kmov = (o & katt) ^ katt;

  if (!kmov) // no evasion
  {
    // 1. Double check

    if (several(state.checkers)) return true;

    // 2. Trivial cases

    const u64 unblockable = katt | piece[WP ^ color]  // Non-slider or
                                 | piece[WN ^ color]; // contact check

    if (state.checkers & unblockable)
    {
      const SQ checker = bitscan(state.checkers); // Verify it can't
      return !is_attacked(checker, occupied());   //  be captured
    }
  }
  return false;
}

u64 Board::calc_hash() const
{
  u64 key = 0ull;
  for (Piece p = BP; p < Piece_N; ++p)
  {
    for (u64 bb = piece[p]; bb; bb = rlsb(bb))
    {
      SQ sq = bitscan(bb);
      key ^= Zobrist::key[p][sq];
    }
  }

  key ^= Zobrist::castle[(u8)state.castling]
      ^  Zobrist::ep[state.ep];

  return color ? key : key ^ Zobrist::turn;
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
  state.pkhash ^= color ? Empty : Zobrist::turn;

  moves_cnt = 0;

  state.checkers = king_attackers();
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

INLINE u64 Board::attack(Piece p, SQ sq) const
{
  switch(pt(p))
  {
    case Pawn:
    case King:
    case Knight: return atts[p][sq];
    case Rook:   return r_att(occupied(), sq);
    case Bishop: return b_att(occupied(), sq);
    case Queen:  return q_att(occupied(), sq);
    default:     assert(false);
  }
  return 0ull;
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
  MoveList pseudo;

  generate_all(pseudo);

  while (!pseudo.empty())
  {
    Move move = pseudo.get_next();

    if (make(move))
    {
      unmake(move);
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

  int gain[32];
  int d = 0;
  Piece p = square[from];

  u64 o       = occ[0] | occ[1];
  u64 xrayers = o ^ (knights() | kings());
  u64 from_bb = bit(from);
  u64 attadef = get_all_attackers(o, to) | from_bb;
  gain[d]     = see_value[square[to]];

  do
  {
    //writeln(attadef.to_bitboard);
    //writeln(o.to_bitboard);
    //writefln("gain[%d] = %d", d, gain[d]);

    d++;
    gain[d]  = see_value[p] - gain[d - 1]; // speculative score, if defended
    attadef ^= from_bb; // reset bit in set to traverse
    o       ^= from_bb; // reset bit in temporary occupancy (for x-Rays)

    if (from_bb & xrayers) attadef |= consider_xrays(o, to);
    from_bb = least_valuable_piece(attadef, ~col(p), p);
  }
  while (from_bb);

  while (--d) gain[d - 1] = -(std::max)(-gain[d - 1], gain[d]);
  return gain[0];
}

Move Board::recognize(Move candidate)
{
  Move result = Move::None;
  MoveList ml;

  generate_all(ml);

  while (!ml.empty())
  {
    Move move = ml.get_next();
    if (!make(move)) continue;
    unmake(move);

    if (similar(move, candidate))
    {
      if (!is_empty(result)) // Ambiguity
        return Move::None;
      result = move;
    }
  }
  return result;
}

Move Board::parse_san(string str)
{
  string old = str;
  char ch;
  Piece p;
  PieceType pt, promote = PieceType_N;
  u64 fr_mask = Full; // allowed
  u64 to_mask = Full; // squares

  auto is_file = [](char ch){ return ch >= 'a' && ch <= 'h'; };
  auto is_rank = [](char ch){ return ch >= '1' && ch <= '8'; };

  if (str.length() < 2) return Move::None;
  if (str == "--") return Move::Null;

  // 1. Qualifiers (discard all)

  dry(str, "x+#!?");

  size_t pos = str.rfind("e.p.");
  if (pos != string::npos)
    str.erase(pos, 4);

  // 2. Special cases

  if (str == "O-O-O")
  {
    p = to_piece(King, color);
    fr_mask = bit(color ? E1 : E8);
    to_mask = bit(color ? C1 : C8);
  }
  else if (str == "O-O")
  {
    p = to_piece(King, color);
    fr_mask = bit(color ? E1 : E8);
    to_mask = bit(color ? G1 : G8);
  }
  else
  {
    // 3. Parsing piece type (if present)

    if (cut_start(str, ch)) pt = to_pt(tolower(ch));
    pt = pt == PieceType_N ? Pawn : pt;
    p = to_piece(pt, color);

    // 4. Parsing promotion piece

    if (pt == Pawn)
    {
      while ((pos = str.rfind("=", pos)) != string::npos)
      {
        if (pos < str.length() - 1)
        {
          promote = to_pt(str[pos + 1]);
        }
      }
    }

    // a1a1 -> ss -> s|s
    // 1a1  -> 1s -> 1|s
    // a1a  -> s1 -> s|1
    // aa   -> aa -> a|a
    // a1   -> s  -> _|s

    // 5. Parsing from-to squares

    struct Token { char ch; SQ sq; u64 mask; };
    vector<Token> tokens;

    for (int i = 0; i < str.length() - 1; ++i)
    {
      if (is_file(str[i])      // fold complete squares
      &&  is_rank(str[i + 1])) // since they are integral
      {
        SQ sq = to_sq(str.substr(i, 2));
        tokens.push_back({ 's', sq });
        i++;
      }
      else tokens.push_back({ str[i], SQ_N });
    }

    if (tokens.size() == 1)
      tokens.insert(tokens.begin(), 1, { '_', SQ_N });

    if (tokens.size() != 2) return Move::None;

    // 6. Build square masks

    for (auto & token : tokens)
    {
      if      (token.sq < SQ_N)   token.mask = bit(token.sq);
      else if (is_file(token.ch)) token.mask = file_bb[token.ch - 'a'];
      else if (is_rank(token.ch)) token.mask = rank_bb[token.ch - '1'];
      else                        token.mask = Full;
    }

    fr_mask = tokens[0].mask;
    to_mask = tokens[1].mask;
  }

  // Searching in legal moves

  MoveList ml;

  generate_all(ml);

  Moves moves;

  while (!ml.empty())
  {
    Move move = ml.get_next();
    if (!make(move)) continue;
    unmake(move);

    const SQ from = get_from(move);
    const SQ to = get_to(move);

    if (bit(from) & fr_mask   // Square from allowed
    &&  bit(to) & to_mask    // Square to allowed
    &&  square[from] == p) // Moving piece equal
    {
      if (pt != Pawn || promoted(move) == promote)
        moves.push_back(move);
    }
  }

  if (moves.size() == 0) return Move::None;
  if (moves.size() == 1) return moves[0];

#ifdef _DEBUG
  string msg;
  for (auto move : moves)
    msg += format("{}, ", move);

  print();
  say("Ambigious {}, candidates are {}\n", old, msg);
  assert(false);
#endif

  return Move::None;
}

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

  const u64 mask = get_mask(cap);

  if (mt == Quiet) // quiet move
  {
    const u64 att = atts[p][from] & (is_king(p) ? ~o : mask); // WARN: is it right? too strict
    if (!(att & bit(to))) return false; // piece can move that way
  }

  if (cap) // simple capture move
  {
    const u64 att = atts[p][from] & (is_king(p) ? occ[~color] : mask); // WARN: is it right? too strict
    if (!(att & bit(to))) return false; // piece can move that way
  }

  if (is_pawn(p))
  {
    if (several(state.checkers)) return false;
    if (mt == Ep) return state.ep < SQ_N && to == state.ep;

    const u64 PromRank = color ? Rank7 : Rank2;

    if (mt == Cap || is_capprom(mt))
    {
      const u64 att = atts[p][from] & mask;
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
      if (state.checkers) return false;

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
      if (state.checkers) return false;

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
    else return is_castle(mt) || mt == Quiet || mt == Cap;
  }
  
  return !is_castle(mt) && !is_prom(mt) && !is_pawn(mt) && !is_ep(mt)
      && !several(state.checkers);
}

int Board::best_cap_value() const
{
  int val = see_value[WP];

  // Most valuable opponents piece
  for (Piece p = WQ ^ color; p > WP; p = p + 2)
  {
    if (piece[p])
    {
      val = see_value[p];
      break;
    }
  }

  // Potential promotion
  if (piece[BP ^ color] & (color ? Rank7 : Rank2))
  {
    val += see_value[WQ] - see_value[WP];
  }

  return val;
}

bool Board::make(Move move)
{
  assert(!is_empty(move));

  const SQ from = get_from(move);
  const SQ to   = get_to(move);
  const MT mt   = get_mt(move);
  const Piece p = square[from];

  *(state_ptr++) = state;

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
      remove(from);
      place(to, p);
      break;
    }
  }

  color = ~color;
  state.bhash ^= Zobrist::turn;
  state.pkhash ^= Zobrist::turn;
  threefold[moves_cnt++] = hash();

#ifdef _DEBUG
  if (hash() != calc_hash())
  {
    print();
    say(" actual hash: 0x{:016X}\n", hash());
    say("correct hash: 0x{:016X}\n", calc_hash());
    assert(false);
  }
#endif

  if (in_check(1))
  {
    unmake(move);
    return false;
  }

  state.checkers = king_attackers();
  state.threats = opp_attacks();

  return true;
}

void Board::unmake(Move move)
{
  assert(!is_empty(move));

  const SQ from = get_from(move);
  const SQ to   = get_to(move);
  const MT mt   = get_mt(move);
  const Piece p = square[to];

  moves_cnt--;
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

  state = *(--state_ptr);
}

void Board::make_null()
{
  *(state_ptr++) = state;

  color = ~color;
  state.ep = SQ_N;
  state.bhash ^= Zobrist::turn;
  state.pkhash ^= Zobrist::turn;
  threefold[moves_cnt++] = hash();
}

void Board::unmake_null()
{
  color = ~color;
    
  moves_cnt--;
  state = *(--state_ptr);
}

}




