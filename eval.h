#pragma once
#include <string>
#include <vector>
#include <utility>
#include "material.h"
#include "piece.h"
#include "duo.h"

//#define DEBUG_EVAL

namespace eia {

const std::string g_tune = ""; // Tunes::CMA_ES_Eth100;

enum Group
{
  Material, Pawns, Mobility,
  Adjust, Pieces, Safety,
  Passers, Complex, Xrays,
  Threats, Various,
};

// Terms optimization
//
//   Group     |  Linearity  |  Optimized
// ------------+-------------+--------------
//  Material   |    yes      |   CMA-ES
//  Pawns      |    yes      |     --
//  Mobility   |    yes      |     --
//  Adjust     |    yes*     |   CMA-ES
//  Pieces     |    yes      |   CMA-ES
//  Safety     |    --       |   CMA-ES
//  Passers    |    yes      |     --
//  Xrays      |    yes      |     --
//  Threats    |    yes      |   CMA-ES
//  Various    |    yes      |   CMA-ES
//
//   * - need to apply multiplier


struct TermInfo { int group, index, size; };

#define TERM(group,x,op,eg)                          x,
#define TERM_ARR(group,x,sz)                         x,
#define TERMS                                        \
  TERM(Material, MatPawn,       82.0000,  134.2492)  \
  TERM(Material, MatKnight,    311.1222,  602.1030)  \
  TERM(Material, MatBishop,    273.4692,  571.2714)  \
  TERM(Material, MatRook,      412.8690,  936.1210)  \
  TERM(Material, MatQueen,     817.5440, 1848.0416)  \
\
  TERM_ARR(Pawns,    Doubled,       8)   \
  TERM_ARR(Pawns,    Isolated,      8)   \
  TERM_ARR(Pawns,    Backward,      4)   \
  TERM_ARR(Pawns,    PawnShield,    16)  \
  TERM_ARR(Pawns,    WeaknessPush,  8)   \
\
  TERM(Pawns,    Connected,      6.3155,    6.3155)  \
\
  TERM_ARR(Mobility, MobN, 9)   \
  TERM_ARR(Mobility, MobB, 14)  \
  TERM_ARR(Mobility, MobR, 15)  \
  TERM_ARR(Mobility, MobQ, 28)  \
\
  TERM(Adjust,  KnightAdj,       0.4903,    0.4903)  \
  TERM(Adjust,  RookAdj,        -7.7480,   -7.7480)  \
\
  TERM(Pieces,  BishopPair,     44.2487,   44.2487)  \
  TERM(Pieces,  BadBishop,     -33.8036,  -33.8036)  \
  TERM(Pieces,  KnightOutpost,  29.8442,   29.8442)  \
  TERM(Pieces,  RookSemi,       10.8155,   10.8155)  \
  TERM(Pieces,  RookOpen,       27.9015,   27.9015)  \
  TERM(Pieces,  Rook7th,        35.3496,   32.6952)  \
  TERM(Pieces,  BadRook,       -29.7549,  -29.7549)  \
  TERM(Pieces,  KnightFork,     37.7980,   37.7980)  \
  TERM(Pieces,  BishopFork,     68.7231,   68.7231)  \
  TERM(Pieces,  TrappedHard,  -150.0000, -150.0000)  \
  TERM(Pieces,  TrappedSoft,   -50.0000,  -50.0000)  \
  TERM(Pieces,  EarlyQueen,     -0.2204,   -0.2204)  \
\
  TERM_ARR(Passers,  Candidate, 5)  \
  TERM_ARR(Passers,  Passer,    6)  \
  TERM_ARR(Passers,  Supported, 6)  \
\
  TERM_ARR(Passers,  PasserKingDef, 7)  \
  TERM_ARR(Passers,  PasserKingAtt, 7)  \
\
  TERM(Passers,  Unstoppable,   400.3234,  400.3234)  \
  TERM(Passers,  FreePasser,     82.9463,   82.9463)  \
\
  TERM_ARR(Xrays,    PinAbsolute, 8)  \
  TERM_ARR(Xrays,    PinPartial,  8)  \
  TERM_ARR(Xrays,    PinRelative, 8)  \
\
  TERM(Threats,  ThreatPawn,    -29.4672,  -29.4672)  \
  TERM(Threats,  ThreatL_P,     -49.1365,  -49.1365)  \
  TERM(Threats,  ThreatL_L,     -29.9130,  -29.9130)  \
  TERM(Threats,  ThreatL_H,     -49.5985,  -49.5985)  \
  TERM(Threats,  ThreatL_K,      -5.7557,   -5.7557)  \
  TERM(Threats,  ThreatR_L,     -24.3737,  -24.3737)  \
  TERM(Threats,  ThreatR_K,     -51.4882,  -51.4882)  \
  TERM(Threats,  ThreatQ_1,     -27.7715,  -27.7715)  \
\
  TERM(Various,  Tempo,          10.7927,    3.7927)   


enum Term
{
  TERMS
  Term_N
};

#undef TERM
#undef TERM_ARR

#define TERM(group,x,op,eg)   1 +
#define TERM_ARR(group,x,sz)  sz +

constexpr int Param_N = TERMS 0;


enum class AttWeight { Light = 2, Rook = 3, Queen = 5 };

struct Board;
struct EvalInfo
{
  SQ  king[Color_N]; // used in king safety calc
  int king_att_weight[Color_N];
  int king_att_count[Color_N];

  u64 weak[Color_N]; // pawn info
  u64 rammed[Color_N];

  u64 occ_not_rq[Color_N]; // used in mobility to
  u64 occ_not_bq[Color_N]; // not penalty batteries

  u64 pawn_atts[Color_N]; // used in threats analysis
  u64 pawn_atts2[Color_N];
  u64 attacked_by[Color_N][PieceType_N];
  u64 attacked_by2[Color_N];
  u64 attacked[Color_N];
  u64 passers;

  void init(const Board * B);

  void add_king_attack(Color col, AttWeight weight, u64 att);
  Val  king_safety(Color col) const;
  Val  king_safety() const;

  void add_attack(Color col, PieceType pt, u64 att);
};


#ifdef DEBUG_EVAL
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

  Duo      data[Param_N];
  TermInfo info[Term_N];
  EvalInfo ei;

  Duo mat[12];
  Duo pst[12][64];
  Duo mob[6][30];
  int passer_scale[8];

  MatInfo mattable[+MatKey::Total];

public:
  Eval(const Tune & tune = {}, bool no_pst = false, bool no_hash = false);
  void init();
  void init_term(int idx, const std::vector<std::pair<f64, f64>> & arr);
  void set_explanations(bool on);

  INLINE Duo get(Term term, int index = 0) const;

  Val eval(const Board * B, Val alpha, Val beta, bool use_phash = true);
  Val mopup(const Board * B, Color weaker);

  Bounds bounds() const;
  std::string to_string() const;
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

  template<Color Col> Duo eval_passers(const Board * B);
  template<Color Col> Duo eval_threats(const Board * B);

#ifdef DEBUG_EVAL
public:
  const EvalDetails & get_details() { return ed; }

private:
  bool explain = false;
  EvalDetails ed;

  Duo apply(const Duo & vals, Piece p, SQ sq, std::string_view factor)
  {
    if (explain && vals) ed.push_back({p, sq, vals, factor});
    return vals;
  }
#endif
};

#ifdef DEBUG_EVAL
#define A(v, p, sq, factor) (apply(v, p, sq, factor))
#else
#define A(v, p, sq, factor) (v)
#endif

#ifdef DEBUG_EVAL
#define APPLY(v, factor) (apply(v, p, sq, factor))
#else
#define APPLY(v, factor) (v)
#endif

INLINE Duo Eval::get(Term term, int offset) const
{
  assert(index >= 0);
  assert(index < info[term].size);
  return data[ info[term].index + offset ];
}

// from CPW-Engine
const int PAdj[9] = {-5, -4, -3, -2, -1, 0, +1, +2, +3};

extern Eval E[1];

}
