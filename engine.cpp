#include <iostream>
#include <format>
#include "engine.h"
#include "solver_pvs.h"

using namespace std;

namespace eia {

Engine::Engine()
{
  S[0] = new SolverPVS(this);
  S[1] = new Reader(this);
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
  //logf("%v", options);

  /*B.set("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
  B.print();
  B.pseudolegal(Move(0x178E));*/

  new_game();
  //log(B);

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
    SearchParams sp;

    string part;
    while(!(part = cut(str)).empty())
    {
      if      (part == "wtime") { sp.time[1] = parse_int(cut(str), Time::Def); break; }
      else if (part == "btime") { sp.time[0] = parse_int(cut(str), Time::Def); break; }
      else if (part == "winc")  { sp.inc[1] = parse_int(cut(str), Time::Inc); break; }
      else if (part == "binc")  { sp.inc[0] = parse_int(cut(str), Time::Inc); break; }
      else if (part == "infinite") { sp.infinite = true; break; }
      else break;
    }
    go(sp);
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
  S[0]->set(B);
  S[1]->set(B);

  for (int i = 0; i < 2; ++i)
  {
    if (!S[i]->is_solver()) continue;

    SolverPVS * PVS = dynamic_cast<SolverPVS *>(S[i]);

    int val = PVS->eval();
    string str = format("Eval: {}\n\n", val);

    auto & details = PVS->eval_details();
    for (const auto & d : details)
    {
      str += format("{} {} {} {}\n", d.p, d.sq, d.vals, d.factor);
    }
    print_message(str);
  }
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
  Undo * undo = &undos[0];
  Move move = B.recognize(mv);
  if (move == Move::None) return false;
  return B.make(move, undo);
}

void Engine::go(const SearchParams sp)
{
  MS time = sp.full_time(B.to_move());

  for (auto solver : S)
  {
    solver->set(B);
    solver->set_analysis(sp.infinite);
    solver->get_move(time);
  }
}

}
