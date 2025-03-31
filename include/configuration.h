#ifndef CONFIGURATION_H
#define CONFIGURATION_H

// backend/device/include/device/hwcnt/sampler/configuration.hpp

#include "hwcnt/prfcnt_set.h"
#include "hwcnt/block_metadata.h"
#include "data_structure/bitset.h"

/**
 * Per-block counters configuration.
 */
struct configuration {
    /** Maximum number of hardware counters per block. */
    size_t max_counters_per_block;

    // /** Type used to represent counter numbers bitmask. */
    // using enable_map_type = std::bitset<max_counters_per_block>;

    /** Block type. */
    enum block_type type;

    /** Performance counters set to activate for this block type. */
    enum prfcnt_set set;

    /** Bitmask of counters numbers to enable for this block type. */
    struct bitset enable_map;
};

void configuration__init(struct configuration* config, enum block_type type, enum prfcnt_set set){
    config->max_counters_per_block = 128;
    config->type = type;
    config->set = set;
    bitset__init(&config->enable_map, config->max_counters_per_block);
}

#endif