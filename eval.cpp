#include <format>
#include <iomanip>
#include "eval.h"
#include "board.h"
#include "value.h"
#include "hash.h"

using namespace std;

namespace eia {

// from Toga
const Val safety_table[100] =
{
    0_cp,   0_cp,   1_cp,   2_cp,   3_cp,   5_cp,   7_cp,   9_cp,  12_cp,  15_cp,
   18_cp,  22_cp,  26_cp,  30_cp,  35_cp,  39_cp,  44_cp,  50_cp,  56_cp,  62_cp,
   68_cp,  75_cp,  82_cp,  85_cp,  89_cp,  97_cp, 105_cp, 113_cp, 122_cp, 131_cp,
  140_cp, 150_cp, 169_cp, 180_cp, 191_cp, 202_cp, 213_cp, 225_cp, 237_cp, 248_cp,
  260_cp, 272_cp, 283_cp, 295_cp, 307_cp, 319_cp, 330_cp, 342_cp, 354_cp, 366_cp,
  377_cp, 389_cp, 401_cp, 412_cp, 424_cp, 436_cp, 448_cp, 459_cp, 471_cp, 483_cp,
  494_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp,
  500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp,
  500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp,
  500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp, 500_cp
};

const Val weakness_push_table[] =
{
  128_cp, 106_cp, 86_cp, 68_cp, 52_cp, 38_cp, 26_cp, 16_cp, 8_cp, 2_cp
};

//    Engine        Score          St         Mo         Ei         Ei         Li    S-B
// 1: Stellar-1.4.3 32,0/40 ·········· =000101011 =1111111=1 1111111111 11101=1111  505,25
// 2: Monarch(v1.7) 26,0/40 =111010100 ·········· 0101010011 1==1010110 111111=111  428,75
// 3: Eia-v0.5      22,0/40 =0000000=0 1010101100 ·········· 11=1=1011= 111110=111  318,50
// 4: Eia_v0_3      13,5/40 0000000000 0==0101001 00=0=0100= ·········· =101011=11  204,50
// 5: Liquid_v0_1   6,5/40  00010=0000 000000=000 000001=000 =010100=00 ··········  134,50
//
// 100 games played / Tournament is finished
// Name of the tournament: 105
// Level: Blitz 0:20/0,2


// All tests were done with control 10s+.1s (h2h-30)
// 
// + xrays & pins (with king)  +92 elo
// + nonlinear mobility       +100 elo
// + connected pawns           +30 elo
// + threats                  +150 elo
// + rebalanced material       +50 elo
// - trapped pieces            ??? elo
// - relative pin on queen     ??? elo
// 
// = bishop rammed pawns       -70 elo (need to tune?)
// = soft mobility             -80 elo

Val Eval::eval(const Board * B, Val alpha, Val beta)
{
  ei.init(B);
  Duo duo{};

  // +70 elo (20s+.2s h2h-30)
  if (B->is_simply_mated()) return -Val::Inf;

#ifdef _DEBUG
  ed.clear();
#endif
  
  for (int i = 0; i < BK; i++) // material
  {
    const u64 bb = B->piece[i];
    duo += mat[i] * popcnt(bb);
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

  // collecting ei here
  duo += evalxrays<White>(B) - evalxrays<Black>(B);

  // Pawn-king hash table | +20 elo (20s+.2 h2h-20)

  Duo pvals;
  auto pk = Hash::pk_probe(B->state.pkhash);
  if (pk == nullptr)
  {
    pvals = evaluateP<White>(B) - evaluateP<Black>(B);

    /*if (pk != nullptr)
    {
      if (pvals != pk->vals
      ||  ei.eg_weak[0] != pk->weak[0]
      ||  ei.eg_weak[1] != pk->weak[1])
      {
        B->print();
        log("Expected: val {} weaks {} {}\n", pvals, ei.eg_weak[0], ei.eg_weak[1]);
        log("Hashed:   val {} weaks {} {}\n", pk->vals, pk->weak[0], pk->weak[1]);
        __debugbreak();
      }
    }*/
    Hash::pk_store(B->state.pkhash, pvals, ei.eg_weak);
  }
  else
  {
    pvals = pk->vals;
    ei.eg_weak[0] = pk->weak[0];
    ei.eg_weak[1] = pk->weak[1];
  }

  duo += pvals;
  duo += evaluateN<White>(B) - evaluateN<Black>(B);
  duo += evaluateB<White>(B) - evaluateB<Black>(B);
  duo += evaluateR<White>(B) - evaluateR<Black>(B);
  duo += evaluateQ<White>(B) - evaluateQ<Black>(B);
  duo += evaluateK<White>(B) - evaluateK<Black>(B);

  duo += eval_threats<White>(B) - eval_threats<Black>(B);

  Val val = duo.tapered(B->phase());
  val += ei.king_safety(); // TODO: not tapered?

  Val score = (B->color ? val : -val) + term[Tempo];
  score = std::clamp(score, -Val::Mate / 2, Val::Mate / 2);
  return score * (1. - B->state.fifty / 100.);
}

template<Color Col>
Duo Eval::evalxrays(const Board * B)
{
  Duo vals{};
  const u64 o = B->occupied();
  const SQ king = bitscan(B->piece[BK ^ Col]);

  u64 blockers  = r_att(o, king) & o;
  u64 attackers = r_att(o ^ blockers, king) & B->ortho<~Col>();

  blockers   = b_att(o, king) & o;
  attackers |= b_att(o ^ blockers, king) & B->diags<~Col>();

  for (u64 bb = attackers; bb; bb = rlsb(bb))
  {
    const SQ  asq = bitscan(bb);
    const SQ   sq = bitscan(between[king][asq] & o);
    const Piece a = B->square[asq];
    const Piece p = B->square[sq];

    if (col(p) == Col) // Pin
    {
      if (pt(p) > Pawn)
      {
        Val cost = cp(see_value[p] - see_value[a]);
        Val penalty = see_value[p] * term[PinMul] / 256
                    + (std::max)(0_cp, cost) / 8;
        
        const Duo v = -Duo::both(penalty);
        vals += APPLY(v, "Pinned");
      }
    }
    else // Xray
    {
      Duo v = -Duo::both(term[Xray]);

      if (pt(p) == Pawn // xrayer is a blocked pawn
      &&  front_one[~Col][sq] & B->piece[BP ^ Col]) v /= 8;

      vals += APPLY(v, "Xrayer");
    }
  }
  return vals;
}

template<Color Col>
Duo Eval::evaluateP(const Board * B)
{
  constexpr Piece p = to_piece(Pawn, Col);
  constexpr Piece opp = to_piece(Pawn, ~Col);

  ei.add_attack(Col, Pawn, ei.pawn_atts[Col]);

  Duo vals{};
  for (u64 bb = B->piece[p]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);

    // pst
    vals += APPLY(pst[p][sq], "PST");

    u64 back_friendly = front[~Col][sq] & B->piece[p];
    u64 fore_friendly = front[Col][sq] & B->piece[p];
    u64 connected  = psupport[Col][sq] & B->piece[p];

    u64 cannot_pass = front[Col][sq] & B->piece[opp];
    u64 has_support = att_rear[Col][sq] & B->piece[p];
    u64 has_sentry = att_span[Col][sq] & B->piece[opp];

    // isolated

    if (!(isolator[sq] & B->piece[p]))
    {
      const Duo v = -Duo::both(term[Isolated]);
      vals += APPLY(v, "Isolated pawn");
    }

    // doubled

    if (back_friendly && !fore_friendly) // most advanced one
    {
      const Duo v = -Duo::both(popcnt(back_friendly) * term[Doubled]);
      vals += APPLY(v, "Doubled pawn");
    }

    // blocked weak

    if (cannot_pass)
    {
      ei.rammed[Col] |= bit(sq);
      if (!has_support) ei.add_weak(Col, sq);
    }

    // backward

    if (!has_support && has_sentry)
    {
      bool developed = Col ? rank(sq) > 3 : rank(sq) < 4;
      if (!developed)
      {
        const Duo v = -Duo::both(term[Backward]);
        vals += APPLY(v, "Backward pawn");
      }

      ei.add_weak(Col, sq);
    }

    else

    // connected

    if (connected)
    {
      const Duo v = Duo::both(term[Connected]);
      vals += APPLY(v, "Connected pawn");
    }

    // passers

    if (!cannot_pass && !fore_friendly)
    {
      const Duo v = eval_passer<Col>(B, sq);
      vals += APPLY(v, "Passers");
    }
  }

  // pawn shield

  Duo shield;
  u64 king  = B->piece[BK ^ Col];
  u64 pawns = B->piece[BP ^ Col];
  u64 row1 = pawns & (Col ? Rank2 : Rank7);
  u64 row2 = pawns & (Col ? Rank3 : Rank6);

  if (king & KWing)
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

  vals += APPLY(shield, "Shield");

  return vals;
}

template<Color Col>
Duo Eval::eval_passer(const Board * B, SQ sq)
{
  const SQ king = ei.king[Col];
  const SQ kopp = ei.king[~Col];

  // kpk probe <- TODO
  Val kpk = 0_cp;
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

  Val v = 0_cp;
  const Piece p = BP ^ Col;
  const u64 sentries = att_span[Col][sq] & B->piece[opp(p)];
  int prank = Col ? rank(sq) : 7 - rank(sq);
  prank += prank == 1; // double pawn move
  prank += B->color == Col; // tempo
  const int scale = passer_scale[prank];

  // TODO: decrease scale factor for edge passers

  if (!sentries) // Passer
  {
    v += term[Passer] / 256 * scale;

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
        v += term[Supported] / 256 * scale;
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
      Val tropism = 20_cp * k_dist(kopp, stop)
                  -  5_cp * k_dist(king, stop);

      if (tropism > 0_cp) v += tropism;
    }
  }
  else if (only_one(sentries)) // Candidate
  {
    SQ j = bitscan(sentries); // simplest case
    if (front[~Col][j] & B->piece[p])
    {
      v += term[Candidate] / 256 * scale;
    }
  }

  v += kpk;
  return Duo(v / 2, v);
}

template<Color Col>
Duo Eval::evaluateN(const Board * B)
{
  constexpr Piece p = to_piece(Knight, Col);
  ei.attacked_by[Col][Knight] = 0ull;
  Duo vals{};

  for (u64 bb = B->piece[p]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);
    const u64 att = B->attack<Knight>(sq);
    ei.add_attack(Col, Knight, att);

    // king attacks

    ei.add_king_attack(Col, AttWeight::Light, att);

    // pst & mobility

    vals += APPLY(pst[p][sq], "PST");

    Val v = mob[Knight][popcnt(att)];
    vals += APPLY(Duo(v, 3 * v / 2), "Mobility");

    // adjustments

    int pawns = popcnt(B->piece[BP ^ Col]);
    const Duo adj = Duo::both(this->n_adj[pawns]);
    vals += APPLY(adj, "Adjustments");

    // forks

    u64 fork = att & (B->piece[WR ^ Col]
                    | B->piece[WQ ^ Col]
                    | B->piece[WK ^ Col]);

    if (rlsb(fork))
    {
      const Duo v = Duo::both(this->term[KnightFork]);
      vals += APPLY(v, "Knight Fork");
    }
  }
  return vals;
}

template<Color Col>
Duo Eval::evaluateB(const Board * B)
{
  constexpr Piece p = to_piece(Bishop, Col);
  ei.attacked_by[Col][Bishop] = 0ull;
  Duo vals{};

  for (u64 bb = B->piece[p]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);
    const u64 att = B->attack<Bishop>(sq, ei.occ_not_bq[Col]);
    ei.add_attack(Col, Bishop, att);

    // king attacks

    ei.add_king_attack(Col, AttWeight::Light, att);

    // pst & mobility

    vals += APPLY(pst[p][sq], "PST");

    Val v = mob[Bishop][popcnt(att)];
    vals += APPLY(Duo(v, 3 * v / 2), "Mobility");

    // rammed bishop

    /*u64 colour = bit(sq) & Light ? Light : Dark;
    Val v = -popcnt(colour & ei.rammed[Col]) * term[RammedBishop];
    vals += APPLY(Duo(v, 2 * v), "Rammed bishop");*/

    // forks

    u64 valuable = B->piece[WR ^ Col]
                 | B->piece[WQ ^ Col]
                 | B->piece[WK ^ Col];

    u64 fork = att & valuable;

    if (rlsb(fork))
    {
      const Duo v = Duo::both(term[BishopFork]);
      vals += APPLY(v, "Bishop Fork");
    }
  }

  // bishop pair

  if (several(B->piece[p])) vals += Duo::both(term[BishopPair]);
  return vals;
}

template<Color Col>
Duo Eval::evaluateR(const Board * B)
{
  Piece p = to_piece(Rook, Col);
  ei.attacked_by[Col][Rook] = 0ull;
  Duo vals{};

  for (u64 bb = B->piece[p]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);
    const u64 att = B->attack<Rook>(sq, ei.occ_not_rq[Col]);
    ei.add_attack(Col, Rook, att);

    // king attacks

    ei.add_king_attack(Col, AttWeight::Rook, att);

    // pst & mobility

    vals += APPLY(pst[p][sq], "PST");

    Val v = mob[Rook][popcnt(att)];
    vals += APPLY(Duo(v, 3 * v / 2), "Mobility");

    // adjustments

    int pawns = popcnt(B->piece[BP ^ Col]);
    const Duo adj = Duo::both(r_adj[pawns]);
    vals += APPLY(adj, "Adjustments");

    // rook on 7th

    const u64 own_pawns = B->piece[to_piece(Pawn, Col)];
    const u64 opp_pawns = B->piece[to_piece(Pawn, ~Col)];
    const int rook_rank = Col == White ? 7 : 1;
    const int king_rank = Col == White ? 8 : 0;

    if (rank(sq) == rook_rank)
    {
      SQ opp_king = bitscan(B->piece[to_piece(King, ~Col)]);

      if (rank(opp_king) == king_rank || several(opp_pawns))
      {
        const Duo v = Duo(term[Rook7thOp], term[Rook7thEg]);
        vals += APPLY(v, "Rook on 7th");
      }
    }

    // rook on open/semi-files

    if (!(front[Col][sq] & own_pawns))
    {
      int i = front[Col][sq] & opp_pawns ? RookSemi : RookOpen;
      vals += APPLY(Duo::both(term[i]), "Rook on open");
    }
  }

  return vals;
}

template<Color Col>
Duo Eval::evaluateQ(const Board * B)
{
  Piece p = to_piece(Queen, Col);
  ei.attacked_by[Col][Queen] = 0ull;
  Duo vals{};

  for (u64 bb = B->piece[p]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);
    const u64 att = B->attack<Queen>(sq);
    ei.add_attack(Col, Queen, att);

    // king attacks

    ei.add_king_attack(Col, AttWeight::Queen, att);

    // pst & mobility

    vals += APPLY(pst[p][sq], "PST");

    Val m = mob[Queen][popcnt(att)];
    vals += APPLY(Duo(m, 3 * m / 2), "Mobility");

    // queen on open/semi-files

    const u64 own_pawns = B->piece[to_piece(Pawn, Col)];
    const u64 opp_pawns = B->piece[to_piece(Pawn, ~Col)];

    if (!(front[Col][sq] & own_pawns))
    {
      int i = front[Col][sq] & opp_pawns ? RookSemi : RookOpen;
      vals += APPLY(Duo::both(term[i] / 2), "Queen on open");
    }

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
    vals += APPLY(v, "Early queen");
  }

  return vals;
}

template<Color Col>
Duo Eval::evaluateK(const Board * B)
{
  Piece p = to_piece(King, Col);
  Duo vals{};
  for (u64 bb = B->piece[p]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);

    // pst

    vals += APPLY(pst[p][sq], "PST");

    // pawn weakness

    const Val push = ei.weakness(~Col, dry(term[WeaknessPush]));
    vals += APPLY(Duo::as_eg(push), "Weakness push");
  }
  return vals;
}

template<Color Col>
Duo Eval::eval_threats(const Board * B)
{
  Duo vals{};
  int cnt;

  // Shamelessly taken from Ethereal

  constexpr Color me = Col;
  constexpr Color opp = ~Col;

  const u64 pawns_atts = ei.attacked_by[opp][Pawn];
  const u64 light_atts = ei.attacked_by[opp][Knight] | ei.attacked_by[opp][Bishop];
  const u64 heavy_atts = ei.attacked_by[opp][Rook]   | ei.attacked_by[opp][Queen];

  // Squares with more attackers, few defenders, and no pawn support
  const u64 poor_defend = (ei.attacked[opp] & ~ei.attacked[me])
                        | (ei.attacked_by2[opp] & ~ei.attacked_by2[me] & ~ei.attacked_by[me][Pawn]);

  const u64 lights = B->lights<Col>();
  const u64 rooks  = B->piece[BR ^ Col];
  const u64 queens = B->piece[BQ ^ Col];
  const u64 weak_light = lights & poor_defend;

  // Penalty for each of our poorly supported pawns
  cnt = popcnt(B->piece[BP ^ Col] & ~pawns_atts & poor_defend);
  vals += A(Duo::both(-cnt * term[ThreatPawn]), BP ^ Col, SQ_N, "Threat pawns");

  // lights <- pawns
  cnt = popcnt(lights & pawns_atts);
  vals += A(Duo::both(-cnt * term[ThreatL_P]), BN ^ Col, SQ_N, "Threat lights <- pawns");

  // lights <- lights
  cnt = popcnt(lights & light_atts);
  vals += A(Duo::both(-cnt * term[ThreatL_L]), BN ^ Col, SQ_N, "Threat lights <- lights");

  // weak lights <- heavy
  cnt = popcnt(weak_light & heavy_atts);
  vals += A(Duo::both(-cnt * term[ThreatL_H]), BN ^ Col, SQ_N, "Threat lights <- heavy");

  // weak lights <- king
  cnt = popcnt(weak_light & ei.attacked_by[opp][King]);
  vals += A(Duo::both(-cnt * term[ThreatL_K]), BN ^ Col, SQ_N, "Threat weak lights <- king");

  // rooks <- pawns, lights
  cnt = popcnt(rooks & (pawns_atts | light_atts));
  vals += A(Duo::both(-cnt * term[ThreatR_L]), BR ^ Col, SQ_N, "Threat rooks <- pawns, lights");

  // weak rooks <- king
  cnt = popcnt(rooks & poor_defend & ei.attacked_by[opp][King]);
  vals += A(Duo::both(-cnt * term[ThreatR_K]), BR ^ Col, SQ_N, "Threat weak rooks <- king");

  // queens <- any
  cnt = popcnt(queens & ei.attacked[opp]);
  vals += A(Duo::both(-cnt * term[ThreatQ_1]), BQ ^ Col, SQ_N, "Threat queens <- any");

  return vals;
}

//////////////////

void EvalInfo::init(const Board * B)
{
  king[0] = bitscan(B->piece[BK]);
  king[1] = bitscan(B->piece[WK]);
  king_att_weight[0] = king_att_weight[1] = 0;
  king_att_count[0] = king_att_count[1] = 0;
  eg_weak[0] = eg_weak[1] = 0_cp;
  rammed[0] = rammed[1] = 0ull;

  occ_not_rq[0] = B->occupied() ^ B->ortho<Black>();
  occ_not_rq[1] = B->occupied() ^ B->ortho<White>();

  occ_not_bq[0] = B->occupied() ^ B->diags<Black>();
  occ_not_bq[1] = B->occupied() ^ B->diags<White>();

  const u64 bpawns = B->piece[BP];
  const u64 wpawns = B->piece[WP];

  pawn_atts[0]  = shift_dl(bpawns) | shift_dr(bpawns);
  pawn_atts[1]  = shift_ul(wpawns) | shift_ur(wpawns);

  pawn_atts2[0] = shift_dl(bpawns) & shift_dr(bpawns);
  pawn_atts2[1] = shift_ul(wpawns) & shift_ur(wpawns);

  attacked[0] = attacked_by[0][King] = atts[BK][king[0]];
  attacked[1] = attacked_by[1][King] = atts[WK][king[1]];

  attacked_by2[0] = attacked_by2[1] = 0ull;

  attacked_by[0][Pawn] = attacked_by[1][Pawn] = 0ull;
}

void EvalInfo::add_king_attack(Color col, AttWeight weight, u64 att)
{
  const int count = popcnt(att & kingzone[~col][king[~col]]);
  king_att_weight[col] += count * static_cast<int>(weight);
  king_att_count[col] += count;
}

Val EvalInfo::king_safety(Color col) const
{
  return king_att_count[col] > 2 ? safety_table[king_att_weight[col]] : 0_cp;
}

Val EvalInfo::king_safety() const
{
  return king_safety(White) - king_safety(Black);
}

void EvalInfo::add_weak(Color col, SQ sq)
{
  Val val = weakness_push_table[k_dist(king[~col], sq)];
  if (val > eg_weak[col]) eg_weak[col] = val;
}

Val EvalInfo::weakness(Color col, int bonus) const
{
  return eg_weak[col] * bonus / 128;
}

void EvalInfo::add_attack(Color col, PieceType pt, u64 att)
{
  attacked_by2[col]    |= att & attacked[col];
  attacked[col]        |= att;
  attacked_by[col][pt] |= att;
}

//////////////////

#undef TERM
#define TERM(x,def,min,max) \
{                           \
  static_assert(max > min); \
  term[x] = def;            \
  term_min[x] = min;        \
  term_max[x] = max;        \
}

Eval::Eval()
{
  for (int i = 0; i < Term_N; i++)
    term[i] = 0_cp;

  TERMS;

  if (!active_tune.empty())
    set(active_tune);

  //log("{}\n", to_string());

  init();
}

Eval::Eval(const Tune & tune)
{
  TERMS;
  set(tune);
}

#undef TERM
#define TERM(x,def,min,max) str += format("{}:{:.4f} ", #x, dry_double(term[x]));

string Eval::to_string() const
{
  string str;
  TERMS;
  return str;
}

#undef TERM
#define TERM(x,def,min,max) str += format("{}:{} ", #x, term[x]);

string Eval::prettify() const
{
  string str;
  TERMS;
  return str;
}

Bounds Eval::bounds() const
{
  Bounds result;
  for (int i = 0; i < Term_N; i++)
  {
    double min = dry_double(term_min[i]);
    double max = dry_double(term_max[i]);
    result.push_back(make_pair(min, max));
  }
  return result;
}

#undef TERM
#define TERM(x,def,min,max) {                            \
  size_t found = str.find(#x, 0);                        \
  if (found != string::npos)                             \
  {                                                      \
    size_t a = str.find(":", found);                     \
    size_t b = str.find(" ", found);                     \
    auto v = parse_double(str.substr(a + 1, b - a - 1)); \
    term[x] = cp(v);                                     \
  }}

void Eval::set(string str)
{
  TERMS;
  init();
}

#undef TERM

string Eval::to_raw() const
{
  string str;
  for (int i = 0; i < Term_N; i++)
    str += format("{} ", term[i]);
  return str;
}

void Eval::set_raw(string str, std::string delim)
{
  for (int i = 0; i < Term_N; i++)
  {
    string part = cut(str, delim);
    double def = dry_double(term[i]);
    double v = parse_double(part, def);
    term[i] = cp(v);
  }
  init();
}

void Eval::set(const Eval & eval)
{
  for (int i = 0; i < Term_N; i++)
    term[i] = eval.term[i];
  init();
}

void Eval::set(const Tune & tune)
{
  for (int i = 0; i < Term_N; i++)
    term[i] = term_min[i] + (term_max[i] - term_min[i]) * tune[i];
  init();
}

Tune Eval::to_tune() const
{
  Tune tune;
  for (int i = 0; i < Term_N; i++)
  {
    double range = dry_double(term_max[i] - term_min[i]);
    double val = dry_double(term[i] - term_min[i]) / range;
    tune.push_back(val);
  }
  return tune;
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

  mat[WP] = Duo(82_cp,             term[MatPawnEg]);
  mat[WN] = Duo(term[MatKnightOp], term[MatKnightEg]);
  mat[WB] = Duo(term[MatBishopOp], term[MatBishopEg]);
  mat[WR] = Duo(term[MatRookOp],   term[MatRookEg]);
  mat[WQ] = Duo(term[MatQueenOp],  term[MatQueenEg]);
  mat[WK] = Duo::both(20000_cp); // no sense great number

  for (int i = 0; i < 12; i += 2)
    mat[i] = -mat[i + 1];

  /*for (int i = 0; i < 12; i++)
    cout << std::format("{}\n", mat[i]);*/

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
  pst[p][D3].op += 10_cp;
  pst[p][E3].op += 10_cp;

  pst[p][D4].op += 20_cp;
  pst[p][E4].op += 20_cp;

  pst[p][D5].op += 10_cp;
  pst[p][E5].op += 10_cp;

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
  for (SQ sq = A1; sq <= H1; ++sq)
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
        cout << std::format("{} ", pst[p][sq].op);
      }
      cout << "\n";
    }
    cout << "\n";
  }*/

  // Mobility //////////////////////////////////////////////////////

  for (int pt = 0; pt < 6; pt++)
    for (int i = 0; i < 30; i++)
      mob[pt][i] = 0_cp;

  // Mobility scale:
  // 
  // all pieces cost -mat[p]/3 when no moves
  // endgame scale starts from 1.5x this value
  // max bonus is 20-30, further have no sense
  // 
  // knight:  3.5 - zero,  9 total
  // bishop:  3.5 - zero, 14 total
  // rook:    7.5 - zero, 15 total
  // queen:   7   - zero, 28 total

  auto mob_func = [](int val, float zero, float steep) -> float
  {
    float x = pow(val / zero, steep / 2); // norming to exp(-1)..1
    return logf(expf(-1) + x * (1 - expf(-1))); // -1..0
  };

  auto mob_val = [&](int x, float x0, int mult, float k) -> Val
  {
    float result = mult * mob_func(x, x0, k);
    return static_cast<Val>(result);
  };

  for (int i = 0; i < 30; i++)
  {
    mob[Knight][i] = mob_val(i, 3.5, term[NMobMult], dry_float(term[NMobSteep]) / 10);
    mob[Bishop][i] = mob_val(i, 3.5, term[BMobMult], dry_float(term[BMobSteep]) / 10);
    mob[Rook][i]   = mob_val(i, 7.5, term[RMobMult], dry_float(term[RMobSteep]) / 10);
    mob[Queen][i]  = mob_val(i, 7.0, term[QMobMult], dry_float(term[QMobSteep]) / 10);

    //cout << std::format("{}\n", mob[Queen][i]);
  }

  // Passers ///////////////////////////////////////////////////////

  auto unzero = [](double x) { return x > 0 ? x : .001; };
  auto pscore = [](double m, double k, int rank)
  {
    auto nexp = [](double k, int x) { return 1 / (1 + exp(6 - k * x)); };
    return static_cast<int>(m * nexp(k, rank) / nexp(k, 6));
  };

  for (int rank = 0; rank < 8; rank++)
  {
    const double k = unzero(dry_double(term[PasserK])) / 32.0;
    passer_scale[rank] = pscore(256, k, rank);

    //cout << std::format("{}\n", passer_scale[rank]);
  }
}

void Eval::set_explanations(bool on)
{
#ifdef _DEBUG
  explain = on;
#endif
}

#undef APPLY
#undef A

// instantiations

template Duo Eval::evalxrays<Black>(const Board * B);
template Duo Eval::evalxrays<White>(const Board * B);

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

template Duo Eval::eval_passer<Black>(const Board * B, SQ sq);
template Duo Eval::eval_passer<White>(const Board * B, SQ sq);

template Duo Eval::eval_threats<Black>(const Board * B);
template Duo Eval::eval_threats<White>(const Board * B);
}
