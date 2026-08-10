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

  Duo ksafety = Duo::both(ei.king_safety());
  duo += ksafety;

  const int progress = 100 - std::max(B->state.fifty, 68);
  const int phase = B->phase();

  Val mixed = duo.tapered(phase, scale) / 32 * progress;
  Val score = B->color ? mixed : -mixed;

#ifdef TUNING
  const float fadeout = progress / 32.f;
  T.factor[OP] = fadeout * (Phase::Total - phase) / Phase::Total;
  T.factor[EG] = fadeout * scale / 128 * phase / Phase::Total;
#endif

  return unzero(score + Tempo);
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

    if (!fore_friendly && !cannot_pass)
    {
      if (!sentries)
      {
        ei.passers |= bit(sq);
      }
      else
      {
        const int push = popcnt(supports) - popcnt(sentries);

        if (push >= 0) // candidate
        {
          vals += apply<Col>(Candidate, r - 1);
        }
        else if (r == 4) // faker on 5th rank
        {
          u64 hiddens = backward<Col>(sentries) & B->piece[p];

          if (push + popcnt(hiddens) >= 0) // hidden passer
          {
            vals += apply<Col>(Candidate, 4);
          }
        }
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

    // knight behind pawn

    if (backward<Col>(pawns) & bit(sq))
    {
      vals += apply<Col>(KnightBehind);
    }

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

    // bishop behind pawn

    int pawns = popcnt(B->piece[BP ^ Col]);

    if (backward<Col>(pawns) & bit(sq))
    {
      vals += apply<Col>(BishopBehind);
    }

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
      vals += apply<Col>(WeaknessPush, k_dist(sq, j) - 1);
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

Eval::Eval(const Tune & tune, bool no_hash) : no_hash(no_hash)
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

std::string Eval::prettify() const
{
  string str;
  for (int i = 0; i < Term_N; i++) // scalar terms
  {
    if (info[i].size > 1) continue;
    const Term term = (Term)i;
    const auto vals = get(term);

    str += format("TERM({:9}, {:13}, {: >10.4f}, {: >10.4f}) \\\n",
      group_str[info[i].group], term,
      dry_double(vals.op), dry_double(vals.eg));
  }

  str += "\n";

  for (int i = 0; i < Term_N; i++) // array-like terms
  {
    if (info[i].size <= 1) continue;
    const Term term = (Term)i;

    //init_term(MobR,
    //{
    //  {-58, -58}, {-40, -40}, {-30, -30}, {-22, -22},
    //  {-16, -16}, {-10, -10}, {-6, -6}, {-1, -1}, {1, 1},
    //  {5, 5}, {8, 8}, {11, 11}, {14, 14}, {16, 16}, {19, 19}
    //});

    str += format("init_term({}, \n{{", term);
    const auto offset = info[term].index;
    const auto size = info[term].size;

    for (int j = 0; j < size; j++)
    {
      const auto vals = data[offset + j];
      /*if (!(j & 3)) */ str += "\n";
      str += format("{{{: >10.4f}, {: >10.4f}}}, ",
        dry_double(vals.op), dry_double(vals.eg));
    }
    str += "\n});\n\n";
  }
  return str + "\n";
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
  // Epoch #1954 (4h) | 10m eth | AdaGrad | loss +0.1197
  //  +42 elo (20+.2 h2h-100)

  // ------------------------------
  //  Material
  // ------------------------------

  init_term(MatValue,
  {
    {   91.6340,   156.9180}, // pawn
    {  308.5523,   605.1204}, // knight
    {  285.2769,   578.5494}, // bishop
    {  409.5337,   940.7799}, // rook
    {  808.7685,  1839.0515}, // queen
  });

  // ------------------------------
  //  Pawn structure
  // ------------------------------

  init_term(Doubled, // by file
  {
    {    1.2309,   -15.0888},
    {    2.6989,    -7.6115},
    {  -14.0255,    -4.3229},
    {  -11.4044,   -11.1352},
    {   -6.2515,    -4.5959},
    {    4.9132,    -6.4967},
    {   14.5957,    -3.5403},
    {   -7.0984,   -12.3623},
  });

  init_term(Isolated, // by file
  {
    {  -11.3672,     2.3634},
    {   -2.8253,   -11.3402},
    {   -9.7818,   -14.0243},
    {   -7.2057,   -20.5382},
    {   -0.6544,   -13.8262},
    {    5.8609,   -12.9358},
    {    7.1428,    -6.7062},
    {    0.7317,    -8.9204},
  });

  init_term(Backward, // by (rank - 1)
  {
    {    6.0579,   -14.5226},
    {  -12.4105,   -22.8644},
    {   -0.0798,   -26.6810},
    {   31.5938,   -23.1460},
  });

  init_term(PawnShield, // by [kingfile][rankdist2king]
  {
    {  -15.1109,     0.5409},
    {   -3.1336,    -9.6382},
    {   -8.4341,    -0.5608},
    {  -29.7069,    -7.2902},
    {  -40.8831,    -5.5621},
    {  -50.6968,    -3.5355},
    {  -95.4641,    -4.5079},
    {  -66.4164,    18.8468},

    {    0.0000,     0.0000}, // absurdly epic (overlaps king)
    {  -16.7956,    -9.0753},
    {  -20.2755,    -1.7897},
    {  -36.9912,    -7.7817},
    {  -55.3257,    -0.7007},
    {  -85.4318,    -7.1946},
    { -114.9245,    -6.7136},
    { -110.6806,    16.9240},
  });

  init_term(WeaknessPush, // by k_dist
  {
    {    5.5971,    15.7694},
    {   -4.8359,    -0.0796},
    {   -0.4153,    -3.1198},
    {    1.9631,    -3.1415},
    {    1.7421,    -8.6611},
    {    1.2181,   -12.6583},
    {  -14.3950,   -10.7134},
  });

  // ------------------------------
  //  Mobility
  // ------------------------------

  init_term(MobN,
  {
    {  -54.6063,   -57.8405},
    {  -18.2620,   -14.7698},
    {  -18.6870,   -12.8605},
    {  -12.7096,    -8.1754},
    {   -4.9736,    -7.3415},
    {   -5.4468,    -2.0804},
    {    1.8128,     4.2374},
    {    7.2001,     5.5057},
    {   15.9593,     0.0422},
  });

  init_term(MobB,
  {
    {  -96.0000,  -100.0000}, // adjusted
    {  -48.8669,   -50.0794},
    {  -21.4608,   -19.5492},
    {   -9.7091,    -4.9727},
    {    6.0766,     5.7951},
    {   11.6156,    13.2815},
    {   24.4135,    23.9210},
    {   29.1822,    39.9804},
    {   37.8572,    45.3018},
    {   39.7154,    54.4781},
    {   41.6714,    53.4336},
    {   55.6730,    52.5767},
    {   58.9790,    57.5894},
    {   64.8349,    61.1780},
  });

  init_term(MobR,
  {
    { -100.0000,  -100.0000}, // adjusted
    {  -60.0000,   -62.0000}, // adjusted
    {  -40.4004,   -41.9422},
    {  -23.3945,   -24.9516},
    {  -13.9464,   -13.6817},
    {  -12.2487,    -7.7427},
    {   -6.5891,    -8.6517},
    {   -2.8742,    -0.7556},
    {    1.3922,     2.6349},
    {    6.2905,     3.9214},
    {   10.2625,     9.9942},
    {   13.5987,    13.4956},
    {   16.8858,    15.7926},
    {   19.4884,    18.8903},
    {   16.0183,    15.4198},
  });

  init_term(MobQ,
  {
    {  -96.0000,   -96.0000}, // adjusted
    {  -48.0000,   -48.0000}, // adjusted
    {  -33.0000,   -33.0000}, // adjusted
    {  -17.5202,   -21.4411},
    {  -13.3424,   -19.5696},
    {   -2.2148,   -12.4710},
    {   -2.5259,   -10.7204},
    {   -5.7706,    -5.9952},
    {   -8.1776,     6.3460},
    {    5.2020,     7.7404},
    {    8.2871,     9.8126},
    {   15.3480,    15.4175},
    {   18.8312,    19.9666},
    {   22.2911,    20.4866},
    {   22.9112,    23.4457},
    {   23.3225,    27.7518},
    {   29.7096,    26.0696},
    {   30.8401,    26.2601},
    {   28.8731,    26.9442},
    {   31.3427,    30.3246},
    {   27.7361,    39.0008},
    {   32.9657,    33.2562},
    {   34.1628,    35.0608},
    {   33.5443,    32.7132},
    {   35.6392,    35.3547},
    {   34.3500,    35.5523},
    {   39.1447,    39.7348},
    {   40.9600,    39.3310},
  });

  // ------------------------------
  //  Outposts
  // ------------------------------

  init_term(KnightOutpost, // [central][defended]
  {
    {   15.4058,   -25.7195}, {   31.0552,    -5.4135},
    {   13.3137,   -15.2400}, {   19.7231,    11.8697},
  });

  init_term(BishopOutpost, // [central][defended]
  {
    {   24.3861,    -4.7933}, {   43.2339,     3.3829},
    {   20.8557,    -1.0915}, {    9.9279,     5.2820},
  });

  // ------------------------------
  //  Passers
  // ------------------------------

  init_term(Candidate, // from 2 rank
  {
    {  -14.8477,    14.3392}, // 2
    {   -0.7337,    21.7241}, // 3
    {   -2.4549,    39.3729}, // 4
    {   14.4630,   102.0547}, // 5 - last candidate
    {    0.0000,     0.0000}, // 6 - hidden passer
  });

  init_term(Passer, // from 2 rank
  {
    {  -28.0559,    19.8246},
    {  -33.9783,    35.2040},
    {  -42.6049,    70.4196},
    {   18.5631,    97.8881},
    {   90.4070,   166.2887},
    {  120.7189,   281.1868},
  });

  init_term(Supported, // from 2 rank
  {
    {   -2.1339,     2.8111},
    {    6.6150,     8.3798},
    {    7.1701,    22.3836},
    {   16.7658,    37.9720},
    {   17.7896,    36.5219},
    {   25.5703,    38.8055},
  });

  init_term(PasserKingDef, // by k_dist
  {
    {    4.3191,     8.5634},
    {    0.7493,     8.4709},
    {   -6.5290,     0.0781},
    {    2.1855,   -11.5054},
    {   12.4865,   -25.2552},
    {    2.2559,   -24.5387},
    {   -1.5302,   -12.3303},
    {   -3.5920,    -7.7031},
  });

  init_term(PasserKingAtt, // by k_dist
  {
    {   -3.2481,   -16.5280},
    {   -5.8395,   -15.5663},
    {    3.5275,     0.7151},
    {    9.9560,    12.9753},
    {    6.8269,    33.4007},
    {   13.6746,    47.9308},
    {   35.6807,    50.6611},
    {   13.2529,    10.1486},
  });

  // ------------------------------
  //  Xrays
  // ------------------------------

  init_term(PinAbsolute,
  {
    // safe
    {  -23.1950,   -26.7428}, // N
    {  -26.8196,   -31.6251}, // B
    {  -42.9585,   -51.4635}, // R
    { -101.4353,  -121.1700}, // Q

    // pawn att
    {  -57.2347,   -66.5789}, // N
    {  -66.4007,   -70.5694}, // B
    {  -97.6985,  -117.1268}, // R
    { -222.8660,  -252.3040}, // Q - yes, it's possible (...rQK..)
  });

  init_term(PinPartial,
  {
    // safe
    {  -16.0000,   -17.0000}, // N | left as is
    {  -17.0000,   -18.0000}, // B | left as is
    {  -25.0000,   -30.0000}, // R | left as is
    {  -52.9191,   -64.0464}, // Q

    // pawn att
    {  -20.0000,   -21.0000}, // N | left as is
    {  -21.0000,   -22.0000}, // B | left as is
    {  -30.0000,   -35.0000}, // R | left as is
    {  -56.7971,   -67.8858}, // Q
  });

  init_term(PinRelative,
  {
    // safe
    {   -7.5890,    -2.3979}, // N
    {   -9.7630,    -6.0772}, // B
    {  -16.4104,   -16.6672}, // R
    {   -5.0709,    -3.8527}, // Q

    // pawn att
    {  -20.2218,   -18.1567}, // N
    {  -16.9825,   -15.1756}, // B
    {  -32.2394,   -33.1084}, // R
    {   -9.0000,    -9.0000}, // Q | left as is
  });

  // ------------------------------
  //  PST
  // ------------------------------

  init_term(PST_P,
  {
    { 227.6142,  198.7541},   {  57.5674, -152.0422},   { 226.1646,  -76.7042},   { 168.6472,    9.2298},
    {  40.0089, -225.4314},   {-122.2925,  -16.2682},   { -94.1329, -161.1756},   { 189.5483,   94.2418},
    { -46.7375,   11.8894},   { -45.4163,    7.1383},   { -37.8453,   12.6789},   { -11.9764,   15.4966},
    { -16.1931,    5.3506},   { -12.2497,   27.9335},   { -16.1747,   -5.7034},   { -30.0196,  -13.1031},
    { -40.7761,  -24.6031},   { -46.9998,   -5.2897},   { -20.6996,    8.9381},   {  -8.7115,  -10.4760},
    {  10.1693,   20.9079},   { -19.6047,   15.6782},   { -10.7604,   -4.5948},   { -18.8583,  -14.2453},
    { -31.7673,    4.5216},   { -38.4806,   13.9391},   {  -9.2431,    2.5588},   {   6.7875,   18.9262},
    {   9.9033,    1.6722},   {  15.4574,    2.1163},   {   8.0216,   -4.1402},   {   2.9179,   -1.7709},
    { -40.6282,    8.2983},   {  -9.5247,   21.6428},   {  -2.3417,   25.7651},   {  23.9574,  -30.2535},
    {  22.3939,   -2.1408},   {  42.7080,    4.5519},   {   5.7944,   21.9172},   {   5.0026,   18.6908},
    { -61.7348,   66.8906},   {  23.4935,   38.2620},   {  38.5934,   11.6648},   {  10.6165,    7.3768},
    {  56.1472,    7.4191},   {  63.8543,   32.7404},   {  51.5379,  -10.9466},   {  -3.0564,   52.8663},
    { 228.2944,   30.8338},   {  57.2991,   33.8439},   {  -0.1944,   41.3416},   { 252.1305,  -33.2805},
    {  41.0312,   -9.2833},   {  40.2728,   87.4404},   {-114.0682,   91.9711},   {-103.2477,   82.8867},
    { 144.1641,  -86.1923},   { 120.9819,   70.8101},   { 196.1112,   34.4058},   {  30.2037, -124.5265},
    {  64.5011,  101.0895},   {  30.5045,  -86.0249},   {  11.6837, -132.7762},   { -30.3397,  155.7071},
  });

  init_term(PST_N,
  {
    { -83.6997,  -60.1394},   { -18.4695, -114.0481},   { -81.9660,  -79.1267},   { -27.0433,  -29.4983},
    { -15.6237,  -15.8388},   { -32.1291,  -33.3108},   { -12.4790, -110.8952},   {-171.1607, -159.9707},
    {-112.7241,  -61.1430},   { -35.0726,  -56.3577},   {  -4.4821,  -70.0958},   {  -6.4831,  -30.6677},
    {  -8.5701,  -18.3438},   { -31.3439,  -40.7718},   { -47.2636,   45.0735},   { -11.2099, -106.1633},
    { -46.6695,  -74.6709},   { -39.2773,  -21.3419},   {  -7.9990,  -33.7717},   {   1.2991,   32.3744},
    {  14.4790,  -11.5982},   {   6.5806,  -37.3750},   {   0.4108,  -28.2528},   { -36.1705,  -85.1366},
    { -22.5241,  -64.5170},   { -51.6290,   17.0561},   {  11.8904,   46.0516},   {  21.0478,   53.7386},
    {  17.9030,   46.3390},   {  -2.9120,   38.5594},   { -23.0042,   41.6211},   { -30.7121,    7.1084},
    {  -6.4277,  -25.5379},   {   7.3353,  -17.5410},   {   2.8092,   34.2882},   {  25.7564,   36.8470},
    {  20.1490,   47.6622},   {   6.0139,   64.7007},   {   6.8047,    4.8301},   {  -2.5007,  -52.9324},
    { -78.5891,  -50.0846},   { -62.7034,   20.2494},   {  53.2977,    9.3607},   {  79.2598,  -18.8998},
    { 107.2457,  -22.6408},   {  71.6316,   19.1447},   {  51.3042,   -7.9683},   {  34.9471,  -79.9239},
    { -32.1415,  -56.4287},   { -63.1006,  -11.0498},   {   3.3968,  -13.9768},   { -37.7040,   49.1881},
    { -12.1165,    4.7563},   { -50.0765,   -0.4458},   {-119.4315,  -71.7901},   {-209.4707,   38.7895},
    {-132.1248,  -97.9577},   {  -2.2658, -116.1803},   { -22.2443,   10.7711},   { 131.6611,   18.2078},
    { -51.8617,   36.3035},   { -79.0392,   30.2859},   {-294.5059,   18.6200},   {-210.1146, -101.9031},
  });

  init_term(PST_B,
  {
    {  43.5262,  -45.6579},   {  23.8808,  -44.9418},   {  17.9055,   27.7397},   {  10.8396,  -11.1464},
    { -10.8517,   18.4631},   {  -0.6536,   21.0820},   { -55.7180,   29.7781},   {   8.9283,   47.3483},
    {  17.1476,   46.5572},   {   8.5114,   -0.5786},   {   4.7987,  -15.0884},   {  -2.1879,  -16.2760},
    {   7.0992,    7.8804},   { -11.5012,  -17.8260},   {  33.2776,   11.9050},   {   2.5260,  -14.3729},
    {   1.8985,   29.0551},   {  32.2116,   12.9149},   { -10.9206,   16.9549},   {  11.1818,    7.0279},
    {  -1.7042,   41.6345},   {   8.6242,   15.3271},   {  -9.7922,    7.2006},   {  31.4425,  -34.7625},
    {   1.9738,    8.5782},   { -24.2456,   40.2758},   {  -9.6279,   29.4097},   {  -2.2488,   36.4211},
    {  14.2906,   14.1802},   { -13.2794,   16.2762},   {  -3.0786,    8.7157},   {   7.4306,    4.9027},
    {  -4.4730,   20.6855},   { -27.4130,   51.3367},   { -25.5751,   58.2213},   {   8.3363,   21.1257},
    {   7.0939,   42.8553},   {  22.3141,  -12.3528},   { -13.4310,   63.0708},   { -21.1238,   41.7666},
    {  -3.4456,   19.5804},   {  13.8154,   47.7503},   {  67.7132,    5.1608},   {  23.6085,   72.9170},
    {  58.7619,  -24.1603},   { -15.0417,   54.7090},   {  49.1800,   22.5757},   {  33.5689,   39.2211},
    {  68.3551,   37.3246},   {  13.0308,   -9.5579},   {  12.4601,   32.5374},   {  36.8716,    1.4114},
    { -21.7275,   60.0057},   {  58.2692,   12.3700},   { -60.0195,   16.5213},   {  52.8931,    4.8041},
    { 242.1867,  -87.1710},   {  -7.4891,   16.5581},   {  79.4695,   15.3019},   {  70.3716,  -31.1834},
    {  -4.4303,    0.5565},   { -70.6130,    1.9960},   {   2.8845,   17.7792},   { -17.2561,  -46.2228},
  });

  init_term(PST_R,
  {
    { -13.1970,   22.4840},   { -18.7314,   45.6566},   {  -7.7230,   35.8352},   {   1.0129,   14.8773},
    {  -9.1402,   20.1539},   {  -4.9943,   39.0313},   { -15.5152,   56.8653},   {   7.4459,   43.0990},
    { -81.9037,   42.6924},   { -12.1638,   21.9414},   { -54.2320,   54.3472},   { -49.7683,   20.2970},
    {  -1.8902,   14.4435},   {  -4.4213,   39.3444},   {  11.7512,   -6.3252},   {  -5.4206,   36.2107},
    {  -8.7068,  -16.1372},   {   8.5044,   13.7255},   { -26.8890,   27.7512},   { -35.9183,   42.1877},
    {  -9.5308,    4.7249},   { -23.6974,   13.8178},   { -13.1648,   23.9190},   { -17.2334,   82.4596},
    {  -7.0342,   32.5172},   { -39.3130,   68.2023},   { -21.8960,   34.2614},   { -19.3397,   48.9638},
    { -31.9351,   60.6848},   { -12.9646,   24.7874},   {  59.7679,   -6.0966},   {  32.7255,   20.0054},
    {  -4.8625,   36.8143},   { -32.4125,   48.9323},   {  18.1163,   37.6427},   {  23.9658,   48.9496},
    {  64.4057,   54.0453},   {  36.5558,   35.0644},   {  31.1267,   20.8513},   {  29.8865,   34.2864},
    {  21.3350,   44.9955},   {  -6.9840,   66.5766},   {   9.9376,   66.5136},   {  -8.3103,   48.5708},
    {  42.5574,   42.9536},   {  60.1178,   58.6619},   { 166.9445,  -10.0091},   {   1.8400,   27.8159},
    {   4.1850,   48.1006},   {   9.3841,   52.4778},   {   2.1170,   46.2412},   {  16.9866,   45.7367},
    {  34.9655,   46.9186},   { 105.7585,   14.1099},   {  54.9067,   22.2768},   {  62.5668,   35.3752},
    { -33.8966,   71.0659},   {  63.0648,   40.1924},   {  32.5094,   61.0950},   { -39.5200,   50.0967},
    {  13.3719,   44.8352},   {  58.7519,   29.3430},   { -26.5112,   92.2548},   { 100.3540,   51.3978},
  });

  init_term(PST_Q,
  {
    {  14.7708,  -53.2642},   {  11.5157,  -41.1369},   {  54.3313,  -51.4811},   {  62.3283,  -76.9461},
    {  44.3369,  -87.0552},   {  64.8947,  -89.0704},   {  60.1042, -188.7790},   {  26.5313,  -66.0443},
    {  23.8797,  -82.6848},   {  45.3986,  -33.2573},   {  55.6560,  -63.0779},   {  50.1698,    4.1601},
    {  52.0510,  -48.6397},   {  45.1097,  -17.9046},   {  71.4735,  -45.3347},   {   3.6396,  -59.4768},
    {   7.0110,  -18.0265},   {  52.8639,  -81.6806},   {  54.4195,  -33.5644},   {  32.0180,  -24.5921},
    {  34.6174,   26.2023},   {  31.3458,   -4.7133},   {  43.3024,  -10.4597},   {  19.4329,   37.1046},
    {   3.2062,  -37.7117},   {  53.8009,   -7.2974},   {  48.3901,  -64.0910},   {  13.3632,   46.6766},
    {  57.7955,    2.4073},   { -10.1454,   75.4400},   {  73.4682,  -82.5938},   {  47.2670,   -2.5625},
    {  52.7926,  -89.8712},   {  36.5001,   -7.4371},   {  -7.6774,   76.2455},   { -11.4776,   57.2464},
    { -27.6213,  141.7424},   {  13.3278,   44.6154},   {  41.8907,  -12.5009},   {   8.2236,   38.0809},
    { -11.8000,   95.1351},   {  20.0315,  -51.1247},   { -13.3907,   -1.0440},   { -65.9944,   97.3195},
    {  32.6049,   36.3816},   {  90.7908,   31.5052},   {   2.3193,    5.2443},   {  -9.5333,   43.2227},
    {  34.3802,   21.4418},   {  -8.9798,   34.8505},   {  88.8393,  -20.7843},   { -63.0608,  121.4010},
    { -97.3394,  148.0980},   {  53.7640,   99.7048},   {  -9.1702,  232.0434},   {  75.4859,  -20.4601},
    {  40.5819,   16.7712},   {  32.2394,   95.1012},   { 268.8020, -122.5538},   {  12.4782,   40.0996},
    { 247.9693, -106.0630},   { 114.6384,  -11.1916},   {  61.9358,  239.5385},   {  73.6614,  -83.4767},
  });

  init_term(PST_K,
  {
    { -91.4450,  -93.3551},   {   0.7427,  -24.8009},   { -16.3120,  -16.2943},   { -89.9038,  -24.0723},
    { -46.3889,  -65.3798},   { -99.6559,   -7.1422},   {   3.2606,  -36.6413},   {  12.7700,  -91.0203},
    {  39.6729,   -6.7767},   {  -2.3764,   -8.3606},   { -62.9252,   16.5589},   { -83.5604,   26.8355},
    { -54.0108,    9.6986},   {-107.3658,   32.3300},   {  14.2890,  -15.0626},   {  17.2099,  -41.8903},
    {  55.1435,  -20.2183},   { -59.3669,   34.3344},   {-116.8612,   42.1617},   { -98.0594,   53.8626},
    {  30.4340,   11.5792},   {  -6.3678,   20.2044},   { -20.2997,    7.7110},   {  36.1747,  -43.6873},
    { -72.8656,    7.1872},   { 137.1529,   -8.1917},   {  25.4101,   15.5017},   {  33.6780,   40.7582},
    {  36.1938,   39.6474},   {  88.0314,   25.4375},   {  15.3630,   22.4104},   {-105.2195,   11.8866},
    {-150.1864,  102.1460},   {  11.9800,   31.7084},   {  85.8165,   38.9023},   { -78.3948,   93.5497},
    {-153.1801,   67.9788},   {  26.5741,   38.6012},   {  67.9704,   20.9810},   { -29.8783,   14.0229},
    { 150.2660,   21.0144},   {  90.3469,   44.8721},   {  -5.9346,   93.4810},   {  97.0152,   60.6444},
    { -93.7171,   49.0760},   {-209.7616,   84.0476},   { 151.4010,   65.6929},   { 128.7129,   -6.4554},
    {   3.7758,  -57.7859},   {-252.0982,   97.1397},   { -20.0100,   11.7449},   {   8.9544,  -14.6082},
    {-155.2950,   16.6141},   {  76.9878,  -19.1948},   {-136.8433,  130.6685},   {-185.5020,   -5.3611},
    {  99.9777, -269.8295},   {-113.8166,  -12.1467},   { 145.4622, -244.7875},   { 150.5612,  -54.9151},
    {-243.1827, -179.2676},   { 230.7231,    1.8997},   {  17.7337,   99.9453},   {-190.3139, -157.1920},
  });
}

void Eval::init_inner()
{
  // ------------------------------
  //  PST
  // ------------------------------

  for (int i = 0; i < 12; i++)
    for (int sq = 0; sq < 64; sq++)
      pst[i][sq].clear();

  // CMA-ES on Ethereal (100k) +131 elo (20s+.2 h2h-20)
  // Best loss: 0.117247

  for (PieceType pt = Pawn; pt < PieceType_N; ++pt)
  {
    const Term term = (Term)(PST_P + pt);

    for (SQ sq = A1; sq < SQ_N; ++sq)
    {
      const Piece p = to_piece(pt, White);
      const Duo vals = get(term, sq);

      pst[p][sq]           =  vals;
      pst[opp(p)][opp(sq)] =  vals; // same sign!
    }
  }

  // Show

  /*for (Piece p = WP; p < Piece_N; p = p + 2)
  {
    log("init_term(PST_{}, \n{{", to_char(p));

    for (SQ sq = A1; sq < SQ_N; ++sq)
    {
      if (!(sq & 3)) log("\n");
      log("  {{{: >9.4f}, {: >9.4f}}}, ",
        dry_double(pst[p][sq].op),
        dry_double(pst[p][sq].eg));
    }
    log("\n}});\n\n");
  }*/

  // ------------------------------
  //  Material info
  // ------------------------------

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
