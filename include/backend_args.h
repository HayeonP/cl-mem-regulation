#ifndef ARM_GPU_BACKEND_ARGS_H
#define ARM_GPU_BACKEND_ARGS_H

// backend/device/src/device/hwcnt/sampler/base/backend_args.hpp
// backend/device/src/device/hwcnt/sampler/vinstr/backend.hpp

#include <stdint.h>
#include <string.h>
#include "mapped_memory.h"
#include "hwcnt/block_extents.h"
#include "hwcnt/features.h"
#include "vinstr_types.h"
#include "sample_layout.h"

struct backend_args {
    /** Hardware counters fd. */
    int fd;
    /** Sampling period (nanoseconds). */
    uint64_t period_us;
    /** Hardware counters features. */
    struct features features_v;
    /** Block extents. */
    struct block_extents extents;
    /** Counters buffer memory. */
    struct mapped_memory memory;
};

struct vinstr_backend_args {
    struct backend_args base_args;
    enum vinstr_reader_features features;
    size_t buffer_size;
    struct sample_layout sample_layout_v;
    size_t max_buffer_count;
};

void vinstr_backend_args__init(struct vinstr_backend_args* args){
    memset(args, 0, sizeof(struct vinstr_backend_args));
    args->max_buffer_count = 32;
    args->buffer_size = 0;
    return;
}

#endif