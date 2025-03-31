#ifndef ARM_GPU_SETUP_H
#define ARM_GPU_SETUP_H

#include <stdint.h>
#include <string.h>
#include "debug.h"
#include "backend_args.h"
#include "hwcnt/block_extents.h"
#include "kbase_types.h"
#include "vinstr_types.h"
#include "filter_block_extents.h"
#include "convert.h"
#include "hwcnt/features.h"
#include "mapped_memory.h"
#include "constants.h"
#include "product_id.h"
#include "sample_layout.h"
#include "instance.h"

// backend/device/src/device/hwcnt/sampler/vinstr/setup.hpp

void init_features(struct features* features, enum vinstr_reader_features reader_features);
void vinstr_setup(struct vinstr_backend_args* result, struct instance* instance, uint64_t period_ns, struct configuration* begin, struct configuration* end);
inline void vinstr_reader_setup(int* vinstr_fd, struct instance* instance, struct hwcnt_reader_setup* setup_args);

void init_features(struct features* result, enum vinstr_reader_features reader_features){
    memset(result, 0, sizeof(struct features));
    result->has_gpu_cycle = !!reader_features;
    result->has_block_state = 0;
    result->has_stretched_flag = 0;
    result->overflow_behavior_defined = 1;

    return;
}

void vinstr_setup(struct vinstr_backend_args* result, struct instance* instance, uint64_t period_ns, struct configuration* begin, struct configuration* end){
    int ret;
    struct block_extents extents;
    vinstr_backend_args__init(result);
    block_extents__init(&extents);

    filter_block_extents(&extents, &instance->block_extents_, begin, end);

    struct hwcnt_reader_setup setup_args;
    memset(&setup_args, 0, sizeof(struct hwcnt_reader_setup));
    convert(&setup_args, begin, end);

    setup_args.buffer_count = result->max_buffer_count;
    int vinstr_fd = -1;

    /* Try to initialize vinstr reader with `max_buffer_count` buffers or fewer. */
    for (; setup_args.buffer_count > 1; setup_args.buffer_count >>= 1) {
        vinstr_reader_setup(&vinstr_fd, instance, &setup_args);
        break;
    }

    if (vinstr_fd < 0) {
        fprintf(stderr, "Failed to set up vinstr reader\n");
        exit(EXIT_FAILURE);
    }

    struct vinstr_reader_api_version api_version;
    memset(&api_version, 0, sizeof(struct vinstr_reader_api_version));
#ifdef ENABLE_IOCTL_DEBUG
        printf("ioctl: KBASE_HWCNT_READER_GET_API_VERSION_WITH_FEATURES\n");
#endif
    ret = ioctl(vinstr_fd, KBASE_HWCNT_READER_GET_API_VERSION_WITH_FEATURES, &api_version);
    if(ret < 0){
        perror("ioctl KBASE_HWCNT_READER_GET_API_VERSION_WITH_FEATURES failed");
        close(vinstr_fd);
        exit(0);
    }

#ifdef ENABLE_DEBUG
    printf("(setup) get_api_version_with_features -> api_version.version: %d / api_version.features: %d\n", (int) api_version.version, (uint32_t)api_version.features);
#endif

    uint32_t buffer_size = 0;
#ifdef ENABLE_IOCTL_DEBUG
    printf("ioctl: KBASE_HWCNT_READER_GET_BUFFER_SIZE\n");
#endif
    ret = ioctl(vinstr_fd, KBASE_HWCNT_READER_GET_BUFFER_SIZE, &buffer_size);
    if(ret < 0){
        perror("ioctl KBASE_HWCNT_READER_GET_BUFFER_SIZE failed");
        close(vinstr_fd);
        exit(0);
    }

    size_t mapping_size = buffer_size * (size_t)setup_args.buffer_count;
    struct mapped_memory memory;
    mapped_memory__init_with_fd(&memory, vinstr_fd, mapping_size);

    struct constants consts = instance->constants_;    
    struct product_id product_id;
    product_id__init(&product_id);
    product_id__from_raw_gpu_id(&product_id, consts.gpu_id);
    enum sample_layout_type sample_layout_type = is_v4_layout(&product_id) ? sample_layout_type_v4: sample_layout_type_non_v4;
#ifdef ENABLE_DEBUG
    printf("(setup) sample layout type: %d\n", (int)sample_layout_type);
#endif

    struct bitset shader_core_mask;
    uint64_to_bitset(consts.shader_core_mask, &shader_core_mask);

    sample_layout__init_with_args(&result->sample_layout_v, &extents, consts.num_l2_slices, &shader_core_mask, sample_layout_type);
    result->base_args.fd = vinstr_fd;
    result->base_args.period_us = period_ns;
    init_features(&result->base_args.features_v, api_version.features);
    block_extents__init_with_args(&result->base_args.extents, extents.num_blocks_of_type_, extents.counters_per_block_, extents.values_type_);
    mapped_memory__move(&result->base_args.memory, &memory);

    result->features = api_version.features;
    result->buffer_size = buffer_size;

#ifdef ENABLE_DEBUG
    printf("(setup) buffer size: %u\n", (uint32_t)buffer_size);
#endif    

    return;
}

/**
 * Setup hardware counters reader handle.
 *
 * @param[in]     instance      Mali device instance.
 * @param[in]     setup_args    Ioctl setup arguments.
 *
 * @return A pair of error code and the reader handle..
 */
void vinstr_reader_setup(int* vinstr_fd, struct instance* instance, struct hwcnt_reader_setup* setup_args) {
    int ret;
    struct kbase_version version = instance->kbase_version_;

    if (version.type_ != ioctl_iface_type_jm_pre_r21) {
#ifdef ENABLE_IOCTL_DEBUG
        printf("ioctl: KBASE_IOCTL_HWCNT_READER_SETUP\n");
#endif
        ret = ioctl(instance->fd_, KBASE_IOCTL_HWCNT_READER_SETUP, setup_args);
        if(ret < 0){
            perror("ioctl KBASE_IOCTL_HWCNT_READER_SETUP failed");
            close(instance->fd_);
            exit(0);
        }
        else if(ret == 12){
            printf("[ERROR] (reader_setup) Not enough memory: return=%d\n", ret);
            exit(0);
        }
        *vinstr_fd = ret;
        return;
    }

    // TODO: Handle other cases
}

#endif