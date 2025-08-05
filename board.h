#pragma once
#include <string>
#include <cassert>
#include "consts.h"
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

  u64 king_atts = Empty;
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

public:
  Board() { clear(); }  
  Board(const Board & board);

  void clear();
  int phase() const;
  bool is_draw() const;
  bool is_repetition() const;

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
  INLINE bool is_attacked(SQ sq, u64 o, int opp = 0) const;
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
  bool pseudolegal(Move move) const;
  int best_cap_value() const;

  template<bool full = true> inline void place(SQ sq, Piece p);
  template<bool full = true> inline void remove(SQ sq);

  bool make(Move move, Undo *& undo);
  void unmake(Move move, Undo *& undo);

  void make_null(Undo *& undo);
  void unmake_null(Undo *& undo);

  INLINE void generate_all(MoveList & ml) const;
  void generate_legal(MoveList & ml);

  template<Color COL>
  void generate_quiets(MoveList & ml) const;

  template<Color COL, bool QS = false>
  void generate_attacks(MoveList & ml) const;

private:
  template<Color COL, bool ATT, PieceType PT>
  INLINE void gen_lookup(MoveList & ml, u64 mask) const;

  template<Color COL, bool ATT, bool DIAG>
  INLINE void gen_slider(MoveList & ml, u64 mask) const;

  INLINE u64 pawns()    const { return piece[BP] | piece[WP]; }
  INLINE u64 knights()  const { return piece[BN] | piece[WN]; }
  INLINE u64 bishops()  const { return piece[BB] | piece[WB]; }
  INLINE u64 rooks()    const { return piece[BR] | piece[WR]; }
  INLINE u64 queens()   const { return piece[BQ] | piece[WQ]; }
  INLINE u64 kings()    const { return piece[BK] | piece[WK]; }

  INLINE u64 lights()   const { return knights() | bishops(); }
  INLINE u64 diags()    const { return bishops() | queens(); }
  INLINE u64 ortho()    const { return rooks()   | queens(); }

  INLINE u64 blights()  const { return piece[BN] | piece[BB]; }
  INLINE u64 wlights()  const { return piece[WN] | piece[WB]; }

  INLINE u64 occupied() const { return occ[0] | occ[1]; }

  INLINE u64 check_ray() const
  {
    SQ attacker = bitscan(state.king_atts);
    SQ king = bitscan(piece[BK ^ color]);
    return between[attacker][king];
  }

  INLINE u64 get_mask(bool cap) const
  {
    if (only_one(state.king_atts))
    {
      return cap ? state.king_atts : check_ray();
    }
    return cap ? occ[~color] : ~occupied();
  }
};


template<PieceType PT>
INLINE u64 Board::attack(SQ sq) const
{
       if constexpr (PT == Bishop) return b_att(occupied(), sq);
  else if constexpr (PT == Rook)   return r_att(occupied(), sq);
  else if constexpr (PT == Queen)  return q_att(occupied(), sq);

  return atts[to_piece(PT, Black)][sq];
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
void Board::generate_quiets(MoveList & ml) const
{
  const auto was = ml.count();
  const u64 o = occ[0] | occ[1];

  if (several(state.king_atts))
  {
    gen_lookup<COL, false, King>(ml, ~o);
    return;
  }

  const u64 mask = !state.king_atts ? ~o : check_ray();

  gen_lookup<COL, false, King>(ml, ~o); // special case (!)
  gen_lookup<COL, false, Knight>(ml, mask);
  gen_slider<COL, false, true>(ml, mask);
  gen_slider<COL, false, false>(ml, mask);

  u64 pawns = piece[to_piece(Pawn, COL)];

  if constexpr (COL)
  {
    // Forward push
    for (u64 bb = pawns & shift_d(mask) & ~Rank7; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s + 8, PawnMove);
    }

    // Double move
    for (u64 bb = pawns & (~o >> 8) & (~o >> 16) & Rank2; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s + 16, PawnMove);
    }
  }
  else
  {
    // Forward push
    for (u64 bb = pawns & shift_u(mask) & ~Rank2; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s - 8, PawnMove);
    }

    // Double move
    for (u64 bb = pawns & (~o << 8) & (~o << 16) & Rank7; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s - 16, PawnMove);
    }
  }

  if (state.king_atts) return;

  // Castlings
  if constexpr (COL)
  {
    if (!!(state.castling & Castling::WK)
    &&   !(o & Span_WK)
    &&   !(state.threats & Path_WK))
    {
      ml.add_move(E1, G1, KCastle);
    }

    if (!!(state.castling & Castling::WQ)
    &&   !(o & Span_WQ)
    &&   !(state.threats & Path_WQ))
    {
      ml.add_move(E1, C1, QCastle);
    }
  }
  else
  {
    if (!!(state.castling & Castling::BK)
    &&   !(o & Span_BK)
    &&   !(state.threats & Path_BK))
    {
      ml.add_move(E8, G8, KCastle);
    }

    if (!!(state.castling & Castling::BQ)
    &&   !(o & Span_BQ)
    &&   !(state.threats & Path_BQ))
    {
      ml.add_move(E8, C8, QCastle);
    }
  }
}

template<Color COL, bool QS>
void Board::generate_attacks(MoveList & ml) const
{
  const u64 me = occ[COL];
  const u64 opp = occ[~COL];
  const u64 o = me | opp;

  if (several(state.king_atts))
  {
    gen_lookup<COL, true, King>(ml, opp);
    return;
  }

  const u64 mask = state.king_atts ? state.king_atts : opp;

  gen_lookup<COL, true, King>(ml, opp); // special case (!)
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

    if (state.ep) // En passant
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

    if (state.ep) // En passant
    {
      for (u64 bb = piece[p] & atts[p ^ 1][state.ep]; bb; bb = rlsb(bb))
      {
        ml.add_move(bitscan(bb), state.ep, Ep);
      }
    }
  }
}

}
