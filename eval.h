#pragma once
#include <string>
#include "piece.h"
#include "duo.h"

namespace eia {

#define TERM(x,def)          x,
#define TERMS                \
  TERM(MatKnight,      320)  \
  TERM(MatBishop,      330)  \
  TERM(MatRook,        500)  \
  TERM(MatQueen,       900)  \
  TERM(PawnFile,         5)  \
  TERM(KnightCenterOp,   5)  \
  TERM(KnightCenterEg,   5)  \
  TERM(KnightRank,       5)  \
  TERM(KnightBackRank,   0)  \
  TERM(KnightTrapped,  100)  \
  TERM(BishopCenterOp,   2)  \
  TERM(BishopCenterEg,   3)  \
  TERM(BishopBackRank,  10)  \
  TERM(BishopDiagonal,   4)  \
  TERM(RookFileOp,       3)  \
  TERM(QueenCenterOp,    0)  \
  TERM(QueenCenterEg,    4)  \
  TERM(QueenBackRank,    5)  \
  TERM(KingFile,        10)  \
  TERM(KingRank,        10)  \
  TERM(KingCenterEg,    20)  \
  TERM(Doubled,         10)  \
  TERM(Isolated,         9)  \
  TERM(Backward,        12)  \
  TERM(WeaknessPush,    40)  \
  TERM(NMob,            64)  \
  TERM(BMob,            64)  \
  TERM(RMob,            48)  \
  TERM(QMob,            16)  \
  TERM(BishopPair,      13)  \
  TERM(BadBishop,       38)  \
  TERM(KnightOutpost,   10)  \
  TERM(RookSemi,        10)  \
  TERM(RookOpen,        20)  \
  TERM(Rook7thOp,       20)  \
  TERM(Rook7thEg,       12)  \
  TERM(BadRook,         20)  \
  TERM(KnightFork,      20)  \
  TERM(BishopFork,      13)  \
  TERM(KnightAdj,        4)  \
  TERM(RookAdj,          3)  \
  TERM(EarlyQueen,       3)  \
  TERM(ContactCheckR,  100)  \
  TERM(ContactCheckQ,  180)  \
  TERM(Shield1,         10)  \
  TERM(Shield2,          5)  \
  TERM(PasserK,         22)  \
  TERM(Candidate,      100)  \
  TERM(Passer,         200)  \
  TERM(Supported,      100)  \
  TERM(Unstoppable,    800)  \
  TERM(FreePasser,      60)  \
  TERM(XrayMul,         12)  \
  TERM(Tempo,           20)   


enum Term
{
  TERMS
  Term_N
};


enum class AttWeight { Light = 2, Rook = 3, Queen = 5 };

struct Board;
struct EvalInfo
{
  SQ king[Color_N];
  int att_weight[Color_N];
  int att_count[Color_N];
  int eg_weak[Color_N];
  u64 pinned;

  void clear(const Board * B);
  void add_attack(Color col, AttWeight weight, u64 att);
  int king_safety(Color col) const;
  int king_safety() const;
  void add_weak(Color col, SQ sq);
  int weakness(Color col, int bonus) const;
};


class Eval
{
  EvalInfo ei;
  int term[Term_N];

  int mat[12];
  Duo pst[12][64];
  int passer_scale[8];
  int n_adj[9];
  int r_adj[9];

public:
  Eval();
  void init();

  std::string to_string() const;
  void set(std::string str);
  void set(Eval * eval);

  int eval(const Board * B, int alpha, int beta);

private:
  int evaluate(const Board * B);
  template<Color col> Duo evaluateP(const Board * B);
  template<Color col> Duo evaluateN(const Board * B);
  template<Color col> Duo evaluateB(const Board * B);
  template<Color col> Duo evaluateR(const Board * B);
  template<Color col> Duo evaluateQ(const Board * B);
  template<Color col> Duo evaluateK(const Board * B);

  template<Color col> Duo eval_passer(const Board * B, SQ sq);
};

const int PFile[8] = {-3, -1, +0, +1, +1, +0, -1, -3};
const int NLine[8] = {-4, -2, +0, +1, +1, +0, -2, -4};
const int NRank[8] = {-2, -1, +0, +1, +2, +3, +2, +1};
const int BLine[8] = {-3, -1, +0, +1, +1, +0, -1, -3};
const int RFile[8] = {-2, -1, +0, +1, +1, +0, -1, -2};
const int QLine[8] = {-3, -1, +0, +1, +1, +0, -1, -3};
const int KLine[8] = {-3, -1, +0, +1, +1, +0, -1, -3};
const int KFile[8] = {+3, +4, +2, +0, +0, +2, +4, +3};
const int KRank[8] = {+1, +0, -2, -3, -4, -5, -6, -7};

}
