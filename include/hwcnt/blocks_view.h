#ifndef ARM_GPU_HWCNT_BLOCKS_VIEW_H
#define ARM_GPU_HWCNT_BLOCKS_VIEW_H

// backend/device/include/device/hwcnt/blocks_view.hpp

#include "backend.h"
#include "handle.h"
#include "block_iterator.h"

struct vinstr_blocks_view {
    struct vinstr_backend* reader_;
    struct sample_handle sample_hndl_;
};

void blocks_view__init(struct vinstr_blocks_view* bv, struct vinstr_backend* r, struct sample_handle* sample_hndl){
    memset(bv, 0, sizeof(struct vinstr_blocks_view));
    bv->reader_ = r;
    bv->sample_hndl_ = *sample_hndl;
    return;
}

struct vinstr_block_iterator blocks_view__begin(struct vinstr_blocks_view* bv) {
    struct vinstr_block_iterator bi;
    block_iterator__init_with_args(&bi, bv->reader_, &bv->sample_hndl_);
    return bi;
}

struct vinstr_block_iterator blocks_view__end(struct vinstr_blocks_view* bv){
    struct vinstr_block_iterator bi;
    block_iterator__init(&bi);
    return bi;
}

#endif