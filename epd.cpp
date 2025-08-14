#include <fstream>
#include "epd.h"

using namespace std;

namespace eia {

bool Epd::read(std::string file)
{
  ifstream fin(file);
  if (!fin.is_open())
  {
    say("Can't open \"{}\" as epd\n", file);
    return false;
  }

  for (string line; getline(fin, line);)
  {
    Problem problem = parse_problem(line);
    problems.push_back(problem);
  }
  return true;
}

Problem Epd::parse_problem(std::string str)
{
  Problem problem;
  Board B;

  for (int i = 0; i < 4; ++i) problem.fen += cut(str) + " ";
  if (!B.set(problem.fen)) return problem;

  auto ops = split(str, "; ");
  for (string op : ops)
  {
    bool done = false;
    string code; // meet first correct command
    while (!(code = cut(op)).empty() && !done)
    {
      done = true;

      if (code == "bm")
      {
        Move move = B.parse_san(op);
        if (move != Move::None)
          problem.best[move] = 0;
      }
      else if (code == "am")
      {
        Move move = B.parse_san(op);
        if (move != Move::None)
          problem.avoid = move;
      }
      else if (code == "id")
      {
        problem.id = trim(op, '"');
      }
      else if (code == "acd")
      {
        problem.depth = parse_int(op, 0);
      }
      else if (code == "acn")
      {
        problem.nodes = parse_int(op, 0);
      }
      else if (code == "acs")
      {
        problem.secs = parse_int(op, 0);
      }
      else if (code == "ce")
      {
        problem.eval = parse_int(op, 0);
      }
      else if (code == "hmvc")
      {
        problem.fifty = parse_int(op, 0);
      }
      else if (code.length() == 2
           &&  code[0] == 'c'
           &&  isdigit(code[1]))
      {
        int i = code[1] - '0';
        problem.comment[i] = trim(op, '"');
      }
      else if (code.length() == 2
           &&  code[0] == 'D'
           &&  isdigit(code[1]))
      {
        int i = code[1] - '0';
        problem.perft[i] = parse_int(op, 0);
      }
      else
      {
        done = false;
      }
    }
  }

  // STS-like quiz

  if (!problem.comment[8].empty()  // scores: "10 5 3"
  &&  !problem.comment[9].empty()) // moves: "f4f5 d4f2 f3g4"
  {
    auto scores = split(problem.comment[8]);
    auto moves  = split(problem.comment[9]);
    bool sync = scores.size() == moves.size();

    for (int i = 0; i < moves.size(); ++i)
    {
      Move move = to_move(moves[i]);
      move = B.recognize(move);

      if (move != Move::None)
      {
        int val = sync ? parse_int(scores[i], 0) : 0;
        problem.best[move] = val;
      }
    }
  }

  problem.valid = true;
  return problem;
}

}
