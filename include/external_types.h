#ifndef ARM_GPU_EXTERNAL_TYPES_H
#define ARM_GPU_EXTERNAL_TYPES_H

#include "hwcpipe_counter.h"
#include "internal_types.h"

// Moved from other sources
struct registered_counter {
    enum hwcpipe_counter counter;
    struct counter_definition definition;
};

#endif