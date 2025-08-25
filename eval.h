#pragma once
#include <string>
#include <vector>
#include "piece.h"
#include "duo.h"

namespace eia {

#define TERM(x,def,min,len)             x,
#define TERMS                           \
  TERM(MatPawnOp,       82,  40,  128)  \
  TERM(MatKnightOp,    426, 300,  256)  \
  TERM(MatBishopOp,    441, 300,  256)  \
  TERM(MatRookOp,      627, 500,  512)  \
  TERM(MatQueenOp,    1292, 900, 1024)  \
  TERM(MatPawnEg,      144,  40,  128)  \
  TERM(MatKnightEg,    475, 300,  256)  \
  TERM(MatBishopEg,    510, 300,  256)  \
  TERM(MatRookEg,      803, 500,  512)  \
  TERM(MatQueenEg,    1623, 900, 1024)  \
  TERM(PawnFile,         5,   0,   16)  \
  TERM(KnightCenterOp,   5,   0,   16)  \
  TERM(KnightCenterEg,   5,   0,   16)  \
  TERM(KnightRank,       5,   0,   16)  \
  TERM(KnightBackRank,   0,   0,   16)  \
  TERM(KnightTrapped,  100,   0,  256)  \
  TERM(BishopCenterOp,   2,   0,   16)  \
  TERM(BishopCenterEg,   3,   0,   16)  \
  TERM(BishopBackRank,  10,   0,   16)  \
  TERM(BishopDiagonal,   4,   0,   16)  \
  TERM(RookFileOp,       3,   0,   16)  \
  TERM(QueenCenterOp,    0,   0,   16)  \
  TERM(QueenCenterEg,    4,   0,   16)  \
  TERM(QueenBackRank,    5,   0,   16)  \
  TERM(KingFile,        10,   0,   32)  \
  TERM(KingRank,        10,   0,   32)  \
  TERM(KingCenterEg,    20,   0,   32)  \
  TERM(Doubled,         10,   0,   32)  \
  TERM(Isolated,         9,   0,   32)  \
  TERM(Backward,        12,   0,   32)  \
  TERM(Connected,        4,   0,   32)  \
  TERM(WeaknessPush,    40,   0,   64)  \
  TERM(NMobMult,       100,   0,  256)  \
  TERM(BMobMult,       100,   0,  256)  \
  TERM(RMobMult,       125,   0,  256)  \
  TERM(QMobMult,       250, 100,  256)  \
  TERM(NMobSteep,       14,   0,   32)  \
  TERM(BMobSteep,       14,   0,   32)  \
  TERM(RMobSteep,       10,   0,   32)  \
  TERM(QMobSteep,        8,   0,   32)  \
  TERM(BishopPair,      13,   0,   64)  \
  TERM(BadBishop,       38,   0,   64)  \
  TERM(RammedBishop,     8,   0,   64)  \
  TERM(KnightOutpost,   10,   0,   64)  \
  TERM(RookSemi,        10,   0,   64)  \
  TERM(RookOpen,        20,   0,   64)  \
  TERM(Rook7thOp,       20,   0,   64)  \
  TERM(Rook7thEg,       12,   0,   64)  \
  TERM(BadRook,         20,   0,   64)  \
  TERM(KnightFork,      20,   0,   64)  \
  TERM(BishopFork,      13,   0,   64)  \
  TERM(KnightAdj,        4,   0,   32)  \
  TERM(RookAdj,          3,   0,   32)  \
  TERM(EarlyQueen,       5,   0,   32)  \
  TERM(ContactCheckR,  100, 100,    1)  \
  TERM(ContactCheckQ,  180, 180,    1)  \
  TERM(Shield1,         10,   0,   32)  \
  TERM(Shield2,          5,   0,   32)  \
  TERM(PasserK,         22,   0,   64)  \
  TERM(Candidate,      100,   0,  256)  \
  TERM(Passer,         200,   0,  512)  \
  TERM(Supported,      100,   0,  256)  \
  TERM(Unstoppable,    800, 700,  256)  \
  TERM(FreePasser,      60,   0,  128)  \
  TERM(Xray,            40,   0,   64)  \
  TERM(PinMul,          12,   0,   64)  \
  TERM(ThreatPawn,      11,   0,   64)  \
  TERM(ThreatL_P,       55,   0,  128)  \
  TERM(ThreatL_L,       25,   0,  128)  \
  TERM(ThreatL_H,       30,   0,  128)  \
  TERM(ThreatL_K,       43,   0,  128)  \
  TERM(ThreatR_L,       48,   0,  128)  \
  TERM(ThreatR_K,       33,   0,  128)  \
  TERM(ThreatQ_1,       50,   0,  128)  \
  TERM(Tempo,           20,   0,   64)   


enum Term
{
  TERMS
  Term_N
};


enum class AttWeight { Light = 2, Rook = 3, Queen = 5 };

struct Board;
struct EvalInfo
{
  SQ  king[Color_N]; // used in king safety calc
  int king_att_weight[Color_N];
  int king_att_count[Color_N];

  int eg_weak[Color_N]; // pawn info
  u64 rammed[Color_N];

  u64 occ_not_rq[Color_N]; // used in mobility to
  u64 occ_not_bq[Color_N]; // not penalty batteries

  u64 pawn_atts[Color_N]; // used in threats analysis
  u64 pawn_atts2[Color_N];
  u64 attacked_by[Color_N][PieceType_N];
  u64 attacked_by2[Color_N];
  u64 attacked[Color_N];

  void init(const Board * B);

  void add_king_attack(Color col, AttWeight weight, u64 att);
  int  king_safety(Color col) const;
  int  king_safety() const;

  void add_weak(Color col, SQ sq);
  int  weakness(Color col, int bonus) const;

  void add_attack(Color col, PieceType pt, u64 att);
};


#ifdef _DEBUG
struct EvalDetail
{
  Piece p;
  SQ sq;
  Duo vals;
  std::string_view factor;
};

using EvalDetails = std::vector<EvalDetail>;
#endif


class Eval
{
  EvalInfo ei;
  int total_bits;
  int term[Term_N];
  int term_min[Term_N];
  int term_bits[Term_N];

  Duo mat[12];
  Duo pst[12][64];
  int mob[6][30];
  int passer_scale[8];
  int n_adj[9];
  int r_adj[9];

public:
  Eval();
  void init();
  void set_explanations(bool on);
  int get_total_bits() const { return total_bits; }

  int eval(const Board * B, int alpha, int beta);

  std::string to_string() const;
  void set(std::string str);
  void set(const Eval & eval);
  void set(const Genome & genome);

  std::string to_raw() const;
  void set_raw(std::string str);

private:
  template<Color Col> Duo evalxrays(const Board * B);
  template<Color Col> Duo evaluateP(const Board * B);
  template<Color Col> Duo evaluateN(const Board * B);
  template<Color Col> Duo evaluateB(const Board * B);
  template<Color Col> Duo evaluateR(const Board * B);
  template<Color Col> Duo evaluateQ(const Board * B);
  template<Color Col> Duo evaluateK(const Board * B);

  template<Color Col> Duo eval_passer(const Board * B, SQ sq);
  template<Color Col> Duo eval_threats(const Board * B);

#ifdef _DEBUG
public:
  const EvalDetails & get_details() { return ed; }

private:
  bool explain = false;
  EvalDetails ed;

  Duo apply(const Duo & vals, Piece p, SQ sq, std::string_view factor)
  {
    if (explain) ed.push_back({p, sq, vals, factor});
    return vals;
  }
#endif
};

#ifdef _DEBUG
#define A(v, p, sq, factor) (apply(v, p, sq, factor))
#else
#define A(v, p, sq, factor) (v)
#endif

#ifdef _DEBUG
#define APPLY(v, factor) (apply(v, p, sq, factor))
#else
#define APPLY(v, factor) (v)
#endif


const int PFile[8] = {-3, -1, +0, +1, +1, +0, -1, -3};
const int NLine[8] = {-4, -2, +0, +1, +1, +0, -2, -4};
const int NRank[8] = {-2, -1, +0, +1, +2, +3, +2, +1};
const int BLine[8] = {-3, -1, +0, +1, +1, +0, -1, -3};
const int RFile[8] = {-2, -1, +0, +1, +1, +0, -1, -2};
const int QLine[8] = {-3, -1, +0, +1, +1, +0, -1, -3};
const int KLine[8] = {-3, -1, +0, +1, +1, +0, -1, -3};
const int KFile[8] = {+3, +4, +2, +0, +0, +2, +4, +3};
const int KRank[8] = {+1, +0, -2, -3, -4, -5, -6, -7};

// from CPW-Engine
const int PAdj[9] = {-5, -4, -3, -2, -1, 0, +1, +2, +3};

}
