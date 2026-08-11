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
  M0ves = 256,
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
  Str M_30 = "5n2/B3K3/2p2Np1/4k3/7P/3bN1P1/2Prn1P1/1q6 w - - 1 1"; // knights dance
};

namespace Tunes
{
  Str Book = ".\\datasets\\Perfect_2011.pgn";

  //const double K100 = 0.002598433573529394; // 1m positions (BCE)
    const double K100 = 0.004647211884938312; // 1m positions (MSE)
 
  //const double K100 = 0.004377508185305807; // 100k positions
};

}
