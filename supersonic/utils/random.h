// Copyright 2012 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: tomasz.kaftal@gmail.com (Tomasz Kaftal)
//
// The file provides an implementation of the RandomBase class used to generate
// random numbers.
// The implementation is based on the Mersenne Twister available at:
// http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html .
//
#ifndef SUPERSONIC_OPENSOURCE_RANDOM_RANDOM_H_
#define SUPERSONIC_OPENSOURCE_RANDOM_RANDOM_H_

#include <memory>

#include "supersonic/utils/integral_types.h"
#include "supersonic/utils/macros.h"

// A wrapper class for random number generation utilities.
class RandomBase {
 public:
  // constructors.  Don't do too much.
  RandomBase() { }
  virtual ~RandomBase() { }

  // Clone: generate a direct copy of this pseudorandom number generator.
  // NB: Returns NULL if Clone is not implemented/available.
  virtual std::unique_ptr<RandomBase> Clone() const ABSTRACT;

  // Generates a random value of a given type.
  virtual uint8_t  Rand8() ABSTRACT;
  virtual uint16_t Rand16() ABSTRACT;
  virtual uint32_t Rand32() ABSTRACT;
  virtual uint64_t Rand64() ABSTRACT;
  virtual double RandDouble();

  virtual int32_t Next() ABSTRACT;
};

// MTRandom: A __simplified__ implementation of the MT19937 RNG class.
// Implements the RandomBase interface.
//
// Example:
//   RandomBase* b = new MTRandom();
//   cout << " Hello, a random number is: " << b->Rand32() << std::endl;
//   delete b;
//
class MTRandom : public RandomBase {
 public:
  // Create an instance of MTRandom using a single seed value.
  // Calls InitSeed() to initialize the context.
  explicit MTRandom(uint32_t seed);

  // Seed MTRandom using an array of uint32_t.  When using this initializer,
  // 'seed' should be well distributed random data of kMTSizeBytes bytes
  // aligned to a uint32_t, since no additional mixing is done.
  // Requires: num_words == kMTNumWords
  MTRandom(const uint32_t* seed, int num_words);

  // Creates an MTRandom generator object that has been seeded using
  // Some weak random data.  (time of day, hostname, etc.).  Uses InitArray().
  MTRandom();

  virtual ~MTRandom();

  virtual std::unique_ptr<RandomBase> Clone() const;

  virtual uint8_t  Rand8();
  virtual uint16_t Rand16();
  virtual uint32_t Rand32();
  virtual uint64_t Rand64();
  virtual int32_t Next();

  static int SeedSize() { return kMTSizeBytes; }

  // The log2 of the RNG buffers (based on uint32_t).
  static const int kMTNumWords = 624;

  // The size of the RNG buffers in bytes.
  static const int kMTSizeBytes = kMTNumWords * sizeof(uint32_t);

  // Reseeds the RNG, as if it had been constructed from this seed.
  // The semantics and restrictions are identical to those in the
  // corresponding constructors.
  void Reset(uint32_t seed);
  void Reset(const uint32_t* seed, int num_words);

 private:
  // InitRaw: Initialize the MTRandom context using an array of raw
  // uint32_t values.  Requires length == SeedSize().
  void InitRaw(const uint32_t* seed, int length);

  // Initialize the MTRandom context using a 32-bit seed.  The seed
  // is distributed across the initial space.
  // NOTE: This will not seed the generator with identical values as
  // either of the seed algorithms in the original paper.  If an
  // identical sequence is required, use InitRaw.
  void InitSeed(uint32_t seed);

  // InitArray: Initialize the MTRandom context using an array of
  // uint32_t values.  The values will be mixed to form an initial seed.
  void InitArray(const uint32_t* seed, int length);

  // The MT context. This holds our RNG state and the current generation
  // of generated numbers.
  struct MTContext {
    int8_t   poolsize;  // For giving back bytes.
    int32_t  randcnt;  // Count of remaining bytes.
    uint32_t pool;
    uint32_t buffer[kMTNumWords];
  };

  // This method cycles the MTContext and generates the next set of random
  // numbers.
  void Cycle();

  MTContext context_;

  DISALLOW_COPY_AND_ASSIGN(MTRandom);
};
#endif  // SUPERSONIC_OPENSOURCE_RANDOM_RANDOM_H_
