#pragma once
#include <string>
#include <vector>
#include "piece.h"
#include "duo.h"

namespace eia {

const std::string active_tune = Tunes::SPSA4;

#define TERM(x,def,min,max)                      x,
#define TERMS                                    \
  TERM(MatPawnOp,       82_cp,  40_cp,  160_cp)  \
  TERM(MatKnightOp,    426_cp, 300_cp,  600_cp)  \
  TERM(MatBishopOp,    441_cp, 300_cp,  600_cp)  \
  TERM(MatRookOp,      627_cp, 500_cp, 1000_cp)  \
  TERM(MatQueenOp,    1292_cp, 900_cp, 2000_cp)  \
  TERM(MatPawnEg,      144_cp,  40_cp,  160_cp)  \
  TERM(MatKnightEg,    475_cp, 300_cp,  600_cp)  \
  TERM(MatBishopEg,    510_cp, 300_cp,  600_cp)  \
  TERM(MatRookEg,      803_cp, 500_cp, 1000_cp)  \
  TERM(MatQueenEg,    1623_cp, 900_cp, 2000_cp)  \
  TERM(PawnFile,         5_cp,   0_cp,   16_cp)  \
  TERM(KnightCenterOp,   5_cp,   0_cp,   16_cp)  \
  TERM(KnightCenterEg,   5_cp,   0_cp,   16_cp)  \
  TERM(KnightRank,       5_cp,   0_cp,   16_cp)  \
  TERM(KnightBackRank,   0_cp,   0_cp,   16_cp)  \
  TERM(KnightTrapped,  100_cp,   0_cp,  256_cp)  \
  TERM(BishopCenterOp,   2_cp,   0_cp,   16_cp)  \
  TERM(BishopCenterEg,   3_cp,   0_cp,   16_cp)  \
  TERM(BishopBackRank,  10_cp,   0_cp,   16_cp)  \
  TERM(BishopDiagonal,   4_cp,   0_cp,   16_cp)  \
  TERM(RookFileOp,       3_cp,   0_cp,   16_cp)  \
  TERM(QueenCenterOp,    0_cp,   0_cp,   16_cp)  \
  TERM(QueenCenterEg,    4_cp,   0_cp,   16_cp)  \
  TERM(QueenBackRank,    5_cp,   0_cp,   16_cp)  \
  TERM(KingFile,        10_cp,   0_cp,   32_cp)  \
  TERM(KingRank,        10_cp,   0_cp,   32_cp)  \
  TERM(KingCenterEg,    20_cp,   0_cp,   32_cp)  \
  TERM(Doubled,         10_cp,   0_cp,   32_cp)  \
  TERM(Isolated,         9_cp,   0_cp,   32_cp)  \
  TERM(Backward,        12_cp,   0_cp,   32_cp)  \
  TERM(Connected,        4_cp,   0_cp,   32_cp)  \
  TERM(WeaknessPush,    40_cp,   0_cp,   64_cp)  \
  TERM(NMobMult,       100_cp,   0_cp,  256_cp)  \
  TERM(BMobMult,       100_cp,   0_cp,  256_cp)  \
  TERM(RMobMult,       125_cp,   0_cp,  256_cp)  \
  TERM(QMobMult,       250_cp, 100_cp,  256_cp)  \
  TERM(NMobSteep,       14_cp,   0_cp,   32_cp)  \
  TERM(BMobSteep,       14_cp,   0_cp,   32_cp)  \
  TERM(RMobSteep,       10_cp,   0_cp,   32_cp)  \
  TERM(QMobSteep,        8_cp,   0_cp,   32_cp)  \
  TERM(BishopPair,      13_cp,   0_cp,   64_cp)  \
  TERM(BadBishop,       38_cp,   0_cp,   64_cp)  \
  TERM(RammedBishop,     8_cp,   0_cp,   64_cp)  \
  TERM(KnightOutpost,   10_cp,   0_cp,   64_cp)  \
  TERM(RookSemi,        10_cp,   0_cp,   64_cp)  \
  TERM(RookOpen,        20_cp,   0_cp,   64_cp)  \
  TERM(Rook7thOp,       20_cp,   0_cp,   64_cp)  \
  TERM(Rook7thEg,       12_cp,   0_cp,   64_cp)  \
  TERM(BadRook,         20_cp,   0_cp,   64_cp)  \
  TERM(KnightFork,      20_cp,   0_cp,   64_cp)  \
  TERM(BishopFork,      13_cp,   0_cp,   64_cp)  \
  TERM(KnightAdj,        4_cp,   0_cp,   32_cp)  \
  TERM(RookAdj,          3_cp,   0_cp,   32_cp)  \
  TERM(EarlyQueen,       5_cp,   0_cp,   32_cp)  \
  TERM(ContactCheckR,  100_cp, 100_cp,  200_cp)  \
  TERM(ContactCheckQ,  180_cp, 180_cp,  200_cp)  \
  TERM(Shield1,         10_cp,   0_cp,   32_cp)  \
  TERM(Shield2,          5_cp,   0_cp,   32_cp)  \
  TERM(PasserK,         22_cp,   0_cp,   64_cp)  \
  TERM(Candidate,      100_cp,   0_cp,  256_cp)  \
  TERM(Passer,         200_cp,   0_cp,  512_cp)  \
  TERM(Supported,      100_cp,   0_cp,  256_cp)  \
  TERM(Unstoppable,    800_cp, 700_cp, 1000_cp)  \
  TERM(FreePasser,      60_cp,   0_cp,  128_cp)  \
  TERM(Xray,            40_cp,   0_cp,   64_cp)  \
  TERM(PinMul,          12_cp,   0_cp,   64_cp)  \
  TERM(ThreatPawn,      11_cp,   0_cp,   64_cp)  \
  TERM(ThreatL_P,       55_cp,   0_cp,  128_cp)  \
  TERM(ThreatL_L,       25_cp,   0_cp,  128_cp)  \
  TERM(ThreatL_H,       30_cp,   0_cp,  128_cp)  \
  TERM(ThreatL_K,       43_cp,   0_cp,  128_cp)  \
  TERM(ThreatR_L,       48_cp,   0_cp,  128_cp)  \
  TERM(ThreatR_K,       33_cp,   0_cp,  128_cp)  \
  TERM(ThreatQ_1,       50_cp,   0_cp,  128_cp)  \
  TERM(Tempo,           20_cp,   0_cp,   64_cp)   


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

  Val eg_weak[Color_N]; // pawn info
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
  Val  king_safety(Color col) const;
  Val  king_safety() const;

  void add_weak(Color col, SQ sq);
  Val  weakness(Color col, int bonus) const;

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
  Val term[Term_N];
  Val term_min[Term_N];
  Val term_max[Term_N];

  Duo mat[12];
  Duo pst[12][64];
  Val mob[6][30];
  int passer_scale[8];
  Val n_adj[9];
  Val r_adj[9];

public:
  Eval();
  Eval(const Tune & tune);
  void init();
  void set_explanations(bool on);

  Val eval(const Board * B, Val alpha, Val beta);

  std::string to_string() const;
  std::string prettify() const;
  Bounds bounds() const;
  void set(std::string str);
  void set(const Eval & eval);
  void set(const Tune & tune);

  std::string to_raw() const;
  void set_raw(std::string str, std::string delim = ",");

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

// from Fruit 2.1
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
