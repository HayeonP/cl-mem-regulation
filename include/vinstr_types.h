#ifndef ARM_GPU_VINSTR_TYPES_H
#define ARM_GPU_VINSTR_TYPES_H

// backend/device/src/device/ioctl/vinstr/types.hpp

#include <stdint.h>

enum vinstr_reader_event {
    /** Manual request for dump. */
    vinstr_reader_manual,
    /** Periodic dump. */
    vinstr_reader_periodic,
    /** Prejob dump request. */
    vinstr_reader_prejob,
    /** Postjob dump request. */
    vinstr_reader_postjob, 
};

/** Features that HWCNT reader supports. */
enum vinstr_reader_features{
    /** HWCNT samples are annotated with the top cycle counter. */
    cycles_top = (1<< 0),
    /** HWCNT samples are annotated with the shader cores cycle counter. */
    cycles_shader_core = (1 << 1),
};

/** GPU clock cycles. */
struct vinstr_reader_metadata_cycles {
    /** The number of cycles associated with the main clock for the GPU. */
    uint64_t top;
    /** The cycles that have elapsed on the GPU shader cores. */
    uint64_t shader_cores;
};

/** HWCNT reader sample buffer metadata. */
struct vinstr_reader_metadata {
    /** Time when sample was collected. */
    uint64_t timestamp;
    /** ID of an event that triggered sample collection. */
    enum vinstr_reader_event event_id;
    /** Position in sampling area where sample buffer was stored. */
    uint32_t buffer_idx;
};

/** HWCNT reader sample buffer metadata annotated with cycle counts. */
struct vinstr_reader_metadata_with_cycles {
    /** Reader metadata. */
    struct vinstr_reader_metadata metadata;
    /** The GPU cycles that occurred since the last sample. */
    struct vinstr_reader_metadata_cycles cycles;
};

/** HWCNT reader API version. */
struct vinstr_reader_api_version {
    /** API version */
    uint32_t version;
    /** Available features in this API version. */
    enum vinstr_reader_features features;
};

// Bitwise operations for reader_features
#define VINSTR_READER_NOT(val) (~(val))
#define VINSTR_READER_AND(lhs, rhs) ((lhs) & (rhs))
#define VINSTR_READER_OR(lhs, rhs) ((lhs) | (rhs))
#define VINSTR_READER_XOR(lhs, rhs) ((lhs) ^ (rhs))
#define VINSTR_READER_OR_ASSIGN(lhs, rhs) ((lhs) |= (rhs))
#define VINSTR_READER_AND_ASSIGN(lhs, rhs) ((lhs) &= (rhs))
#define VINSTR_READER_XOR_ASSIGN(lhs, rhs) ((lhs) ^= (rhs))

#endif 