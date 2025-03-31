#ifndef ARM_GPU_FEATURES_H
#define ARM_GPU_FEATURES_H

// backend/device/include/device/hwcnt/features.hpp

/**
 *  Features that the kernel-side hardware counters infrastructure supports.
 *
 * The device library supports different versions of the hardware
 * counters ioctl interface. Some of them provide certain counters
 * annotation meta-data, and some of them don't. This structure lists
 * which features are supported.
 *
 * @par Example
 * @code
 * // Check the feature before accessing gpu_cycle
 * if (reader.get_features().has_gpu_cycle)
 *     printf("GPU timestamp = %lu\n", sample.get_metadata().gpu_cycle);
 * else
 *     printf("No GPU Timestamp data!\n");
 * @endcode
 */
struct features {
    /**
     * True if HWC samples are annotated with the number of
     * GPU and shader cores cycles since the last sample.
     *
     * When true, @ref sample_metadata::gpu_cycle and @ref sample_metadata::sc_cycle
     * values are set.
     */
    int has_gpu_cycle;

    /** True if @ref block_metadata::state values are set. */
    int has_block_state;

    /**
     * True if hardware counters back-end can detect sample period stretched due to
     * the counters ring buffer overflow. If true, @ref sample_flags::stretched
     * value is meaningful.
     */
    int has_stretched_flag;

    /**
     * True if overflow behavior is defined.
     *
     * The defined overflow behavior is:
     * When a counter reaches its maximum, the value will saturate when it is incremented,
     * i.e. they will stay maximum until sampled.
     */
    int overflow_behavior_defined;
};

#endif