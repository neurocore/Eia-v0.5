#pragma once
#include <vector>
#include <string>
#include <random>
#include "moves.h"

namespace eia {

// Tree structure in one monotonic
//  memory block, it's simple and
//  clean and good for caching

class Book
{
  struct Entry
  {
    Move move;
    std::string str;
    // TODO: more data?

    std::vector<size_t> children;
  };

  std::vector<Entry> entries;

public:
  Book();
  bool read_pgn(std::string pgn);
  bool read_abk(std::string abk);
  Moves get_random_line();
  void print_some(int depth, int ply = 0, size_t pos = 0) const;

private:
  void parse_line(std::vector<std::string> line);
  size_t add_move(size_t pos, Move move);
  inline size_t get_next_pos() const;

  using Distr = std::uniform_int_distribution<int>;
  using param_t = Distr::param_type;
  std::mt19937 gen;
  Distr distr;
};

}
