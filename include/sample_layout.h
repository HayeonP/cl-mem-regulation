#ifndef SAMPLE_LAYOUT_H
#define SAMPLE_LAYOUT_H

// backend/device/src/device/hwcnt/sampler/vinstr/sample_layout.hpp

#include <stdint.h>
#include "product_id.h"
#include "hwcnt/block_metadata.h"
#include "hwcnt/block_extents.h"
#include "data_structure/bitset.h"

size_t max_shader_cores = 64;

enum sample_layout_type {
    sample_layout_type_v4,
    sample_layout_type_non_v4,
};

/** Sample layout entry. */
struct entry {
    /** Block type. */
    enum block_type type;
    /** Block index. */
    uint8_t index;
    /** Block offset from the sample start. */
    size_t offset;
};

inline int is_v4_layout(struct product_id* id);

int is_v4_layout(struct product_id* id){
    struct product_id t760;
    product_id__init_with_raw_product_id(&t760, 0x0760);

    switch(product_id__get_version_style(id, id->product_id_)) {
        case version_style_legacy_t60x:
            return 1;
        case version_style_legacy_txxx:
            return id->product_id_ < t760.product_id_;
        default:
            return 0;
    }
}

struct sample_layout {
    size_t max_blocks_fe;
    size_t max_blocks_tiler;
    size_t max_blocks_memory;
    size_t max_blocks;
    size_t counters_per_block;
    size_t block_size;

    struct entry* layout_;
    size_t num_blocks_;
    enum sample_layout_type sample_layout_type_;
};

void sample_layout__init(struct sample_layout* sl){
    sl->max_blocks_fe = 1;
    sl->max_blocks_tiler = 1;
    sl->max_blocks_memory = 16;
    sl->max_blocks = sl->max_blocks_fe + sl->max_blocks_tiler + sl->max_blocks_memory + max_shader_cores;
    sl->counters_per_block = 64;
    sl->block_size = sl->counters_per_block * sizeof(uint32_t);
    sl->num_blocks_ = 0;
    sl->layout_ = (struct entry*)malloc(sizeof(struct entry) * sl->max_blocks);

    return;
}

/**
 * Add layout entry for a block.
 *
 * @param[in] value    Entry to add.
 */
void sample_layout__push_back(struct sample_layout* sl, struct entry* value){
    sl->layout_[sl->num_blocks_] = *value;
    sl->num_blocks_ += 1;
    return;
}

/**
 * Populate the relevant blocks to support a v4 block layout
 */
void sample_layout_v4(struct sample_layout* sl, struct block_extents* extents, struct bitset* sc_mask){
    // TODO
}

/**
 * Populate the relevant blocks to support a v5/6 block layout
 */
void sample_layout_non_v4(struct sample_layout* sl, struct block_extents* extents, uint64_t num_l2_slices, struct bitset* sc_mask){
    /* Populate front-end block. */
    size_t offset_fe = 0;
    if(extents->num_blocks_of_type_[block_type_fe] != 0){
        struct entry entry;
        memset(&entry, 0, sizeof(struct entry));
        entry.type = block_type_fe;
        entry.index = 0;
        entry.offset = offset_fe;
        sample_layout__push_back(sl, &entry);
#ifdef ENABLE_DEBUG
        printf("(sample_layout_non_v4) offset_fe: %lu\n", offset_fe);
#endif
    }
    
    /* Populate tiler block */
    size_t offset_tiler = offset_fe + sl->block_size;
    if(extents->num_blocks_of_type_[block_type_tiler] != 0){
        struct entry entry;
        memset(&entry, 0, sizeof(struct entry));
        entry.type = block_type_tiler;
        entry.index = 0;
        entry.offset = offset_tiler;
        sample_layout__push_back(sl, &entry);
#ifdef ENABLE_DEBUG        
        printf("(sample_layout_non_v4) offset_tiler: %lu\n", offset_tiler);
#endif        
    }
    
    /* Populate memory block. */        
    size_t offset_memory = offset_tiler + sl->block_size;
    if(extents->num_blocks_of_type_[block_type_memory] != 0){
        for (uint8_t i = 0; i < (uint8_t)(num_l2_slices); ++i){
            struct entry entry;
            memset(&entry, 0, sizeof(struct entry));
            entry.type = block_type_memory;
            entry.index = i;
            entry.offset = offset_memory + i * sl->block_size;
            sample_layout__push_back(sl, &entry);
#ifdef ENABLE_DEBUG            
            printf("(sample_layout_non_v4) offset_memory: %lu\n", entry.offset);
#endif            
        }
    }
    
    /* Populate shader cores block */
    size_t offset_sc = (size_t)(offset_memory + sl->block_size * num_l2_slices);
    if(extents->num_blocks_of_type_[block_type_core] != 0){
        uint8_t sc_index = 0;
        for (uint8_t i = 0; i < (uint8_t)(sc_mask->size); ++i){
            if(!bitset__get_bit(sc_mask, i)) continue;

            struct entry entry;
            memset(&entry, 0, sizeof(struct entry));
            entry.type = block_type_core;
            entry.index = sc_index;
            entry.offset = offset_sc + sl->block_size * i;
            sample_layout__push_back(sl, &entry);
#ifdef ENABLE_DEBUG            
            printf("(sample_layout_non_v4) offset_memory: %lu\n", entry.offset);
#endif            
            ++sc_index;
        }
    }
    
    return;
}


void sample_layout__init_with_args(struct sample_layout* sl, struct block_extents* extents, uint64_t num_l2_slices, struct bitset* sc_mask, enum sample_layout_type sample_layout_type){
    sample_layout__init(sl);
    sl->sample_layout_type_ = sample_layout_type;

    switch(sl->sample_layout_type_){
        case sample_layout_type_v4:
            // sample_layout_v4(); // TODO
            break;
        case sample_layout_type_non_v4:
            sample_layout_non_v4(sl, extents, num_l2_slices, sc_mask);
            break;
    }
}

#endif