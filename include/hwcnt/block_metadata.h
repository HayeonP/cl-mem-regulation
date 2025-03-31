#ifndef BLOCK_METADATA_H
#define BLOCK_METADATA_H

#include <stdint.h>
#include "hwcnt/prfcnt_set.h"

enum block_type {
    /** Front End. */
    block_type_fe,
    /** Tiler. */
    block_type_tiler,
    /** Memory System. */
    block_type_memory,
    /** Shader Core. */
    block_type_core,
    /** First block type. */
    block_type_first = block_type_fe,
    /** Last block type. */
    block_type_last = block_type_core,
};

/**
 * Block state during the counters sample time.
 *
 * @note If no bits are set, the block is in unknown state,
 * backend_features::has_block_state must be false.
 */
struct block_state {
    /** This block was powered on for at least some portion of the sample */
    uint32_t on : 1;

    /** This block was powered off for at least some portion of the sample */
    uint32_t off : 1;

    /** This block was available to this VM for at least some portion of the sample */
    uint32_t available : 1;

    /** This block was not available to this VM for at least some portion of the sample
     *  Note that no data is collected when the block is not available to the VM.
     */
    uint32_t unavailable : 1;

    /** This block was operating in "normal" (non-protected) mode for at least some portion of the sample */
    uint32_t normal : 1;

    /** This block was operating in "protected" mode for at least some portion of the sample.
     * Note that no data is collected when the block is in protected mode.
     */
    uint32_t protected_mode : 1;
};

/**
 * Hardware counters block metadata.
 *
 * A hardware counters sample is structured as an array of block. Each block has its own type
 * and index. A block type represent the hardware unit that these counters
 * were collected from. And the index is the instance number of this hardware block.
 */
struct block_metadata {
    /** Type of this block. */
    enum block_type type;

    /** Index of this block within the set of blocks of its type. */
    uint8_t index;

    /** Hardware counters set number this block stores. */
    enum prfcnt_set set;

    /** State of this block during the counters sampling time. */
    struct block_state state;

    /**
     * Hardware counters values array.
     *
     * The number of array elements is stored in @ref block_extents::counters_per_block.
     * The value types are in @ref block_extents::values_type.
     *
     * Raw pointer to the shared user-kernel memory.
     * The values here are only valid between @ref reader::get_sample @ref reader::put_sample calls.
     */
    const void *values;
};


#endif