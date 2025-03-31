#ifndef ARM_GPU_FILTER_BLOCK_EXTENTS_H
#define ARM_GPU_FILTER_BLOCK_EXTENTS_H

#include <stdint.h>
#include "hwcnt/block_extents.h"
#include "configuration.h"

// backend/device/src/device/hwcnt/sampler/filter_block_extents.hpp

inline void filter_block_extents(struct block_extents* output, struct block_extents* input, struct configuration* begin, struct configuration* end);

/**
 * Filter block extents.
 *
 * @param[in] extents     Extents to filter.
 * @param[in] begin       Counters configuration begin.
 * @param[in] end         Counters configuration end.
 * @return Pair of error code and block extents filtered.
 */
void filter_block_extents(struct block_extents* output, struct block_extents* input, struct configuration* begin, struct configuration* end) {
    if (begin == NULL) {
        printf("[ERROR] (filter_block_extents) Null pointer argument.\n");
        exit(0);
    }

    block_extents__init(output);
    
    if (begin->type > end->type) {
        printf("[ERROR] (filter_block_extents) begin > end.\n");
        exit(0);
    }

    for (struct configuration* it = begin; it != end; ++it) {
        size_t block_type_idx = (size_t)it->type;
        output->num_blocks_of_type_[block_type_idx] = input->num_blocks_of_type_[block_type_idx];
    }

    output->counters_per_block_ = input->counters_per_block_;
    output->values_type_ = input->values_type_;

    return;
}

#endif 