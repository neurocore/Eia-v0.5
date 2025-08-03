#pragma once
#include "options.h"
#include "solver.h"
#include "board.h"

namespace eia {

struct SearchParams
{
  MS time[2] = { Time::Def, Time::Def };
  MS inc[2]  = { Time::Inc, Time::Inc };
  bool infinite = false;

  INLINE MS full_time(Color c) const { return time[c] + inc[c]; }

  // not supported by engine
  Move searchmoves = Move::None;
  bool ponder = false;
  int movestogo = Val::Inf, depth = Val::Inf;
  int nodes = Val::Inf, mate = Val::Inf;
  MS movetime = limits<i64>::max();
};

class Engine
{
  Options options;
  Solver * S[2];
  Undo undos[2];
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
  void go(const SearchParams sp);
};

}
