#pragma once
#include <string>
#include <vector>
#include "piece.h"
#include "duo.h"

namespace eia {

const std::string g_tune = ""; // Tunes::CMA_ES_Eth1;

#define TERM(x,def,min,max)                           x,
#define TERMS                                         \
  TERM(MatKnightOp,    311.1222_cp, 300_cp,  600_cp)  \
  TERM(MatBishopOp,    273.4692_cp, 300_cp,  600_cp)  \
  TERM(MatRookOp,      412.8690_cp, 500_cp, 1000_cp)  \
  TERM(MatQueenOp,     817.5440_cp, 900_cp, 2000_cp)  \
  TERM(MatPawnEg,      134.2492_cp,  40_cp,  160_cp)  \
  TERM(MatKnightEg,    602.1030_cp, 300_cp,  600_cp)  \
  TERM(MatBishopEg,    571.2714_cp, 300_cp,  600_cp)  \
  TERM(MatRookEg,      936.1210_cp, 500_cp, 1000_cp)  \
  TERM(MatQueenEg,    1848.0416_cp, 900_cp, 2000_cp)  \
  TERM(Doubled,          0.1866_cp,   0_cp,   32_cp)  \
  TERM(Isolated,         13.6457_cp,  0_cp,   32_cp)  \
  TERM(Backward,         1.9255_cp,   0_cp,   32_cp)  \
  TERM(Connected,        6.3155_cp,   0_cp,   32_cp)  \
  TERM(WeaknessPush,    19.3053_cp,   0_cp,   64_cp)  \
  TERM(NMobMult,        52.9208_cp,   0_cp,  256_cp)  \
  TERM(BMobMult,       168.9628_cp,   0_cp,  256_cp)  \
  TERM(RMobMult,        58.4744_cp,   0_cp,  256_cp)  \
  TERM(QMobMult,        96.9716_cp, 100_cp,  256_cp)  \
  TERM(NMobSteep,        3.4208_cp,   0_cp,   32_cp)  \
  TERM(BMobSteep,        7.4938_cp,   0_cp,   32_cp)  \
  TERM(RMobSteep,       15.2661_cp,   0_cp,   32_cp)  \
  TERM(QMobSteep,        9.9879_cp,   0_cp,   32_cp)  \
  TERM(BishopPair,      44.2487_cp,   0_cp,   64_cp)  \
  TERM(BadBishop,       63.8036_cp,   0_cp,   64_cp)  \
  TERM(RammedBishop,    23.5441_cp,   0_cp,   64_cp)  \
  TERM(KnightOutpost,   29.8442_cp,   0_cp,   64_cp)  \
  TERM(RookSemi,        10.8155_cp,   0_cp,   64_cp)  \
  TERM(RookOpen,        27.9015_cp,   0_cp,   64_cp)  \
  TERM(Rook7thOp,       35.3496_cp,   0_cp,   64_cp)  \
  TERM(Rook7thEg,       32.6952_cp,   0_cp,   64_cp)  \
  TERM(BadRook,         29.7549_cp,   0_cp,   64_cp)  \
  TERM(KnightFork,      37.7980_cp,   0_cp,   64_cp)  \
  TERM(BishopFork,      68.7231_cp,   0_cp,   64_cp)  \
  TERM(KnightAdj,        0.4903_cp,   0_cp,   32_cp)  \
  TERM(RookAdj,          7.7480_cp,   0_cp,   32_cp)  \
  TERM(EarlyQueen,       0.2204_cp,   0_cp,   32_cp)  \
  TERM(ContactCheckR,   72.8489_cp, 100_cp,  200_cp)  \
  TERM(ContactCheckQ,  182.2690_cp, 180_cp,  200_cp)  \
  TERM(Shield1,         37.2179_cp,   0_cp,   32_cp)  \
  TERM(Shield2,         27.2802_cp,   0_cp,   32_cp)  \
  TERM(PasserK,         33.3901_cp,   0_cp,   64_cp)  \
  TERM(Candidate,      114.9568_cp,   0_cp,  256_cp)  \
  TERM(Passer,         170.4616_cp,   0_cp,  512_cp)  \
  TERM(Supported,       28.9072_cp,   0_cp,  256_cp)  \
  TERM(Unstoppable,    400.3234_cp, 700_cp, 1000_cp)  \
  TERM(FreePasser,      82.9463_cp,   0_cp,  128_cp)  \
  TERM(Xray,             2.3989_cp,   0_cp,   64_cp)  \
  TERM(PinMul,           9.8395_cp,   0_cp,   64_cp)  \
  TERM(ThreatPawn,      29.4672_cp,   0_cp,   64_cp)  \
  TERM(ThreatL_P,       49.1365_cp,   0_cp,  128_cp)  \
  TERM(ThreatL_L,       29.9130_cp,   0_cp,  128_cp)  \
  TERM(ThreatL_H,       49.5985_cp,   0_cp,  128_cp)  \
  TERM(ThreatL_K,        5.7557_cp,   0_cp,  128_cp)  \
  TERM(ThreatR_L,       24.3737_cp,   0_cp,  128_cp)  \
  TERM(ThreatR_K,       51.4882_cp,   0_cp,  128_cp)  \
  TERM(ThreatQ_1,       27.7715_cp,   0_cp,  128_cp)  \
  TERM(Tempo,            5.7927_cp,   0_cp,   64_cp)   

  //TERM(MatPawnOp,       82_cp,  40_cp,  160_cp) // anchor
  // 
  //TERM(KnightTrapped,   37.1417_cp,   0_cp,  256_cp) // this may be helpful

  //TERM(PawnFile,         0.2905_cp,   0_cp,   16_cp) // got rid of these
  //TERM(KnightCenterOp,   7.9365_cp,   0_cp,   16_cp) // in favor of PST
  //TERM(KnightCenterEg,  18.1720_cp,   0_cp,   16_cp) // tables tuning
  //TERM(KnightRank,       4.3720_cp,   0_cp,   16_cp) 
  //TERM(KnightBackRank,   9.0901_cp,   0_cp,   16_cp) 
  //TERM(BishopCenterOp,  10.1865_cp,   0_cp,   16_cp) 
  //TERM(BishopCenterEg,   0.7212_cp,   0_cp,   16_cp) 
  //TERM(BishopBackRank,  11.2654_cp,   0_cp,   16_cp) 
  //TERM(BishopDiagonal,   7.4783_cp,   0_cp,   16_cp) 
  //TERM(RookFileOp,       0.9131_cp,   0_cp,   16_cp) 
  //TERM(QueenCenterOp,    2.6029_cp,   0_cp,   16_cp) 
  //TERM(QueenCenterEg,    0.4428_cp,   0_cp,   16_cp) 
  //TERM(QueenBackRank,    1.0177_cp,   0_cp,   16_cp) 
  //TERM(KingFile,        14.0800_cp,   0_cp,   32_cp) 
  //TERM(KingRank,        12.8056_cp,   0_cp,   32_cp) 
  //TERM(KingCenterEg,    12.4389_cp,   0_cp,   32_cp) 

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
  bool no_pst;
  bool no_hash;

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
  Eval(const Tune & tune = {}, bool no_pst = false, bool no_hash = false);
  void init();
  void set_explanations(bool on);

  Val eval(const Board * B, Val alpha, Val beta, bool use_phash = true);

  std::string to_string() const;
  std::string prettify() const;
  Bounds bounds() const;
  void set(std::string str);
  void set(const Eval & eval);
  void set(const Tune & tune);
  Tune to_tune() const;

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
