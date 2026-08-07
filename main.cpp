#include <iostream>
#include <format>
//#include "eval.h"
#include "engine.h"

using namespace std;
using namespace eia;

int main()
{
  if (Input.is_console())
  {
    say("Chess engine {} v{} by {} (c) 2025-2026\n", Name, Vers, Auth);
    report_num_threads();
  }

  //log("{}\n", E->prettify());

  Engine * engine = new Engine;
  engine->start();

  return 0;
}
