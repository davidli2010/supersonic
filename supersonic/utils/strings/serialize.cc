// Copyright 2003, Google Inc.  All rights reserved.

#include "supersonic/utils/strings/serialize.h"

#include <cstddef>
#include <cstdlib>
#include <unordered_map>
#include <string>
namespace supersonic {using std::string; }
#include <utility>
#include "supersonic/utils/std_namespace.h"
#include <vector>
using std::vector;

#include "supersonic/utils/casts.h"
#include "supersonic/utils/integral_types.h"
#include "supersonic/utils/stringprintf.h"
#include "supersonic/utils/strtoint.h"
#include "supersonic/utils/strings/join.h"
#include "supersonic/utils/strings/split.h"
#include "supersonic/utils/hash/hash.h"

// Convert a uint32_t to a 4-byte string.
string Uint32ToKey(uint32_t u32) {
  string key;
  KeyFromUint32(u32, &key);
  return key;
}

string Uint64ToKey(uint64_t fp) {
  string key;
  KeyFromUint64(fp, &key);
  return key;
}

// Convert a uint128 to a 16-byte string.
string Uint128ToKey(uint128 u128) {
  string key;
  KeyFromUint128(u128, &key);
  return key;
}

// Converts int32 to a 4-byte string key
// NOTE: Lexicographic ordering of the resulting strings does not in
// general correspond to any natural ordering of the corresponding
// integers. For non-negative inputs, lexicographic ordering of the
// resulting strings corresponds to increasing ordering of the
// integers. However, negative inputs are sorted *after* the non-negative
// inputs. To obtain keys such that lexicographic ordering corresponds
// to the natural total order on the integers, use OrderedStringFromInt32()
// or ReverseOrderedStringFromInt32() instead.
void KeyFromInt32(int32_t i32, string* key) {
  // TODO(user): Redefine using bit_cast<> and KeyFromUint32()?
  key->resize(sizeof(i32));
  for (int i = sizeof(i32) - 1; i >= 0; --i) {
    (*key)[i] = i32 & 0xff;
    i32  = (i32 >> 8);
  }
}

// Converts a 4-byte string key (typically generated by KeyFromInt32)
// into an int32 value
int32_t KeyToInt32(StringPiece key) {
  int32_t i32 = 0;
  CHECK_EQ(key.size(), sizeof(i32));
  for (size_t i = 0; i < sizeof(i32); ++i) {
    i32 = (i32 << 8) | static_cast<unsigned char>(key[i]);
  }
  return i32;
}

// Converts a double value to an 8-byte string key, so that
// the string keys sort in the same order as the original double values.
void KeyFromDouble(double x, string* key) {
  uint64_t n = bit_cast<uint64_t>(x);
  // IEEE standard 754 floating point representation
  //   [sign-bit] [exponent] [mantissa]
  //
  // Let "a", "b" be two double values, and F(.) be following
  // transformation.  We have:
  //   If 0 < a < b:
  //     0x80000000ULL < uint64_t(F(a)) < uint64_t(F(b))
  //   If a == -0.0, b == +0.0:
  //     uint64_t(F(-0.0)) == uint64_t(F(+0.0)) = 0x80000000ULL
  //   If a < b < 0:
  //     uint64_t(F(a)) < uint64_t(F(b)) < 0x80000000ULL
  const uint64_t sign_bit = GG_ULONGLONG(1) << 63;
  if ((n & sign_bit) == 0) {
    n += sign_bit;
  } else {
    n = -n;
  }
  KeyFromUint64(n, key);
}

// Version of KeyFromDouble that returns the key.
string DoubleToKey(double x) {
  string key;
  KeyFromDouble(x, &key);
  return key;
}

// Converts key generated by KeyFromDouble() back to double.
double KeyToDouble(StringPiece key) {
  int64_t n = KeyToUint64(key);
  if (n & (GG_ULONGLONG(1) << 63))
    n -= (GG_ULONGLONG(1) << 63);
  else
    n = -n;
  return bit_cast<double>(n);
}

// Converts int32 to a 4-byte string key such that lexicographic
// ordering of strings is equivalent to sorting in increasing order by
// integer values. This can be useful when constructing secondary
void OrderedStringFromInt32(int32_t i32, string* key) {
  uint32_t ui32 = static_cast<uint32_t>(i32) ^ 0x80000000;
  key->resize(sizeof ui32);
  for ( int i = (sizeof ui32) - 1; i >= 0; --i ) {
    (*key)[i] = ui32 & 0xff;
    ui32  = (ui32 >> 8);
  }
}

string Int32ToOrderedString(int32_t i32) {
  string key;
  OrderedStringFromInt32(i32, &key);
  return key;
}

// The inverse of the above function.
int32_t OrderedStringToInt32(StringPiece key) {
  uint32_t ui32 = 0;
  CHECK(key.size() == sizeof ui32);
  for ( int i = 0; i < sizeof ui32; ++i ) {
    ui32 = (ui32 << 8);
    ui32 = ui32 | static_cast<unsigned char>(key[i]);
  }
  return static_cast<int32_t>(ui32 ^ 0x80000000);
}

// Converts int64 to a 8-byte string key such that lexicographic
// ordering of strings is equivalent to sorting in increasing order by
// integer values.
void OrderedStringFromInt64(int64_t i64, string* key) {
  uint64_t ui64 = static_cast<uint64_t>(i64) ^ (GG_ULONGLONG(1) << 63);
  key->resize(sizeof ui64);
  for ( int i = (sizeof ui64) - 1; i >= 0; --i ) {
    (*key)[i] = ui64 & 0xff;
    ui64  = (ui64 >> 8);
  }
}

string Int64ToOrderedString(int64_t i64) {
  string key;
  OrderedStringFromInt64(i64, &key);
  return key;
}

// The inverse of the above function.
int64_t OrderedStringToInt64(StringPiece key) {
  uint64_t ui64 = 0;
  CHECK(key.size() == sizeof ui64);
  for ( int i = 0; i < sizeof ui64; ++i ) {
    ui64 = (ui64 << 8);
    ui64 = ui64 | static_cast<unsigned char>(key[i]);
  }
  return static_cast<int64_t>(ui64 ^ (GG_ULONGLONG(1) << 63));
}

// Converts int32 to a 4-byte string key such that lexicographic
// ordering of strings is equivalent to sorting in decreasing order
// by integer values. This can be useful when constructing secondary
void ReverseOrderedStringFromInt32(int32_t i32, string* key) {
  // ~ is like -, but works even for INT_MIN. (-INT_MIN == INT_MIN,
  // but ~x = -x - 1, so ~INT_MIN = -INT_MIN - 1 = INT_MIN - 1 = INT_MAX).
  OrderedStringFromInt32(~i32, key);
}

string Int32ToReverseOrderedString(int32_t i32) {
  string key;
  ReverseOrderedStringFromInt32(i32, &key);
  return key;
}

// The inverse of the above function.
int32_t ReverseOrderedStringToInt32(StringPiece key) {
  return ~OrderedStringToInt32(key);
}

// Converts int64 to an 8-byte string key such that lexicographic
// ordering of strings is equivalent to sorting in decreasing order
// by integer values. This can be useful when constructing secondary
void ReverseOrderedStringFromInt64(int64_t i64, string* key) {
  return OrderedStringFromInt64(~i64, key);
}

string Int64ToReverseOrderedString(int64_t i64) {
  string key;
  ReverseOrderedStringFromInt64(i64, &key);
  return key;
}

// The inverse of the above function.
int64_t ReverseOrderedStringToInt64(StringPiece key) {
  return ~OrderedStringToInt64(key);
}

// --------------------------------------------------------------------------
// DictionaryInt32Encode
// DictionaryInt64Encode
// DictionaryDoubleEncode
// DictionaryInt32Decode
// DictionaryInt64Decode
// DictionaryDoubleDecode
//   Routines to serialize/unserialize simple dictionaries
//   (string->T hashmaps). We use ':' to separate keys and values,
//   and commas to separate entries.
// --------------------------------------------------------------------------

string DictionaryInt32Encode(const std::unordered_map<string, int32_t>* dictionary) {
  vector<string> entries;
  for (const auto & iter : *dictionary) {
    entries.push_back(StringPrintf("%s:%d", iter.first.c_str(), iter.second));
  }

  string result;
  JoinStrings(entries, ",", &result);
  return result;
}

string DictionaryInt64Encode(const std::unordered_map<string, int64_t>* dictionary) {
  vector<string> entries;
  for (const auto & iter : *dictionary) {
    entries.push_back(StringPrintf("%s:%" GG_INT64_FORMAT,
                                   iter.first.c_str(), iter.second));
  }

  string result;
  JoinStrings(entries, ",", &result);
  return result;
}

string DictionaryDoubleEncode(const std::unordered_map<string, double>* dictionary) {
  vector<string> entries;
  for (const auto & iter : *dictionary) {
    entries.push_back(StringPrintf("%s:%g", iter.first.c_str(), iter.second));
  }

  string result;
  JoinStrings(entries, ",", &result);
  return result;
}

bool DictionaryParse(const string& encoded_str,
                      vector<std::pair<string, string> >* items) {
  vector<string> entries;
  SplitStringUsing(encoded_str, ",", &entries);
  for (const auto & entrie : entries) {
    vector<string> fields = strings::Split(entrie, ":");
    if (fields.size() != 2)  // parsing error
      return false;
    items->push_back(make_pair(fields[0], fields[1]));
  }
  return true;
}

bool DictionaryInt32Decode(std::unordered_map<string, int32_t>* dictionary,
                           const string& encoded_str) {
  vector<std::pair<string, string> > items;
  if (!DictionaryParse(encoded_str, &items))
    return false;

  dictionary->clear();
  for (auto& item: items) {
    char *error = NULL;
    const int32_t value = strto32(item.second.c_str(), &error, 0);
    if (error == item.second.c_str() || *error != '\0') {
      // parsing error
      return false;
    }
    (*dictionary)[item.first] = value;
  }
  return true;
}

bool DictionaryInt64Decode(std::unordered_map<string, int64_t>* dictionary,
                           const string& encoded_str) {
  vector<std::pair<string, string> > items;
  if (!DictionaryParse(encoded_str, &items))
    return false;

  dictionary->clear();
  for (auto& item: items) {
    char *error = NULL;
    const int64_t value = strto64(item.second.c_str(), &error, 0);
    if (error == item.second.c_str() || *error != '\0')  {
      // parsing error
      return false;
    }
    (*dictionary)[item.first] = value;
  }
  return true;
}


bool DictionaryDoubleDecode(std::unordered_map<string, double>* dictionary,
                            const string& encoded_str) {
  vector<std::pair<string, string> > items;
  if (!DictionaryParse(encoded_str, &items))
    return false;

  dictionary->clear();
  for (auto& item: items) {
    char *error = NULL;
    const double value = strtod(item.second.c_str(), &error);
    if (error == item.second.c_str() || *error != '\0') {
      // parsing error
      return false;
    }
    (*dictionary)[item.first] = value;
  }
  return true;
}
