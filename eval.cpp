#include <format>
#include <iomanip>
#include "eval.h"
#include "board.h"

using namespace std;

namespace eia {

int Eval::eval(const Board * B, int alpha, int beta)
{
  int val = 0;
    
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

  val += evaluate(B); // collecting ei
  //val += ei.king_safety();

  int score = (B->color ? val : -val) + term[Tempo];
  return score * (100 - B->state.fifty) / 100;
}

int Eval::evaluate(const Board * B)
{
  Duo vals{};
  //vals += eval_xrays!White(B) - eval_xrays!Black(B);
  vals += evaluateP<White>(B)  - evaluateP<Black>(B);
  vals += evaluateN<White>(B)  - evaluateN<Black>(B);
  vals += evaluateB<White>(B)  - evaluateB<Black>(B);
  vals += evaluateR<White>(B)  - evaluateR<Black>(B);
  vals += evaluateQ<White>(B)  - evaluateQ<Black>(B);
  //vals += evaluateK<White>(B)  - evaluateK<Black>(B);

  return vals.tapered(B->phase());
}

template<Color col>
Duo Eval::evaluateP(const Board * B)
{
  constexpr Piece me = to_piece(Pawn, col);
  constexpr Piece opp = to_piece(Pawn, ~col);
  Duo vals;
  for (u64 bb = B->piece[me]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);

    // pst

    vals += pst[me][sq];

    u64 back_friendly = front[~col][sq] & B->piece[me];
    u64 fore_friendly = front[col][sq] & B->piece[me];

    u64 cannot_pass = front[col][sq] & B->piece[opp];
    //u64 has_support = att_rear[col][sq] & B->piece[me];
    u64 has_sentry = att_span[col][sq] & B->piece[opp];

    // isolated

    /*if (!(isolator[sq] & B->piece[me]))
    {
      vals -= Duo::both(term[Isolated]);
    }*/

    // doubled

    if (back_friendly && !fore_friendly) // most advanced one
    {
      vals -= Duo::both(popcnt(back_friendly) * term[Doubled]);
    }

    // blocked weak

    /*if (cannot_pass && !has_support)
    {
      ei.add_weak(col, sq);
    }*/

    // backward

    //if (!has_support && has_sentry)
    //{
    //  bool developed = col ? rank(sq) > 3 : rank(sq) < 4;
    //  if (!developed) vals -= Duo::both(term[Backward]);

    //  //ei.add_weak(col, sq);
    //}

    // passers

    if (!cannot_pass && !fore_friendly)
    {
      //vals += eval_passer<col>(B, sq);
    }
  }
  return vals;
}

template<Color col>
Duo Eval::evaluateN(const Board * B)
{
  constexpr Piece p = to_piece(Knight, col);
  Duo vals;
  for (u64 bb = B->piece[p]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);
    const u64 att = B->attack<Knight>(sq);

    // pst & mobility

    vals += pst[p][sq];
    vals += Duo::both(term[NMob] * popcnt(att) / 32);

    // adjustments

    int pawns = popcnt(B->piece[BP ^ col]);
    vals += Duo::both(n_adj[pawns]);

    // forks

    u64 fork = att & (B->piece[WR ^ col]
                    | B->piece[WQ ^ col]
                    | B->piece[WK ^ col]);

    if (rlsb(fork)) vals += Duo::both(term[KnightFork]);
  }
  return vals;
}

template<Color col>
Duo Eval::evaluateB(const Board * B)
{
  constexpr Piece p = to_piece(Bishop, col);
  Duo vals;
  for (u64 bb = B->piece[p]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);
    const u64 att = B->attack<Bishop>(sq);

    // pst & mobility

    vals += pst[p][sq];
    vals += Duo::both(term[BMob] * popcnt(att) / 32);

    // forks

    u64 valuable = B->piece[WR ^ col]
                 | B->piece[WQ ^ col]
                 | B->piece[WK ^ col];

    u64 fork = att & valuable;

    if (rlsb(fork)) vals += Duo::both(term[BishopFork]);
  }

  // bishop pair

  if (several(B->piece[p])) vals += Duo::both(term[BishopPair]);
  return vals;
}

template<Color col>
Duo Eval::evaluateR(const Board * B)
{
  Piece p = to_piece(Rook, col);
  Duo vals;
  for (u64 bb = B->piece[p]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);
    const u64 att = B->attack<Rook>(sq);

    // pst & mobility

    vals += pst[p][sq];
    vals += Duo::both(term[RMob] * popcnt(att) / 32);

    // adjustments

    int pawns = popcnt(B->piece[BP ^ col]);
    vals += Duo::both(r_adj[pawns]);

    // rook on 7th

    u64 own_pawns = B->piece[to_piece(Pawn, col)];
    u64 opp_pawns = B->piece[to_piece(Pawn, ~col)];
    const int rook_rank = col == White ? 7 : 1;
    const int king_rank = col == White ? 8 : 0;

    if (rank(sq) == rook_rank)
    {
      SQ opp_king = bitscan(B->piece[to_piece(King, ~col)]);

      if (rank(opp_king) == king_rank || several(opp_pawns))
        vals += Duo(term[Rook7thOp], term[Rook7thEg]);
    }

    // rook on open/semi-files

    if (!(front[col][sq] & own_pawns))
    {
      if (front[col][sq] & opp_pawns)
        vals += Duo::both(term[RookSemi]);
      else
        vals += Duo::both(term[RookOpen]);
    }
  }

  return vals;
}

template<Color col>
Duo Eval::evaluateQ(const Board * B)
{
  Piece p = to_piece(Queen, col);
  Duo vals;
  for (u64 bb = B->piece[p]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);
    const u64 att = B->attack<Queen>(sq);

    // pst & mobility

    vals += pst[p][sq];
    vals += Duo::both(term[QMob] * popcnt(att) / 32);

    // early queen

    u64 undeveloped;
    if constexpr (col)
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
        undeveloped  = B->piece[WN] & (bit(B8) | bit(G8));
        undeveloped |= B->piece[WB] & (bit(C8) | bit(F8));
      }
    }
    const int penalty = popcnt(undeveloped);
    vals -= Duo::as_op(penalty * term[EarlyQueen]);
  }

  return vals;
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

template Duo Eval::evaluateP<Black>(const Board * B);
template Duo Eval::evaluateN<Black>(const Board * B);
template Duo Eval::evaluateB<Black>(const Board * B);
template Duo Eval::evaluateR<Black>(const Board * B);
template Duo Eval::evaluateQ<Black>(const Board * B);
template Duo Eval::evaluateK<Black>(const Board * B);

template Duo Eval::evaluateP<White>(const Board * B);
template Duo Eval::evaluateN<White>(const Board * B);
template Duo Eval::evaluateB<White>(const Board * B);
template Duo Eval::evaluateR<White>(const Board * B);
template Duo Eval::evaluateQ<White>(const Board * B);
template Duo Eval::evaluateK<White>(const Board * B);

}
