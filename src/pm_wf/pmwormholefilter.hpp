#ifndef PMWORMHOLE_FILTER_HPP_
#define PMWORMHOLE_FILTER_HPP_

#include <iostream>
#include <libpmemobj.h>
#include <random>
#include <stdint.h>
#include <stdlib.h>

using namespace std;

class PMWF_TwoIndependentMultiplyShift
{
    unsigned __int128 multiply_, add_;

public:
    PMWF_TwoIndependentMultiplyShift()
    {
        ::std::random_device random;
        for (auto v : {&multiply_, &add_})
        {
            *v = random();
            for (int i = 1; i <= 4; ++i)
            {
                *v = *v << 32;
                *v |= random();
            }
        }
    }

    uint64_t operator()(uint64_t key) const
    {
        return key;
    }
};

#define SLOT_PER_BUK 4

#define BITS_PER_TAG 16
#define BITS_PER_DIS 4
#define BITS_PER_FPT 12

#define TAG_MASK (0xFFFFULL)
#define DIS_MASK (0x000FULL)
#define FPT_MASK (0xFFF0ULL)

#define MAX_PROB 16

#define MOD(idx, num_buckets_) ((idx) % num_buckets_)

#define haszero16(x) (((x)-0x0001000100010001ULL) & (~(x)) & 0x8000800080008000ULL)
#define hasvalue16(x, n) (haszero16((x) ^ (0x0001000100010001ULL * (n))))

POBJ_LAYOUT_BEGIN(pmwormholefilter);
POBJ_LAYOUT_ROOT(pmwormholefilter, struct pmwormholefilter_root);
POBJ_LAYOUT_TOID(pmwormholefilter, struct pmwormholefilter);
POBJ_LAYOUT_END(pmwormholefilter);

struct pmwormholefilter
{
    uint32_t num_buckets_;

    PMWF_TwoIndependentMultiplyShift hasher_;

    uint64_t buckets_[];
};

struct pmwormholefilter_root
{
    TOID(struct pmwormholefilter)
    pmwormholefilter;
};

void pmwormholefilter_init(PMEMobjpool *pop, TOID(struct pmwormholefilter_root) pmwormholefilter_root, uint32_t max_num_keys);

void pmwormholefilter_destroy(PMEMobjpool *pop, TOID(struct pmwormholefilter_root) pmwormholefilter_root);

int pmwormholefilter_insert(PMEMobjpool *pop, TOID(struct pmwormholefilter_root) pmwormholefilter_root, uint64_t key_);

int pmwormholefilter_lookup(PMEMobjpool *pop, TOID(struct pmwormholefilter_root) pmwormholefilter_root, uint64_t key_);

int pmwormholefilter_delete(PMEMobjpool *pop, TOID(struct pmwormholefilter_root) pmwormholefilter_root, uint64_t key_);

int pmwormholefilter_bytes(PMEMobjpool *pop, TOID(struct pmwormholefilter_root) pmwormholefilter_root);

void pmwormholefilter_info(PMEMobjpool *pop, TOID(struct pmwormholefilter_root) pmwormholefilter_root);

void pmwormholefilter_init(PMEMobjpool *pop, TOID(struct pmwormholefilter_root) pmwormholefilter_root, uint32_t max_num_keys)
{
    TX_BEGIN(pop)
    {
        pmemobj_tx_add_range_direct(D_RW(pmwormholefilter_root), sizeof(*D_RW(pmwormholefilter_root)));
        struct pmwormholefilter_root *p_pmwormholefilter_root = D_RW(pmwormholefilter_root);
        p_pmwormholefilter_root->pmwormholefilter = TX_ZALLOC(struct pmwormholefilter, sizeof(struct pmwormholefilter) + (sizeof(uint64_t) * int((max_num_keys / SLOT_PER_BUK) / 0.8)));

        struct pmwormholefilter *p_pmwormholefilter = D_RW(p_pmwormholefilter_root->pmwormholefilter);
        pmemobj_tx_add_range_direct(p_pmwormholefilter, sizeof(struct pmwormholefilter));

        p_pmwormholefilter->num_buckets_ = int((max_num_keys / SLOT_PER_BUK) / 0.8);

        PMWF_TwoIndependentMultiplyShift hash_;
        p_pmwormholefilter->hasher_ = hash_;
    }
    TX_END;

    return;
}

void pmwormholefilter_destroy(PMEMobjpool *pop, TOID(struct pmwormholefilter_root) pmwormholefilter_root)
{
    TX_BEGIN(pop)
    {
        pmemobj_tx_add_range_direct(D_RW(pmwormholefilter_root), sizeof(*D_RW(pmwormholefilter_root)));
        TX_FREE(D_RW(pmwormholefilter_root)->pmwormholefilter);
        D_RW(pmwormholefilter_root)->pmwormholefilter = TOID_NULL(struct pmwormholefilter);
    }
    TX_END;
}

inline uint32_t index_hash(uint32_t hv, uint32_t num_buckets_)
{
    return hv % num_buckets_;
}

inline uint32_t tag_hash(uint32_t hv)
{
    uint32_t tag = hv & (((1ULL << BITS_PER_FPT) - 1));
    tag += (tag == 0);
    return (tag << BITS_PER_DIS);
}

inline uint32_t ReadTag(struct pmwormholefilter *pmwormholefilter, const uint32_t i, const uint32_t j)
{
    uint32_t i_m = MOD(i, pmwormholefilter->num_buckets_);
    return ((uint16_t *)(&(pmwormholefilter->buckets_[i_m])))[j];
}

inline void WriteTag(struct pmwormholefilter *pmwormholefilter, const uint32_t i, const uint32_t j, const uint32_t t)
{
    uint32_t i_m = MOD(i, pmwormholefilter->num_buckets_);
    ((uint16_t *)(&(pmwormholefilter->buckets_[i_m])))[j] = t;
}

inline void PMWriteTag(PMEMobjpool *pop, struct pmwormholefilter *pmwormholefilter, const uint32_t i, const uint32_t j, const uint32_t t)
{
    uint32_t i_m = MOD(i, pmwormholefilter->num_buckets_);
    ((uint16_t *)(&(pmwormholefilter->buckets_[i_m])))[j] = t;

    pmemobj_persist(pop, &pmwormholefilter->buckets_[i_m], sizeof(uint64_t));
}

int pmwormholefilter_insert(PMEMobjpool *pop, TOID(struct pmwormholefilter_root) pmwormholefilter_root, uint64_t key_)
{
    struct pmwormholefilter *p_pmwormholefilter = D_RW(D_RW(pmwormholefilter_root)->pmwormholefilter);

    const uint64_t hash = p_pmwormholefilter->hasher_(key_);
    uint64_t init_buck_idx = index_hash(hash, p_pmwormholefilter->num_buckets_);
    uint64_t tag = tag_hash(hash >> 32);

    for (uint32_t curr_buck_idx = init_buck_idx; curr_buck_idx < init_buck_idx + p_pmwormholefilter->num_buckets_; curr_buck_idx++)
    {
        for (uint32_t curr_tag_idx = 0; curr_tag_idx < SLOT_PER_BUK; curr_tag_idx++)
        {
            if (ReadTag(p_pmwormholefilter, curr_buck_idx, curr_tag_idx) == 0)
            {
                while ((curr_buck_idx - init_buck_idx) >= MAX_PROB)
                {
                    bool has_cadi = false;
                    for (uint32_t prob = MAX_PROB - 1; prob > 0; prob--)
                    {
                        uint32_t cadi_buck_idx = curr_buck_idx - prob;
                        bool find_cadi = false;
                        for (uint32_t cadi_tag_idx = 0; cadi_tag_idx < SLOT_PER_BUK; cadi_tag_idx++)
                        {
                            uint32_t cadi_tag = ReadTag(p_pmwormholefilter, cadi_buck_idx, cadi_tag_idx);

                            if ((cadi_tag & DIS_MASK) + prob < MAX_PROB)
                            {
                                PMWriteTag(pop, p_pmwormholefilter, curr_buck_idx, curr_tag_idx, ((cadi_tag & FPT_MASK) | ((cadi_tag & DIS_MASK) + prob)));
                                curr_buck_idx = cadi_buck_idx;
                                curr_tag_idx = cadi_tag_idx;
                                find_cadi = true;
                                break;
                            }
                        }
                        if (find_cadi)
                        {
                            has_cadi = true;
                            break;
                        }
                    }
                    if (!has_cadi)
                    {
                        return false;
                    }
                }
                WriteTag(p_pmwormholefilter, curr_buck_idx, curr_tag_idx, (tag | (curr_buck_idx - init_buck_idx)));
                return true;
            }
        }
    }
    return false;
}

int pmwormholefilter_lookup(PMEMobjpool *pop, TOID(struct pmwormholefilter_root) pmwormholefilter_root, uint64_t key_)
{
    struct pmwormholefilter *p_pmwormholefilter = D_RW(D_RW(pmwormholefilter_root)->pmwormholefilter);
    const uint64_t hash = p_pmwormholefilter->hasher_(key_);

    uint64_t init_buck_idx = index_hash(hash, p_pmwormholefilter->num_buckets_);
    uint64_t tag = tag_hash(hash >> 32);

    for (uint32_t prob = 0; prob < MAX_PROB; prob++)
    {
        uint32_t curr_buck_idx_mod = MOD(init_buck_idx + prob, p_pmwormholefilter->num_buckets_);
        if (hasvalue16(p_pmwormholefilter->buckets_[curr_buck_idx_mod], (tag | prob)))
        {
            return true;
        }
    }

    return false;
}

int pmwormholefilter_delete(PMEMobjpool *pop, TOID(struct pmwormholefilter_root) pmwormholefilter_root, uint64_t key_)
{
    struct pmwormholefilter *p_pmwormholefilter = D_RW(D_RW(pmwormholefilter_root)->pmwormholefilter);

    const uint64_t hash = p_pmwormholefilter->hasher_(key_);
    uint64_t init_buck_idx = index_hash(hash, p_pmwormholefilter->num_buckets_);
    uint64_t tag = tag_hash(hash >> 32);

    for (uint32_t prob = 0; prob < MAX_PROB; prob++)
    {
        for (size_t curr_tag_idx = 0; curr_tag_idx < SLOT_PER_BUK; curr_tag_idx++)
        {
            if (ReadTag(p_pmwormholefilter, init_buck_idx + prob, curr_tag_idx) == (tag | prob))
            {
                WriteTag(p_pmwormholefilter, init_buck_idx + prob, curr_tag_idx, 0);
                return true;
            }
        }
    }
    return false;
}

int pmwormholefilter_bytes(PMEMobjpool *pop, TOID(struct pmwormholefilter_root) pmwormholefilter_root)
{
    struct pmwormholefilter *p_pmwormholefilter = D_RW(D_RW(pmwormholefilter_root)->pmwormholefilter);
    return sizeof(uint64_t) * p_pmwormholefilter->num_buckets_;
}

void pmwormholefilter_info(PMEMobjpool *pop, TOID(struct pmwormholefilter_root) pmwormholefilter_root)
{
    struct pmwormholefilter *p_pmwormholefilter = D_RW(D_RW(pmwormholefilter_root)->pmwormholefilter);

    cout << "INFO:" << endl;
    cout << p_pmwormholefilter->hasher_(1) << endl;
    cout << p_pmwormholefilter->hasher_(2) << endl;

    return;
}

#endif // PMWORMHOLE_FILTER_HPP_