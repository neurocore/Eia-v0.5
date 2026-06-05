#pragma once
#include "consts.h"
#include "moves.h"
#include "value.h"
#include "bitboard.h"
#include "duo.h"

namespace eia::Hash {

// This hash model partly borrowed from Ethereal
//
//  - no bucket system, no aging
//  - replacement is simply 'always'

enum Type { None, Lower, Upper, Exact };

struct alignas(16) Entry
{
  u32 key32;      // 4
  Move move;      // 2
  u8 depth, type; // 2
  i16 val;        // 2
  i16 eval;       // 2
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
  return static_cast<u16>(key >> 32);
}

const Entry entry0 { Empty, Move::None, 0u, 0u, 0u, 0 };


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

    if (e.key32 != key_high(key)) return false;

    Entry result = e;
    result.val = val_from(result.val, height);
    entry = result; 
    return true;
  }

  void store(u64 key, int height, Move move, Val val, Val eval, int depth, Type type)
  {
    Entry & entry = table[key & (size - 1)];

    if (type != Type::Exact
    &&  key_high(key) == entry.key32
    &&  depth < entry.depth - 2) return;

    entry = Entry
    {
      key_high(key), move,
      (u8)depth, (u8)type,
      (i16)val_to(dry(val), height),
      (i16)dry(val)
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


// Pawn hash table with kings positions

struct PK_Entry
{
  u64 key, passers;
  Val weak[2];
  Duo vals;
};

constexpr int PK_HASH_BITS = 16;
constexpr int PK_HASH_MASK = (1 << PK_HASH_BITS) - 1;

static PK_Entry pk_table[PK_HASH_MASK]; // 2 mb

inline PK_Entry const * pk_probe(u64 key)
{
  PK_Entry const * entry = &pk_table[key & PK_HASH_MASK];
  return entry->key == key ? entry : nullptr;
}

inline void pk_store(u64 key, Duo vals, Val weak[2], u64 passers = 0ull)
{
  pk_table[key & PK_HASH_MASK] =
  {
    .key = key,
    .passers = passers,
    .weak = { weak[0], weak[1] },
    .vals = vals
  };
}

}
