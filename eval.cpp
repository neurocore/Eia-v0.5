#include <format>
#include <iomanip>
#include "eval.h"
#include "board.h"

using namespace std;

namespace eia {

// from Toga
const int safety_table[100] =
{
    0,   0,   1,   2,   3,   5,   7,   9,  12,  15,
   18,  22,  26,  30,  35,  39,  44,  50,  56,  62,
   68,  75,  82,  85,  89,  97, 105, 113, 122, 131,
  140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
  260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
  377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
  494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
  500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
  500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
  500, 500, 500, 500, 500, 500, 500, 500, 500, 500
};

const int weakness_push_table[] =
{
  128, 106, 86, 68, 52, 38, 26, 16, 8, 2
};

template<bool Expl>
int EvalSimple<Expl>::eval(const Board * B, int alpha, int beta)
{
  int val = 0;
  Duo pos;

  for (int i = 0; i < BK; i++)
  {
    u64 bb = B->piece[i];
    val += mat[i] * popcnt(bb); // material

    for (; bb; bb = rlsb(bb)) // pst
    {
      SQ sq = bitscan(bb);
      int sign = i & 1 ? 1 : -1;
      pos += pst[i][sq] * sign;
    }
  }

  val += pos.tapered(B->phase());

  int score = (B->color ? val : -val) + term[Tempo];
  return score * (100 - B->state.fifty) / 100;
}

template<bool Expl>
int EvalAdvanced<Expl>::eval(const Board * B, int alpha, int beta)
{
  ei.clear(B);
  int val = 0;

  if constexpr (Expl) ed.clear();

  for (int i = 0; i < BK; i++) // material
  {
    const u64 bb = B->piece[i];
    val += mat[i] * popcnt(bb);
  }

  //const int margin = 300; // Lazy Eval
  //if (val - margin > alpha
  //&&  val + margin < beta)
  //{
  //  // probably check here:
  //  // - far advanced opponent passer
  //  // - king in danger (not in endgame)
  //  return val;
  //}

  Duo duo{}; // collecting ei here
  //duo += eval_xrays<1>(B) - eval_xrays<0>(B);
  duo += evaluateP<1>(B)  - evaluateP<0>(B);
  duo += evaluateN<1>(B)  - evaluateN<0>(B);
  duo += evaluateB<1>(B)  - evaluateB<0>(B);
  duo += evaluateR<1>(B)  - evaluateR<0>(B);
  duo += evaluateQ<1>(B)  - evaluateQ<0>(B);
  duo += evaluateK<1>(B)  - evaluateK<0>(B);

  val += duo.tapered(B->phase());
  val += ei.king_safety(); // TODO: not tapered?

  int score = (B->color ? val : -val) + term[Tempo];
  return score * (100 - B->state.fifty) / 100;
}

template<bool Expl>
template<Color Col>
Duo EvalAdvanced<Expl>::evaluateP(const Board * B)
{
  constexpr Piece me = to_piece(Pawn, Col);
  constexpr Piece opp = to_piece(Pawn, ~Col);
  Duo vals;
  for (u64 bb = B->piece[me]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);

    // pst
    vals += this->apply(this->pst[me][sq], me, sq, "PST");

    u64 back_friendly = front[~Col][sq] & B->piece[me];
    u64 fore_friendly = front[Col][sq] & B->piece[me];

    u64 cannot_pass = front[Col][sq] & B->piece[opp];
    u64 has_support = att_rear[Col][sq] & B->piece[me];
    u64 has_sentry = att_span[Col][sq] & B->piece[opp];

    // isolated

    if (!(isolator[sq] & B->piece[me]))
    {
      const Duo v = -Duo::both(this->term[Isolated]);
      vals += this->apply(v, me, sq, "Isolated pawn");
    }

    // doubled

    if (back_friendly && !fore_friendly) // most advanced one
    {
      const Duo v = -Duo::both(popcnt(back_friendly) * this->term[Doubled]);
      vals += this->apply(v, me, sq, "Doubled pawn");
    }

    // blocked weak

    if (cannot_pass && !has_support)
    {
      this->ei.add_weak(Col, sq);
    }

    // backward

    if (!has_support && has_sentry)
    {
      bool developed = Col ? rank(sq) > 3 : rank(sq) < 4;
      if (!developed)
      {
        const Duo v = -Duo::both(this->term[Backward]);
        vals += this->apply(v, me, sq, "Backward pawn");
      }

      this->ei.add_weak(Col, sq);
    }

    // passers

    if (!cannot_pass && !fore_friendly)
    {
      const Duo v = eval_passer<Col>(B, sq);
      vals += this->apply(v, me, sq, "Passers");
    }
  }
  return vals;
}

template<bool Expl>
template<Color Col>
Duo EvalAdvanced<Expl>::eval_passer(const Board * B, SQ sq)
{
  const SQ king = ei.king[Col];
  const SQ kopp = ei.king[~Col];

  // kpk probe <- TODO
  int kpk = 0;
  /*if (!B->has_pieces(Col) && !B->has_pieces(~Col))
  {
    int win = Kpk::probe<Col>(Col, king, sq, kopp);
    if (win > 0)
    {
      u64 pawns = B->piece[BP] | B->piece[WP];
      if (only_one(pawns))
      {
        kpk += term[Unstoppable];
      }
    }
  }*/

  int v = 0;
  const Piece p = BP ^ Col;
  const u64 sentries = att_span[Col][sq] & B->piece[opp(p)];
  int prank = Col ? rank(sq) : 7 - rank(sq);
  prank += prank == 1; // double pawn move
  prank += B->color == Col; // tempo
  const int scale = passer_scale[prank];

  // TODO: decrease scale factor for edge passers

  if (!sentries) // Passer
  {
    v += term[Passer] * scale / 256;

    if (!(front[Col][sq] & B->occ[Col]) // Unstoppable
    &&  !B->has_pieces(~Col))
    {
      SQ prom = to_sq(file(sq), Col ? 7 : 0);
      int turn = static_cast<int>(B->color != Col);

      // opp king is not in square
      if (k_dist(kopp, prom) - turn > k_dist(sq, prom))
      {
        v += term[Unstoppable];
      }
    }
    else
    if (!(bit(sq) & (FileA | FileH)) // King passer
    &&  !B->has_pieces(~Col))
    {
      SQ prom = to_sq(file(sq), Col ? 7 : 0);

      // own king controls all promote path
      if (file(king) != file(sq)
      &&  k_dist(king, sq) <= 1
      &&  k_dist(king, prom) <= 1)
      {
        v += term[Unstoppable];
      }
    }
    else // Bonuses for increasing passers potential
    {
      if (psupport[Col][sq] & B->piece[p]) // Supported
      {
        v += term[Supported] * scale / 256;
      }

      const u64 o = B->occ[0] | B->occ[1];
      if (!(front_one[Col][sq] & o)) // Free passer
      {
        SQ stop = Col ? sq + 8 : sq - 8;
        Move move = to_move(sq, stop);
        if (B->see(move) > 0)
        {
          v += term[FreePasser];
        }
      }

      // King tropism to stop square

      SQ stop = Col ? sq + 8 : sq - 8;
      int tropism = 20 * k_dist(kopp, stop)
                  -  5 * k_dist(king, stop);

      if (tropism > 0) v += tropism;
    }
  }
  else if (only_one(sentries)) // Candidate
  {
    SQ j = bitscan(sentries); // simplest case
    if (front[~Col][j] & B->piece[p])
    {
      v += term[Candidate] * scale / 256;
    }
  }

  v += kpk;
  return Duo(v / 2, v);
}

template<bool Expl>
template<Color Col>
Duo EvalAdvanced<Expl>::evaluateN(const Board * B)
{
  constexpr Piece p = to_piece(Knight, Col);
  Duo vals;
  for (u64 bb = B->piece[p]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);
    const u64 att = B->attack<Knight>(sq);

    // king attacks

    this->ei.add_attack(Col, AttWeight::Light, att);

    // pst & mobility

    vals += apply(pst[p][sq], p, sq, "PST");

    const Duo mob = Duo::both(this->term[NMob] * popcnt(att) / 32);
    vals += apply(mob, p, sq, "Mobility");

    // adjustments

    int pawns = popcnt(B->piece[BP ^ Col]);
    const Duo adj = Duo::both(this->n_adj[pawns]);
    vals += apply(adj, p, sq, "Adjustments");

    // forks

    u64 fork = att & (B->piece[WR ^ Col]
                    | B->piece[WQ ^ Col]
                    | B->piece[WK ^ Col]);

    if (rlsb(fork))
    {
      const Duo v = Duo::both(this->term[KnightFork]);
      vals += apply(v, p, sq, "Fork");
    }
  }
  return vals;
}

template<bool Expl>
template<Color Col>
Duo EvalAdvanced<Expl>::evaluateB(const Board * B)
{
  constexpr Piece p = to_piece(Bishop, Col);
  Duo vals;
  for (u64 bb = B->piece[p]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);
    const u64 att = B->attack<Bishop>(sq);

    // king attacks

    ei.add_attack(Col, AttWeight::Light, att);

    // pst & mobility

    vals += apply(pst[p][sq], p, sq, "PST");

    const Duo mob = Duo::both(term[BMob] * popcnt(att) / 32);
    vals += apply(mob, p, sq, "Mobility");

    // forks

    u64 valuable = B->piece[WR ^ Col]
                 | B->piece[WQ ^ Col]
                 | B->piece[WK ^ Col];

    u64 fork = att & valuable;

    if (rlsb(fork))
    {
      const Duo v = Duo::both(term[BishopFork]);
      vals += apply(v, p, sq, "Fork");
    }
  }

  // bishop pair

  if (several(B->piece[p])) vals += Duo::both(term[BishopPair]);
  return vals;
}

template<bool Expl>
template<Color Col>
Duo EvalAdvanced<Expl>::evaluateR(const Board * B)
{
  Piece p = to_piece(Rook, Col);
  Duo vals;
  for (u64 bb = B->piece[p]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);
    const u64 att = B->attack<Rook>(sq);

    // king attacks

    ei.add_attack(Col, AttWeight::Rook, att);

    // pst & mobility

    vals += apply(pst[p][sq], p, sq, "PST");

    const Duo mob = Duo::both(term[RMob] * popcnt(att) / 32);
    vals += apply(mob, p, sq, "Mobility");

    // adjustments

    int pawns = popcnt(B->piece[BP ^ Col]);
    const Duo adj = Duo::both(r_adj[pawns]);
    vals += apply(adj, p, sq, "Adjustments");

    // rook on 7th

    u64 own_pawns = B->piece[to_piece(Pawn, Col)];
    u64 opp_pawns = B->piece[to_piece(Pawn, ~Col)];
    const int rook_rank = Col == White ? 7 : 1;
    const int king_rank = Col == White ? 8 : 0;

    if (rank(sq) == rook_rank)
    {
      SQ opp_king = bitscan(B->piece[to_piece(King, ~Col)]);

      if (rank(opp_king) == king_rank || several(opp_pawns))
      {
        const Duo v = Duo(term[Rook7thOp], term[Rook7thEg]);
        vals += apply(v, p, sq, "Rook on 7th");
      }
    }

    // rook on open/semi-files

    if (!(front[Col][sq] & own_pawns))
    {
      int i = front[Col][sq] & opp_pawns ? RookSemi : RookOpen;
      vals += apply(Duo::both(term[i]), p, sq, "Rook on open");
    }
  }

  return vals;
}

template<bool Expl>
template<Color Col>
Duo EvalAdvanced<Expl>::evaluateQ(const Board * B)
{
  Piece p = to_piece(Queen, Col);
  Duo vals;
  for (u64 bb = B->piece[p]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);
    const u64 att = B->attack<Queen>(sq);

    // king attacks

    ei.add_attack(Col, AttWeight::Queen, att);

    // pst & mobility

    vals += apply(pst[p][sq], p, sq, "PST");

    const Duo mob = Duo::both(term[QMob] * popcnt(att) / 32);
    vals += apply(mob, p, sq, "Mobility");

    // early queen

    u64 undeveloped;
    if constexpr (Col)
    {
      if (rank(sq) > 1)
      {
        undeveloped  = B->piece[WN] & (bit(B1) | bit(G1));
        undeveloped |= B->piece[WB] & (bit(C1) | bit(F1));
      }
    }
    else
    {
      if (rank(sq) < 6)
      {
        undeveloped  = B->piece[BN] & (bit(B8) | bit(G8));
        undeveloped |= B->piece[BB] & (bit(C8) | bit(F8));
      }
    }
    const int penalty = popcnt(undeveloped); // 0..4
    const Duo v = -Duo::as_op(penalty * term[EarlyQueen] / 4);
    vals += apply(v, p, sq, "Early queen");
  }

  return vals;
}

template<bool Expl>
template<Color Col>
Duo EvalAdvanced<Expl>::evaluateK(const Board * B)
{
  Piece p = to_piece(King, Col);
  Duo vals;
  for (u64 bb = B->piece[p]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);

    // pst

    vals += apply(pst[p][sq], p, sq, "PST");

    // pawn weakness

    const int push = ei.weakness(~Col, term[WeaknessPush]);
    vals += apply(Duo::as_eg(push), p, sq, "Weakness push");

    // pawn shield

    Duo shield;
    u64 pawns = B->piece[BP ^ Col];
    u64 row1 = pawns & (Col ? Rank2 : Rank7);
    u64 row2 = pawns & (Col ? Rank3 : Rank6);

    if (file(sq) > 4)
    {
      if      (row1 & FileF) shield += Duo::as_op(term[Shield1]);
      else if (row2 & FileF) shield += Duo::as_op(term[Shield2]);

      if      (row1 & FileG) shield += Duo::as_op(term[Shield1]);
      else if (row2 & FileG) shield += Duo::as_op(term[Shield2]);

      if      (row1 & FileH) shield += Duo::as_op(term[Shield1]);
      else if (row2 & FileH) shield += Duo::as_op(term[Shield2]);
    }
    else
    {
      if      (row1 & FileA) shield += Duo::as_op(term[Shield1]);
      else if (row2 & FileA) shield += Duo::as_op(term[Shield2]);

      if      (row1 & FileB) shield += Duo::as_op(term[Shield1]);
      else if (row2 & FileB) shield += Duo::as_op(term[Shield2]);

      if      (row1 & FileC) shield += Duo::as_op(term[Shield1]);
      else if (row2 & FileC) shield += Duo::as_op(term[Shield2]);
    }

    vals += apply(shield, p, sq, "Shield");
  }
  return vals;
}

//////////////////

void EvalInfo::clear(const Board * B)
{
  king[0] = bitscan(B->piece[BK]);
  king[1] = bitscan(B->piece[WK]);
  att_weight[0] = att_weight[1] = 0;
  att_count[0] = att_count[1] = 0;
  eg_weak[0] = eg_weak[1] = 0;
  pinned = Empty;
}

void EvalInfo::add_attack(Color col, AttWeight weight, u64 att)
{
  const int count = popcnt(att & kingzone[~col][king[~col]]);
  att_weight[col] += count * static_cast<int>(weight);
  att_count[col] += count;
}

int EvalInfo::king_safety(Color col) const
{
  return att_count[col] > 2 ? safety_table[att_weight[col]] : 0;
}

int EvalInfo::king_safety() const
{
  return king_safety(White) - king_safety(Black);
}

void EvalInfo::add_weak(Color col, SQ sq)
{
  int val = weakness_push_table[k_dist(king[col], sq)];
  if (val > eg_weak[col]) eg_weak[col] = val;
}

int EvalInfo::weakness(Color col, int bonus) const
{
  return eg_weak[col] * bonus / 128;
}

//////////////////

#undef TERM
#define TERM(x,def) term[x] = def;

Eval::Eval()
{
  for (int i = 0; i < Term_N; i++)
    term[i] = 0;

  TERMS;
  init();
}

#undef TERM
#define TERM(x,def) str += format("{}:{} ", #x, term[x]);

string Eval::to_string() const
{
  string str;
  TERMS;
  return str;
}

#undef TERM
#define TERM(x,def) {                                       \
  size_t found = str.find(#x, 0);                           \
  if (found != string::npos)                                \
  {                                                         \
    size_t start = str.find(":", found);                    \
    size_t end = str.find(" ", found);                      \
    term[x] = stoi(str.substr(start + 1, end - start - 1)); \
  }}

void Eval::set(string str)
{
  TERMS;
  init();
}

#undef TERM

void Eval::set(Eval * eval)
{
  for (int i = 0; i < Term_N; i++)
    term[i] = eval->term[i];
    init();
}

void Eval::init()
{
  // Piece adjustments /////////////////////////////////////////////

  for (int i = 0; i < 9; i++)
  {
    n_adj[i] = term[KnightAdj] * PAdj[i];
    r_adj[i] = -term[RookAdj] * PAdj[i];
  }

  // Material //////////////////////////////////////////////////////

  mat[WP] = 100;
  mat[WN] = term[MatKnight];
  mat[WB] = term[MatBishop];
  mat[WR] = term[MatRook];
  mat[WQ] = term[MatQueen];
  mat[WK] = 20000;

  for (int i = 0; i < 12; i += 2)
    mat[i] = -mat[i + 1];

  // PST ///////////////////////////////////////////////////////////

  for (int i = 0; i < 12; i++)
    for (int sq = 0; sq < 64; sq++)
      pst[i][sq].clear();

  // Pawns //////////////////////////////////////////////

  int p = WP; 

  // file
  for (SQ sq = A1; sq < SQ_N; ++sq)
  {
    pst[p][sq].op += PFile[file(sq)] * term[PawnFile];
  }

  // center control
  pst[p][D3].op += 10;
  pst[p][E3].op += 10;

  pst[p][D4].op += 20;
  pst[p][E4].op += 20;

  pst[p][D5].op += 10;
  pst[p][E5].op += 10;

  // Knights ////////////////////////////////////////////

  p = WN;

  // center
  for (SQ sq = A1; sq < SQ_N; ++sq)
  {
    pst[p][sq].op += NLine[file(sq)] * term[KnightCenterOp];
    pst[p][sq].op += NLine[rank(sq)] * term[KnightCenterOp];
    pst[p][sq].eg += NLine[file(sq)] * term[KnightCenterEg];
    pst[p][sq].eg += NLine[rank(sq)] * term[KnightCenterEg];
  }

  // rank
  for (SQ sq = A1; sq < SQ_N; ++sq)
  {
    pst[p][sq].op += NRank[rank(sq)] * term[KnightRank];
  }

  // back rank
  for (int sq = A1; sq <= H1; sq++)
  {
    pst[p][sq].op -= term[KnightBackRank];
  }

  // "trapped"
  pst[p][A8].op -= term[KnightTrapped];
  pst[p][H8].op -= term[KnightTrapped];

  // Bishops ////////////////////////////////////////////

  p = WP;

  // center
  for (SQ sq = A1; sq < SQ_N; ++sq)
  {
    pst[p][sq].op += BLine[rank(sq)] * term[BishopCenterOp];
    pst[p][sq].op += BLine[file(sq)] * term[BishopCenterOp];
    pst[p][sq].eg += BLine[rank(sq)] * term[BishopCenterEg];
    pst[p][sq].eg += BLine[file(sq)] * term[BishopCenterEg];
  }

  // back rank
  for (SQ sq = A1; sq <= H1; ++sq)
  {
    pst[p][sq].op -= term[BishopBackRank];
  }

  // main diagonals
  for (int i = 0; i < 8; i++)
  {
    SQ sq = to_sq(i, i);
    pst[p][sq].op      += term[BishopDiagonal];
    pst[p][opp(sq)].op += term[BishopDiagonal];
  }

  // Rooks //////////////////////////////////////////////

  p = WR;

  // file
  for (SQ sq = A1; sq < SQ_N; ++sq)
  {
    pst[p][sq].op += RFile[file(sq)] * term[RookFileOp];
  }

  // Queens /////////////////////////////////////////////

  p = WQ;

  // center
  for (SQ sq = A1; sq < SQ_N; ++sq)
  {
    pst[p][sq].op += QLine[file(sq)] * term[QueenCenterOp];
    pst[p][sq].op += QLine[rank(sq)] * term[QueenCenterOp];
    pst[p][sq].eg += QLine[file(sq)] * term[QueenCenterEg];
    pst[p][sq].eg += QLine[rank(sq)] * term[QueenCenterEg];
  }

  // back rank
  for (SQ sq = A1; sq <= H1; ++sq)
  {
    pst[p][sq].op -= term[QueenBackRank];
  }

  // Kings //////////////////////////////////////////////

  p = WK;

  for (SQ sq = A1; sq < SQ_N; ++sq)
  {
    pst[p][sq].op += KFile[file(sq)] * term[KingFile];
    pst[p][sq].op += KRank[rank(sq)] * term[KingRank];
    pst[p][sq].eg += KLine[file(sq)] * term[KingCenterEg];
    pst[p][sq].eg += KLine[rank(sq)] * term[KingCenterEg];
  }

  // Symmetrical copy for black

  for (Piece p = BP; p < Piece_N; p = p + 2)
  {
    for (SQ sq = A1; sq < SQ_N; ++sq)
      pst[p][sq] = pst[p + 1][opp(sq)];
  }

  // Show

  /*for (Piece p = BP; p < Piece_N; ++p)
  {
    cout << to_char(p) << "\n";
    for (int y = 7; y >= 0; y--)
    {
      for (int x = 0; x < 8; x++)
      {
        SQ sq = to_sq(x,y);
        cout << setw(11) << pst[p][sq];
      }
      cout << "\n";
    }
    cout << "\n";
  }*/

  // Passers ///////////////////////////////////////////////////////

  auto unzero = [](double x) { return x > 0 ? x : .001; };
  auto pscore = [](double m, double k, int rank)
  {
    auto nexp = [](double k, int x) { return 1 / (1 + exp(6 - k * x)); };
    return static_cast<int>(m * nexp(k, rank) / nexp(k, 6));
  };

  for (int rank = 0; rank < 8; rank++)
  {
    const double k = unzero(term[PasserK]) / 32.0;
    passer_scale[rank] = pscore(256, k, rank);
  }
}

// instantiations

template int EvalSimple<0>::eval(const Board * B, int alpha, int beta);
template int EvalSimple<1>::eval(const Board * B, int alpha, int beta);

template int EvalAdvanced<0>::eval(const Board * B, int alpha, int beta);
template int EvalAdvanced<1>::eval(const Board * B, int alpha, int beta);

template Duo EvalAdvanced<0>::evaluateP<Black>(const Board * B);
template Duo EvalAdvanced<0>::evaluateN<Black>(const Board * B);
template Duo EvalAdvanced<0>::evaluateB<Black>(const Board * B);
template Duo EvalAdvanced<0>::evaluateR<Black>(const Board * B);
template Duo EvalAdvanced<0>::evaluateQ<Black>(const Board * B);
template Duo EvalAdvanced<0>::evaluateK<Black>(const Board * B);

template Duo EvalAdvanced<1>::evaluateP<Black>(const Board * B);
template Duo EvalAdvanced<1>::evaluateN<Black>(const Board * B);
template Duo EvalAdvanced<1>::evaluateB<Black>(const Board * B);
template Duo EvalAdvanced<1>::evaluateR<Black>(const Board * B);
template Duo EvalAdvanced<1>::evaluateQ<Black>(const Board * B);
template Duo EvalAdvanced<1>::evaluateK<Black>(const Board * B);

template Duo EvalAdvanced<0>::evaluateP<White>(const Board * B);
template Duo EvalAdvanced<0>::evaluateN<White>(const Board * B);
template Duo EvalAdvanced<0>::evaluateB<White>(const Board * B);
template Duo EvalAdvanced<0>::evaluateR<White>(const Board * B);
template Duo EvalAdvanced<0>::evaluateQ<White>(const Board * B);
template Duo EvalAdvanced<0>::evaluateK<White>(const Board * B);

template Duo EvalAdvanced<1>::evaluateP<White>(const Board * B);
template Duo EvalAdvanced<1>::evaluateN<White>(const Board * B);
template Duo EvalAdvanced<1>::evaluateB<White>(const Board * B);
template Duo EvalAdvanced<1>::evaluateR<White>(const Board * B);
template Duo EvalAdvanced<1>::evaluateQ<White>(const Board * B);
template Duo EvalAdvanced<1>::evaluateK<White>(const Board * B);

template Duo EvalAdvanced<0>::eval_passer<Black>(const Board * B, SQ sq);
template Duo EvalAdvanced<1>::eval_passer<Black>(const Board * B, SQ sq);
template Duo EvalAdvanced<0>::eval_passer<White>(const Board * B, SQ sq);
template Duo EvalAdvanced<1>::eval_passer<White>(const Board * B, SQ sq);

}
