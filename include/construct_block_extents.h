#ifndef CONSTRUCT_BLOCK_EXTENTS_H
#define CONSTRUCT_BLOCK_EXTENTS_H

// backend/device/src/device/hwcnt/sampler/vinstr/construct_block_extents.hpp

#include <stdint.h>
#include "product_id.h"
#include "hwcnt/block_extents.h"
#include "sample_layout.h"

/**
 * Construct block extents.
 *
 * @param[in] pid                   Product ID.
 * @param[in] num_l2_slices         Number of L2 slices, which will be parsed to determine the number of memory blocks.
 * @param[in] num_shader_cores      Number of shader cores.
 * @param[in] counters_per_block    Counters per block.
 * @param[in] values_type           Type of hardware counters values.
 * @param[in] extents               Output block extents
 */

inline void construct_block_extents(struct product_id* pid, uint64_t num_l2_slices, uint64_t num_shader_cores, struct block_extents* extents);

void construct_block_extents(struct product_id* pid, uint64_t num_l2_slices, uint64_t num_shader_cores, struct block_extents* extents){
    uint8_t num_memory_blocks;
    if(is_v4_layout(pid))
        num_memory_blocks = 1;
    else
        num_memory_blocks = num_l2_slices;
    uint8_t num_blocks_of_type_v[4] = {
        1,                          // num_fe_blocks
        1,                          // num_tiler_blocks
        num_memory_blocks,          // num_memory_blocks
        (uint8_t)(num_shader_cores) // num_core_blocks
    };

    block_extents__init_with_args(extents, num_blocks_of_type_v, 64, sample_values_uint32);
    return;
}

#endif