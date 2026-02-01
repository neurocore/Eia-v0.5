#include <iostream>
#include <format>
#include <memory>
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

void Engine::print_info(string str)
{
  if (options.flag_debug)
  {
    say<1>("info string {}\n", str);
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
    say<1>("id name {} v{}\nid author {}\n{}uciok\n",
            Name, Vers, Auth, options);
  }
  else if (cmd == "isready") [[likely]]
  {
    say<1>("readyok\n");
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
    say<1>("registration ok\n");
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
  else if (cmd == "tune")
  {
    tune();
  }
  else
  {
    say<1>("Unknown command \"{}\"\n", cmd);
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
  Val val = E.eval(&B, -Val::Inf, Val::Inf);
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

  say<1>("{}", str);
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
      say<1>("can't perform move \"{}\"", move);
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
  say<1>("-- Tune mode\n");

  Eval eval;
  TunerCfg cfg{.verbose = false};
  Tuner tuner(cfg);
  tuner.init();

  bool success = true;
  while (success)
  {
    std::string str;
    getline(cin, str);

    if (str.length() > 0)
    {
      string cmd = cut(str);

      if (cmd == "get")
      {
        string op = cut(str);

        if      (op == "games") say<1>("{}\n", tuner.cfg.games);
        else if (op == "depth") say<1>("{}\n", tuner.cfg.depth);
        else if (op == "eval")  say<1>("{}\n", eval.to_raw());
        else say<1>("Wrong operator \"{}\" in \"get\"\n", op);
      }
      else if (cmd == "set")
      {
        string op = cut(str);

        if      (op == "games") tuner.cfg.games = parse_int(str, 10);
        else if (op == "depth") tuner.cfg.depth = parse_int(str, 3);
        else if (op == "eval")  eval.set_raw(str);
        else say<1>("Wrong operator \"{}\" in \"set\"\n", op);
      }
      else if (cmd == "score")
      {
        auto score = tuner.score(eval);
        say<1>("{}\n", score);
      }
      else if (cmd == "exit")
      {
        success = false;
      }
      else
      {
        say<1>("Wrong command \"{}\" in tune mode \n", cmd);
        say<1>("Use \"exit\" to leave to main mode\n");
      }
    }
  }

  say<1>("-- Main mode\n");
}

}
