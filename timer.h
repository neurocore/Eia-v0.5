#pragma once
#include <chrono>
#include "types.h"

namespace eia {

class Timer
{
  using Clock = std::chrono::high_resolution_clock;
  using Milli = std::chrono::milliseconds;
  Clock::time_point clock;

public:
  Timer() {}
  void start() { clock = Clock::now(); }
  MS getms() const
  {
    return std::chrono::duration_cast<Milli>(Clock::now() - clock).count();
  }
};

}
