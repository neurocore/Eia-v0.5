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
  Str Fine = "8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - -"; // Kb1!
  Str Corr = "8/8/8/1p6/1P6/3P1k2/3K4/8 w - - 0 1"; // Kc2!
  Str See1 = "1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - -"; // Re1e5?
  Str See2 = "1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - -"; // Nd3e5?
  Str Mith = "8/k7/P2b2P1/KP1Pn2P/4R3/8/6np/8 w - - 0 1"; // b6! Re1!!
  Str Mine = "8/8/8/4k3/4B3/6p1/5bP1/4n2K b - - 0 1"; // Bg1!! Bh2! ... Nf2#
};

namespace Tunes
{
  Str Def = "MatPawnOp:234 MatKnightOp:408 MatBishopOp:560 MatRookOp:1162 MatQueenOp:2022 MatPawnEg:214 MatKnightEg:534 MatBishopEg:518 MatRookEg:852 MatQueenEg:1790 PawnFile:10 KnightCenterOp:10 KnightCenterEg:20 KnightRank:6 KnightBackRank:2 KnightTrapped:374 BishopCenterOp:16 BishopCenterEg:8 BishopBackRank:22 BishopDiagonal:16 RookFileOp:6 QueenCenterOp:20 QueenCenterEg:22 QueenBackRank:14 KingFile:24 KingRank:38 KingCenterEg:6 Doubled:20 Isolated:30 Backward:26 Connected:34 WeaknessPush:38 NMobMult:264 BMobMult:220 RMobMult:120 QMobMult:398 NMobSteep:56 BMobSteep:18 RMobSteep:54 QMobSteep:6 BishopPair:44 BadBishop:50 RammedBishop:32 KnightOutpost:40 RookSemi:78 RookOpen:16 Rook7thOp:70 Rook7thEg:88 BadRook:26 KnightFork:88 BishopFork:24 KnightAdj:24 RookAdj:58 EarlyQueen:30 ContactCheckR:100 ContactCheckQ:180 Shield1:16 Shield2:58 PasserK:48 Candidate:200 Passer:1006 Supported:128 Unstoppable:956 FreePasser:104 Xray:6 PinMul:84 ThreatPawn:24 ThreatL_P:100 ThreatL_L:112 ThreatL_H:236 ThreatL_K:166 ThreatR_L:118 ThreatR_K:0 ThreatQ_1:0 Tempo:64";
  
  // 80000 iters (1, .1, 100 | 3h) +7 elo h2h-40
  Str SPSA1 = "MatPawnOp:102.9390 MatKnightOp:495.6238 MatBishopOp:528.9422 MatRookOp:741.1415 MatQueenOp:1489.2236 MatPawnEg:109.9711 MatKnightEg:476.7209 MatBishopEg:429.5764 MatRookEg:757.5353 MatQueenEg:1495.9585 PawnFile:6.3754 KnightCenterOp:4.6830 KnightCenterEg:6.0712 KnightRank:6.6471 KnightBackRank:7.3224 KnightTrapped:123.7949 BishopCenterOp:5.8907 BishopCenterEg:4.0955 BishopBackRank:7.8955 BishopDiagonal:6.0962 RookFileOp:7.4411 QueenCenterOp:4.1937 QueenCenterEg:6.9536 QueenBackRank:6.4046 KingFile:16.2326 KingRank:15.7900 KingCenterEg:13.5898 Doubled:12.9877 Isolated:12.3360 Backward:3.2707 Connected:7.6393 WeaknessPush:29.5432 NMobMult:58.1234 BMobMult:91.6174 RMobMult:50.9788 QMobMult:160.9159 NMobSteep:9.7145 BMobSteep:12.6118 RMobSteep:8.3578 QMobSteep:3.4750 BishopPair:39.5647 BadBishop:31.4268 RammedBishop:30.5293 KnightOutpost:31.3256 RookSemi:18.5593 RookOpen:29.1275 Rook7thOp:30.9059 Rook7thEg:34.0291 BadRook:29.5934 KnightFork:27.6345 BishopFork:31.0512 KnightAdj:16.1834 RookAdj:7.6082 EarlyQueen:-5.5110 ContactCheckR:157.1333 ContactCheckQ:191.5707 Shield1:16.6388 Shield2:19.2340 PasserK:31.6448 Candidate:135.8530 Passer:177.4820 Supported:80.8580 Unstoppable:848.3423 FreePasser:63.0926 Xray:29.9682 PinMul:32.4749 ThreatPawn:30.8608 ThreatL_P:68.4522 ThreatL_L:37.0866 ThreatL_H:55.0792 ThreatL_K:57.5149 ThreatR_L:62.9046 ThreatR_K:55.1021 ThreatQ_1:64.4827 Tempo:39.8164";
  
  // 3400 iters (1, .1, 100 | 1h) Ethereal book 100k
  Str SPSA_Eth1 = "MatKnightOp:422.1101 MatBishopOp:448.2621 MatRookOp:621.3595 MatQueenOp:1266.9741 MatPawnEg:138.9058 MatKnightEg:484.4485 MatBishopEg:497.3356 MatRookEg:836.5337 MatQueenEg:1595.4962 PawnFile:4.8243 KnightCenterOp:4.4254 KnightCenterEg:5.1195 KnightRank:4.7749 KnightBackRank:-0.1280 KnightTrapped:100.3582 BishopCenterOp:2.3594 BishopCenterEg:2.9651 BishopBackRank:10.0134 BishopDiagonal:3.2385 RookFileOp:2.6796 QueenCenterOp:-0.0664 QueenCenterEg:4.0253 QueenBackRank:5.0908 KingFile:11.2410 KingRank:9.7997 KingCenterEg:18.9052 Doubled:10.0417 Isolated:9.1925 Backward:9.7627 Connected:2.8678 WeaknessPush:39.4301 NMobMult:84.4111 BMobMult:90.7755 RMobMult:123.0811 QMobMult:248.0471 NMobSteep:12.3363 BMobSteep:12.8046 RMobSteep:9.0547 QMobSteep:5.7889 BishopPair:15.3414 BadBishop:37.9714 RammedBishop:8.3299 KnightOutpost:9.2936 RookSemi:9.2018 RookOpen:22.3960 Rook7thOp:20.4585 Rook7thEg:14.0140 BadRook:21.4096 KnightFork:20.2959 BishopFork:12.8743 KnightAdj:2.4666 RookAdj:4.2240 EarlyQueen:4.4015 ContactCheckR:99.1824 ContactCheckQ:180.0991 Shield1:10.4913 Shield2:5.7729 PasserK:28.6138 Candidate:103.7882 Passer:140.7254 Supported:87.8018 Unstoppable:797.7776 FreePasser:60.1912 Xray:33.9767 PinMul:11.7021 ThreatPawn:16.2786 ThreatL_P:56.9661 ThreatL_L:30.5472 ThreatL_H:33.2952 ThreatL_K:41.5175 ThreatR_L:47.9028 ThreatR_K:33.6434 ThreatQ_1:49.8171 Tempo:13.1693";

  // 8400 iters (1, .1, 100 | 3h) Ethereal book 100k
  Str SPSA_Eth2 = "MatKnightOp:422.8339 MatBishopOp:446.6869 MatRookOp:610.2199 MatQueenOp:1267.9169 MatPawnEg:137.4901 MatKnightEg:488.0714 MatBishopEg:496.1531 MatRookEg:837.4840 MatQueenEg:1590.8413 PawnFile:4.8215 KnightCenterOp:4.2837 KnightCenterEg:5.1900 KnightRank:4.7129 KnightBackRank:-0.1392 KnightTrapped:100.5865 BishopCenterOp:2.5246 BishopCenterEg:2.7685 BishopBackRank:10.0006 BishopDiagonal:2.8802 RookFileOp:2.6576 QueenCenterOp:-0.1692 QueenCenterEg:3.9991 QueenBackRank:5.0344 KingFile:11.6397 KingRank:9.6479 KingCenterEg:18.3171 Doubled:9.9325 Isolated:9.3248 Backward:8.6874 Connected:2.5722 WeaknessPush:39.3270 NMobMult:79.7248 BMobMult:89.4329 RMobMult:121.2640 QMobMult:248.1615 NMobSteep:11.9096 BMobSteep:12.5714 RMobSteep:8.9071 QMobSteep:5.2479 BishopPair:16.1626 BadBishop:38.0917 RammedBishop:8.3044 KnightOutpost:9.4231 RookSemi:8.8690 RookOpen:23.1463 Rook7thOp:20.7181 Rook7thEg:14.5062 BadRook:21.5343 KnightFork:20.3483 BishopFork:12.7941 KnightAdj:2.2767 RookAdj:4.1893 EarlyQueen:4.0422 ContactCheckR:99.2635 ContactCheckQ:179.9933 Shield1:10.5626 Shield2:6.1785 PasserK:29.8107 Candidate:104.5159 Passer:141.9436 Supported:84.5538 Unstoppable:797.5138 FreePasser:59.9957 Xray:30.2256 PinMul:11.8954 ThreatPawn:18.1043 ThreatL_P:56.9561 ThreatL_L:32.3050 ThreatL_H:34.6113 ThreatL_K:41.4543 ThreatR_L:46.5523 ThreatR_K:33.9453 ThreatQ_1:49.6344 Tempo:10.8519";

  // 11200 iters (1, .1, 100 | 4h) Ethereal book 100k
  Str SPSA_Eth3 = "MatKnightOp:422.6846 MatBishopOp:445.8542 MatRookOp:607.9214 MatQueenOp:1267.4793 MatPawnEg:136.9861 MatKnightEg:489.4614 MatBishopEg:496.1136 MatRookEg:838.7924 MatQueenEg:1591.2172 PawnFile:4.8130 KnightCenterOp:4.2574 KnightCenterEg:5.2306 KnightRank:4.7032 KnightBackRank:-0.1693 KnightTrapped:100.8663 BishopCenterOp:2.5792 BishopCenterEg:2.7015 BishopBackRank:9.9958 BishopDiagonal:2.7566 RookFileOp:2.6399 QueenCenterOp:-0.1964 QueenCenterEg:3.9954 QueenBackRank:5.0082 KingFile:11.8216 KingRank:9.5177 KingCenterEg:18.1561 Doubled:9.8543 Isolated:9.4022 Backward:8.3046 Connected:2.5221 WeaknessPush:39.3487 NMobMult:78.5749 BMobMult:89.0735 RMobMult:120.8407 QMobMult:248.0132 NMobSteep:11.7710 BMobSteep:12.5636 RMobSteep:8.9276 QMobSteep:5.0676 BishopPair:16.5088 BadBishop:38.0843 RammedBishop:8.2675 KnightOutpost:9.3945 RookSemi:8.7426 RookOpen:23.4433 Rook7thOp:20.8434 Rook7thEg:14.6944 BadRook:21.5330 KnightFork:20.3731 BishopFork:12.7830 KnightAdj:2.2288 RookAdj:4.1634 EarlyQueen:3.9452 ContactCheckR:99.2997 ContactCheckQ:179.9921 Shield1:10.5974 Shield2:6.3418 PasserK:30.1760 Candidate:105.0056 Passer:143.4397 Supported:83.7116 Unstoppable:797.0453 FreePasser:60.0588 Xray:28.8131 PinMul:11.9259 ThreatPawn:18.7841 ThreatL_P:56.8911 ThreatL_L:32.6167 ThreatL_H:34.9982 ThreatL_K:41.2372 ThreatR_L:46.1421 ThreatR_K:34.1334 ThreatQ_1:49.3877 Tempo:10.1175";

  // 16000 iters (1, .1, 100 | 6h) Ethereal book 100k | +20 elo (2s+.2 h2h-20)
  Str SPSA_Eth4 = "MatKnightOp:422.7620 MatBishopOp:444.6179 MatRookOp:604.0623 MatQueenOp:1268.0675 MatPawnEg:136.4094 MatKnightEg:490.4885 MatBishopEg:496.2202 MatRookEg:839.6476 MatQueenEg:1591.3225 PawnFile:4.8060 KnightCenterOp:4.2218 KnightCenterEg:5.2927 KnightRank:4.6927 KnightBackRank:-0.1796 KnightTrapped:100.8849 BishopCenterOp:2.6750 BishopCenterEg:2.6091 BishopBackRank:9.9855 BishopDiagonal:2.6099 RookFileOp:2.6082 QueenCenterOp:-0.2077 QueenCenterEg:4.0111 QueenBackRank:4.9795 KingFile:12.0048 KingRank:9.3856 KingCenterEg:17.8907 Doubled:9.7510 Isolated:9.5294 Backward:7.8525 Connected:2.4346 WeaknessPush:39.4445 NMobMult:77.1465 BMobMult:88.9233 RMobMult:120.0937 QMobMult:248.0363 NMobSteep:11.6511 BMobSteep:12.5084 RMobSteep:8.8522 QMobSteep:4.8886 BishopPair:16.9668 BadBishop:37.9505 RammedBishop:8.1776 KnightOutpost:9.4098 RookSemi:8.5968 RookOpen:23.7821 Rook7thOp:21.0512 Rook7thEg:15.0002 BadRook:21.5734 KnightFork:20.4026 BishopFork:12.8140 KnightAdj:2.1108 RookAdj:4.1455 EarlyQueen:3.7832 ContactCheckR:99.2997 ContactCheckQ:180.0068 Shield1:10.6415 Shield2:6.5634 PasserK:30.5687 Candidate:105.2039 Passer:144.3839 Supported:82.2003 Unstoppable:796.9241 FreePasser:60.0606 Xray:26.9215 PinMul:11.9887 ThreatPawn:19.6310 ThreatL_P:56.7151 ThreatL_L:33.0645 ThreatL_H:35.8308 ThreatL_K:41.1037 ThreatR_L:45.3179 ThreatR_K:33.9797 ThreatQ_1:49.1802 Tempo:9.4533";

  // TODO: use whole dataset (10m positions) to fine tune

  Str Book = ".\\datasets\\Perfect_2011.pgn";
};

}
