#pragma once
#include <string>

namespace eia {

using Str = const std::string;

const std::string Name = "Eia";
const std::string Vers = "0.5";
const std::string Auth = "Nick Kurgin";

namespace Phase
{
  const int Light = 1;
  const int Rook = 2;
  const int Queen = 4;
  const int Endgame = 7;
  const int Total = 2 * (Light * 4 + Rook * 2 + Queen);
};

enum Time
{
  Def = 60000,
  Inc =  1000,
};

enum Limits
{
  Plies = 128,
};

enum HashTables
{
  Size = 64
};

namespace Pos
{
  Str Init = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  Str Fine = "8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - -";
  Str Pos3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
  Str Pos4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
  Str See1 = "1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - -"; // Re1e5?
  Str See2 = "1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - -"; // Nd3e5?
  Str Mith = "8/k7/P2b2P1/KP1Pn2P/4R3/8/6np/8 w - - 0 1"; // b6! Re1!!
  Str Mine = "8/8/8/4k3/4B3/6p1/5bP1/4n2K b - - 0 1"; // Bg1!! Bh2! ... Nf2#
};

namespace Tunes
{
  Str Def = "MatPawnOp:234 MatKnightOp:408 MatBishopOp:560 MatRookOp:1162 MatQueenOp:2022 MatPawnEg:214 MatKnightEg:534 MatBishopEg:518 MatRookEg:852 MatQueenEg:1790 PawnFile:10 KnightCenterOp:10 KnightCenterEg:20 KnightRank:6 KnightBackRank:2 KnightTrapped:374 BishopCenterOp:16 BishopCenterEg:8 BishopBackRank:22 BishopDiagonal:16 RookFileOp:6 QueenCenterOp:20 QueenCenterEg:22 QueenBackRank:14 KingFile:24 KingRank:38 KingCenterEg:6 Doubled:20 Isolated:30 Backward:26 Connected:34 WeaknessPush:38 NMobMult:264 BMobMult:220 RMobMult:120 QMobMult:398 NMobSteep:56 BMobSteep:18 RMobSteep:54 QMobSteep:6 BishopPair:44 BadBishop:50 RammedBishop:32 KnightOutpost:40 RookSemi:78 RookOpen:16 Rook7thOp:70 Rook7thEg:88 BadRook:26 KnightFork:88 BishopFork:24 KnightAdj:24 RookAdj:58 EarlyQueen:30 ContactCheckR:100 ContactCheckQ:180 Shield1:16 Shield2:58 PasserK:48 Candidate:200 Passer:1006 Supported:128 Unstoppable:956 FreePasser:104 Xray:6 PinMul:84 ThreatPawn:24 ThreatL_P:100 ThreatL_L:112 ThreatL_H:236 ThreatL_K:166 ThreatR_L:118 ThreatR_K:0 ThreatQ_1:0 Tempo:64";
  
  // 80000 iterations (1, .1, 100 | 3h) +7 elo h2h-40
  Str SPSA1 = "MatPawnOp:102.9390 MatKnightOp:495.6238 MatBishopOp:528.9422 MatRookOp:741.1415 MatQueenOp:1489.2236 MatPawnEg:109.9711 MatKnightEg:476.7209 MatBishopEg:429.5764 MatRookEg:757.5353 MatQueenEg:1495.9585 PawnFile:6.3754 KnightCenterOp:4.6830 KnightCenterEg:6.0712 KnightRank:6.6471 KnightBackRank:7.3224 KnightTrapped:123.7949 BishopCenterOp:5.8907 BishopCenterEg:4.0955 BishopBackRank:7.8955 BishopDiagonal:6.0962 RookFileOp:7.4411 QueenCenterOp:4.1937 QueenCenterEg:6.9536 QueenBackRank:6.4046 KingFile:16.2326 KingRank:15.7900 KingCenterEg:13.5898 Doubled:12.9877 Isolated:12.3360 Backward:3.2707 Connected:7.6393 WeaknessPush:29.5432 NMobMult:58.1234 BMobMult:91.6174 RMobMult:50.9788 QMobMult:160.9159 NMobSteep:9.7145 BMobSteep:12.6118 RMobSteep:8.3578 QMobSteep:3.4750 BishopPair:39.5647 BadBishop:31.4268 RammedBishop:30.5293 KnightOutpost:31.3256 RookSemi:18.5593 RookOpen:29.1275 Rook7thOp:30.9059 Rook7thEg:34.0291 BadRook:29.5934 KnightFork:27.6345 BishopFork:31.0512 KnightAdj:16.1834 RookAdj:7.6082 EarlyQueen:-5.5110 ContactCheckR:157.1333 ContactCheckQ:191.5707 Shield1:16.6388 Shield2:19.2340 PasserK:31.6448 Candidate:135.8530 Passer:177.4820 Supported:80.8580 Unstoppable:848.3423 FreePasser:63.0926 Xray:29.9682 PinMul:32.4749 ThreatPawn:30.8608 ThreatL_P:68.4522 ThreatL_L:37.0866 ThreatL_H:55.0792 ThreatL_K:57.5149 ThreatR_L:62.9046 ThreatR_K:55.1021 ThreatQ_1:64.4827 Tempo:39.8164";
  
  Str Book = ".\\datasets\\Perfect_2011.pgn";
};

}
