#include "bitboard.h"

using namespace std;

ostream & operator << (ostream & os, const BitBoard & bb)
{
  for (int y = 7; y >= 0; --y)
  {
    os << "  ";
    for (int x = 0; x < 8; ++x)
    {
      SQ sq = to_sq(x, y);
      u64 bit = Bit << sq;
      char ch = (bb.val & bit) ? '*' : '.';
      os << ch;
    }
    os << endl;
  }
  os << endl;
  return os;
}

void print64(u64 bb)
{
  cout << BitBoard{bb} << endl;
}

u64 shift(u64 bb, Dir dir)
{
  switch (dir)
  {
    case Dir::U  : return shift_u(bb);
    case Dir::D  : return shift_d(bb);
    case Dir::L  : return shift_l(bb);
    case Dir::R  : return shift_r(bb);
    case Dir::UL : return shift_ul(bb);
    case Dir::UR : return shift_ur(bb);
    case Dir::DL : return shift_dl(bb);
    case Dir::DR : return shift_dr(bb);
    default      : return bb;
  }
}
