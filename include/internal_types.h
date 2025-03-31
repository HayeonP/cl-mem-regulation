#ifndef INTERNAL_TYPES_H
#define INTERNAL_TYPES_H

#include <stdint.h>
#include "hwcnt/block_metadata.h"

/**
 * Structure representing the block type/offset address and shift
 * scaling of a counter within a particular GPU's PMU data.
 */
struct block_offset{
    uint32_t offset;
    uint32_t shift;
    enum block_type block_type;
};

/**
 * Counters can either be raw hardware counters, or they are expressions based
 * on some combination of hardware counters, constants or other expressions.
 *
 * For hardware counters, we need to know their block/offset addresses so that
 * we can read them as we iterate over the block types. For expression counters
 * we need to call the function that will evaluate the formula.
 */
struct hardware_counter_definition {
    struct block_offset address;
    uint8_t type;
};

/**
 * Counters can either be raw hardware counters, or they are expressions based
 * on some combination of hardware counters, constants or other expressions.
 *
 * For hardware counters, we need to know their block/offset addresses so that
 * we can read them as we iterate over the block types. For expression counters
 * we need to call the function that will evaluate the formula.
 */

enum counter_type { invalid, hardware, expression };

struct counter_definition {
    struct block_offset address;
    enum counter_type tag;
};

void config_definition__init(struct counter_definition* def, struct block_offset* address){
    def->tag = hardware;
    def->address = *address;

    return;
}

#endif