#pragma once
#include <limits>
#include <cstdalign>

namespace eia {

#define INLINE inline constexpr
#define ALIGN64 alignas(64)

using i8  = char;
using i16 = short;
using i32 = int;
using i64 = long long;

using u8  = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using MB = u32; // megabytes

const int I32_MIN = std::numeric_limits<int>::min();
const i64 I64_MIN = std::numeric_limits<i64>::min();

}
