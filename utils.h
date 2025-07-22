#pragma once
#include <windows.h>
#include <conio.h>
#include <string_view>
#include <string>
#include <iostream>
#include <charconv>
#include <algorithm>
#include "types.h"

namespace eia {

template<typename T> INLINE int sgn(T val)
{
  return (T(0) < val) - (val < T(0));
}

template<typename T, typename R>
R compare(T a, T b, R less, R equal, R more)
{
  return (a < b ? less : (a > b ? more : equal));
}

INLINE ptrdiff_t index_of(std::string_view str, char ch)
{
  auto it = std::find(str.begin(), str.end(), ch);
  return it != str.end() ? it - str.begin() : -1;
}

INLINE std::string cut(std::string & str, std::string delim = " ")
{
  auto pos = str.find(delim);
  std::string part = str.substr(0, pos);
  str = pos == std::string::npos ? std::string()
      : str.substr(pos + delim.length(), str.length());
  return part;
}

inline int parse_int(const std::string_view str, int def = 0)
{
  int result = def;
  std::from_chars(str.data(), str.data() + str.size(), result);
  return result;
}

class InputHandler
{
  DWORD mode;
  HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
  BOOL console = GetConsoleMode(handle, &mode);

public:
  inline bool available()
  {
    if (!console)
    {
      DWORD bytes_avail;
      if (!PeekNamedPipe(handle, 0, 0, 0, &bytes_avail, 0)) return true;
      return bytes_avail > 0;
    }
    else return _kbhit();
  }

  inline bool is_console() const
  {
    return console;
  }
};

static InputHandler Input;

template<bool flush = false, typename... Args>
INLINE void say(std::format_string<Args...> fmt, Args&&... args)
{
  std::cout << std::format(fmt, std::forward<Args>(args)...);
  if constexpr (flush) std::cout.flush();
}

template<typename... Args>
INLINE void log(std::format_string<Args...> fmt, Args&&... args)
{
#ifdef _DEBUG
  //std::cout << std::format(fmt, std::forward<Args>(args)...);
#endif
}

}
