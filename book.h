#pragma once
#include <vector>
#include <string>
#include <random>
#include "moves.h"
#include "solver.h"

namespace eia {

// Tree structure in one monotonic
//  memory block, it's simple and
//  clean and good for caching

PACKED__
struct ABK_Entry
{
  char from;
  char to;
  char promotion; /* 0 none, +-1 rook, +-2 knight, +-3 bishop, +-4 queen */
  char priority;
  int ngames;
  int nwon;
  int nlost;
  int plycount;
  int next_move;
  int next_sibling;
};__PACKED


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
  void clear() { entries.clear(); }
  Moves get_random_line();
  void print_some(int depth, int ply = 0, size_t pos = 0) const;

private:
  using Distr = std::uniform_int_distribution<int>;
  using param_t = Distr::param_type;
  std::mt19937 gen;
  Distr distr;

  friend class BookReader;
};


class BookReader
{
  Book * book;

public:
  BookReader(Book * book) : book(book) {}
  bool read_pgn(std::string pgn);
  bool read_abk(std::string abk);
  
private:
  void parse_line(std::vector<std::string> line);
  void traverse_abk(size_t abk, size_t pos);
  size_t add_move(size_t pos, Move move);
  inline size_t get_next_pos() const;

  Board B;
  std::vector<ABK_Entry> abk_list;
};

}
