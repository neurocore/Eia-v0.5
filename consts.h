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
  
  // 17800 iterations
  Str SPSA1 = "MatPawnOp:97.8601 MatKnightOp:448.4972 MatBishopOp:450.1994 MatRookOp:751.7509 MatQueenOp:1439.0384 MatPawnEg:97.0332 MatKnightEg:449.1505 MatBishopEg:450.0482 MatRookEg:748.0561 MatQueenEg:1442.6750 PawnFile:7.9553 KnightCenterOp:7.7984 KnightCenterEg:7.8977 KnightRank:7.9128 KnightBackRank:7.9195 KnightTrapped:127.8555 BishopCenterOp:8.0346 BishopCenterEg:7.7562 BishopBackRank:7.9209 BishopDiagonal:7.8986 RookFileOp:7.8085 QueenCenterOp:7.8519 QueenCenterEg:7.9527 QueenBackRank:7.9893 KingFile:15.5826 KingRank:15.5622 KingCenterEg:10.9260 Doubled:15.7663 Isolated:15.8806 Backward:15.4154 Connected:15.2260 WeaknessPush:31.0726 NMobMult:124.4145 BMobMult:120.7376 RMobMult:123.8449 QMobMult:175.9120 NMobSteep:15.1264 BMobSteep:14.9359 RMobSteep:15.2239 QMobSteep:15.2096 BishopPair:32.1597 BadBishop:32.2074 RammedBishop:32.0614 KnightOutpost:31.5213 RookSemi:31.8322 RookOpen:31.3673 Rook7thOp:32.2852 Rook7thEg:31.6489 BadRook:31.5333 KnightFork:32.3840 BishopFork:31.6983 KnightAdj:15.6672 RookAdj:16.3032 EarlyQueen:15.2863 ContactCheckR:150.3501 ContactCheckQ:189.9262 Shield1:15.8032 Shield2:16.0464 PasserK:32.0334 Candidate:122.6040 Passer:222.4908 Supported:127.9596 Unstoppable:848.4373 FreePasser:64.0918 Xray:31.9807 PinMul:32.0595 ThreatPawn:31.6962 ThreatL_P:62.9810 ThreatL_L:63.8967 ThreatL_H:64.1880 ThreatL_K:63.3950 ThreatR_L:63.5048 ThreatR_K:63.5742 ThreatQ_1:63.1778 Tempo:19.9976";
  
  // 27400 iterations
  Str SPSA2 = "MatPawnOp:97.4006 MatKnightOp:448.0062 MatBishopOp:450.0076 MatRookOp:750.8328 MatQueenOp:1438.2134 MatPawnEg:96.1219 MatKnightEg:449.0629 MatBishopEg:450.3422 MatRookEg:746.5869 MatQueenEg:1440.3561 PawnFile:7.9449 KnightCenterOp:7.7655 KnightCenterEg:7.8845 KnightRank:7.8992 KnightBackRank:7.9097 KnightTrapped:127.6331 BishopCenterOp:8.0364 BishopCenterEg:7.6710 BishopBackRank:7.9252 BishopDiagonal:7.9068 RookFileOp:7.7571 QueenCenterOp:7.8339 QueenCenterEg:7.9632 QueenBackRank:7.9767 KingFile:15.5033 KingRank:15.5020 KingCenterEg:10.2720 Doubled:15.7651 Isolated:15.9062 Backward:15.2477 Connected:15.0288 WeaknessPush:31.0418 NMobMult:122.7003 BMobMult:119.3257 RMobMult:123.2289 QMobMult:175.6217 NMobSteep:14.9086 BMobSteep:14.6951 RMobSteep:15.1489 QMobSteep:15.1315 BishopPair:32.0410 BadBishop:32.3484 RammedBishop:31.9364 KnightOutpost:31.5269 RookSemi:31.8212 RookOpen:31.2586 Rook7thOp:32.2680 Rook7thEg:31.6523 BadRook:31.4070 KnightFork:32.3654 BishopFork:31.7494 KnightAdj:15.5742 RookAdj:16.4536 EarlyQueen:15.1306 ContactCheckR:150.3643 ContactCheckQ:189.9164 Shield1:15.7988 Shield2:16.0612 PasserK:32.0276 Candidate:121.9003 Passer:216.7780 Supported:127.5109 Unstoppable:848.4868 FreePasser:64.1071 Xray:31.9775 PinMul:32.1430 ThreatPawn:31.7097 ThreatL_P:62.9003 ThreatL_L:63.7205 ThreatL_H:63.9993 ThreatL_K:63.1847 ThreatR_L:63.2217 ThreatR_K:63.7085 ThreatQ_1:63.2067 Tempo:18.4753";
  
  // 44400 iterations
  Str SPSA3 = "MatPawnOp:96.9023 MatKnightOp:446.4538 MatBishopOp:450.3412 MatRookOp:750.8330 MatQueenOp:1438.0452 MatPawnEg:94.9605 MatKnightEg:449.0852 MatBishopEg:449.6469 MatRookEg:744.7508 MatQueenEg:1437.4142 PawnFile:7.9732 KnightCenterOp:7.7044 KnightCenterEg:7.8838 KnightRank:7.8444 KnightBackRank:7.8834 KnightTrapped:127.3152 BishopCenterOp:8.0601 BishopCenterEg:7.5865 BishopBackRank:7.9393 BishopDiagonal:7.8687 RookFileOp:7.7193 QueenCenterOp:7.8117 QueenCenterEg:7.9494 QueenBackRank:7.9657 KingFile:15.4521 KingRank:15.5037 KingCenterEg:9.5468 Doubled:15.7745 Isolated:15.8847 Backward:15.0418 Connected:14.8015 WeaknessPush:30.7504 NMobMult:120.6318 BMobMult:116.7195 RMobMult:122.5356 QMobMult:175.1047 NMobSteep:14.6189 BMobSteep:14.5181 RMobSteep:15.0502 QMobSteep:14.9089 BishopPair:32.1811 BadBishop:32.3115 RammedBishop:31.8518 KnightOutpost:31.5506 RookSemi:31.7287 RookOpen:31.0382 Rook7thOp:32.1981 Rook7thEg:31.6357 BadRook:31.2822 KnightFork:32.3783 BishopFork:31.7289 KnightAdj:15.5194 RookAdj:16.6314 EarlyQueen:14.9579 ContactCheckR:150.2863 ContactCheckQ:189.8956 Shield1:15.7206 Shield2:16.0318 PasserK:32.0105 Candidate:121.1584 Passer:207.0763 Supported:126.9759 Unstoppable:848.0688 FreePasser:63.8476 Xray:31.9373 PinMul:32.1988 ThreatPawn:31.6163 ThreatL_P:62.9639 ThreatL_L:63.4178 ThreatL_H:64.1387 ThreatL_K:63.3076 ThreatR_L:63.0344 ThreatR_K:63.6618 ThreatQ_1:62.9848 Tempo:16.5616";

  // 58000 iterations
  Str SPSA4 = "MatPawnOp:96.5998 MatKnightOp:445.8089 MatBishopOp:450.2728 MatRookOp:750.1254 MatQueenOp:1436.3193 MatPawnEg:93.9483 MatKnightEg:449.2836 MatBishopEg:449.4913 MatRookEg:743.3024 MatQueenEg:1436.7573 PawnFile:7.9796 KnightCenterOp:7.6336 KnightCenterEg:7.8464 KnightRank:7.8322 KnightBackRank:7.8606 KnightTrapped:128.0743 BishopCenterOp:8.0785 BishopCenterEg:7.5474 BishopBackRank:7.9336 BishopDiagonal:7.8955 RookFileOp:7.6536 QueenCenterOp:7.7990 QueenCenterEg:7.9098 QueenBackRank:7.9505 KingFile:15.4325 KingRank:15.4788 KingCenterEg:9.0643 Doubled:15.7706 Isolated:15.8152 Backward:14.9080 Connected:14.6531 WeaknessPush:30.8162 NMobMult:119.5800 BMobMult:114.6063 RMobMult:121.5924 QMobMult:174.9422 NMobSteep:14.4618 BMobSteep:14.2657 RMobSteep:14.9164 QMobSteep:14.7968 BishopPair:32.1729 BadBishop:32.2889 RammedBishop:31.8691 KnightOutpost:31.5787 RookSemi:31.6405 RookOpen:30.9461 Rook7thOp:32.1740 Rook7thEg:31.6304 BadRook:31.2672 KnightFork:32.3270 BishopFork:31.8144 KnightAdj:15.4652 RookAdj:16.7418 EarlyQueen:14.8231 ContactCheckR:150.1968 ContactCheckQ:189.8959 Shield1:15.6700 Shield2:16.0483 PasserK:31.8628 Candidate:120.2396 Passer:201.1446 Supported:126.9439 Unstoppable:848.1103 FreePasser:63.7815 Xray:31.9809 PinMul:32.1048 ThreatPawn:31.4693 ThreatL_P:63.0621 ThreatL_L:63.4241 ThreatL_H:64.4148 ThreatL_K:63.2700 ThreatR_L:63.1554 ThreatR_K:63.5611 ThreatQ_1:63.0354 Tempo:15.2939";

  Str Book = ".\\datasets\\Perfect_2011.pgn";
};

}
