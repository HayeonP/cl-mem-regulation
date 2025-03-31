#ifndef ARM_GPU_CONVERT_H
#define ARM_GPU_CONVERT_H

// backend/device/src/device/hwcnt/sampler/vinstr/convert.hpp

#include <stdint.h>
#include "debug.h"
#include "configuration.h"
#include "kbase_types.h"
#include "hwcnt/block_extents.h"
#include "hwcnt/prfcnt_set.h"
#include "data_structure/bitset.h"

inline uint32_t convert_by_mask(struct bitset* mask);
inline void convert(struct hwcnt_reader_setup* result, struct configuration *begin, struct configuration *end);

/**
 * Convert from hwcpipe enable mask to vinstr enable mask.
 *
 * @param[in] mask Hwcpipe mask to convert.
 * @return vinstr counters mask.
 */
uint32_t convert_by_mask(struct bitset* mask) {
    uint32_t result = 0;
    size_t idx = 0;
    int remaining_size = mask->size;

    while (remaining_size > 0) {
        uint32_t current_bits = mask->bits[0] & 0b1111;
        if (current_bits) {
            result |= (1U << idx);
        }

        // Right shift the mask by COUNTERS_PER_BIT bits
        for (size_t i = 0; i < (mask->size + 31) / 32; i++) {
            if (i + 1 < (mask->size + 31) / 32) {
                mask->bits[i] = (mask->bits[i] >> 0b1111) | (mask->bits[i + 1] << (32 - 0b1111));
            } else {
                mask->bits[i] >>= 0b1111;
            }
        }

        remaining_size -= 0b111;
        idx++;
    }

    return result;
}

/**
 * Convert hwcpipe configuration arguments to vinstr setup arguments.
 *
 * @param[in] begin    Begin iterator.
 * @param[in] end End iterator.
 * @return A pair of error code and `hwcnt_reader_setup` structure.
 */
void convert(struct hwcnt_reader_setup* result, struct configuration *begin, struct configuration *end) {
    for (struct configuration* it = begin; it != end; ++it) {
        if (it->set != prfcnt_set_primary){
            printf("[ERROR] (convert) Not supported.\n");
            exit(0);
        }

        switch (it->type) {
        case block_type_fe:
            result->fe_bm |= convert_by_mask(&it->enable_map);
            break;
        case block_type_tiler:
            result->tiler_bm |= convert_by_mask(&it->enable_map);
            break;
        case block_type_memory:
            // result->mmu_l2_bm |= convert_by_mask(&it->enable_map);
            
            // TODO: hardcoding
            
            // result->mmu_l2_bm = 1152; // for ExtBusRd, EXBusWr
            result->mmu_l2_bm = 2304; // for ExtBusRdBt, EXBusWrBt
            break;
        case block_type_core:
            result->shader_bm |= convert_by_mask(&it->enable_map);
            break;
        }
    }
#ifdef ENABLE_DEBUG
    printf("(convert) result.fe_bm: %u\n", (uint32_t)result->fe_bm);
    printf("(convert) result.tiler_bm: %u\n", (uint32_t)result->tiler_bm);
    printf("(convert) result.mmu_l2_bm: %u\n", (uint32_t)result->mmu_l2_bm);
    printf("(convert) result.shader_bm: %u\n", (uint32_t)result->shader_bm);
#endif

    return;
}

#endif
