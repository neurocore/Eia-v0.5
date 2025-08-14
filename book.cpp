#include <cassert>
#include <fstream>
#include "book.h"
#include "board.h"

using namespace std;

namespace eia {

Book::Book() : gen(random_device{}()) {}

bool Book::read_pgn(std::string pgn)
{
  entries.clear();
  entries.push_back({Move::None, {}});

  ifstream fin(pgn);
  if (!fin.is_open())
  {
    say("Can't open \"{}\" as pgn book\n", pgn);
    return false;
  }

  // Game lines in pgn have fixed width
  //  so we need to read it token by token

  vector<string> line;
  while(!fin.eof())
  {
    string str;
    fin >> str;
    dry(str);

    auto i = str.find('.'); // Trimming move numbers
    if (i != string::npos)
    {
      string num = str.substr(0, i);
      str = str.substr(i + 1, str.length());

      if (num == "1") // Start of the line
      {
         if (!line.empty())
           parse_line(line);
         line.clear();
      }
    }
    line.push_back(str);
  }
  return entries.size() > 1;
}

bool Book::read_abk(std::string abk)
{
  entries.clear();
  entries.push_back({Move::None, {}});

  ifstream fin(abk);
  if (!fin.is_open())
  {
    say("Can't open \"{}\" as abk book\n", abk);
    return false;
  }

  // TODO: read abk

  return true;
}

void Book::parse_line(vector<string> line)
{
  static Board B;
  B.set();

  size_t pos = 0;
  for (auto part : line)
  {
    const Move move = to_move(part);
    const Move mv = B.recognize(move);

    if (mv == Move::None)
    {
      B.print();
      say("Move {} isn't legal!\n", move);
      assert(false);
      return;
    }

    pos = add_move(pos, move);
    B.make(mv);
  }
}

size_t Book::add_move(size_t pos, Move move)
{
  Entry & parent = entries[pos];

  for (size_t j : parent.children)
  {
    if (entries[j].move == move)
      return j;
  }

  pos = get_next_pos();
  parent.children.push_back(pos);
  entries.push_back({move, to_string(move), {}});

  assert(pos == entries.size() - 1);
  return pos;
}

inline size_t Book::get_next_pos() const
{
  return entries.size();
}

Moves Book::get_random_line()
{
  Moves moves;
  size_t pos = 0;

  for (int i = 0; i < 1000; ++i)
  {
    const Entry & entry = entries[pos];
    if (pos) moves.push_back(entry.move);

    const size_t cnt = entry.children.size();
    if (cnt <= 0) break;

    distr.param(param_t(0, static_cast<size_t>(cnt - 1)));
    const size_t j = distr(gen);
    pos = entry.children[j];
  }
  return moves;
}

void Book::print_some(int depth, int ply, size_t pos) const
{
  if (depth <= 0) return;
  const Entry & parent = entries[pos];

  for (size_t j : parent.children)
  {
    for (int k = 0; k < ply; k++) say("  ");
    say("{}\n", entries[j].move);

    print_some(depth - 1, ply + 1, j);
  }
}

}
