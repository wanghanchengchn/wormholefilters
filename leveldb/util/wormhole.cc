// Copyright (c) 2012 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <iostream>
#include <random>
#include <bitset>

#include "leveldb/filter_policy.h"
#include "leveldb/slice.h"

#include "util/hash.h"

#define BIT_PER_TAG 16
#define BIT_PER_FPT 12
#define TAG_PER_BUK 4

#define TAG_MASK (0xFFFFULL)
#define DIS_MASK (0x000FULL)
#define FPT_MASK (0xFFF0ULL)

#define MAX_PROB 16

#define MOD(idx, num_buckets_) ((idx) & (num_buckets_ - 1))

#define haszero16(x) \
  (((x)-0x0001000100010001ULL) & (~(x)) & 0x8000800080008000ULL)
#define hasvalue16(x, n) (haszero16((x) ^ (0x0001000100010001ULL * (n))))

namespace leveldb {

namespace {

static uint64_t WormholeHash(const Slice& key) {
  return Hash64(key.data(), key.size(), 0xbc9f1d34bc9f1d34);
}

inline uint32_t IndexHash(uint32_t hv, uint64_t num_buckets_) {
  return hv & (num_buckets_ - 1);
}

inline uint32_t TagHash(uint32_t hv) {
  uint32_t tag;
  tag = hv & ((1ULL << BIT_PER_FPT) - 1);
  tag += (tag == 0);
  return tag;
}

inline uint64_t upperpower2(uint64_t x) {
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  x++;
  return x;
}

inline uint32_t ReadTag(const uint32_t i, const uint32_t j,const char* array,
                        uint64_t num_buckets_, uint32_t kTagMask) {
  uint32_t i_m = MOD(i, num_buckets_);
  const char* p = array + i_m * 8;
  uint32_t tag;
  p += (j << 1);
  tag = *((uint16_t*)p);
  return tag & kTagMask;
}

inline void WriteTag(const uint32_t i, const uint32_t j, const uint32_t t,
                     char* array, uint64_t num_buckets_, uint32_t kTagMask) {
  uint32_t i_m = MOD(i, num_buckets_);
  char* p = array + i_m * 8;
  uint32_t tag = t & kTagMask;
  ((uint16_t*)p)[j] = tag;
}

bool InsertItem(uint64_t hashcode, char* array, uint64_t num_buckets_,
                uint32_t kTagMask) {
  uint64_t init_buck_idx = IndexHash(hashcode, num_buckets_);
  uint64_t tag = TagHash(hashcode >> 32);

  for (uint32_t curr_buck_idx = init_buck_idx;
       curr_buck_idx < init_buck_idx + num_buckets_; curr_buck_idx++) {
    for (uint32_t curr_tag_idx = 0; curr_tag_idx < TAG_PER_BUK;
         curr_tag_idx++) {
      if (ReadTag(curr_buck_idx, curr_tag_idx, array, num_buckets_, kTagMask) ==
          0) {
        while ((curr_buck_idx - init_buck_idx) >= MAX_PROB) {
          bool has_cadi = false;
          for (uint32_t prob = MAX_PROB - 1; prob > 0; prob--) {
            uint32_t cadi_buck_idx = curr_buck_idx - prob;
            bool find_cadi = false;
            for (uint32_t cadi_tag_idx = 0; cadi_tag_idx < TAG_PER_BUK;
                 cadi_tag_idx++) {
              uint32_t cadi_tag = ReadTag(cadi_buck_idx, cadi_tag_idx, array,
                                          num_buckets_, kTagMask);
              if ((cadi_tag & DIS_MASK) + prob < MAX_PROB) {
                WriteTag(curr_buck_idx, curr_tag_idx,
                         (cadi_tag & FPT_MASK) | ((cadi_tag & DIS_MASK) + prob),
                         array, num_buckets_, kTagMask);
                curr_buck_idx = cadi_buck_idx;
                curr_tag_idx = cadi_tag_idx;
                find_cadi = true;
                break;
              }
            }
            if (find_cadi) {
              has_cadi = true;
              break;
            }
          }
          if (!has_cadi) {
            return false;
          }
        }
        WriteTag(curr_buck_idx, curr_tag_idx,
                 ((tag << 4) | (curr_buck_idx - init_buck_idx)), array,
                 num_buckets_, kTagMask);
        return true;
      }
    }
  }
  return false;
}

class WormholeFilterPolicy : public FilterPolicy {
 public:
  explicit WormholeFilterPolicy() {}

  const char* Name() const override { return "leveldb.BuiltinWormholeFilter2"; }

  void CreateFilter(const Slice* keys, int n, std::string* dst) const override {
    // Compute Wormhole filter size
    const uint32_t kBytesPerBucket = (BIT_PER_TAG * TAG_PER_BUK + 7) >> 3;
    const uint32_t kTagMask = (1ULL << BIT_PER_TAG) - 1;
    uint64_t num_buckets_ = upperpower2(std::max<uint64_t>(1, n / TAG_PER_BUK));
    double frac = (double)n / num_buckets_ / TAG_PER_BUK;
    if (frac > 0.8) {
      num_buckets_ <<= 1;
    }

    size_t bytes = kBytesPerBucket * num_buckets_;

    const size_t init_size = dst->size();
    dst->resize(init_size + bytes, 0);
    char* array = &(*dst)[init_size];

    for (int i = 0; i < n; i++) {
      uint64_t hashcode = WormholeHash(keys[i]);
      InsertItem(hashcode, array, num_buckets_, kTagMask);
    }

    dst->resize(init_size + bytes + 8, 0);
    array = &(*dst)[init_size + bytes];
    for (int i = 0; i < 8; i++) {
      array[i] = ((char*)&num_buckets_)[i];
    }
  }

  bool KeyMayMatch(const Slice& key,
                   const Slice& Wormhole_filter) const override {
    const size_t len = Wormhole_filter.size();
    if (len < 2) return false;

    const char* array = Wormhole_filter.data();

    uint64_t num_buckets_;
    for (int i = 0; i < 8; i++) {
      ((char*)&num_buckets_)[i] = array[len - 8 + i];
    }

    uint64_t hashcode = WormholeHash(key);
    uint64_t init_buck_idx = IndexHash(hashcode, num_buckets_);
    uint64_t tag = TagHash(hashcode >> 32);
    for (uint32_t prob = 0; prob < MAX_PROB; prob++) {
      uint32_t curr_buck_idx_mod = MOD(init_buck_idx + prob, num_buckets_);
      const char* p = array + curr_buck_idx_mod * 8;
      if (hasvalue16(*((uint64_t*)p), (tag << 4) | (prob))) {
        return true;
      }
    }
    return false;
  }
};
}  // namespace

const FilterPolicy* NewWormholeFilterPolicy() {
  return new WormholeFilterPolicy();
}

}  // namespace leveldb
