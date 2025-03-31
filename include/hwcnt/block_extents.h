#ifndef BLOCK_EXTENTS
#define BLOCK_EXTENTS

// backend/device/include/device/hwcnt/block_extents.hpp

#include "hwcnt/block_metadata.h"

enum sample_values_type{
    sample_values_uint32 = 0,
    sample_values_uint64 = 1,
};

struct block_extents {
    int num_block_types;
    uint8_t *num_blocks_of_type_;
    uint16_t counters_per_block_;
    enum sample_values_type values_type_;
};

void block_extents__init(struct block_extents* extents){
    memset(extents, 0, sizeof(struct block_extents));
    extents->num_block_types = block_type_last + 1;
    extents->num_blocks_of_type_ = (uint8_t *)malloc(sizeof(uint8_t) * extents->num_block_types);
    memset(extents->num_blocks_of_type_, 0, sizeof(uint8_t) * extents->num_block_types);
    extents->counters_per_block_ = 0;
    
    return;
}

void block_extents__init_with_args(struct block_extents* extents, uint8_t* num_blocks_of_type_v, uint16_t counters_per_block, enum sample_values_type values_type){
    memset(extents, 0, sizeof(struct block_extents));
    extents->num_block_types = block_type_last + 1;
    extents->num_blocks_of_type_ = (uint8_t *)malloc(sizeof(uint8_t) * extents->num_block_types);
    memset(extents->num_blocks_of_type_, 0, sizeof(uint8_t) * extents->num_block_types);
    for(int i = 0; i < extents->num_block_types; i++){        
        extents->num_blocks_of_type_[i] = num_blocks_of_type_v[i];
    }
    extents->counters_per_block_ = counters_per_block;
    extents->values_type_ = values_type;

    return;
}

uint8_t block_extents__num_blocks(struct block_extents* extents){
    uint8_t result;
    for (int i = 0; i < extents->num_block_types; i++){
        result += (uint8_t)(extents->num_blocks_of_type_[i]);
    }

    return result;
}


#endif