#include <format>
#include <iomanip>
#include "eval.h"
#include "board.h"
#include "value.h"
#include "hash.h"

using namespace std;

namespace eia {

Eval E[1];

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

//    Engine        Score                    Ei                   St                   Mo                   Ei    S-B
// 1: Eia-v0.5_new  39,5/60 ···················· 0101001=011000101111 001111010=1011011111 1110111010111=111110  1005,2
// 2: Stellar-1.4.3 39,5/60 1010110=100111010000 ···················· 0=010011=111011=11=1 111111111011=110=111  982,25
// 3: Monarch(v1.7) 22,5/60 110000101=0100100000 1=101100=000100=00=0 ···················· 00=111=111=00=010000  699,75
// 4: Eia_v0_3      18,5/60 0001000101000=000001 000000000100=001=000 11=000=000=11=101111 ····················  543,75
// 
// 120 games played / Tournament is finished
// Name of the tournament: 260
// Level: Blitz 0:20/0,2


// All tests were done with control 10s+.1s (h2h-30)
// 
// + xrays & pins (with king)  +92 elo
// + nonlinear mobility       +100 elo
// + connected pawns           +30 elo
// + threats                  +150 elo
// + rebalanced material       +50 elo
// + trapped pieces            +10 elo
// + bad bishop                +15 elo
// + relative pin on queen     +10 elo
// + knight & bishop outpost   +40 elo
// 
// - pawn storm                ??? elo
// - wider passers             ??? elo

Val Eval::eval(const Board * B, Val alpha, Val beta, bool use_phash)
{
  ei.init(B);
  Duo duo{};

#ifdef TUNING
  T.clear();
#else
  // +70 elo (20s+.2s h2h-30)
  if (B->is_simply_mated()) return -Val::Inf;
#endif

  int scale = 128;

  if (is_correct(B->mkey))
  {
    const auto matinfo = mattable[get_index(B->mkey)];
    scale = matinfo.scale;
  }

  for (Piece p = BP; p < BK; ++p) // material
  {
    const int cnt = popcnt(B->piece[p]);
    duo += apply(col(p) ? cnt : -cnt, MatValue, pt(p));
  }

  // collecting ei here
  duo += evalxrays<White>(B) - evalxrays<Black>(B);

  // Pawn-king hash table | +27.85 elo (5+.05 h2h-100)

  Duo pvals;

  if (!no_hash && use_phash && !TRACE)
  {
    auto pk = Hash::pk_probe(B->state.pkhash);
    if (pk == nullptr)
    {
      pvals = evaluateP<White>(B) - evaluateP<Black>(B);
      Hash::pk_store(B->state.pkhash, pvals, ei.weak, ei.passers);
    }
    else
    {
      pvals = pk->vals;
      ei.weak[0] = pk->weak & B->occ[0];
      ei.weak[1] = pk->weak & B->occ[1];
      ei.passers = pk->passers;
    }
  }
  else
  {
    pvals = evaluateP<White>(B) - evaluateP<Black>(B);
  }

  duo += pvals;
  duo += evaluateN<White>(B) - evaluateN<Black>(B);
  duo += evaluateB<White>(B) - evaluateB<Black>(B);
  duo += evaluateR<White>(B) - evaluateR<Black>(B);
  duo += evaluateQ<White>(B) - evaluateQ<Black>(B);
  duo += evaluateK<White>(B) - evaluateK<Black>(B);

  duo += eval_passers<White>(B) - eval_passers<Black>(B);
  duo += eval_threats<White>(B) - eval_threats<Black>(B);
  duo += eval_tempo<White>(B)   - eval_tempo<Black>(B);

  Duo ksafety = Duo::both(ei.king_safety());
  duo += ksafety;

  const int phase = B->phase();
  Val score = duo.tapered(phase, scale);
  Val val = B->color ? score : -score;

  int progress = 100 - std::max(B->state.fifty, 68);

#ifdef TUNING
  const float fadeout = progress / 32.f;
  T.rho = fadeout * (Phase::Total - phase) / Phase::Total;
  T.phi = fadeout * scale / 128 * phase / Phase::Total;
#endif

  return unzero(val / 32 * progress);
}

Val Eval::mopup(const Board * B, Color weaker)
{
  SQ king0 = bitscan(B->piece[BK ^ weaker]);
  SQ king1 = bitscan(B->piece[WK ^ weaker]);
  int cmd = center_manh(king0);
  int md = k_dist(king0, king1);
  double mopup = 4.7 * cmd + 1.6 * (14 - md);
  return cp(mopup);
}

template<Color Col>
Duo Eval::evalxrays(const Board * B)
{
  Duo vals{};
  const u64 o = B->occupied();
  const SQ king = bitscan(B->piece[BK ^ Col]);

  for (u64 bb = B->discovered<Col>(king); bb; bb = rlsb(bb))
  {
    const SQ  asq = bitscan(bb);
    const u64 ray = between[king][asq];
    const SQ   sq = bitscan(ray & o);
    const Piece a = B->square[asq];
    const Piece p = B->square[sq];

    if (col(p) == Col && pt(p) > Pawn) // Pinned piece
    {
      // 1. Absolute or partial pin
      
      const bool absolute = !(atts[p][sq] & ray);
      const Term term = absolute ? PinAbsolute : PinPartial;

      if (absolute || see_value[a] < see_value[p])
      {
        // 2. Is attacked by pawn?

        const bool pawn_att = !!(ei.pawn_atts[~Col] & bit(sq));

        vals += apply<Col>(term, 4 * pawn_att + pt(p) - 1);
      }
    }
  }
  return vals;
}

template<Color Col>
Duo Eval::evaluateP(const Board * B)
{
  constexpr Piece p   = to_piece(Pawn,  Col);
  constexpr Piece opp = to_piece(Pawn, ~Col);

  // INFO: Pawn attacks moved to ei.init()

  Duo vals{};
  for (u64 bb = B->piece[p]; bb; bb = rlsb(bb))
  {
    const SQ sq = bitscan(bb);
    const int f = file(sq);
    const int r = Col ? rank(sq) : 7 - rank(sq);

    // pst
    vals += pst[p][sq];

    u64 back_friendly = front[~Col][sq] & B->piece[p];
    u64 fore_friendly = front[Col][sq] & B->piece[p];
    u64 connected  = psupport[Col][sq] & B->piece[p];

    u64 blocked = front_one[Col][sq] & B->piece[opp];
    u64 cannot_pass = front[Col][sq] & B->piece[opp];
    u64 supports = att_rear[Col][sq] & B->piece[p];
    u64 sentries = att_span[Col][sq] & B->piece[opp];

    // isolated

    if (!(isolator[sq] & B->piece[p]))
    {
      vals += apply<Col>(Isolated, f);
    }

    // doubled

    if (back_friendly && !fore_friendly) // most advanced one
    {
      vals += apply<Col>(popcnt(back_friendly), Doubled, f);
    }

    // blocked weak

    if (blocked)
    {
      ei.rammed[Col] |= bit(sq);
      if (!supports) ei.weak[Col] |= bit(sq);
    }

    // backward

    if (!supports && sentries)
    {
      if (r < 5)
      {
        vals += apply<Col>(Backward, r - 1);
      }

      ei.weak[Col] |= bit(sq);
    }

    else

    // connected

    if (connected)
    {
      vals += apply<Col>(Connected);
    }

    // passers

    if (!cannot_pass && !fore_friendly)
    {
      if (!sentries)
      {
        ei.passers |= Bit << sq;
      }
      else if (popcnt(supports) >= popcnt(sentries)) // candidate
      {
        vals += apply<Col>(Candidate, r - 1);
      }
    }
  }

  // pawn shield

  const SQ king = ei.king[Col];
  const int kf = file(king);

  for (int f = std::max(0, kf - 1); f <= std::min(7, kf + 1); f++)
  {
    u64 not_back = fwd[~Col][king] ^ Full;
    u64 pawns    = B->piece[p] & file_bb[f] & not_back;
    SQ  sq       = pawns ? backmost<Col>(pawns) : SQ_N;
    int dist     = pawns ? abs(rank(king) - rank(sq)) : 7;

    vals += apply<Col>(PawnShield, (f == kf) * 8 + dist);
  }

  return vals;
}

template<Color Col>
Duo Eval::eval_passers(const Board * B)
{
  constexpr int forward = Col ? 8 : -8;
  constexpr Piece p = to_piece(Pawn, Col);
  const SQ king = ei.king[Col];
  const SQ kopp = ei.king[~Col];
  Duo vals = 0_cp;

  for (u64 bb = ei.passers & B->occ[Col]; bb; bb = rlsb(bb))
  {
    SQ sq = bitscan(bb);

    const u64 sentries = att_span[Col][sq] & B->piece[opp(p)];
    const int prank = Col ? rank(sq) : 7 - rank(sq);

    if (!sentries) // Passer
    {
      vals += apply<Col>(Passer, prank - 1);

      if (!(front[Col][sq] & B->occ[Col]) // Unstoppable
      &&  !B->has_pieces(~Col))
      {
        SQ prom = to_sq(file(sq), Col ? 7 : 0);
        int turn = static_cast<int>(B->color != Col);

        // opp king is not in square
        if (k_dist(kopp, prom) - turn > k_dist(sq, prom))
        {
          vals += apply<Col>(Unstoppable);
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
          vals += apply<Col>(Unstoppable);
        }
      }
      else // Bonuses for increasing passers potential
      {
        if (psupport[Col][sq] & B->piece[p]) // Supported
        {
          vals += apply<Col>(Supported, prank - 1);
        }

        const u64 o = B->occupied();
        if (prank > 4 && !(front_one[Col][sq] & o)) // Free passer
        {
          SQ stop = Col ? sq + 8 : sq - 8;
          Move move = to_move(sq, stop);
          if (B->see(move) > 0)
          {
            vals += apply<Col>(FreePasser);
          }
        }

        // Kings attack/defence of stop square

        SQ stop = Col ? sq + 8 : sq - 8;
        int katt = k_dist(kopp, stop);
        int kdef = k_dist(king, stop);

        vals += apply<Col>(PasserKingAtt, katt);
        vals += apply<Col>(PasserKingDef, kdef);
      }
    }
  }
  return vals;
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

    vals += pst[p][sq];

    const u64 opp_pawns = ei.pawn_atts[~Col];
    const u64 safe_att = att & ~opp_pawns;
    vals += apply<Col>(MobN, popcnt(safe_att));

    // adjustments

    int pawns = popcnt(B->piece[BP ^ Col]);
    vals += apply<Col>(PAdj[pawns], KnightAdj);

    // trapped

    const u64 mask = Col ? bits(H8, A8, H7, A7)
                         : bits(H1, A1, H2, A2);

    if (bit(sq) & mask && !safe_att)
    {
      vals += apply<Col>(TrappedHard);
    }

    // outpost

    if (bit(sq) & outpost[Col]
    && !(front_span[Col][sq] & opp_pawns))
    {
      const bool central  = bit(sq) & InnerFiles;
      const bool defended = bit(sq) & ei.pawn_atts[Col];

      vals += apply<Col>(KnightOutpost, 2 * central + defended);
    }

    // forks

    if (several(att & B->valuable<Col>()))
    {
      vals += apply<Col>(KnightFork);
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

    vals += pst[p][sq];
    vals += apply<Col>(MobB, popcnt(att));

    // bad bishop

    u64 pawn_mob = B->attack<Bishop>(sq, B->piece[BP ^ Col]);
    int cnt = popcnt(pawn_mob & fwd[Col][sq]);

    if (cnt < 5)
    {
      vals += apply<Col>(BadBishop);
    }
    else if (cnt < 12)
    {
      vals += apply<Col>(1, 2, BadBishop); // relaxed
    }

    // trapped

    const u64 opp_pawns = B->piece[WP ^ Col];

    constexpr u64 bishops[] =
    {
      Col ? bit(H7) : bit(H2),
      Col ? bit(A7) : bit(A2)
    };

    constexpr u64 blockers[] =
    {
      Col ? bits(F7, G6) : bits(F2, G3),
      Col ? bits(C7, B6) : bits(C2, B3)
    };

    for (int i = 0; i < 2; i++)
    {
      if (bit(sq) & bishops[i]
      &&  contains(opp_pawns, blockers[i]))
      {
        vals += apply<Col>(TrappedSoft);
        break;
      }
    }

    // outpost

    if (bit(sq) & outpost[Col]
    && !(front_span[Col][sq] & opp_pawns))
    {
      const bool central  = bit(sq) & InnerFiles;
      const bool defended = bit(sq) & ei.pawn_atts[Col];

      vals += apply<Col>(BishopOutpost, 2 * central + defended);
    }

    // forks

    if (several(att & B->valuable<Col>()))
    {
      vals += apply<Col>(BishopFork);
    }
  }

  // bishop pair

  if (several(B->piece[p]))
  {
    vals += apply<Col>(BishopPair);
  }
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

    vals += pst[p][sq];
    vals += apply<Col>(MobR, popcnt(att));

    // adjustments

    int pawns = popcnt(B->piece[BP ^ Col]);
    vals += apply<Col>(PAdj[pawns], RookAdj);

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
        vals += apply<Col>(Rook7th);
      }
    }

    // rook on open/semi-files

    if (!(front[Col][sq] & own_pawns))
    {
      const bool semi = front[Col][sq] & opp_pawns;
      vals += apply<Col>(semi ? RookSemi : RookOpen);
    }

    // bad rook

    constexpr u64 rooks[] =
    {
      Col ? bits(H1, H2, G1, G2) : bits(H8, H7, G8, G7),
      Col ? bits(A1, A2, B1, B2) : bits(A8, A7, B8, B7)
    };

    constexpr u64 kings[] =
    {
      Col ? bits(F1, G1)     : bits(F8, G8),
      Col ? bits(D1, C1, B1) : bits(D8, C8, B8)
    };

    const SQ king = ei.king[Col];

    for (int i = 0; i < 2; i++)
    {
      if (bit(sq)   & rooks[i]
      &&  bit(king) & kings[i])
      {
        vals += apply<Col>(BadRook);
        break;
      }
    }
  }
  return vals;
}

template<Color Col>
Duo Eval::evaluateQ(const Board * B)
{
  const u64 o = B->occupied();
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

    // relative pins

    for (u64 att = B->discovered<Col>(sq); att; att = rlsb(att))
    {
      const SQ  asq = bitscan(att);
      const SQ  psq = bitscan(between[sq][asq] & o);
      const Piece p = B->square[psq];

      if (col(p) == Col && pt(p) > Pawn) // Pinned piece
      {
        const bool pawn_att = !!(ei.pawn_atts[~Col] & bit(psq));

        vals += apply<Col>(PinRelative, 4 * pawn_att + pt(p) - 1);
      }
    }

    // pst & mobility

    vals += pst[p][sq];
    vals += apply<Col>(MobQ, popcnt(att));

    // queen on open/semi-files

    const u64 own_pawns = B->piece[to_piece(Pawn, Col)];
    const u64 opp_pawns = B->piece[to_piece(Pawn, ~Col)];

    if (!(front[Col][sq] & own_pawns))
    {
      const bool semi = front[Col][sq] & opp_pawns;
      vals += apply<Col>(1, 2,  semi ? RookSemi : RookOpen);
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
    vals += apply<Col>(penalty, 4, EarlyQueen);
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

    vals += pst[p][sq];

    // pawn weakness

    for (u64 ww = ei.weak[~Col]; ww; ww = rlsb(ww))
    {
      SQ j = bitscan(ww);
      vals += apply<Col>(WeaknessPush, k_dist(sq, j));
    }
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
  vals += apply<Col>(cnt, ThreatPawn);

  // lights <- pawns
  cnt = popcnt(lights & pawns_atts);
  vals += apply<Col>(cnt, ThreatL_P);

  // lights <- lights
  cnt = popcnt(lights & light_atts);
  vals += apply<Col>(cnt, ThreatL_L);

  // weak lights <- heavy
  cnt = popcnt(weak_light & heavy_atts);
  vals += apply<Col>(cnt, ThreatL_H);

  // weak lights <- king
  cnt = popcnt(weak_light & ei.attacked_by[opp][King]);
  vals += apply<Col>(cnt, ThreatL_K);

  // rooks <- pawns, lights
  cnt = popcnt(rooks & (pawns_atts | light_atts));
  vals += apply<Col>(cnt, ThreatR_L);

  // weak rooks <- king
  cnt = popcnt(rooks & poor_defend & ei.attacked_by[opp][King]);
  vals += apply<Col>(cnt, ThreatR_K);

  // queens <- any
  cnt = popcnt(queens & ei.attacked[opp]);
  vals += apply<Col>(cnt, ThreatQ_1);

  return vals;
}

template<Color Col>
Duo Eval::eval_tempo(const Board * B)
{
  //Duo v = apply<Col>(B->color == Col, Tempo);
  //return v;
  return {};
}

//////////////////

void EvalInfo::init(const Board * B)
{
  king[0] = bitscan(B->piece[BK]);
  king[1] = bitscan(B->piece[WK]);
  king_att_weight[0] = king_att_weight[1] = 0;
  king_att_count[0] = king_att_count[1] = 0;
  weak[0] = weak[1] = 0ull;
  rammed[0] = rammed[1] = 0ull;

  occ_not_rq[0] = B->occupied() ^ B->ortho<Black>();
  occ_not_rq[1] = B->occupied() ^ B->ortho<White>();

  occ_not_bq[0] = B->occupied() ^ B->diags<Black>();
  occ_not_bq[1] = B->occupied() ^ B->diags<White>();

  const u64 bpawns = B->piece[BP];
  const u64 wpawns = B->piece[WP];

  //rammed[0] = shift_u(wpawns) & bpawns;
  //rammed[1] = shift_d(bpawns) & wpawns;

  pawn_atts[0]  = shift_dl(bpawns) | shift_dr(bpawns);
  pawn_atts[1]  = shift_ul(wpawns) | shift_ur(wpawns);

  pawn_atts2[0] = shift_dl(bpawns) & shift_dr(bpawns);
  pawn_atts2[1] = shift_ul(wpawns) & shift_ur(wpawns);

  attacked[0] = attacked_by[0][King] = atts[BK][king[0]];
  attacked[1] = attacked_by[1][King] = atts[WK][king[1]];

  attacked_by2[0]       = pawn_atts[0] & attacked[0];
  attacked[0]          |= pawn_atts[0];
  attacked_by[0][Pawn]  = pawn_atts[0];

  attacked_by2[1]       = pawn_atts[1] & attacked[1];
  attacked[1]          |= pawn_atts[1];
  attacked_by[1][Pawn]  = pawn_atts[1];

  passers = 0ull;
}

void EvalInfo::add_king_attack(Color col, AttWeight weight, u64 att)
{
  const int count = popcnt(att & kingzone[~col][king[~col]]);
  king_att_weight[col] += count * static_cast<int>(weight);
  king_att_count[col] += count;
}

Val EvalInfo::king_safety(Color col) const
{
  int weight = (std::min)(king_att_weight[col], 99);
  return king_att_count[col] > 2 ? safety_table[weight] : 0_cp;
}

Val EvalInfo::king_safety() const
{
  return king_safety(White) - king_safety(Black);
}

void EvalInfo::add_attack(Color col, PieceType pt, u64 att)
{
  attacked_by2[col]    |= att & attacked[col];
  attacked[col]        |= att;
  attacked_by[col][pt] |= att;
}

//////////////////

#undef TERM
#define TERM(group,x,op,eg)       \
  data[idx] = {op##_cp, eg##_cp}; \
  info[x]   = {group, idx++, 1};

#undef TERM_ARR
#define TERM_ARR(group,x,sz)  \
  info[x] = {group, idx, sz}; \
  idx += sz;

Eval::Eval(const Tune & tune, bool no_pst, bool no_hash)
  : no_pst(no_pst), no_hash(no_hash)
{
  for (int i = 0; i < Param_N; i++)
    data[i].clear();

  int idx = 0;
  TERMS; // can't skip - it builds info!

  if (!tune.empty())
  {
    set(tune);
  }
  else if (!l_tune.empty())
  {
    set(l_tune);
  }
  else if (!g_tune.empty())
  {
    set(g_tune);
  }
  else
  {
    init_term_arrays();
    init_inner();
  }
}

#undef TERM
#define TERM(group,x,op,eg)            \
str += format("{}:{} ", #x, get(x));

#undef TERM_ARR
#define TERM_ARR(group,x,sz)           \
str += format("{}: ", #x);             \
for (int i = 0; i < info[x].size; i++) \
  str += format("{} ", get(x, i));


string Eval::to_string() const
{
  string str;
  TERMS;
  return str;
}

Bounds Eval::bounds() const
{
  Bounds result;
  for (int i = 0; i < Param_N; i++)
  {
    result.push_back({0, 0});
  }
  return result;
}

//#undef TERM
//#define TERM(group,x,op,eg) {                            \
//  size_t found = str.find(#x, 0);                        \
//  if (found != string::npos)                             \
//  {                                                      \
//    size_t a = str.find(":", found);                     \
//    size_t b = str.find(" ", found);                     \
//    auto v = parse_double(str.substr(a + 1, b - a - 1)); \
//    term[x] = cp(v);                                     \
//  }}
//
void Eval::set(string str)
{
  //TERMS;
  init_inner();
}
//
//#undef TERM
//#undef TERM_ARR

string Eval::to_raw() const
{
  string str;
  for (int i = 0; i < Param_N; i++)
    str += format("{} ", data[i]);
  return str;
}

void Eval::set_raw(string str, std::string delim)
{
  for (int i = 0; i < Param_N; i++)
  {
    string part1 = cut(str, delim);
    string part2 = cut(str, delim);
    double op = parse_double(part1);
    double eg = parse_double(part2);
    data[i] = { cp(op), cp(eg) };
  }
  init_inner();
}

void Eval::set(const Eval & eval)
{
  for (int i = 0; i < Param_N; i++)
    data[i] = eval.data[i];
  init_inner();
}

void Eval::set(const Tune & tune)
{
  for (int i = 0; i < Param_N; i++)
    data[i] = { cp(tune.param[i][0]), cp(tune.param[i][1]) };
  init_inner();
}

Tune Eval::to_tune() const
{
  Tune tune;
  for (int i = 0; i < Param_N; i++)
  {
    tune.param[i][0] = dry_double(data[i].op);
    tune.param[i][1] = dry_double(data[i].eg);
  }
  return tune;
}

void Eval::init_term(int term, const vector<pair<f64, f64>> & arr)
{
  assert(arr.size() == info[term].size);

  int idx = info[term].index;
  for (const auto & val : arr)
  {
    data[idx++] = Duo(cp(val.first), cp(val.second));
  }
}

void Eval::init_term_arrays()
{
  // ------------------------------
  //  Material
  // ------------------------------

  init_term(MatValue,
  {
    { 82.0000,  134.2492}, // pawn
    {311.1222,  602.1030}, // knight
    {273.4692,  571.2714}, // bishop
    {412.8690,  936.1210}, // rook
    {817.5440, 1848.0416}  // queen
  });

  // ------------------------------
  //  Pawn structure
  // ------------------------------

  init_term(Doubled, // by file
  {
    { 3, -14}, { 0, -15}, {-6,  -9}, {-7, -10},
    {-4,  -9}, {-2, -10}, { 0, -13}, { 0, -17}
  });

  init_term(Isolated, // by file
  {
    {-13, -12}, {-1, -16}, { 1, -16}, { 3, -18},
    {  7, -19}, { 3, -15}, {-4, -14}, {-4, -17},
  });

  init_term(Backward, // by (rank - 1)
  {
    {-9, -32}, {-5, -30}, {3, -31}, {29, -41}
  });

  init_term(PawnShield, // by [kingfile][rankdist2king]
  {
    {-5,  0}, {0, 0}, {-10, 0}, {-20, 0},
    {-40, 0}, {-60, 0}, {-90, 0}, {-120, 0},
    {-10, 0}, {0, 0}, {-14, 0}, {-30, 0},
    {-60, 0}, {-80, 0}, {-110, 0}, {-140, 0},
  });
  
  init_term(WeaknessPush, // by k_dist
  {
    {0, 12}, {0, 10}, {0, 8}, {0, 6},
    {0, 5},  {0, 3},  {0, 2}, {0, 1},
  });

  // ------------------------------
  //  Mobility
  // ------------------------------

  init_term(MobN,
  {
    {-52, -52}, {-6, -6}, {-3, -3}, {0, 0},
    {0, 0}, {2, 2}, {3, 3}, {4, 4}, {4, 4}
  });

  init_term(MobB,
  {
    {-168, -168}, {-45, -45}, {-21, -21}, {-6, -6},
    {5, 5}, {14, 14}, {22, 22}, {29, 29}, {34, 34},
    {40, 40}, {44, 44}, {49, 49}, {53, 53}, {57, 57}
  });

  init_term(MobR,
  {
    {-58, -58}, {-40, -40}, {-30, -30}, {-22, -22},
    {-16, -16}, {-10, -10}, {-6, -6}, {-1, -1}, {1, 1},
    {5, 5}, {8, 8}, {11, 11}, {14, 14}, {16, 16}, {19, 19}
  });

  init_term(MobQ,
  {
    {-96, -96}, {-48, -48}, {-33, -33}, {-23, -23},
    {-16, -16}, {-9, -9}, {-4, -4}, {0, 0}, {4, 4},
    {7, 7}, {11, 11}, {14, 14}, {17, 17}, {19, 19},
    {22, 22}, {24, 24}, {27, 27}, {29, 29}, {31, 31},
    {33, 33}, {35, 35}, {36, 36}, {38, 38}, {40, 40},
    {41, 41}, {43, 43}, {44, 44}, {46, 46}
  });

  // ------------------------------
  //  Outposts
  // ------------------------------

  init_term(KnightOutpost, // [central][defended]
  {
    {12, -32}, {40, 0}, {7, -24}, {21, -3}
  });

  init_term(BishopOutpost, // [central][defended]
  {
    {16, -16}, {50, -3}, {9, -9}, {-4, -4}
  });

  // ------------------------------
  //  Passers
  // ------------------------------

  init_term(Candidate, // from 2 rank
  {
    {-12, 21}, {-7, 27}, {2, 53}, {22, 116}, {49, 78}
  });

  init_term(Passer, // from 2 rank
  {
    {-28, 23}, {-40, 35}, {-55, 60}, {8, 89}, {95, 166}, {124, 293}
  });

  init_term(Supported, // from 2 rank
  {
    {-4, 7}, {-2, 9}, {0, 17}, {7, 39}, {16, 26}, {20, 34}
  });

  init_term(PasserKingAtt, // by k_dist
  {
    {5, -1}, {5, -1}, {7, 0}, {9, 11}, {0, 25}, {1, 37}, {16, 37}, {0, 0}
  });

  init_term(PasserKingDef, // by k_dist
  {
    {-3, 1}, {-3, 1}, {0, -4}, {5, -13}, {6, -19}, {-9, -19}, {-9, -7}, {0, 0}
  });

  // ------------------------------
  //  Xrays
  // ------------------------------

  init_term(PinAbsolute,
  {
    {-32, -35}, {-34, -38}, {-50,   -60}, {-110, -130}, // safe
    {-64, -70}, {-70, -76}, {-100, -120}, {-220, -250}, // pawn att
  });

  init_term(PinPartial,
  {
    {-16, -17}, {-17, -18}, {-25, -30}, {-55, -65}, // safe
    {-20, -21}, {-21, -22}, {-30, -35}, {-62, -72}, // pawn att
  });

  init_term(PinRelative,
  {
    {-12, -10}, {-13, -10}, {-20, -20}, {-5, -5}, // safe
    {-24, -20}, {-26, -20}, {-40, -40}, {-9, -9}, // pawn att
  });
}

void Eval::init_inner()
{
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
    vector<f64> pst_tune = { 2.276142,1.987541,0.575674,-1.520422,2.261646,-0.767042,1.686472,0.092298,0.400089,-2.254314,-1.222925,-0.162682,-0.941329,-1.611756,1.895483,0.942418,-0.467375,0.118894,-0.454163,0.071383,-0.378453,0.126789,-0.119764,0.154966,-0.161931,0.053506,-0.122497,0.279335,-0.161747,-0.057034,-0.300196,-0.131031,-0.407761,-0.246031,-0.469998,-0.052897,-0.206996,0.089381,-0.087115,-0.104760,0.101693,0.209079,-0.196047,0.156782,-0.107604,-0.045948,-0.188583,-0.142453,-0.317673,0.045216,-0.384806,0.139391,-0.092431,0.025588,0.067875,0.189262,0.099033,0.016722,0.154574,0.021163,0.080216,-0.041402,0.029179,-0.017709,-0.406282,0.082983,-0.095247,0.216428,-0.023417,0.257651,0.239574,-0.302535,0.223939,-0.021408,0.427080,0.045519,0.057944,0.219172,0.050026,0.186908,-0.617348,0.668906,0.234935,0.382620,0.385934,0.116648,0.106165,0.073768,0.561472,0.074191,0.638543,0.327404,0.515379,-0.109466,-0.030564,0.528663,2.282944,0.308338,0.572991,0.338439,-0.001944,0.413416,2.521305,-0.332805,0.410312,-0.092833,0.402728,0.874404,-1.140682,0.919711,-1.032477,0.828867,1.441641,-0.861923,1.209819,0.708101,1.961112,0.344058,0.302037,-1.245265,0.645011,1.010895,0.305045,-0.860249,0.116837,-1.327762,-0.303397,1.557071,-0.836997,-0.601394,-0.184695,-1.140481,-0.819660,-0.791267,-0.270433,-0.294983,-0.156237,-0.158388,-0.321291,-0.333108,-0.124790,-1.108952,-1.711607,-1.599707,-1.127241,-0.611430,-0.350726,-0.563577,-0.044821,-0.700958,-0.064831,-0.306677,-0.085701,-0.183438,-0.313439,-0.407718,-0.472636,0.450735,-0.112099,-1.061633,-0.466695,-0.746709,-0.392773,-0.213419,-0.079990,-0.337717,0.012991,0.323744,0.144790,-0.115982,0.065806,-0.373750,0.004108,-0.282528,-0.361705,-0.851366,-0.225241,-0.645170,-0.516290,0.170561,0.118904,0.460516,0.210478,0.537386,0.179030,0.463390,-0.029120,0.385594,-0.230042,0.416211,-0.307121,0.071084,-0.064277,-0.255379,0.073353,-0.175410,0.028092,0.342882,0.257564,0.368470,0.201490,0.476622,0.060139,0.647007,0.068047,0.048301,-0.025007,-0.529324,-0.785891,-0.500846,-0.627034,0.202494,0.532977,0.093607,0.792598,-0.188998,1.072457,-0.226408,0.716316,0.191447,0.513042,-0.079683,0.349471,-0.799239,-0.321415,-0.564287,-0.631006,-0.110498,0.033968,-0.139768,-0.377040,0.491881,-0.121165,0.047563,-0.500765,-0.004458,-1.194315,-0.717901,-2.094707,0.387895,-1.321248,-0.979577,-0.022658,-1.161803,-0.222443,0.107711,1.316611,0.182078,-0.518617,0.363035,-0.790392,0.302859,-2.945059,0.186200,-2.101146,-1.019032,0.435262,-0.456579,0.238808,-0.449418,0.179055,0.277397,0.108396,-0.111464,-0.108517,0.184631,-0.006536,0.210820,-0.557180,0.297781,0.089283,0.473483,0.171476,0.465572,0.085114,-0.005786,0.047987,-0.150884,-0.021879,-0.162760,0.070992,0.078804,-0.115012,-0.178260,0.332776,0.119050,0.025260,-0.143729,0.018985,0.290551,0.322116,0.129150,-0.109206,0.169549,0.111818,0.070279,-0.017042,0.416345,0.086242,0.153271,-0.097922,0.072006,0.314425,-0.347625,0.019738,0.085782,-0.242456,0.402758,-0.096279,0.294097,-0.022488,0.364211,0.142906,0.141802,-0.132794,0.162762,-0.030786,0.087157,0.074306,0.049027,-0.044730,0.206855,-0.274130,0.513367,-0.255752,0.582213,0.083363,0.211257,0.070939,0.428553,0.223141,-0.123528,-0.134310,0.630708,-0.211238,0.417666,-0.034456,0.195804,0.138154,0.477503,0.677132,0.051608,0.236085,0.729170,0.587619,-0.241603,-0.150417,0.547090,0.491800,0.225757,0.335689,0.392211,0.683551,0.373246,0.130308,-0.095579,0.124601,0.325374,0.368716,0.014114,-0.217275,0.600057,0.582692,0.123700,-0.600195,0.165213,0.528931,0.048041,2.421867,-0.871710,-0.074891,0.165581,0.794695,0.153019,0.703716,-0.311834,-0.044303,0.005565,-0.706130,0.019960,0.028845,0.177792,-0.172561,-0.462228,-0.131970,0.224840,-0.187314,0.456566,-0.077230,0.358352,0.010129,0.148773,-0.091402,0.201539,-0.049943,0.390313,-0.155152,0.568653,0.074459,0.430990,-0.819037,0.426924,-0.121638,0.219414,-0.542320,0.543472,-0.497683,0.202970,-0.018902,0.144435,-0.044213,0.393444,0.117512,-0.063252,-0.054206,0.362107,-0.087068,-0.161372,0.085044,0.137255,-0.268890,0.277512,-0.359183,0.421877,-0.095308,0.047249,-0.236974,0.138178,-0.131648,0.239190,-0.172334,0.824596,-0.070342,0.325172,-0.393130,0.682023,-0.218960,0.342614,-0.193397,0.489638,-0.319351,0.606848,-0.129646,0.247874,0.597679,-0.060966,0.327255,0.200054,-0.048625,0.368143,-0.324125,0.489323,0.181163,0.376427,0.239658,0.489496,0.644057,0.540453,0.365558,0.350644,0.311267,0.208513,0.298865,0.342864,0.213350,0.449955,-0.069840,0.665766,0.099376,0.665136,-0.083103,0.485708,0.425574,0.429536,0.601178,0.586619,1.669445,-0.100091,0.018400,0.278159,0.041850,0.481006,0.093841,0.524778,0.021170,0.462412,0.169866,0.457367,0.349655,0.469186,1.057585,0.141099,0.549067,0.222768,0.625668,0.353752,-0.338966,0.710659,0.630648,0.401924,0.325094,0.610950,-0.395200,0.500967,0.133719,0.448352,0.587519,0.293430,-0.265112,0.922548,1.003540,0.513979,0.147708,-0.532642,0.115157,-0.411369,0.543313,-0.514811,0.623283,-0.769461,0.443369,-0.870552,0.648947,-0.890704,0.601042,-1.887790,0.265313,-0.660443,0.238797,-0.826848,0.453986,-0.332573,0.556560,-0.630779,0.501698,0.041601,0.520510,-0.486397,0.451097,-0.179046,0.714735,-0.453347,0.036396,-0.594768,0.070110,-0.180265,0.528639,-0.816806,0.544195,-0.335644,0.320180,-0.245921,0.346174,0.262024,0.313458,-0.047133,0.433024,-0.104597,0.194329,0.371046,0.032062,-0.377117,0.538009,-0.072974,0.483901,-0.640910,0.133632,0.466766,0.577955,0.024073,-0.101454,0.754400,0.734682,-0.825938,0.472670,-0.025625,0.527926,-0.898712,0.365001,-0.074371,-0.076774,0.762455,-0.114776,0.572464,-0.276213,1.417424,0.133278,0.446154,0.418907,-0.125009,0.082236,0.380809,-0.118000,0.951351,0.200315,-0.511247,-0.133907,-0.010440,-0.659944,0.973195,0.326049,0.363816,0.907908,0.315052,0.023193,0.052443,-0.095333,0.432227,0.343802,0.214418,-0.089798,0.348505,0.888393,-0.207843,-0.630608,1.214010,-0.973394,1.480980,0.537640,0.997048,-0.091702,2.320434,0.754859,-0.204601,0.405819,0.167712,0.322394,0.951012,2.688020,-1.225538,0.124782,0.400996,2.479693,-1.060630,1.146384,-0.111916,0.619358,2.395385,0.736614,-0.834767,-0.914450,-0.933551,0.007427,-0.248009,-0.163120,-0.162943,-0.899038,-0.240723,-0.463889,-0.653798,-0.996559,-0.071422,0.032606,-0.366413,0.127700,-0.910203,0.396729,-0.067767,-0.023764,-0.083606,-0.629252,0.165589,-0.835604,0.268355,-0.540108,0.096986,-1.073658,0.323300,0.142890,-0.150626,0.172099,-0.418903,0.551435,-0.202183,-0.593669,0.343344,-1.168612,0.421617,-0.980594,0.538626,0.304340,0.115792,-0.063678,0.202044,-0.202997,0.077110,0.361747,-0.436873,-0.728656,0.071872,1.371529,-0.081917,0.254101,0.155017,0.336780,0.407582,0.361938,0.396474,0.880314,0.254375,0.153630,0.224104,-1.052195,0.118866,-1.501864,1.021460,0.119800,0.317084,0.858165,0.389023,-0.783948,0.935497,-1.531801,0.679788,0.265741,0.386012,0.679704,0.209810,-0.298783,0.140229,1.502660,0.210144,0.903469,0.448721,-0.059346,0.934810,0.970152,0.606444,-0.937171,0.490760,-2.097616,0.840476,1.514010,0.656929,1.287129,-0.064554,0.037758,-0.577859,-2.520982,0.971397,-0.200100,0.117449,0.089544,-0.146082,-1.552950,0.166141,0.769878,-0.191948,-1.368433,1.306685,-1.855020,-0.053611,0.999777,-2.698295,-1.138166,-0.121467,1.454622,-2.447875,1.505612,-0.549151,-2.431827,-1.792676,2.307231,0.018997,0.177337,0.999453,-1.903139,-1.571920 };

    for (PieceType pt = Pawn; pt < PieceType_N; ++pt)
    {
      for (SQ sq = A1; sq < SQ_N; ++sq)
      {
        const Piece p = to_piece(pt, White);
        const int index = pt * 64 + sq;

        pst[p][sq].op = 100_cp * pst_tune[2 * index + 0];
        pst[p][sq].eg = 100_cp * pst_tune[2 * index + 1];

        pst[opp(p)][opp(sq)].op = -100_cp * pst_tune[2 * index + 0];
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

  // Material info /////////////////////////////////////////////////

  for (int wq = 0; wq < 2; wq++) for (int bq = 0; bq < 2; bq++)
  for (int wr = 0; wr < 3; wr++) for (int br = 0; br < 3; br++)
  for (int wb = 0; wb < 3; wb++) for (int bb = 0; bb < 3; bb++)
  for (int wn = 0; wn < 3; wn++) for (int bn = 0; bn < 3; bn++)
  for (int wp = 0; wp < 9; wp++) for (int bp = 0; bp < 9; bp++)
  {
    auto mat = get_matinfo({bp, wp, bn, wn, bb, wb, br, wr, bq, wq});
    mattable[get_index(mat.first)] = mat.second;
  }
}

void Eval::set_explanations(bool on)
{
#ifdef DEBUG_EVAL
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

template Duo Eval::eval_passers<Black>(const Board * B);
template Duo Eval::eval_passers<White>(const Board * B);

template Duo Eval::eval_threats<Black>(const Board * B);
template Duo Eval::eval_threats<White>(const Board * B);
}
