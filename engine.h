#pragma once
#include "options.h"
#include "solver.h"
#include "board.h"

namespace eia {

class Engine
{
  Options options;
  Solver * S[2];
  Board B;

public:
  Engine();
  ~Engine();
  void print_message(std::string str);
  void print_info(std::string str);

  void start();
  bool parse(std::string str);

  // commands
  void new_game();
  void stop();
  void perft(int depth = 1);
  void plegt();
  void evalt(int depth = 6);
  void eval();
  void set_debug(bool val);
  void set_pos(std::string fen, std::vector<Move> moves);
  bool do_move(Move mv);
  void go(const SearchCfg & cfg);
  void tune();
};

}
