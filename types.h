#pragma once
#include <limits>
#include <vector>
#include <cstdalign>

namespace eia {

#define NOMINMAX
#define INLINE inline constexpr
#define ALIGN64 alignas(64)

#ifdef __GNUC__
#define PACKED__ 
#define __PACKED __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACKED__ __pragma(pack(push, 1))
#define __PACKED __pragma(pack(pop))
#endif

using i8  = char;
using i16 = short;
using i32 = int;
using i64 = long long;

using u8  = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using MB = u32; // megabytes
using MS = u64; // milliseconds

template<typename T>
using limits = std::numeric_limits<T>;

using Genome = std::vector<bool>;

}
