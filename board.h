#pragma once
#include <string>
#include <cassert>
#include "consts.h"
//#include "value.h"
#include "movelist.h"
#include "tables.h"
#include "magics.h"
#include "zobrist.h"

namespace eia {

const int see_value[] =
{
  100, 100, 325, 325, 325, 325, 500, 500, 
  1000, 1000, 20000, 20000, 0, 0, 0, 0
};

struct State
{
  SQ ep = SQ_N;
  Piece cap = NOP;
  Castling castling = Castling::NO;
  int fifty = 0;
  u64 bhash = Empty;

  u64 checkers = Empty;
  u64 threats = Empty;
};

struct Undo;

struct Board
{
  Color color;
  u64 piece[Piece_N];
  u64 occ[Color_N];
  Piece square[SQ_N];

  State state;
  u64 threefold[8192];
  int moves_cnt;

  State states[128];
  State * state_ptr;

public:
  Board() { clear(); }  
  Board(const Board & board);

  void clear();
  void revert_states();
  int phase() const;
  bool is_draw() const;
  bool is_repetition() const;
  bool is_simply_mated() const;

  inline int ply() const
  {
    return static_cast<int>(state_ptr - states);
  }

  inline u64 hash() const
  {
    return state.bhash
      ^ Zobrist::castle[(u8)state.castling]
      ^ Zobrist::ep[state.ep];
  }

  u64 calc_hash() const;

  INLINE bool has_pieces(Color col) const
  {
    u64 pieces = occ[col] ^ piece[BK ^ col] ^ piece[BP ^ col];
    return !!pieces;
  }

  INLINE bool is_pawn_eg(Color col) const
  {
    return !has_pieces(~col) && !!piece[WP ^ col];
  }

  template<CastlingType CT>
  INLINE bool can_castle() const;

  bool is_correct(std::string & details) const;
  bool operator == (const Board & board) const;

  INLINE Piece operator [] (const SQ sq) const { return square[sq]; }
  INLINE Color to_move() const { return color; }
  INLINE int fifty() const { return state.fifty; }
  INLINE State get_state() const { return state; }

  bool set(std::string fen = Pos::Init);
  std::string to_fen();

  void print() const;
  void print(Move move) const;

  template<PieceType PT>
  INLINE u64  attack(SQ sq) const;

  template<PieceType PT>
  INLINE u64  attack(SQ sq, u64 o) const;

  INLINE u64  attack(Piece p, SQ sq) const;
  INLINE bool is_attacked(SQ sq, u64 o, int opp = 0) const;

  template<Color COL, bool King = true>
  INLINE u64  get_attackers(u64 o, SQ sq) const;

  INLINE u64  get_all_attackers(u64 o, SQ sq) const;
  INLINE bool in_check(int opp = 0) const;
  INLINE bool castling_attacked(SQ from, SQ to) const;
  INLINE u64  king_attackers(int opp = 0) const;
  INLINE u64  opp_attacks() const;

  template<Color COL>
  INLINE u64  king_attrs() const;

  template<Color COL>
  INLINE u64  opp_atts() const;

  int see(Move move) const;
  Move recognize(Move move);
  Move parse_san(std::string str);
  bool pseudolegal(Move move) const;
  int best_cap_value() const;

  INLINE u64 pawns()   const { return piece[BP] | piece[WP]; }
  INLINE u64 knights() const { return piece[BN] | piece[WN]; }
  INLINE u64 bishops() const { return piece[BB] | piece[WB]; }
  INLINE u64 rooks()   const { return piece[BR] | piece[WR]; }
  INLINE u64 queens()  const { return piece[BQ] | piece[WQ]; }
  INLINE u64 kings()   const { return piece[BK] | piece[WK]; }

  template<Color COL = Color_N> INLINE u64 lights() const;
  template<Color COL = Color_N> INLINE u64 diags() const;
  template<Color COL = Color_N> INLINE u64 ortho() const;

  INLINE u64 occupied() const { return occ[0] | occ[1]; }

  template<bool full = true> inline void place(SQ sq, Piece p);
  template<bool full = true> inline void remove(SQ sq);

  bool make(Move move);
  void unmake(Move move);

  void make_null();
  void unmake_null();

  INLINE void generate_all(MoveList & ml) const;
  void generate_legal(MoveList & ml);

  template<Color COL>
  void generate_quiets(MoveList & ml) const;

  template<Color COL, bool QS = false>
  void generate_attacks(MoveList & ml) const;

  template<Color COL>
  void generate_checks(MoveList & ml) const;

private:
  template<Color COL, bool ATT, PieceType PT>
  INLINE void gen_lookup(MoveList & ml, u64 mask) const;

  template<Color COL, bool ATT, bool DIAG>
  INLINE void gen_slider(MoveList & ml, u64 mask) const;

  template<Color COL>
  INLINE void gen_kill_checker(MoveList & ml) const;

  template<Color COL>
  INLINE void gen_pawn_pushes(MoveList & ml, u64 mask) const;

  template<Color COL>
  INLINE void gen_pawn_double(MoveList & ml, u64 mask) const;

  INLINE u64 check_ray() const
  {
    SQ attacker = bitscan(state.checkers);
    SQ king = bitscan(piece[BK ^ color]);
    return between[attacker][king];
  }

  INLINE u64 get_mask(bool cap) const
  {
    if (only_one(state.checkers))
    {
      return cap ? state.checkers : check_ray();
    }
    return cap ? occ[~color] : ~occupied();
  }
};


template<Color COL>
INLINE u64 Board::lights() const
{
  if constexpr (COL == White) return piece[WN] | piece[WB];
  if constexpr (COL == Black) return piece[BN] | piece[BB];
  return knights() | bishops();
}

template<Color COL>
INLINE u64 Board::diags() const
{
  if constexpr (COL == White) return piece[WQ] | piece[WB];
  if constexpr (COL == Black) return piece[BQ] | piece[BB];
  return bishops() | queens();
}

template<Color COL>
INLINE u64 Board::ortho() const
{
  if constexpr (COL == White) return piece[WQ] | piece[WR];
  if constexpr (COL == Black) return piece[BQ] | piece[BR];
  return rooks() | queens();
}


template<CastlingType CT>
INLINE bool Board::can_castle() const
{
  const u64 span[] = { Span_BK, Span_WK, Span_BQ, Span_WQ };
  const u64 path[] = { Path_BK, Path_WK, Path_BQ, Path_WQ };
  constexpr Castling castle = static_cast<Castling>(1 << CT);

  return (!!(state.castling & castle)    // has rights
      &&   !(occupied() & span[CT])      // no obstruction
      &&   !(state.threats & path[CT])); // not attacked path
}

template<PieceType PT>
INLINE u64 Board::attack(SQ sq) const
{
  if constexpr (PT == Bishop) return b_att(occupied(), sq);
  if constexpr (PT == Rook)   return r_att(occupied(), sq);
  if constexpr (PT == Queen)  return q_att(occupied(), sq);

  return atts[to_piece(PT, Black)][sq];
}

template<PieceType PT>
INLINE u64 Board::attack(SQ sq, u64 o) const
{
  if constexpr (PT == Bishop) return b_att(o, sq);
  if constexpr (PT == Rook)   return r_att(o, sq);
  if constexpr (PT == Queen)  return q_att(o, sq);

  return atts[to_piece(PT, Black)][sq];
}

template<Color COL, bool King>
INLINE u64 Board::get_attackers(u64 o, SQ sq) const
{
  u64 att = Empty;
  att |= b_att(o, sq) & (piece[BB ^ COL] | piece[BQ ^ COL]);
  att |= r_att(o, sq) & (piece[BR ^ COL] | piece[BQ ^ COL]);
  att |= atts[BN][sq] &  piece[BN ^ COL];
  att |= atts[WP ^ COL][sq] & piece[BP ^ COL];

  if constexpr (King) att |= atts[BK][sq] & piece[BK ^ COL];
  return att;
}

template<Color COL>
INLINE u64 Board::king_attrs() const
{
  u64 att = Empty;
  const u64 o = occupied();
  const SQ king = bitscan(piece[BK ^ COL]);

  const u64 bq = piece[WB ^ COL] | piece[WQ ^ COL];
  const u64 rq = piece[WR ^ COL] | piece[WQ ^ COL];

  att |= b_att(o, king) & bq;
  att |= r_att(o, king) & rq;
  att |= atts[BN][king] & piece[WN ^ COL];
  att |= atts[BP ^ COL][king] & piece[WP ^ COL];
  return att;
}

template<Color COL>
INLINE u64 Board::opp_atts() const
{
  u64 att;
  const u64 o = occupied();
  const u64 pawns = piece[WP ^ COL];

  if constexpr (COL) att = shift_dl(pawns) | shift_dr(pawns);
  else               att = shift_ul(pawns) | shift_ur(pawns);

  for (u64 bb = piece[WN ^ COL]; bb; bb = rlsb(bb))
  {
    SQ sq = bitscan(bb);
    att |= atts[WN ^ COL][sq];
  }

  for (u64 bb = piece[WK ^ COL]; bb; bb = rlsb(bb))
  {
    SQ sq = bitscan(bb);
    att |= atts[WK ^ COL][sq];
  }

  const u64 bq = piece[WB ^ COL] | piece[WQ ^ COL];
  const u64 rq = piece[WR ^ COL] | piece[WQ ^ COL];

  for (u64 bb = bq; bb; bb = rlsb(bb))
  {
    SQ sq = bitscan(bb);
    att |= b_att(o, sq);
  }

  for (u64 bb = rq; bb; bb = rlsb(bb))
  {
    SQ sq = bitscan(bb);
    att |= r_att(o, sq);
  }

  return att;
}

template<bool full>
void Board::place(SQ sq, Piece p)
{
  piece[p]    ^= bit(sq);
  occ[col(p)] ^= bit(sq);
  square[sq]   = p;

  if constexpr (full)
  {
    // state.pst += E->pst[p][sq];
    // state.mkey += matkey[p];

    state.bhash ^= Zobrist::key[p][sq];
  }
}

template<bool full>
void Board::remove(SQ sq)
{
  Piece p = square[sq];
  assert(p < Piece_N);

  piece[p]    ^= bit(sq);
  occ[col(p)] ^= bit(sq);
  square[sq]   = NOP;

  if constexpr (full)
  {
    // state.pst -= E->pst[p][sq];
    // state.mkey -= matkey[p];

    state.bhash ^= Zobrist::key[p][sq];
  }
}


template<Color COL, bool ATT, PieceType PT>
INLINE void Board::gen_lookup(MoveList & ml, u64 mask) const
{
  constexpr MT type = ATT ? Cap : Quiet;
  constexpr Piece p = to_piece(PT, COL);

  for (u64 bb = piece[p]; bb; bb = rlsb(bb))
  {
    const SQ s = bitscan(bb);
    for (u64 att = attack<PT>(s) & mask; att; att = rlsb(att))
    {
      ml.add_move(s, bitscan(att), type);
    }
  }
}

template<Color COL, bool ATT, bool DIAG>
INLINE void Board::gen_slider(MoveList & ml, u64 mask) const
{
  constexpr MT type = ATT ? Cap : Quiet;
  constexpr PieceType PT = DIAG ? Bishop : Rook;
  u64 bb = DIAG ? piece[BB ^ COL] | piece[BQ ^ COL]
                : piece[BR ^ COL] | piece[BQ ^ COL];

  for (; bb; bb = rlsb(bb))
  {
    const SQ s = bitscan(bb);
    for (u64 att = attack<PT>(s) & mask; att; att = rlsb(att))
    {
      ml.add_move(s, bitscan(att), type);
    }
  }
}

template<Color COL>
INLINE void Board::gen_kill_checker(MoveList & ml) const
{
  // !! This code is slightly faster, but tested lesser

  // Excluding king-checker capture since it already
  //  included on king attacks generation

  assert(only_one(state.checkers));

  // Capturing attacking piece

  const Piece p = BP ^ COL; // own pawn
  const u64 o = occupied();
  const SQ checker = bitscan(state.checkers);
  const u64 attackers = get_attackers<COL, false>(o, checker);
  const u64 passers = piece[p] & (COL ? Rank7 : Rank2);

  for (u64 bb = attackers & passers; bb; bb = rlsb(bb))
  {
    ml.add_capprom<true>(bitscan(bb), checker);
  }

  for (u64 bb = attackers & ~passers; bb; bb = rlsb(bb))
  {
    ml.add_move(bitscan(bb), checker, Cap);
  }

  // En passant (attacker is pawn)

  if (!!(state.checkers & piece[~p]) && state.ep < SQ_N)
  {
    for (u64 bb = piece[p] & atts[~p][state.ep]; bb; bb = rlsb(bb))
    {
      ml.add_move(bitscan(bb), state.ep, Ep);
    }
  }
}

template<Color COL>
INLINE void Board::gen_pawn_pushes(MoveList & ml, u64 mask) const
{
  const u64 o = occupied();
  const u64 pawns = piece[to_piece(Pawn, COL)];

  if constexpr (COL) // Forward push
  {
    for (u64 bb = pawns & shift_d(mask) & ~Rank7; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s + 8, PawnMove);
    }
  }
  else
  {
    for (u64 bb = pawns & shift_u(mask) & ~Rank2; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s - 8, PawnMove);
    }
  }
}

template<Color COL>
INLINE void Board::gen_pawn_double(MoveList & ml, u64 mask) const
{
  const u64 o = occupied();
  const u64 pawns = piece[to_piece(Pawn, COL)];

  if constexpr (COL) // Double move
  {
    for (u64 bb = pawns & (~o >> 8) & (mask >> 16) & Rank2; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s + 16, PawnMove);
    }
  }
  else
  {
    for (u64 bb = pawns & (~o << 8) & (mask << 16) & Rank7; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s - 16, PawnMove);
    }
  }
}

template<Color COL>
void Board::generate_quiets(MoveList & ml) const
{
  const u64 o = occupied();

  gen_lookup<COL, false, King>(ml, ~o); // special case (!)

  if (several(state.checkers)) return; // double check -> only evade

  const u64 mask = !state.checkers ? ~o : check_ray();
  gen_lookup<COL, false, Knight>(ml, mask);
  gen_slider<COL, false, true>(ml, mask);
  gen_slider<COL, false, false>(ml, mask);

  gen_pawn_pushes<COL>(ml, mask);
  gen_pawn_double<COL>(ml, mask);

  if (state.checkers) return;

  if constexpr (COL) // Castlings
  {
    if (can_castle<CT_WK>()) ml.add_move(E1, G1, KCastle);
    if (can_castle<CT_WQ>()) ml.add_move(E1, C1, QCastle);
  }
  else
  {
    if (can_castle<CT_BK>()) ml.add_move(E8, G8, KCastle);
    if (can_castle<CT_BQ>()) ml.add_move(E8, C8, QCastle);
  }
}

template<Color COL, bool QS>
void Board::generate_attacks(MoveList & ml) const
{
  const u64 me = occ[COL];
  const u64 opp = occ[~COL];
  const u64 o = me | opp;

  gen_lookup<COL, true, King>(ml, opp); // special case (!)

  /*if (only_one(state.checkers)) // have no speedup
  {
    gen_kill_checker<COL>(ml);
    return;
  }*/

  if (several(state.checkers)) return;

  const u64 mask = state.checkers ? state.checkers : opp;

  gen_lookup<COL, true, Knight>(ml, mask);
  gen_slider<COL, true, true>(ml, mask);
  gen_slider<COL, true, false>(ml, mask);

  Piece p = to_piece(Pawn, color);

  if constexpr (COL)
  {
    // Promotion
    for (u64 bb = piece[p] & shift_d(~o) & Rank7; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_prom<QS>(s, s + 8);
    }

    // Promotion with capture
    for (u64 bb = piece[p] & shift_dl(mask) & Rank7; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_capprom<QS>(s, s + 9);
    }

    // Promotion with capture
    for (u64 bb = piece[p] & shift_dr(mask) & Rank7; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_capprom<QS>(s, s + 7);
    }

    // Right pawn capture
    for (u64 bb = piece[p] & shift_dl(mask) & ~Rank7; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s + 9, Cap);
    }

    // Left pawn capture
    for (u64 bb = piece[p] & shift_dr(mask) & ~Rank7; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s + 7, Cap);
    }

    if (state.ep < SQ_N) // En passant
    {
      for (u64 bb = piece[p] & atts[p ^ 1][state.ep]; bb; bb = rlsb(bb))
      {
        ml.add_move(bitscan(bb), state.ep, Ep);
      }
    }
  }
  else
  {
    // Promotion
    for (u64 bb = piece[p] & shift_u(~o) & Rank2; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_prom<QS>(s, s - 8);
    }

    // Promotion with capture
    for (u64 bb = piece[p] & shift_ur(mask) & Rank2; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_capprom<QS>(s, s - 9);
    }

    // Promotion with capture
    for (u64 bb = piece[p] & shift_ul(mask) & Rank2; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_capprom<QS>(s, s - 7);
    }

    // Right pawn capture
    for (u64 bb = piece[p] & shift_ur(mask) & ~Rank2; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s - 9, Cap);
    }

    // Left pawn capture
    for (u64 bb = piece[p] & shift_ul(mask) & ~Rank2; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s - 7, Cap);
    }

    if (state.ep < SQ_N) // En passant
    {
      for (u64 bb = piece[p] & atts[p ^ 1][state.ep]; bb; bb = rlsb(bb))
      {
        ml.add_move(bitscan(bb), state.ep, Ep);
      }
    }
  }
}

template<Color COL>
void Board::generate_checks(MoveList & ml) const
{
  // Notice that we're looking for quiet checks only
  //  since this generator primarily used in qs
  //  with addition to attacking moves

  const u64 o = occupied();
  const SQ king = bitscan(piece[WK ^ COL]);

  // 1. Pieces that give check

  gen_lookup<COL, false, Knight>(ml, atts[BN][king] & ~o);
  gen_slider<COL, false, false>(ml, r_att(o, king) & ~o);
  gen_slider<COL, false, true>(ml, b_att(o, king) & ~o);

  const u64 mask = COL ? (atts[WP][king] << 8) & ~o
                       : (atts[BP][king] >> 8) & ~o;

  gen_pawn_pushes<COL>(ml, mask);
  gen_pawn_double<COL>(ml, mask);

  if constexpr (COL) // castling may give check too
  {
    if (!between[king][F1] && can_castle<CT_WK>()) ml.add_move(E1, G1, KCastle);
    if (!between[king][D1] && can_castle<CT_WQ>()) ml.add_move(E1, C1, QCastle);
  }
  else
  {
    if (!between[king][F8] && can_castle<CT_BK>()) ml.add_move(E8, G8, KCastle);
    if (!between[king][D8] && can_castle<CT_BQ>()) ml.add_move(E8, C8, QCastle);
  }

  // 2. Discovered checks by piece

  u64 blockers  = r_att(o, king) & occ[COL];
  u64 attackers = r_att(o ^ blockers, king) & ortho<!COL>();

  blockers   = b_att(o, king) & occ[COL];
  attackers |= b_att(o ^ blockers, king) & diags<!COL>();

  for (u64 bb = attackers; bb; bb = rlsb(bb))
  {
    const u64 ray = between[king][bitscan(bb)];
    const SQ   sq = bitscan(ray & o);
    const Piece p = square[sq];

    const u64 mask = ~o & ~(ray | bit(king)); // must discover

    switch (pt(p))
    {
      case King:   gen_lookup<COL, false, King>(ml, mask); break;
      case Knight: gen_lookup<COL, false, Knight>(ml, mask); break;
      case Rook:   gen_slider<COL, false, false>(ml, mask); break;
      case Bishop: gen_slider<COL, false, true>(ml, mask); break;
      case Queen: /* behold the tragedy of powerful piece */ break;

      case Pawn: // don't forget, only quiets here
      {
        gen_pawn_pushes<COL>(ml, mask);
        gen_pawn_double<COL>(ml, mask);
        break;
      }

      default: assert(false);
    }
  }
}

}
