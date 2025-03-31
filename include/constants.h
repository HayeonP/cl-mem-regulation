#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>

/**
 * Mali Device Constants.
 *
 * Various properties of a physical Mali GPU. The values here are to be mainly
 * used for derived counters computation.
 *
 * @warning
 * Do not use this structure to figure out how many blocks of a type are present.
 * Instead, @ref hwcnt::block_extents must be used.
 */
struct constants {
    /** GPU id. */
    uint64_t gpu_id;

    /** CSF Firmware version. */
    uint64_t fw_version;

    /** AXI bus width in bits. */
    uint64_t axi_bus_width;

    /** Number of shader cores. */
    uint64_t num_shader_cores;

    /** The shader core mask. */
    uint64_t shader_core_mask;

    /** Number of L2 cache slices. */
    uint64_t num_l2_slices;

    /** L2 cache slice size in bytes. */
    uint64_t l2_slice_size;

    /** Maximum number of execution engines (per core, over all cores). */
    uint64_t num_exec_engines;

    /** Tile size in pixels. */
    uint64_t tile_size;

    /** Warp width. */
    uint64_t warp_width;
};

#endif