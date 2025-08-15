#include <iostream>
#include <format>
#include "engine.h"
#include "solver_pvs.h"
#include "tuning.h"

using namespace std;

namespace eia {

Engine::Engine()
{
  S[0] = new SolverPVS(new Eval);
  S[1] = new Reader();
  cout.sync_with_stdio(false);
  cin.sync_with_stdio(false);
}

Engine::~Engine()
{
  delete S[1];
  delete S[0];
}

void Engine::print_message(string str)
{
  cout << str << endl;
  cout.flush();
}

void Engine::print_info(string str)
{
  if (options.flag_debug)
  {
    cout << "info string" << str << endl;
    cout.flush();
  }
}

void Engine::start()
{
  new_game();

  bool success = true;
  while (success)
  {
    std::string str;
    getline(cin, str);

    if (str.length() > 0)
    {
      success = parse(str);
    }
  }
}

bool Engine::parse(string str)
{
  string cmd = cut(str);

  if (cmd == "uci")
  {
    print_message(format
    (
      "id name {} v{}\nid author {}\n{}uciok",
      Name, Vers, Auth, options
    ));
  }
  else if (cmd == "isready") [[likely]]
  {
    print_message("readyok");
  }
  else if (cmd == "quit")
  {
    return false;
  }
  else if (cmd == "ucinewgame")
  {
    new_game();
  }
  else if (cmd == "stop") [[likely]]
  {
    stop();
  }
  else if (cmd == "perft") [[unlikely]]
  {
    string part = cut(str);
    int depth = parse_int(part);
    if (depth < 1) depth = 1;
    perft(depth);
  }
  else if (cmd == "plegt") [[unlikely]]
  {
    string part = cut(str);
    int unused = parse_int(part);
    plegt();
  }
  else if (cmd == "eval") [[unlikely]]
  {
    string part = cut(str);
    int unused = parse_int(part);
    eval();
  }
  else if (cmd == "debug") [[unlikely]]
  {
    string part = cut(str);
    if (part == "on") set_debug(true);
    if (part == "off") set_debug(false);
  }
  else if (cmd == "register") [[unlikely]]
  {
    print_message("registration ok");
  }
  else if (cmd == "position") [[likely]]
  {
    string fen;
    string op = cut(str);

    if (op == "startpos") fen = Pos::Init;
    else if (op == "fen") fen = cut(str, "moves");

    string part = cut(str); // reading out "moves"
    if (part != "moves") str = part + str;

    vector<Move> moves;
    while (true)
    {
      part = cut(str);
      if (part.length() <= 0) break;
      Move move = to_move(part);
      moves.emplace_back(move);
    }
    set_pos(fen, moves);
  }
  else if (cmd == "go") [[likely]]
  {
    SearchCfg cfg;

    while(true)
    {
      string part = cut(str);
      if (part.empty()) break;

      if      (part == "wtime")    cfg.time[1] = parse_int(cut(str), Time::Def);
      else if (part == "btime")    cfg.time[0] = parse_int(cut(str), Time::Def);
      else if (part == "winc")     cfg.inc[1] = parse_int(cut(str), Time::Inc); 
      else if (part == "binc")     cfg.inc[0] = parse_int(cut(str), Time::Inc); 
      else if (part == "depth")    cfg.depth = parse_int(cut(str), Time::Inc); 
      else if (part == "infinite") cfg.infinite = true; 
      else break;
    }
    go(cfg);
  }
  else if (cmd == "tune") [[unlikely]]
  {
    tune();
  }
  else if (cmd == "")
  {

  }

  return true;
}

void Engine::new_game()
{
  B.set();
  S[0]->set(B);
  S[1]->set(B);
  S[0]->new_game();
  S[1]->new_game();
}

void Engine::stop()
{
  S[0]->stop();
  S[1]->stop();
}

void Engine::perft(int depth)
{
  S[0]->set(B);
  S[1]->set(B);
  S[0]->perft(depth);
  S[1]->perft(depth);
}

void Engine::plegt()
{
  S[0]->set(B);
  S[1]->set(B);
  S[0]->plegt();
  S[1]->plegt();
}

void Engine::eval()
{
  Eval E;

  E.set_explanations(true);
  int val = E.eval(&B, -Val::Inf, Val::Inf);
  E.set_explanations(false);

  string str = format("Eval: {}\n\n", val);

#ifdef _DEBUG
  auto & details = E.get_details();
  for (const auto & d : details)
  {
    if (d.vals.eg == 0 && d.vals.op == 0) continue;
    str += format("{} {} {} {}\n", d.p, d.sq, d.vals, d.factor);
  }
#else
  str += "Use in debug mode to get details\n";
#endif

  print_message(str);
}

void Engine::set_debug(bool val)
{
  options.flag_debug = val;
}

void Engine::set_pos(string fen, std::vector<Move> moves)
{
  stop();
  B.set(fen);

  for (auto move : moves)
  {
    if (!do_move(move))
    {
      print_message(format("can't perform move \"{}\"", move));
      break;
    }
  }
  B.print();
}

bool Engine::do_move(Move mv)
{
  Move move = B.recognize(mv);
  if (move == Move::None) return false;

  bool success = B.make(move);
  B.revert_states();

  return success;
}

void Engine::go(const SearchCfg & cfg)
{
  for (auto solver : S)
  {
    solver->set(B);
    solver->get_move(cfg);
  }
}

void Engine::tune()
{
  Eval E;
  const int bits = E.get_total_bits();
  
  Tuning T(bits, 100, 10, 2);
  T.start();
}

}
