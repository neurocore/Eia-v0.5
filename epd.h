#pragma once
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include "board.h"

namespace eia {

using MoveScores = std::unordered_map<Move, int>;
using MoveSet = std::unordered_set<Move, int>;

struct Problem
{
  bool valid = false;
  std::string fen, id;
  std::string comment[10];
  MoveScores best;
  Move avoid;
  int perft[10] = {0, };
};

class Epd
{
  std::vector<Problem> problems;

public:
  bool read(std::string file);

private:
  Problem parse_problem(std::string str);
};

}
