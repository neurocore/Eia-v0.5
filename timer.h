#pragma once
#include <chrono>
#include "types.h"

namespace eia {

using Clock = std::chrono::high_resolution_clock;
using Milli = std::chrono::milliseconds;
using Timestamp = Clock::time_point;

inline MS elapsed(Timestamp start)
{
  return std::chrono::duration_cast<Milli>(Clock::now() - start).count();
}

}
