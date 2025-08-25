#pragma once
#include <string>

namespace eia {

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

enum Val
{
  Draw  = 0,
  Inf   = 32767,
  Mate  = 32000,
};

enum HashTables
{
  Size = 64
};

namespace Pos
{
  const std::string Init = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  const std::string Fine = "8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - -";
  const std::string Pos3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
  const std::string Pos4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
  const std::string See1 = "1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - -"; // Re1e5?
  const std::string See2 = "1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - -"; // Nd3e5?
  const std::string Mith = "8/k7/P2b2P1/KP1Pn2P/4R3/8/6np/8 w - - 0 1"; // b6! Re1!!
  const std::string Mine = "8/8/8/4k3/4B3/6p1/5bP1/4n2K b - - 0 1"; // Bg1!! Bh2! ... Nf3!
};

namespace Tune
{
  const std::string Def = "MatPawnOp:234 MatKnightOp:408 MatBishopOp:560 MatRookOp:1162 MatQueenOp:2022 MatPawnEg:214 MatKnightEg:534 MatBishopEg:518 MatRookEg:852 MatQueenEg:1790 PawnFile:10 KnightCenterOp:10 KnightCenterEg:20 KnightRank:6 KnightBackRank:2 KnightTrapped:374 BishopCenterOp:16 BishopCenterEg:8 BishopBackRank:22 BishopDiagonal:16 RookFileOp:6 QueenCenterOp:20 QueenCenterEg:22 QueenBackRank:14 KingFile:24 KingRank:38 KingCenterEg:6 Doubled:20 Isolated:30 Backward:26 Connected:34 WeaknessPush:38 NMobMult:264 BMobMult:220 RMobMult:120 QMobMult:398 NMobSteep:56 BMobSteep:18 RMobSteep:54 QMobSteep:6 BishopPair:44 BadBishop:50 RammedBishop:32 KnightOutpost:40 RookSemi:78 RookOpen:16 Rook7thOp:70 Rook7thEg:88 BadRook:26 KnightFork:88 BishopFork:24 KnightAdj:24 RookAdj:58 EarlyQueen:30 ContactCheckR:100 ContactCheckQ:180 Shield1:16 Shield2:58 PasserK:48 Candidate:200 Passer:1006 Supported:128 Unstoppable:956 FreePasser:104 Xray:6 PinMul:84 ThreatPawn:24 ThreatL_P:100 ThreatL_L:112 ThreatL_H:236 ThreatL_K:166 ThreatR_L:118 ThreatR_K:0 ThreatQ_1:0 Tempo:64";
  const std::string Book = ".\\datasets\\Perfect_2011.pgn";
};

}
