#ifndef KBASE_TYPES_H
#define KBASE_TYPES_H

// backend/device/src/device/ioctl/kbase/types.hpp

#include <stdint.h>

/** Check version compatibility between kernel and userspace. */
struct version_check {
    /** Major version number. */
    uint16_t major;
    /** Minor version number */
    uint16_t minor;
};

/** Set kernel context creation flags. */
struct set_flags {
    /** kernel context creation flags. */
    uint32_t create_flags;
};

enum gpuprop_size {
        /** Property type is uint8_t. */
        gpuprop_uint8 = 0x0,
        /** Property type is uint16_t. */
        gpuprop_uint16 = 0x1,
        /** Property type is uint32_t. */
        gpuprop_uint32 = 0x2,
        /** Property type is uint64_t. */
        gpuprop_uint64 = 0x3,
    };

 /** GPU properties codes. */
enum gpuprop_code {
    /** Product id. */
    product_id = 1,
    /** Version status. */
    version_status = 2,
    /** Minor revision. */
    minor_revision = 3,
    /** Major revision. */
    major_revision = 4,
    /** GPU freq khz max. */
    gpu_freq_khz_max = 6,
    /** Log2 program counter size. */
    log2_program_counter_size = 8,
    /** Texture features 0. */
    texture_features_0 = 9,
    /** Texture features 1. */
    texture_features_1 = 10,
    /** Texture features 2. */
    texture_features_2 = 11,
    /** GPU available memory size. */
    gpu_available_memory_size = 12,
    /** L2 log2 line size. */
    l2_log2_line_size = 13,
    /** L2 log2 cache size. */
    l2_log2_cache_size = 14,
    /** L2 num l2 slices. */
    l2_num_l2_slices = 15,
    /** Tiler bin size bytes. */
    tiler_bin_size_bytes = 16,
    /** Tiler max active levels. */
    tiler_max_active_levels = 17,
    /** Max threads. */
    max_threads = 18,
    /** Max workgroup size. */
    max_workgroup_size = 19,
    /** Max barrier size. */
    max_barrier_size = 20,
    /** Max registers. */
    max_registers = 21,
    /** Max task queue. */
    max_task_queue = 22,
    /** Max thread group split. */
    max_thread_group_split = 23,
    /** Impl tech. */
    impl_tech = 24,
    /** Raw shader present. */
    raw_shader_present = 25,
    /** Raw tiler present. */
    raw_tiler_present = 26,
    /** Raw l2 present. */
    raw_l2_present = 27,
    /** Raw stack present. */
    raw_stack_present = 28,
    /** Raw l2 features. */
    raw_l2_features = 29,
    /** Raw core features. */
    raw_core_features = 30,
    /** Raw mem features. */
    raw_mem_features = 31,
    /** Raw mmu features. */
    raw_mmu_features = 32,
    /** Raw as present. */
    raw_as_present = 33,
    /** Raw js present. */
    raw_js_present = 34,
    /** Raw js features 0. */
    raw_js_features_0 = 35,
    /** Raw js features 1. */
    raw_js_features_1 = 36,
    /** Raw js features 2. */
    raw_js_features_2 = 37,
    /** Raw js features 3. */
    raw_js_features_3 = 38,
    /** Raw js features 4. */
    raw_js_features_4 = 39,
    /** Raw js features 5. */
    raw_js_features_5 = 40,
    /** Raw js features 6. */
    raw_js_features_6 = 41,
    /** Raw js features 7. */
    raw_js_features_7 = 42,
    /** Raw js features 8. */
    raw_js_features_8 = 43,
    /** Raw js features 9. */
    raw_js_features_9 = 44,
    /** Raw js features 10. */
    raw_js_features_10 = 45,
    /** Raw js features 11. */
    raw_js_features_11 = 46,
    /** Raw js features 12. */
    raw_js_features_12 = 47,
    /** Raw js features 13. */
    raw_js_features_13 = 48,
    /** Raw js features 14. */
    raw_js_features_14 = 49,
    /** Raw js features 15. */
    raw_js_features_15 = 50,
    /** Raw tiler features. */
    raw_tiler_features = 51,
    /** Raw texture features 0. */
    raw_texture_features_0 = 52,
    /** Raw texture features 1. */
    raw_texture_features_1 = 53,
    /** Raw texture features 2. */
    raw_texture_features_2 = 54,
    /** Raw GPU id. */
    raw_gpu_id = 55,
    /** Raw thread max threads. */
    raw_thread_max_threads = 56,
    /** Raw thread max workgroup size. */
    raw_thread_max_workgroup_size = 57,
    /** Raw thread max barrier size. */
    raw_thread_max_barrier_size = 58,
    /** Raw thread features. */
    raw_thread_features = 59,
    /** Raw coherency mode. */
    raw_coherency_mode = 60,
    /** Coherency num groups. */
    coherency_num_groups = 61,
    /** Coherency num core groups. */
    coherency_num_core_groups = 62,
    /** Coherency coherency. */
    coherency_coherency = 63,
    /** Coherency group 0. */
    coherency_group_0 = 64,
    /** Coherency group 1. */
    coherency_group_1 = 65,
    /** Coherency group 2. */
    coherency_group_2 = 66,
    /** Coherency group 3. */
    coherency_group_3 = 67,
    /** Coherency group 4. */
    coherency_group_4 = 68,
    /** Coherency group 5. */
    coherency_group_5 = 69,
    /** Coherency group 6. */
    coherency_group_6 = 70,
    /** Coherency group 7. */
    coherency_group_7 = 71,
    /** Coherency group 8. */
    coherency_group_8 = 72,
    /** Coherency group 9. */
    coherency_group_9 = 73,
    /** Coherency group 10. */
    coherency_group_10 = 74,
    /** Coherency group 11. */
    coherency_group_11 = 75,
    /** Coherency group 12. */
    coherency_group_12 = 76,
    /** Coherency group 13. */
    coherency_group_13 = 77,
    /** Coherency group 14. */
    coherency_group_14 = 78,
    /** Coherency group 15. */
    coherency_group_15 = 79,
    /** Texture features 3. */
    texture_features_3 = 80,
    /** Raw texture features 3. */
    raw_texture_features_3 = 81,
    /** Num exec engines. */
    num_exec_engines = 82,
    /** Raw thread tls alloc. */
    raw_thread_tls_alloc = 83,
    /** Tls alloc. */
    tls_alloc = 84,
    /** Raw GPU features. */
    raw_gpu_features = 85,
};

struct get_gpuprops {
    /** Pointer to the buffer to store properties into. */
    uint8_t* buffer;
    /** Size of the buffer. */
    uint32_t size;
    /** Flags - must be zero for now. */
    uint32_t flags;
};

/** Input parameters type. */
struct in_type {
    /**
     * The maximum number of groups to be read. Can be 0, in which case groups_ptr is
     * unused.
     */
    uint32_t max_group_num;
    /**
     * The maximum number of CSs to be read. Can be 0, in which case streams_ptr is
     * unused.
     */
    uint32_t max_total_stream_num;
    /** Pointer where to store all the group data (sequentially). */
    uint64_t groups_ptr;
    /** Pointer where to store all the CS data (sequentially). */
    uint64_t streams_ptr;
};

/** Output parameters type. */
struct out_type {
    /** Global interface version. */
    uint32_t glb_version;
    /** Bit mask of features (e.g. whether certain types of job can be suspended). */
    uint32_t features;
    /** Number of CSGs supported. */
    uint32_t group_num;
    /**
     * Size of CSF performance counters, in bytes. Bits 31:16 hold the size of firmware
     * performance counter data and 15:0 hold the size of hardware performance counter
     * data.
     */
    uint32_t prfcnt_size;
    /** Total number of CSs, summed across all groups. */
    uint32_t total_stream_num;
    /**
     * Instrumentation features. Bits 7:4 hold the maximum size of events. Bits 3:0
     * hold the offset update rate. (csf >= 1.1.0)
     */
    uint32_t instr_features;
};

/** Request the global control block of CSF interface capabilities. */
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
union cs_get_glb_iface {
    /** Input parameters. */
    struct in_type in;
    /** Output parameters. */
    struct out_type out;
};

/** Setup HWC dumper/reader. */
struct hwcnt_reader_setup {
    /** Requested number of dumping buffers. */
    uint32_t buffer_count;
    /** Counters selection bitmask (Front end). */
    uint32_t fe_bm;
    /** Counters selection bitmask (Shader). */
    uint32_t shader_bm;
    /** Counters selection bitmask (Tiler). */
    uint32_t tiler_bm;
    /** Counters selection bitmask (MMU_L2). */
    uint32_t mmu_l2_bm;
};

void get_gpuprops__destroy(struct get_gpuprops* get_props){
    free(get_props->buffer);
}


#endif