#pragma once
#include <string>
#include <cassert>
#include "consts.h"
#include "movelist.h"
#include "tables.h"
#include "magics.h"

namespace eia {

struct Key
{
  u64 hash;
  bool irrev;
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

struct Undo
{
  State state;
  MoveList ml;
  // Vals pst;
  Move curr, best;
};

struct Board
{
  Color color;
  u64 piece[Piece_N];
  u64 occ[Color_N];
  Piece square[SQ_N];

  State state;
  //Key threefold[3000];

public:
  Board() { clear(); }  
  Board(const Board & board);

  void clear();
  INLINE int phase() const;
  INLINE bool is_draw() const;

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
  INLINE u64 attack(SQ sq) const;

  /*INLINE u64 attack(Piece p, SQ sq) const
  {
    switch (p)
    {
      case BB: case WB: return attack<Bishop>(sq);
      case BR: case WR: return attack<Rook>(sq);
      case BQ: case WQ: return attack<Queen>(sq);
      default: return atts[p][sq];
    }
  }*/

  INLINE bool is_attacked(SQ sq, u64 o, int opp = 0) const;
  INLINE u64  get_attacks(u64 o, SQ sq) const;
  INLINE bool in_check(int opp = 0) const;
  INLINE bool castling_attacked(SQ from, SQ to) const;

  Move recognize(Move move);

  template<bool full = true> void place(SQ sq, Piece p);
  template<bool full = true> void remove(SQ sq);

  bool make(Move move, Undo *& undo);
  void unmake(Move move, Undo *& undo);

  INLINE void generate_all(MoveList & ml) const;

  template<Color COL>
  void generate_quiets(MoveList & ml) const;

  template<Color COL, bool QS = false>
  void generate_attacks(MoveList & ml) const;

private:
  template<Color COL, bool ATT, PieceType PT>
  INLINE void gen(MoveList & ml) const;

  INLINE u64 pawns()    const { return piece[BP] | piece[WP]; }
  INLINE u64 knights()  const { return piece[BN] | piece[WN]; }
  INLINE u64 bishops()  const { return piece[BB] | piece[WB]; }
  INLINE u64 rooks()    const { return piece[BR] | piece[WR]; }
  INLINE u64 queens()   const { return piece[BQ] | piece[WQ]; }
  INLINE u64 kings()    const { return piece[BK] | piece[WK]; }

  INLINE u64 lights()   const { return knights() | bishops(); }
  INLINE u64 diags()    const { return bishops() | queens(); }
  INLINE u64 ortho()    const { return rooks()   | queens(); }

  INLINE u64 occupied() const { return occ[0] | occ[1]; }
};


template<PieceType PT>
INLINE u64 Board::attack(SQ sq) const
{
        if constexpr (PT == Bishop) return b_att(occupied(), sq);
  else if constexpr (PT == Rook)   return r_att(occupied(), sq);
  else if constexpr (PT == Queen)  return q_att(occupied(), sq);

  return atts[to_piece(PT, Black)][sq];
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

    // state.bhash ^= hash_key[p][sq]; // TODO
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

    // state.bhash ^= hash_key[p][sq]; // TODO
  }
}


template<Color COL, bool ATT, PieceType PT>
INLINE void Board::gen(MoveList & ml) const
{
  const Piece p = to_piece(PT, COL);
  const u64 mask = ATT ? occ[~COL] : ~occupied();
  const MT type = ATT ? Cap : Quiet;

  for (u64 bb = piece[p]; bb; bb = rlsb(bb))
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
  const u64 o = occ[0] | occ[1];

  gen<COL, false, Knight>(ml);
  gen<COL, false, Bishop>(ml);
  gen<COL, false, Rook>(ml);
  gen<COL, false, Queen>(ml);
  gen<COL, false, King>(ml);

  // Castlings
  if constexpr (COL)
  {
    if (!!(state.castling & Castling::WK) && !(o & Span_WK))
    {
      if (!castling_attacked(E1, G1)) ml.add_move(E1, G1, KCastle);
    }

    if (!!(state.castling & Castling::WQ) && !(o & Span_WQ))
    {
      if (!castling_attacked(E1, C1)) ml.add_move(E1, C1, QCastle);
    }
  }
  else
  {
    if (!!(state.castling & Castling::BK) && !(o & Span_BK))
    {
      if (!castling_attacked(E8, G8)) ml.add_move(E8, G8, KCastle);
    }

    if (!!(state.castling & Castling::BQ) && !(o & Span_BQ))
    {
      if (!castling_attacked(E8, C8)) ml.add_move(E8, C8, QCastle);
    }
  }

  u64 pawns = piece[to_piece(Pawn, COL)];

  if constexpr (COL)
  {
    // Forward push
    for (u64 bb = pawns & shift_d(~o) & ~Rank7; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s + 8);
    }

    // Double move
    for (u64 bb = pawns & (~o >> 8) & (~o >> 16) & Rank2; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s + 16, Pawn2);
    }
  }
  else
  {
    // Forward push
    for (u64 bb = pawns & shift_u(~o) & ~Rank2; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s - 8);
    }

    // Double move
    for (u64 bb = pawns & (~o << 8) & (~o << 16) & Rank7; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s - 16, Pawn2);
    }
  }
}

template<Color COL, bool QS>
void Board::generate_attacks(MoveList & ml) const
{
  const u64 me = occ[color];
  const u64 opp = occ[~color];
  const u64 o = me | opp;

  gen<COL, true, Knight>(ml);
  gen<COL, true, Bishop>(ml);
  gen<COL, true, Rook>(ml);
  gen<COL, true, Queen>(ml);
  gen<COL, true, King>(ml);

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
    for (u64 bb = piece[p] & shift_dl(opp) & Rank7; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_capprom<QS>(s, s + 9);
    }

    // Promotion with capture
    for (u64 bb = piece[p] & shift_dr(opp) & Rank7; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_capprom<QS>(s, s + 7);
    }

    // Right pawn capture
    for (u64 bb = piece[p] & shift_dl(opp) & ~Rank7; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s + 9, Cap);
    }

    // Left pawn capture
    for (u64 bb = piece[p] & shift_dr(opp) & ~Rank7; bb; bb = rlsb(bb))
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
    for (u64 bb = piece[p] & shift_ur(opp) & Rank2; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_capprom<QS>(s, s - 9);
    }

    // Promotion with capture
    for (u64 bb = piece[p] & shift_ul(opp) & Rank2; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_capprom<QS>(s, s - 7);
    }

    // Right pawn capture
    for (u64 bb = piece[p] & shift_ur(opp) & ~Rank2; bb; bb = rlsb(bb))
    {
      SQ s = bitscan(bb);
      ml.add_move(s, s - 9, Cap);
    }

    // Left pawn capture
    for (u64 bb = piece[p] & shift_ul(opp) & ~Rank2; bb; bb = rlsb(bb))
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
