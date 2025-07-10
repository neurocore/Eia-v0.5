#pragma once
#include <unordered_map>
#include <algorithm>
#include <string>
#include <format>
#include <vector>
#include "types.h"
#include "utils.h"

namespace eia {

enum class OptionType { None, Check, Spin, Combo, Button, String };

INLINE std::string to_str(OptionType ot)
{
  switch(ot)
  {
    case OptionType::Check:  return "check";
    case OptionType::Spin:   return "spin";
    case OptionType::Combo:  return "combo";
    case OptionType::Button: return "button";
    case OptionType::String: return "string";
    default: return "unknown";
  }
}

class Option
{
protected:
  OptionType type = OptionType::None;

public:
  Option(OptionType type) : type(type) {}
  OptionType get_type() const { return type; }
  virtual std::string get_str() const { return ""; }
  virtual void set(std::string str) {}
};


class OptionCheck : public Option
{
  bool val, def;

public:
  OptionCheck(bool val)
    : Option(OptionType::Check)
    , val(val), def(val)
  {}

  std::string get_str() const override
  {
    return std::string("default ") + std::to_string(def);
  }

  void set(std::string str) override
  {
    val = static_cast<bool>(parse_int(str));
  }
};


class OptionSpin : public Option
{
  int val, def;
  int min, max;

public:
  OptionSpin(int val, int min, int max)
    : Option(OptionType::Spin)
    , val(val), def(val)
    , min(min), max(max)
  {}

  std::string get_str() const override
  {
    return std::string("default ") + std::to_string(def)
         + std::string(" min ") + std::to_string(min)
         + std::string(" max ") + std::to_string(max);
  }

  void set(std::string str) override
  {
    val = parse_int(str);
    if (val < min) val = min;
    if (val > max) val = max;
  }
};


using Strings = std::vector<std::string>;
class OptionCombo : public Option
{
  int val, def;
  Strings strings;

public:
  OptionCombo(int val, Strings strings)
    : Option(OptionType::Combo)
    , val(val), def(val), strings(strings)
  {}

  std::string get_str() const override
  {
    return std::string("default ") + std::to_string(def);
  }

  void set(std::string str) override
  {
    auto it = std::find(strings.begin(), strings.end(), str);
    if (it != strings.end())
      val = static_cast<int>(it - strings.begin());
  }
};


class OptionButton : public Option
{
  std::string val, def;

public:
  OptionButton(std::string val)
    : Option(OptionType::Button)
    , val(val), def(val)
  {}

  std::string get_str() const override
  {
    return std::string("default ") + def;
  }

  void set(std::string str) override {}
};


class OptionString : public Option
{
  std::string val, def;

public:
  OptionString(std::string val)
    : Option(OptionType::String)
    , val(val), def(val)
  {}

  std::string get_str() const override
  {
    return std::string("default ") + def;
  }

  void set(std::string str) override
  {
    val = str;
  }
};


struct Options
{
  bool flag_debug = true;
  std::unordered_map<std::string, Option *> options;
  Strings order;

  Options()
  {
    add("Hash", new OptionSpin(4, 1, 1024));
    add("NullMove", new OptionCheck(false));
    add("OwnBook", new OptionCheck(false));
    add("UCI_ShowCurrLine", new OptionCheck(true));
    add("TestButton", new OptionButton("I am a button!"));
    add("TestString", new OptionString("I am a string!"));
    add("TestCombo", new OptionCombo(0, {"A", "B", "C"}));
  }

  void add(std::string name, Option * option)
  {
    options[name] = option;
    order.emplace_back(name);
  }

  void set(std::string name, std::string val)
  {
    auto it = options.find(name);
    if (it == options.end()) return;

    it->second->set(val);
  }

  std::string to_string() const
  {
    std::string result;
    for (std::string name : order)
    {
      auto opt = options.at(name);

      result += "option name " + name + " ";
      result += "type " + to_str(opt->get_type()) + " ";
      result += opt->get_str() + "\n";
    }
    return result;
  }
};

}

template<>
struct std::formatter<eia::Options> : std::formatter<std::string>
{
  auto format(const eia::Options & opts, std::format_context & ctx) const
  {
    std::string str = opts.to_string();
    return std::formatter<std::string>::format(str, ctx);
  }
};
