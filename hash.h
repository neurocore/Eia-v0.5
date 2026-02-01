#pragma once
#include "consts.h"
#include "moves.h"
#include "bitboard.h"

namespace eia::Hash {

// This hash model partly borrowed from Ethereal

enum Type { None, Lower, Upper, Exact };

struct Entry // 8
{
  u16 key16;      // 2
  Move move;      // 2
  u8 depth, type; // 2
  i16 val;        // 2
};

inline int val_from(int val, int height)
{
  return val >=  dry(Val::Mate) ? val - height
       : val <= -dry(Val::Mate) ? val + height : val;
}

inline int val_to(int val, int height)
{
  return val >=  dry(Val::Mate) ? val + height
       : val <= -dry(Val::Mate) ? val - height : val;
}

inline u16 key_high(u64 key)
{
  return static_cast<u16>(key >> 48);
}

const Entry entry0 { Empty, Move::None, 0u, 0u, 0 };


class Table
{
  int size;
  Entry * table;

public:
  Table(int size_mb = HashTables::Size) { init(size_mb); }
  ~Table() { delete[] table; table = nullptr; }

  void clear()
  {
    for (int i = 0; i < size; ++i)
      table[i] = entry0;
  }

  void init(int size_mb)
  {
    constexpr int entry_sz = sizeof(Entry);

    if constexpr (only_one(entry_sz))
    {
      assert(size_mb <= 2048 * entry_sz);
      size = size_mb << (20 - bitscan(entry_sz));
    }
    else // ensure it has power of two
    {
      assert(size_mb <= 2048);
      int n = (size_mb << 20) / sizeof(Entry);
      size = msb(n);
    }
    
    if (table != nullptr) delete[] table;
    table = new Entry[size];

    clear();
  }

  bool probe(u64 key, int height, Entry & entry)
  {
    Entry & e = table[key & (size - 1)];

    if (e.key16 != key_high(key)) return false;

    Entry result = e;
    result.val = val_from(result.val, height);
    entry = result; 
    return true;
  }

  void store(u64 key, int height, Move move, Val val, int depth, Type type)
  {
    Entry & entry = table[key & (size - 1)];

    if (type != Type::Exact
    &&  key_high(key) == entry.key16
    &&  depth < entry.depth - 2) return;

    entry = Entry
    {
      key_high(key), move,
      (u8)depth, (u8)type,
      (i16)val_to(dry(val), height)
    };
  }

  int hashfull() const
  {
    int used = 0; // good estimate
    
    for (int i = 0; i < 1000; i++)
      used += !!table[i].type;
    
    return used;
  }
};

}
