// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "util/hash.h"

#include <cstring>

#include "util/coding.h"

// The FALLTHROUGH_INTENDED macro can be used to annotate implicit fall-through
// between switch labels. The real definition should be provided externally.
// This one is a fallback version for unsupported compilers.
#ifndef FALLTHROUGH_INTENDED
#define FALLTHROUGH_INTENDED \
  do {                       \
  } while (0)
#endif

namespace leveldb {

uint32_t Hash(const char* data, size_t n, uint32_t seed) {
  // Similar to murmur hash
  const uint32_t m = 0xc6a4a793;
  const uint32_t r = 24;
  const char* limit = data + n;
  uint32_t h = seed ^ (n * m);

  // Pick up four bytes at a time
  while (data + 4 <= limit) {
    uint32_t w = DecodeFixed32(data);
    data += 4;
    h += w;
    h *= m;
    h ^= (h >> 16);
  }

  // Pick up remaining bytes
  switch (limit - data) {
    case 3:
      h += static_cast<uint8_t>(data[2]) << 16;
      FALLTHROUGH_INTENDED;
    case 2:
      h += static_cast<uint8_t>(data[1]) << 8;
      FALLTHROUGH_INTENDED;
    case 1:
      h += static_cast<uint8_t>(data[0]);
      h *= m;
      h ^= (h >> r);
      break;
  }
  return h;
}


uint64_t Hash64(const char* data, size_t n, uint64_t seed) {
  const uint64_t m = 0xc6a4a7935bd1e995ULL;
  const uint64_t r = 47;

  const char* limit = data + n;
  uint64_t h = seed ^ (n * m);

  while (data + 8 <= limit) {
    uint64_t k = *reinterpret_cast<const uint64_t*>(data);
    data += 8;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  switch (limit - data) {
    case 7:
      h ^= static_cast<uint64_t>(data[6]) << 48;
    case 6:
      h ^= static_cast<uint64_t>(data[5]) << 40;
    case 5:
      h ^= static_cast<uint64_t>(data[4]) << 32;
    case 4:
      h ^= static_cast<uint64_t>(data[3]) << 24;
    case 3:
      h ^= static_cast<uint64_t>(data[2]) << 16;
    case 2:
      h ^= static_cast<uint64_t>(data[1]) << 8;
    case 1:
      h ^= static_cast<uint64_t>(data[0]);
      h *= m;
  }

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}


}  // namespace leveldb
