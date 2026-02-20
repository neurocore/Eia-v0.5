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

Val Eval::eval(const Board * B, Val alpha, Val beta, bool use_phash)
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

  //if (!no_hash && use_phash)
  //{
  //  auto pk = Hash::pk_probe(B->state.pkhash);
  //  if (pk == nullptr)
  //  {
  //    pvals = evaluateP<White>(B) - evaluateP<Black>(B);

  //    /*if (pk != nullptr)
  //    {
  //      if (pvals != pk->vals
  //      ||  ei.eg_weak[0] != pk->weak[0]
  //      ||  ei.eg_weak[1] != pk->weak[1])
  //      {
  //        B->print();
  //        log("Expected: val {} weaks {} {}\n", pvals, ei.eg_weak[0], ei.eg_weak[1]);
  //        log("Hashed:   val {} weaks {} {}\n", pk->vals, pk->weak[0], pk->weak[1]);
  //        __debugbreak();
  //      }
  //    }*/
  //    Hash::pk_store(B->state.pkhash, pvals, ei.eg_weak);
  //  }
  //  else
  //  {
  //    pvals = pk->vals;
  //    ei.eg_weak[0] = pk->weak[0];
  //    ei.eg_weak[1] = pk->weak[1];
  //  }
  //}
  //else
  {
    pvals = evaluateP<White>(B) - evaluateP<Black>(B);
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

  vals += A(shield, p, bitscan(king), "Shield");

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
  // TODO: fix bug, probably from pk_hash table
  //if (king_att_weight[col] > 100) cout << king_att_weight[col] << endl;
  int weight = (std::min)(king_att_weight[col], 99);
  return king_att_count[col] > 2 ? safety_table[weight] : 0_cp;
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

Eval::Eval(const Tune & tune, bool no_pst, bool no_hash)
  : no_pst(no_pst), no_hash(no_hash)
{
  for (int i = 0; i < Term_N; i++)
    term[i] = 0_cp;

  if (!tune.empty())
  {
    set(tune);
  }
  else if (!g_tune.empty())
  {
    set(g_tune);
  }
  else
  {
    TERMS;
    init();
  }
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

  if (!no_pst)
  {
    // [ init PST ]
    // planning to move PST tables
    // outside of evaluation to
    // calculate it with material
    // incrementally while traversing
    // game tree

    // CMA-ES on Ethereal (100k) +131 elo (20s+.2 h2h-20)
    // Best loss: 0.117247
    Tune pst_tune = { 2.276142,1.987541,0.575674,-1.520422,2.261646,-0.767042,1.686472,0.092298,0.400089,-2.254314,-1.222925,-0.162682,-0.941329,-1.611756,1.895483,0.942418,-0.467375,0.118894,-0.454163,0.071383,-0.378453,0.126789,-0.119764,0.154966,-0.161931,0.053506,-0.122497,0.279335,-0.161747,-0.057034,-0.300196,-0.131031,-0.407761,-0.246031,-0.469998,-0.052897,-0.206996,0.089381,-0.087115,-0.104760,0.101693,0.209079,-0.196047,0.156782,-0.107604,-0.045948,-0.188583,-0.142453,-0.317673,0.045216,-0.384806,0.139391,-0.092431,0.025588,0.067875,0.189262,0.099033,0.016722,0.154574,0.021163,0.080216,-0.041402,0.029179,-0.017709,-0.406282,0.082983,-0.095247,0.216428,-0.023417,0.257651,0.239574,-0.302535,0.223939,-0.021408,0.427080,0.045519,0.057944,0.219172,0.050026,0.186908,-0.617348,0.668906,0.234935,0.382620,0.385934,0.116648,0.106165,0.073768,0.561472,0.074191,0.638543,0.327404,0.515379,-0.109466,-0.030564,0.528663,2.282944,0.308338,0.572991,0.338439,-0.001944,0.413416,2.521305,-0.332805,0.410312,-0.092833,0.402728,0.874404,-1.140682,0.919711,-1.032477,0.828867,1.441641,-0.861923,1.209819,0.708101,1.961112,0.344058,0.302037,-1.245265,0.645011,1.010895,0.305045,-0.860249,0.116837,-1.327762,-0.303397,1.557071,-0.836997,-0.601394,-0.184695,-1.140481,-0.819660,-0.791267,-0.270433,-0.294983,-0.156237,-0.158388,-0.321291,-0.333108,-0.124790,-1.108952,-1.711607,-1.599707,-1.127241,-0.611430,-0.350726,-0.563577,-0.044821,-0.700958,-0.064831,-0.306677,-0.085701,-0.183438,-0.313439,-0.407718,-0.472636,0.450735,-0.112099,-1.061633,-0.466695,-0.746709,-0.392773,-0.213419,-0.079990,-0.337717,0.012991,0.323744,0.144790,-0.115982,0.065806,-0.373750,0.004108,-0.282528,-0.361705,-0.851366,-0.225241,-0.645170,-0.516290,0.170561,0.118904,0.460516,0.210478,0.537386,0.179030,0.463390,-0.029120,0.385594,-0.230042,0.416211,-0.307121,0.071084,-0.064277,-0.255379,0.073353,-0.175410,0.028092,0.342882,0.257564,0.368470,0.201490,0.476622,0.060139,0.647007,0.068047,0.048301,-0.025007,-0.529324,-0.785891,-0.500846,-0.627034,0.202494,0.532977,0.093607,0.792598,-0.188998,1.072457,-0.226408,0.716316,0.191447,0.513042,-0.079683,0.349471,-0.799239,-0.321415,-0.564287,-0.631006,-0.110498,0.033968,-0.139768,-0.377040,0.491881,-0.121165,0.047563,-0.500765,-0.004458,-1.194315,-0.717901,-2.094707,0.387895,-1.321248,-0.979577,-0.022658,-1.161803,-0.222443,0.107711,1.316611,0.182078,-0.518617,0.363035,-0.790392,0.302859,-2.945059,0.186200,-2.101146,-1.019032,0.435262,-0.456579,0.238808,-0.449418,0.179055,0.277397,0.108396,-0.111464,-0.108517,0.184631,-0.006536,0.210820,-0.557180,0.297781,0.089283,0.473483,0.171476,0.465572,0.085114,-0.005786,0.047987,-0.150884,-0.021879,-0.162760,0.070992,0.078804,-0.115012,-0.178260,0.332776,0.119050,0.025260,-0.143729,0.018985,0.290551,0.322116,0.129150,-0.109206,0.169549,0.111818,0.070279,-0.017042,0.416345,0.086242,0.153271,-0.097922,0.072006,0.314425,-0.347625,0.019738,0.085782,-0.242456,0.402758,-0.096279,0.294097,-0.022488,0.364211,0.142906,0.141802,-0.132794,0.162762,-0.030786,0.087157,0.074306,0.049027,-0.044730,0.206855,-0.274130,0.513367,-0.255752,0.582213,0.083363,0.211257,0.070939,0.428553,0.223141,-0.123528,-0.134310,0.630708,-0.211238,0.417666,-0.034456,0.195804,0.138154,0.477503,0.677132,0.051608,0.236085,0.729170,0.587619,-0.241603,-0.150417,0.547090,0.491800,0.225757,0.335689,0.392211,0.683551,0.373246,0.130308,-0.095579,0.124601,0.325374,0.368716,0.014114,-0.217275,0.600057,0.582692,0.123700,-0.600195,0.165213,0.528931,0.048041,2.421867,-0.871710,-0.074891,0.165581,0.794695,0.153019,0.703716,-0.311834,-0.044303,0.005565,-0.706130,0.019960,0.028845,0.177792,-0.172561,-0.462228,-0.131970,0.224840,-0.187314,0.456566,-0.077230,0.358352,0.010129,0.148773,-0.091402,0.201539,-0.049943,0.390313,-0.155152,0.568653,0.074459,0.430990,-0.819037,0.426924,-0.121638,0.219414,-0.542320,0.543472,-0.497683,0.202970,-0.018902,0.144435,-0.044213,0.393444,0.117512,-0.063252,-0.054206,0.362107,-0.087068,-0.161372,0.085044,0.137255,-0.268890,0.277512,-0.359183,0.421877,-0.095308,0.047249,-0.236974,0.138178,-0.131648,0.239190,-0.172334,0.824596,-0.070342,0.325172,-0.393130,0.682023,-0.218960,0.342614,-0.193397,0.489638,-0.319351,0.606848,-0.129646,0.247874,0.597679,-0.060966,0.327255,0.200054,-0.048625,0.368143,-0.324125,0.489323,0.181163,0.376427,0.239658,0.489496,0.644057,0.540453,0.365558,0.350644,0.311267,0.208513,0.298865,0.342864,0.213350,0.449955,-0.069840,0.665766,0.099376,0.665136,-0.083103,0.485708,0.425574,0.429536,0.601178,0.586619,1.669445,-0.100091,0.018400,0.278159,0.041850,0.481006,0.093841,0.524778,0.021170,0.462412,0.169866,0.457367,0.349655,0.469186,1.057585,0.141099,0.549067,0.222768,0.625668,0.353752,-0.338966,0.710659,0.630648,0.401924,0.325094,0.610950,-0.395200,0.500967,0.133719,0.448352,0.587519,0.293430,-0.265112,0.922548,1.003540,0.513979,0.147708,-0.532642,0.115157,-0.411369,0.543313,-0.514811,0.623283,-0.769461,0.443369,-0.870552,0.648947,-0.890704,0.601042,-1.887790,0.265313,-0.660443,0.238797,-0.826848,0.453986,-0.332573,0.556560,-0.630779,0.501698,0.041601,0.520510,-0.486397,0.451097,-0.179046,0.714735,-0.453347,0.036396,-0.594768,0.070110,-0.180265,0.528639,-0.816806,0.544195,-0.335644,0.320180,-0.245921,0.346174,0.262024,0.313458,-0.047133,0.433024,-0.104597,0.194329,0.371046,0.032062,-0.377117,0.538009,-0.072974,0.483901,-0.640910,0.133632,0.466766,0.577955,0.024073,-0.101454,0.754400,0.734682,-0.825938,0.472670,-0.025625,0.527926,-0.898712,0.365001,-0.074371,-0.076774,0.762455,-0.114776,0.572464,-0.276213,1.417424,0.133278,0.446154,0.418907,-0.125009,0.082236,0.380809,-0.118000,0.951351,0.200315,-0.511247,-0.133907,-0.010440,-0.659944,0.973195,0.326049,0.363816,0.907908,0.315052,0.023193,0.052443,-0.095333,0.432227,0.343802,0.214418,-0.089798,0.348505,0.888393,-0.207843,-0.630608,1.214010,-0.973394,1.480980,0.537640,0.997048,-0.091702,2.320434,0.754859,-0.204601,0.405819,0.167712,0.322394,0.951012,2.688020,-1.225538,0.124782,0.400996,2.479693,-1.060630,1.146384,-0.111916,0.619358,2.395385,0.736614,-0.834767,-0.914450,-0.933551,0.007427,-0.248009,-0.163120,-0.162943,-0.899038,-0.240723,-0.463889,-0.653798,-0.996559,-0.071422,0.032606,-0.366413,0.127700,-0.910203,0.396729,-0.067767,-0.023764,-0.083606,-0.629252,0.165589,-0.835604,0.268355,-0.540108,0.096986,-1.073658,0.323300,0.142890,-0.150626,0.172099,-0.418903,0.551435,-0.202183,-0.593669,0.343344,-1.168612,0.421617,-0.980594,0.538626,0.304340,0.115792,-0.063678,0.202044,-0.202997,0.077110,0.361747,-0.436873,-0.728656,0.071872,1.371529,-0.081917,0.254101,0.155017,0.336780,0.407582,0.361938,0.396474,0.880314,0.254375,0.153630,0.224104,-1.052195,0.118866,-1.501864,1.021460,0.119800,0.317084,0.858165,0.389023,-0.783948,0.935497,-1.531801,0.679788,0.265741,0.386012,0.679704,0.209810,-0.298783,0.140229,1.502660,0.210144,0.903469,0.448721,-0.059346,0.934810,0.970152,0.606444,-0.937171,0.490760,-2.097616,0.840476,1.514010,0.656929,1.287129,-0.064554,0.037758,-0.577859,-2.520982,0.971397,-0.200100,0.117449,0.089544,-0.146082,-1.552950,0.166141,0.769878,-0.191948,-1.368433,1.306685,-1.855020,-0.053611,0.999777,-2.698295,-1.138166,-0.121467,1.454622,-2.447875,1.505612,-0.549151,-2.431827,-1.792676,2.307231,0.018997,0.177337,0.999453,-1.903139,-1.571920 };

    for (PieceType pt = Pawn; pt < PieceType_N; ++pt)
    {
      for (SQ sq = A1; sq < SQ_N; ++sq)
      {
        const Piece p = to_piece(pt, White);
        const int index = pt * 64 + sq;

        pst[p][sq].op = 100_cp * pst_tune[2 * index];
        pst[p][sq].eg = 100_cp * pst_tune[2 * index + 1];

        pst[opp(p)][opp(sq)].op = -100_cp * pst_tune[2 * index];
        pst[opp(p)][opp(sq)].eg = -100_cp * pst_tune[2 * index + 1];
      }
    }
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
