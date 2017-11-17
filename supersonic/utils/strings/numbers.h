// Copyright 2010 Google Inc. All Rights Reserved.
// Maintainer: mec@google.com (Michael Chastain)
//
// Convert strings to numbers or numbers to strings.

#ifndef STRINGS_NUMBERS_H_
#define STRINGS_NUMBERS_H_

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <limits>
#include "supersonic/utils/std_namespace.h"
#include <string>
namespace supersonic {using std::string; }
#include <vector>
using std::vector;

#include "supersonic/utils/int128.h"
#include "supersonic/utils/integral_types.h"
#include "supersonic/utils/macros.h"
#include "supersonic/utils/port.h"
#include "supersonic/utils/stringprintf.h"
#include "supersonic/utils/strings/ascii_ctype.h"
#include "supersonic/utils/strings/stringpiece.h"

// START DOXYGEN NumbersFunctions grouping
/* @defgroup NumbersFunctions
 * @{ */

// Convert strings to numeric values, with strict error checking.
// Leading and trailing spaces are allowed.
// Negative inputs are not allowed for unsigned ints (unlike strtoul).
//
// Base must be [0, 2-36].
// Base 0:
//   auto-select base from first two chars:
//    "0x" -> hex
//    "0" -> octal
//    else -> decimal
// Base 16:
//   Number can start with "0x"
//
// On error, returns false, and sets *value to:
//   std::numeric_limits<T>::max() on overflow
//   std::numeric_limits<T>::min() on underflow
//   conversion of leading substring if available ("123@@@" -> 123)
//   0 if no leading substring available
// The effect on errno is unspecified.
// Do not depend on testing errno.
bool safe_strto32_base(StringPiece text, int32_t* value, int base);
bool safe_strto64_base(StringPiece text, int64_t* value, int base);
bool safe_strtou32_base(StringPiece text, uint32_t* value, int base);
bool safe_strtou64_base(StringPiece text, uint64_t* value, int base);
bool safe_strtosize_t_base(StringPiece text, size_t* value, int base);

// Convenience functions with base == 10.
inline bool safe_strto32(StringPiece text, int32_t* value) {
  return safe_strto32_base(text, value, 10);
}

inline bool safe_strto64(StringPiece text, int64_t* value) {
  return safe_strto64_base(text, value, 10);
}

inline bool safe_strtou32(StringPiece text, uint32_t* value) {
  return safe_strtou32_base(text, value, 10);
}

inline bool safe_strtou64(StringPiece text, uint64_t* value) {
  return safe_strtou64_base(text, value, 10);
}

inline bool safe_strtosize_t(StringPiece text, size_t* value) {
  return safe_strtosize_t_base(text, value, 10);
}

// 8 obsolete functions.
// I would like to deprecate these functions but there are technical
// problems with SplitStringAndParse callbacks.
//
//  4  safe_strtofoo(const char*, ...);
//  4  safe_strtofoo(const string&, ...);
//
// If you want to see the technical problems, delete these overloads
// and build strings:strings.

// ---

inline bool safe_strto32(const char* str, int32_t* value) {
  return safe_strto32(StringPiece(str), value);
}

inline bool safe_strto32(const string& str, int32_t* value) {
  return safe_strto32(StringPiece(str), value);
}

// ---

inline bool safe_strto64(const char* str, int64_t* value) {
  return safe_strto64(StringPiece(str), value);
}

inline bool safe_strto64(const string& str, int64_t* value) {
  return safe_strto64(StringPiece(str), value);
}

// ---

inline bool safe_strtou32(const char* str, uint32_t* value) {
  return safe_strtou32(StringPiece(str), value);
}

inline bool safe_strtou32(const string& str, uint32_t* value) {
  return safe_strtou32(StringPiece(str), value);
}

// ---

inline bool safe_strtou64(const char* str, uint64_t* value) {
  return safe_strtou64(StringPiece(str), value);
}

inline bool safe_strtou64(const string& str, uint64_t* value) {
  return safe_strtou64(StringPiece(str), value);
}

// ---

// Convert a fingerprint to 16 hex digits.
string FpToString(Fprint fp);
// Convert between uint128 and 32-digit hex string (sans leading 0x).
string Uint128ToHexString(uint128 ui128);
// Returns true on successful conversion; false on invalid input.
bool HexStringToUint128(StringPiece hex, uint128* value);

// Convert strings to floating point values.
// Leading and trailing spaces are allowed.
// Values may be rounded on over- and underflow.
bool safe_strtof(const char* str, float* value);
bool safe_strtof(const string& str, float* value);
bool safe_strtod(const char* str, double* value);
bool safe_strtod(const string& str, double* value);

// Parses text (case insensitive) into a boolean. The following strings are
// interpreted to boolean true: "true", "t", "yes", "y", "1". The following
// strings are interpreted to boolean false: "false", "f", "no", "n", "0".
// Returns true on success. On failure the boolean value will not have been
// changed.
bool safe_strtob(StringPiece str, bool* value);

// u64tostr_base36()
//    The inverse of safe_strtou64_base, converts the number agument to
//    a string representation in base-36.
//    Conversion fails if buffer is too small to to hold the string and
//    terminating NUL.
//    Returns number of bytes written, not including terminating NUL.
//    Return value 0 indicates error.
size_t u64tostr_base36(uint64_t number, size_t buf_size, char* buffer);

// Similar to atoi(s), except s could be like "16k", "32M", "2G", "4t".
uint64_t atoi_kmgt(const char* s);
inline uint64_t atoi_kmgt(const string& s) { return atoi_kmgt(s.c_str()); }

// ----------------------------------------------------------------------
// FastIntToBuffer()
// FastHexToBuffer()
// FastHex64ToBuffer()
// FastHex32ToBuffer()
// FastTimeToBuffer()
//    These are intended for speed.  FastHexToBuffer() puts output in
//    hex rather than decimal.  FastTimeToBuffer() puts the output
//    into RFC822 format.
//
//    FastHex64ToBuffer() puts a 64-bit unsigned value in hex-format,
//    padded to exactly 16 bytes (plus one byte for '\0')
//
//    FastHex32ToBuffer() puts a 32-bit unsigned value in hex-format,
//    padded to exactly 8 bytes (plus one byte for '\0')
//
//    All functions take the output buffer as an arg.  FastInt() uses
//    at most 22 bytes, FastTime() uses exactly 30 bytes.  They all
//    return a pointer to the beginning of the output, which for
//    FastHex() may not be the beginning of the input buffer.  (For
//    all others, we guarantee that it is.)
//
//    NOTE: In 64-bit land, sizeof(time_t) is 8, so it is possible
//    to pass to FastTimeToBuffer() a time whose year cannot be
//    represented in 4 digits. In this case, the output buffer
//    will contain the string "Invalid:<value>"
// ----------------------------------------------------------------------

// Previously documented minimums -- the buffers provided must be at least this
// long, though these numbers are subject to change:
//     Int32, UInt32:                   12 bytes
//     Int64, UInt64, Hex, Int, Uint:   22 bytes
//     Time:                            30 bytes
//     Hex32:                            9 bytes
//     Hex64:                           17 bytes
// Use kFastToBufferSize rather than hardcoding constants.
static const int kFastToBufferSize = 32;

char* FastUInt32ToBuffer(uint32_t i, char* buffer);
char* FastUInt64ToBuffer(uint64_t i, char* buffer);
char* FastHexToBuffer(int i, char* buffer) MUST_USE_RESULT;
char* FastTimeToBuffer(time_t t, char* buffer);
char* FastHex64ToBuffer(uint64_t i, char* buffer);
char* FastHex32ToBuffer(uint32_t i, char* buffer);

// ----------------------------------------------------------------------
// FastInt32ToBufferLeft()
// FastUInt32ToBufferLeft()
// FastInt64ToBufferLeft()
// FastUInt64ToBufferLeft()
//
// Like the Fast*ToBuffer() functions above, these are intended for speed.
// Unlike the Fast*ToBuffer() functions, however, these functions write
// their output to the beginning of the buffer (hence the name, as the
// output is left-aligned).  The caller is responsible for ensuring that
// the buffer has enough space to hold the output.
//
// Returns a pointer to the end of the string (i.e. the null character
// terminating the string).
// ----------------------------------------------------------------------

char* FastInt32ToBufferLeft(int32_t i, char* buffer);    // at least 12 bytes
char* FastUInt32ToBufferLeft(uint32_t i, char* buffer);    // at least 12 bytes
char* FastInt64ToBufferLeft(int64_t i, char* buffer);    // at least 22 bytes
char* FastUInt64ToBufferLeft(uint64_t i, char* buffer);    // at least 22 bytes

// Just define these in terms of the above.

inline char* FastInt32ToBuffer(int32_t i, char* buffer) {
  FastInt32ToBufferLeft(i, buffer);
  return buffer;
}
inline char* FastUInt32ToBuffer(uint32_t i, char* buffer) {
  FastUInt32ToBufferLeft(i, buffer);
  return buffer;
}
inline char* FastInt64ToBuffer(int64_t i, char* buffer) {
  FastInt64ToBufferLeft(i, buffer);
  return buffer;
}
inline char* FastUInt64ToBuffer(uint64_t i, char* buffer) {
  FastUInt64ToBufferLeft(i, buffer);
  return buffer;
}
inline char* FastIntToBuffer(int i, char* buffer) {
  return (sizeof(i) == 4 ?
          FastInt32ToBuffer(i, buffer) : FastInt64ToBuffer(i, buffer));
}
inline char* FastUIntToBuffer(unsigned int i, char* buffer) {
  return (sizeof(i) == 4 ?
          FastUInt32ToBuffer(i, buffer) : FastUInt64ToBuffer(i, buffer));
}

// ----------------------------------------------------------------------
// HexDigitsPrefix()
//  returns 1 if buf is prefixed by "num_digits" of hex digits
//  returns 0 otherwise.
//  The function checks for '\0' for string termination.
// ----------------------------------------------------------------------
int HexDigitsPrefix(const char* buf, int num_digits);

// ----------------------------------------------------------------------
// ConsumeStrayLeadingZeroes
//    Eliminates all leading zeroes (unless the string itself is composed
//    of nothing but zeroes, in which case one is kept: 0...0 becomes 0).
void ConsumeStrayLeadingZeroes(string* str);

// ----------------------------------------------------------------------
// ParseLeadingInt32Value
//    A simple parser for int32 values. Returns the parsed value
//    if a valid integer is found; else returns deflt. It does not
//    check if str is entirely consumed.
//    This cannot handle decimal numbers with leading 0s, since they will be
//    treated as octal.  If you know it's decimal, use ParseLeadingDec32Value.
// --------------------------------------------------------------------
int32_t ParseLeadingInt32Value(const char* str, int32_t deflt);
inline int32_t ParseLeadingInt32Value(const string& str, int32_t deflt) {
  return ParseLeadingInt32Value(str.c_str(), deflt);
}

// ParseLeadingUInt32Value
//    A simple parser for uint32_t values. Returns the parsed value
//    if a valid integer is found; else returns deflt. It does not
//    check if str is entirely consumed.
//    This cannot handle decimal numbers with leading 0s, since they will be
//    treated as octal.  If you know it's decimal, use ParseLeadingUDec32Value.
// --------------------------------------------------------------------
uint32_t ParseLeadingUInt32Value(const char* str, uint32_t deflt);
inline uint32_t ParseLeadingUInt32Value(const string& str, uint32_t deflt) {
  return ParseLeadingUInt32Value(str.c_str(), deflt);
}

// ----------------------------------------------------------------------
// ParseLeadingDec32Value
//    A simple parser for decimal int32 values. Returns the parsed value
//    if a valid integer is found; else returns deflt. It does not
//    check if str is entirely consumed.
//    The string passed in is treated as *10 based*.
//    This can handle strings with leading 0s.
//    See also: ParseLeadingDec64Value
// --------------------------------------------------------------------
int32_t ParseLeadingDec32Value(const char* str, int32_t deflt);
inline int32_t ParseLeadingDec32Value(const string& str, int32_t deflt) {
  return ParseLeadingDec32Value(str.c_str(), deflt);
}

// ParseLeadingUDec32Value
//    A simple parser for decimal uint32_t values. Returns the parsed value
//    if a valid integer is found; else returns deflt. It does not
//    check if str is entirely consumed.
//    The string passed in is treated as *10 based*.
//    This can handle strings with leading 0s.
//    See also: ParseLeadingUDec64Value
// --------------------------------------------------------------------
uint32_t ParseLeadingUDec32Value(const char* str, uint32_t deflt);
inline uint32_t ParseLeadingUDec32Value(const string& str, uint32_t deflt) {
  return ParseLeadingUDec32Value(str.c_str(), deflt);
}

// ----------------------------------------------------------------------
// ParseLeadingUInt64Value
// ParseLeadingInt64Value
// ParseLeadingHex64Value
// ParseLeadingDec64Value
// ParseLeadingUDec64Value
//    A simple parser for long long values.
//    Returns the parsed value if a
//    valid integer is found; else returns deflt
// --------------------------------------------------------------------
uint64_t ParseLeadingUInt64Value(const char* str, uint64_t deflt);
inline uint64_t ParseLeadingUInt64Value(const string& str, uint64_t deflt) {
  return ParseLeadingUInt64Value(str.c_str(), deflt);
}
int64_t ParseLeadingInt64Value(const char* str, int64_t deflt);
inline int64_t ParseLeadingInt64Value(const string& str, int64_t deflt) {
  return ParseLeadingInt64Value(str.c_str(), deflt);
}
uint64_t ParseLeadingHex64Value(const char* str, uint64_t deflt);
inline uint64_t ParseLeadingHex64Value(const string& str, uint64_t deflt) {
  return ParseLeadingHex64Value(str.c_str(), deflt);
}
int64_t ParseLeadingDec64Value(const char* str, int64_t deflt);
inline int64_t ParseLeadingDec64Value(const string& str, int64_t deflt) {
  return ParseLeadingDec64Value(str.c_str(), deflt);
}
uint64_t ParseLeadingUDec64Value(const char* str, uint64_t deflt);
inline uint64_t ParseLeadingUDec64Value(const string& str, uint64_t deflt) {
  return ParseLeadingUDec64Value(str.c_str(), deflt);
}

// ----------------------------------------------------------------------
// ParseLeadingDoubleValue
//    A simple parser for double values. Returns the parsed value
//    if a valid double is found; else returns deflt. It does not
//    check if str is entirely consumed.
// --------------------------------------------------------------------
double ParseLeadingDoubleValue(const char* str, double deflt);
inline double ParseLeadingDoubleValue(const string& str, double deflt) {
  return ParseLeadingDoubleValue(str.c_str(), deflt);
}

// ----------------------------------------------------------------------
// ParseLeadingBoolValue()
//    A recognizer of boolean string values. Returns the parsed value
//    if a valid value is found; else returns deflt.  This skips leading
//    whitespace, is case insensitive, and recognizes these forms:
//    0/1, false/true, no/yes, n/y
// --------------------------------------------------------------------
bool ParseLeadingBoolValue(const char* str, bool deflt);
inline bool ParseLeadingBoolValue(const string& str, bool deflt) {
  return ParseLeadingBoolValue(str.c_str(), deflt);
}

// ----------------------------------------------------------------------
// AutoDigitStrCmp
// AutoDigitLessThan
// StrictAutoDigitLessThan
// autodigit_less
// autodigit_greater
// strict_autodigit_less
// strict_autodigit_greater
//    These are like less<string> and greater<string>, except when a
//    run of digits is encountered at corresponding points in the two
//    arguments.  Such digit strings are compared numerically instead
//    of lexicographically.  Therefore if you sort by
//    "autodigit_less", some machine names might get sorted as:
//        exaf1
//        exaf2
//        exaf10
//    When using "strict" comparison (AutoDigitStrCmp with the strict flag
//    set to true, or the strict version of the other functions),
//    strings that represent equal numbers will not be considered equal if
//    the string representations are not identical.  That is, "01" < "1" in
//    strict mode, but "01" == "1" otherwise.
// ----------------------------------------------------------------------

int AutoDigitStrCmp(const char* a, int alen,
                    const char* b, int blen,
                    bool strict);

bool AutoDigitLessThan(const char* a, int alen,
                       const char* b, int blen);

bool StrictAutoDigitLessThan(const char* a, int alen,
                             const char* b, int blen);

struct autodigit_less
  : public std::binary_function<const string&, const string&, bool> {
  bool operator()(const string& a, const string& b) const {
    return AutoDigitLessThan(a.data(), a.size(), b.data(), b.size());
  }
};

struct autodigit_greater
  : public std::binary_function<const string&, const string&, bool> {
  bool operator()(const string& a, const string& b) const {
    return AutoDigitLessThan(b.data(), b.size(), a.data(), a.size());
  }
};

struct strict_autodigit_less
  : public std::binary_function<const string&, const string&, bool> {
  bool operator()(const string& a, const string& b) const {
    return StrictAutoDigitLessThan(a.data(), a.size(), b.data(), b.size());
  }
};

struct strict_autodigit_greater
  : public std::binary_function<const string&, const string&, bool> {
  bool operator()(const string& a, const string& b) const {
    return StrictAutoDigitLessThan(b.data(), b.size(), a.data(), a.size());
  }
};

// ----------------------------------------------------------------------
// SimpleItoa()
//    Description: converts an integer to a string.
//    Faster than printf("%d").
//
//    Return value: string
// ----------------------------------------------------------------------
inline string SimpleItoa(int32_t i) {
  char buf[16];  // Longest is -2147483648
  return string(buf, FastInt32ToBufferLeft(i, buf));
}

// We need this overload because otherwise SimpleItoa(5U) wouldn't compile.
inline string SimpleItoa(uint32_t i) {
  char buf[16];  // Longest is 4294967295
  return string(buf, FastUInt32ToBufferLeft(i, buf));
}

inline string SimpleItoa(int64_t i) {
  char buf[32];  // Longest is -9223372036854775808
  return string(buf, FastInt64ToBufferLeft(i, buf));
}

// We need this overload because otherwise SimpleItoa(5ULL) wouldn't compile.
inline string SimpleItoa(uint64_t i) {
  char buf[32];  // Longest is 18446744073709551615
  return string(buf, FastUInt64ToBufferLeft(i, buf));
}

// SimpleAtoi converts a string to an integer.
// Uses safe_strto?() for actual parsing, so strict checking is
// applied, which is to say, the string must be a base-10 integer, optionally
// followed or preceded by whitespace, and value has to be in the range of
// the corresponding integer type.
//
// Returns true if parsing was successful.
template <typename int_type>
bool MUST_USE_RESULT SimpleAtoi(const char* s, int_type* out) {
  // Must be of integer type (not pointer type), with more than 16-bitwidth.
  COMPILE_ASSERT(sizeof(*out) == 4 || sizeof(*out) == 8,
                 SimpleAtoiWorksWith32Or64BitInts);
  if (std::numeric_limits<int_type>::is_signed) {  // Signed
    if (sizeof(*out) == 64 / 8) {  // 64-bit
      return safe_strto64(s, reinterpret_cast<int64_t*>(out));
    } else {  // 32-bit
      return safe_strto32(s, reinterpret_cast<int32_t*>(out));
    }
  } else {  // Unsigned
    if (sizeof(*out) == 64 / 8) {  // 64-bit
      return safe_strtou64(s, reinterpret_cast<uint64_t*>(out));
    } else {  // 32-bit
      return safe_strtou32(s, reinterpret_cast<uint32_t*>(out));
    }
  }
}

template <typename int_type>
bool MUST_USE_RESULT SimpleAtoi(const string& s, int_type* out) {
  return SimpleAtoi(s.c_str(), out);
}

// ----------------------------------------------------------------------
// SimpleDtoa()
// SimpleFtoa()
//    Description: converts a double or float to a string which, if passed to
//    strtod() or strtof() respectively, will produce the exact same original
//    double or float.  Exception: for NaN values, strtod(SimpleDtoa(NaN)) or
//    strtof(SimpleFtoa(NaN)) may produce any NaN value, not necessarily the
//    exact same original NaN value.
//
//    The output string is not guaranteed to be as short as possible.
//
//    The output string, including terminating NUL, will have length
//    less than or equal to kFastToBufferSize defined above.  Of course,
//    we would prefer that your code not depend on this property of
//    the output string.  This guarantee derives from a similar guarantee
//    from the previous generation of char-buffer-based functions.
//    We had to carry it forward to preserve compatibility.
// ----------------------------------------------------------------------
string SimpleDtoa(double value);
string SimpleFtoa(float value);

// DEPRECATED(mec).  Call SimpleDtoa or SimpleFtoa instead.
// Required buffer size for DoubleToBuffer is kFastToBufferSize.
// Required buffer size for FloatToBuffer is kFastToBufferSize.
char* DoubleToBuffer(double i, char* buffer);  // DEPRECATED(mec)
char* FloatToBuffer(float i, char* buffer);  // DEPRECATED(mec)

// ----------------------------------------------------------------------
// SimpleItoaWithCommas()
//    Description: converts an integer to a string.
//    Puts commas every 3 spaces.
//    Faster than printf("%d")?
//
//    Return value: string
// ----------------------------------------------------------------------

template <typename IntType>
inline string SimpleItoaWithCommas(IntType ii) {
  string s1 = SimpleItoa(ii);
  StringPiece sp1(s1);
  string output;
  // Copy leading non-digit characters unconditionally.
  // This picks up the leading sign.
  while (sp1.size() > 0 && !ascii_isdigit(sp1[0])) {
    output.push_back(sp1[0]);
    sp1.remove_prefix(1);
  }
  // Copy rest of input characters.
  for (size_t i = 0; i < sp1.size(); ++i) {
    if (i > 0 && i < sp1.size() && (sp1.size() - i) % 3 == 0) {
      output.push_back(',');
    }
    output.push_back(sp1[i]);
  }
  return output;
}

// Converts a boolean to a string, which if passed to safe_strtob will produce
// the exact same original boolean. The returned string will be "true" if
// value == true, and "false" otherwise.
string SimpleBtoa(bool value);

// ----------------------------------------------------------------------
// ItoaKMGT()
//    Description: converts an integer to a string
//    Truncates values to K, G, M or T as appropriate
//    Opposite of atoi_kmgt()
//    e.g. 3000 -> 2K   57185920 -> 45M
//
//    Return value: string
// ----------------------------------------------------------------------
string ItoaKMGT(int64_t i);

// ----------------------------------------------------------------------
// ParseDoubleRange()
//    Parse an expression in 'text' of the form: <double><sep><double>
//    where <double> may be a double-precision number and <sep> is a
//    single char or "..", and must be one of the chars in parameter
//    'separators', which may contain '-' or '.' (which means "..") or
//    any chars not allowed in a double. If allow_unbounded_markers,
//    <double> may also be a '?' to indicate unboundedness (if on the
//    left of <sep>, means unbounded below; if on the right, means
//    unbounded above). Depending on num_required_bounds, which may be
//    0, 1, or 2, <double> may also be the empty string, indicating
//    unboundedness. If require_separator is false, then a single
//    <double> is acceptable and is parsed as a range bounded from
//    below. We also check that the character following the range must
//    be in acceptable_terminators. If null_terminator_ok, then it is
//    also OK if the range ends in \0 or after len chars. If
//    allow_currency is true, the first <double> may be optionally
//    preceded by a '$', in which case *is_currency will be true, and
//    the second <double> may similarly be preceded by a '$'. In these
//    cases, the '$' will be ignored (otherwise it's an error). If
//    allow_comparators is true, the expression in 'text' may also be
//    of the form <comparator><double>, where <comparator> is '<' or
//    '>' or '<=' or '>='. separators and require_separator are
//    ignored in this format, but all other parameters function as for
//    the first format. Return true if the expression parsed
//    successfully; false otherwise. If successful, output params are:
//    'end', which points to the char just beyond the expression;
//    'from' and 'to' are set to the values of the <double>s, and are
//    -inf and inf (or unchanged, depending on dont_modify_unbounded)
//    if unbounded. Output params are undefined if false is
//    returned. len is the input length, or -1 if text is
//    '\0'-terminated, which is more efficient.
// ----------------------------------------------------------------------
struct DoubleRangeOptions {
  const char* separators;
  bool require_separator;
  const char* acceptable_terminators;
  bool null_terminator_ok;
  bool allow_unbounded_markers;
  uint32_t num_required_bounds;
  bool dont_modify_unbounded;
  bool allow_currency;
  bool allow_comparators;
};

// NOTE: The instruction below creates a Module titled
// NumbersFunctions within the auto-generated Doxygen documentation.
// This instruction is needed to expose global functions that are not
// within a namespace.
//
bool ParseDoubleRange(const char* text, int len, const char** end,
                      double* from, double* to, bool* is_currency,
                      const DoubleRangeOptions& opts);

// END DOXYGEN SplitFunctions grouping
/* @} */

#endif  // STRINGS_NUMBERS_H_
