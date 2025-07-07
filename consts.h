#pragma once
#include <string>

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
  Moves = 256,
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
  Size = 128
};

namespace Pos
{
  const std::string Init = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  const std::string Fine = "8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - -";
  const std::string Kiwi = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
  const std::string See1 = "1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - -"; // Re1e5?
  const std::string See2 = "1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - -"; // Nd3e5?
  const std::string Mith = "8/k7/P2b2P1/KP1Pn2P/4R3/8/6np/8 w - - 0 1"; // b6! Re1!!
  const std::string Mine = "8/8/8/4k3/4B3/6p1/5bP1/4n2K b - - 0 1"; // Bg1!! Bh2! ... Nf3!
};

namespace Tune
{
  const std::string Def = std::string()
                        + "MatKnight:320 "
                        + "MatBishop:330 "
                        + "MatRook:500 "
                        + "MatQueen:900 "
                        + "PawnFile:5 "
                        + "KnightCenterOp:5 "
                        + "KnightCenterEg:5 "
                        + "KnightRank:5 "
                        + "KnightBackRank:0 "
                        + "KnightTrapped:100 "
                        + "BishopCenterOp:2 "
                        + "BishopCenterEg:3 "
                        + "BishopBackRank:10 "
                        + "BishopDiagonal:4 "
                        + "RookFileOp:3 "
                        + "QueenCenterOp:0 "
                        + "QueenCenterEg:4 "
                        + "QueenBackRank:5 "
                        + "KingFile:10 "
                        + "KingRank:10 "
                        + "KingCenterEg:20 "
                        + "Doubled:10 "
                        + "Isolated:9 "
                        + "Backward:12 "
                        + "WeaknessPush:40 "
                        + "NMob:64 "
                        + "BMob:64 "
                        + "RMob:48 "
                        + "QMob:16 "
                        + "BishopPair:13 "
                        + "BadBishop:38 "
                        + "KnightOutpost:10 "
                        + "RookSemi:10 "
                        + "RookOpen:20 "
                        + "Rook7thOp:20 "
                        + "Rook7thEg:12 "
                        + "BadRook:20 "
                        + "KnightFork:20 "
                        + "BishopFork:13 "
                        + "KnightAdj:4 "
                        + "RookAdj:3 "
                        + "EarlyQueen:3 "
                        + "ContactCheckR:100 "
                        + "ContactCheckQ:180 "
                        + "Shield1:10 "
                        + "Shield2:5 "
                        + "PasserK:22 "
                        + "Candidate:100 "
                        + "Passer:200 "
                        + "Unstoppable:800 "
                        + "Supported:100 "
                        + "FreePasser:60 "
                        + "XrayMul:12 "
                        + "Tempo:15 ";
};
