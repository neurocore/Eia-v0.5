#pragma once
#include <cassert>
#include "consts.h"
#include "movelist.h"

struct Key
{
  u64 hash;
  bool irrev;
};

struct State
{
  SQ ep;
  Piece cap;
  Castling castling;
  int fifty;
  u64 bhash;

  u64 king_atts;
  u64 threats;
};

struct Undo
{
  State state;
  MoveList ml;
  // Vals pst;
  Move curr, best;
};

struct BoardInner
{
  Color color;
  u64 piece[Piece_N];
  u64 occ[Color_N];
  Piece square[SQ_N];
  State state;
};

class Board : private BoardInner
{
  Key threefold[3000];

public:
  Board() { clear(); }  

  void clear();
  int phase() const;
  bool is_draw() const;

  INLINE bool has_pieces(Color col) const
  {
    u64 pieces = occ[col] ^ piece[BK ^ col] ^ piece[BP ^ col];
    return !!pieces;
  }

  INLINE bool is_pawn_eg(Color col) const
  {
    return !has_pieces(~col) && !!piece[WP ^ col];
  }

  INLINE Piece operator [] (const SQ sq) const { return square[sq]; }
  INLINE Color to_move() const { return color; }
  INLINE int fifty() const { return state.fifty; }
  INLINE State get_state() const { return state; }

  bool is_attacked(SQ king, u64 o, int opp = 0) const;
  u64  get_attacks(u64 o, SQ sq) const;
  bool in_check(int opp = 0) const;

  template<bool full = true>
  void place(SQ sq, Piece p)
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

  template<bool full = true>
  void remove(SQ sq)
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

  bool set(std::string fen = Pos::Init)
  {
    SQ sq = A8; // TODO

    //string[] parts = fen.split(' ');
    //if (parts.length < 4) return error("less than 4 parts");

    //clear();

    //foreach (char ch; parts[0]) // parsing main part
    //{
    //  if (isDigit(ch)) sq += ch - '0';
    //  else
    //  {
    //    Piece p = ch.to_piece();
    //    if (p == Piece.NOP) continue;

    //    place(sq, p);
    //    sq++;
    //  }

    //  if (!(sq & 7)) // row wrap
    //  {
    //    sq -= 16;
    //    if (sq < 0) break;
    //  }
    //}

    //foreach (ch; parts[1]) // parsing color
    //  color = ch.to_color();

    //state.castling = Castling.init;
    //foreach (ch; parts[2]) // parsing castling
    //{
    //  state.castling |= ch.to_castling();
    //}

    //state.ep = parts[3].to_sq; // en passant

    //string fifty = parts.length > 4 ? parts[4] : "";
    //state.fifty = fifty.safe_to!u8; // fifty move counter

    //// full move counter - no need

    //state.bhash ^= hash_wtm[color];
    //threefold ~= Key(state.hash, true);

    return true;
  }

  std::string to_fen()
  {
    std::string fen; // TODO
    return fen;
  }

  bool make(Move move, Undo *& undo);
  void unmake(Move move, Undo *& undo);

private:
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



