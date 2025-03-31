#ifndef ARM_GPU_BACKEND_H
#define ARM_GPU_BACKEND_H

// backend/device/include/device/hwcnt/sampler/detail/backend.hpp
// backend/device/src/device/hwcnt/sampler/base/backend.hpp

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "debug.h"
#include "instance.h"
#include "configuration.h"
#include "mapped_memory.h"
#include "vinstr_types.h"
#include "backend_args.h"
#include "hwcnt/block_extents.h"
#include "vinstr_setup.h"
#include "hwcnt/detail/handle.h"
#include "sample_poll.h"
#include "vinstr_types.h"
#include "session.h"
#include "tools.h"
#include "hwcnt/block_metadata.h"
#include "ioctl_commands.h"


#include "data_structure/uint64_queue.h"
#include "data_structure/session_queue.h"

/** Sampler type. */
enum sampler_type {
    /** Manual sampler. */
    sampler_type_manual,
    /** Periodic sampler. */
    sampler_type_periodic,
};

struct vinstr_backend { // Inherit detail::backend, reader
    /** Hardware counters file descriptor. */
    int fd_; // vinsr_fd
    /** Features. */
    struct features features_;
    /** Block extents. */
    struct block_extents block_extents_;
    /** Sampling period. 0 for manual sampling. */
    uint64_t period_ns_;
    /** Counters memory. */
    struct mapped_memory memory_;
    /** Vinstr reader features. */
    enum vinstr_reader_features reader_features_;
    /** Hardware counters buffer size. */
    size_t buffer_size_;
    /** Mutex protecting access to the active flag. */
    pthread_mutex_t access_;
    /** Sampler state. */
    char active_;
    /** True if sampling thread is running. */
    char sampling_;
    /** User data for manual samples. */
    struct uint64_queue user_data_manual_;
    /** Maximum profiling sessions being tracked at a time.
     *
     * Every session stop() results into a manual sample. There could be at most
     * `args_type::buffer_count` in the ring buffer. Therefore, we can
     * have `args_type::buffer_count` start/stop pairs and one extra `start`
     * (because it does not require a sample). Since a queue size must be a power of two,
     * we take the next power of two.
     */
    size_t max_sessions;
    /** Profiling session states. */
    struct session_queue sessions_;
    /** Counter to allocate values for sample_metadata::sample_nr. */
    uint64_t sample_nr_alloc_;
    /** Sample layout data structure. */
    struct sample_layout sample_layout_;
};



void vinstr_backend__start(struct vinstr_backend* b, uint64_t user_data);
void vinstr_backend__stop(struct vinstr_backend *b, uint64_t user_data);
void vinstr_backend__request_sample(struct vinstr_backend *b, uint64_t user_data);
void vinstr_backend__get_reader(struct vinstr_backend* b);
void vinstr_backend__create(struct vinstr_backend_args* args, struct instance* inst, uint64_t period_ns, struct configuration* config, size_t config_len);
void vinstr_backend__init(struct vinstr_backend* b, struct vinstr_backend_args* args);
int vinstr_backend__sampler_type(struct vinstr_backend* b);
void vinstr_backend__request_sample_no_lock(struct vinstr_backend* b, uint64_t user_data);
int vinstr_backend__next(struct vinstr_backend* b, struct sample_handle* sample_handle_raw, struct block_metadata* bm, struct block_handle* block_hndl_raw);
void vinstr_backend__vinstr_backend_create(struct vinstr_backend_args* args, struct instance* inst, uint64_t period_ns, struct configuration* config, size_t config_len);
void vinstr_backend__put_sample(struct vinstr_backend* b, struct sample_handle* sample_hndl_raw);
void vinstr_backend__create(struct vinstr_backend_args* args, struct instance* inst, uint64_t period_ns, struct configuration* config, size_t config_len);
void vinstr_backend__vinstr_backend_create(struct vinstr_backend_args* args, struct instance* inst, uint64_t period_ns, struct configuration* config, size_t config_len);

int vinstr_backend__sampler_type(struct vinstr_backend* b){
    return b->period_ns_ ? sampler_type_periodic : sampler_type_manual; 
}

void vinstr_backend__clear(struct vinstr_backend* b){
    int ret;
#ifdef ENABLE_IOCTL_DEBUG
    printf("ioctl: KBASE_HWCNT_READER_CLEAR\n");
#endif
    ret = ioctl(b->fd_, KBASE_HWCNT_READER_CLEAR, 0);
    if(ret < 0){
        perror("ioctl KBASE_HWCNT_READER_CLEAR failed");
        close(b->fd_);
        exit(0);
    }
}

/**
 * Start counters sampling or accumulation.
 *
 * If the backend was configured as periodic, the method starts counters sampling.
 * If the backend was configured as manual, the method starts counters accumulation.
 *
 * If the backend was active at the destruction time, it will be stopped implicitly
 * by the destructor.
 *
 * @param[in] user_data    User data value to be stored in `sample_metadata::user_data`.
 * @return Error code.
 */
void vinstr_backend__start(struct vinstr_backend* b, uint64_t user_data){    
    struct timespec now;

    vinstr_backend__clear(b);

    pthread_mutex_lock(&b->access_);

    if(b->active_){
        pthread_mutex_unlock(&b->access_);
        return;
    }

    if (clock_gettime(CLOCK_REALTIME, &now) == -1) {
        perror("clock_gettime");
        printf("[ERROR] clock_gettime error\n");
        pthread_mutex_unlock(&b->access_);
        return;
    }

    if(vinstr_backend__sampler_type(b) == sampler_type_periodic){
        // TODO: Handle sampler type for periodic
        printf("(start) sampler type: periodic\n");    
    }

    struct session session;
    uint64_t start_ts_ns;
    start_ts_ns = timespec_to_uint64(now);
    session__init_with_args(&session, start_ts_ns, user_data);
    session_queue__push(&b->sessions_, session);
    b->active_ = 1;
    pthread_mutex_unlock(&b->access_);

    return;
}

/**
 * Stop counters sampling or accumulation.
 *
 * If the backend was configured as periodic, the method stops counters sampling.
 * If the backend was configured as manual, the method stops counters accumulation.
 * Before stopping, one last sample is done synchronously.
 *
 * @param[in] user_data    User data value to be stored in `sample_metadata::user_data`.
 * @return Error code.
 */
void vinstr_backend__stop(struct vinstr_backend *b, uint64_t user_data){
    pthread_mutex_lock(&b->access_);

    if(!b->active_){
        pthread_mutex_unlock(&b->access_);
        return;
    }

    if(vinstr_backend__sampler_type(b) == sampler_type_periodic && b->sampling_){
        // TODO
    }
    pthread_mutex_unlock(&b->access_);
    vinstr_backend__request_sample_no_lock(b, user_data);

    return;
}

/**
 * Request manual sample.
 *
 * The backend must have been created as manual, otherwise this function fails.
 *
 * @param[in] user_data    User data value to be stored in `sample_metadata::user_data`.
 * @return Error code.
 */
void vinstr_backend__request_sample(struct vinstr_backend *b, uint64_t user_data){
    if(vinstr_backend__sampler_type(b) != sampler_type_manual){
        printf("[ERROR](request_sample) Invalid argment.\n");
        exit(0);
    }

    // pthread_mutex_lock(&b->access_);
    // pthread_mutex_unlock(&b->access_);
    vinstr_backend__request_sample_no_lock(b, user_data);

    return;
}


// Get sample is moved to vinstr_sample.h

int vinstr_backend__next(struct vinstr_backend* b, struct sample_handle* sample_handle_raw, struct block_metadata* bm, struct block_handle* block_hndl_raw){
    struct vinstr_reader_metadata* sample_hndl;
    size_t* block_index;

    sample_hndl = (struct vinstr_reader_metadata*)sample_handle__get(sample_handle_raw, sizeof(struct vinstr_reader_metadata), alignof(struct vinstr_reader_metadata));
    block_index = (size_t*)block_handle__get(block_hndl_raw, sizeof(size_t), alignof(size_t));

    if(*block_index == b->sample_layout_.num_blocks_)
        return 0;
    
    struct entry* layout_entry = &(b->sample_layout_.layout_[*block_index]);

    bm->type = layout_entry->type;
    bm->index = layout_entry->index;
    bm->set = prfcnt_set_primary;
    bm->values = (uint8_t*)(b->memory_.data_) 
                    + b->buffer_size_ * sample_hndl->buffer_idx
                    + layout_entry->offset;
    
    ++(*block_index);
    
#ifdef ENABLE_DEBUG
    printf("(backend next) bm.type: %lu\n", (uint64_t) bm->type);
    printf("(backend next) bm.index: %lu\n", (uint64_t) bm->index);
    printf("(backend next) bm.set: %lu\n", (uint64_t) bm->set);
    printf("(backend next) bm.values: %lu\n", (uint64_t) bm->values);
#endif

    return 1;
}

/**
 * Request manual sample dump.
 *
 * @pre `access_` must be locked.
 *
 * @param[in] user_data    User data.
 * @return Error code.
 */
void vinstr_backend__request_sample_no_lock(struct vinstr_backend* b, uint64_t user_data) {
    int ret;

#ifdef ENABLE_DEBUG
    printf("(request_sample_no_lock)\n");
#endif

    if (!b->active_){
        printf("[ERROR] Sampling is requsted during not activated.\n");
        exit(0);
    }

#ifdef ENABLE_IOCTL_DEBUG
    printf("ioctl: KBASE_HWCNT_READER_DUMP\n");
#endif
    ret = ioctl(b->fd_, KBASE_HWCNT_READER_DUMP, 0);
    if(ret < 0){
        perror("ioctl KBASE_HWCNT_READER_DUMP failed");
        close(b->fd_);
        exit(0);
    }

    uint64_queue__push(&b->user_data_manual_, user_data);
    return;
}

/**
 * Create hardware counters reader instance.
 *
 * @param[in] inst         Mali device instance.
 * @param[in] period_ns    Period in nanoseconds between samples taken. Zero for manual context.
 * @param[in] config       Which counters to enable on per-block basis.
 * @param[in] config_len   Len of @p config array.
 * @return Backend instance, nullptr if failed.
 */
void vinstr_backend__create(struct vinstr_backend_args* args, struct instance* inst, uint64_t period_ns, struct configuration* config, size_t config_len){
    if(config == NULL) {
        printf("[ERROR] (vinstr_backend__crate) config is NULL.\n");
        exit(0);
    }
    vinstr_backend__vinstr_backend_create(args, inst, period_ns, config, config_len);
    // TODO: Handle kinstr cases.
    return;
}

/** Create kinstr_prfcnt backend if possible.
 *
 * @param inst    Instance implementation reference.
 * @param[in] period_ns    Period in nanoseconds between samples taken. Zero for manual context.
 * @param[in] config       Which counters to enable on per-block basis.
 * @param[in] config_len   Len of @p config array.
 * @return backend pointer, if created.
 */
void vinstr_backend__kinstr_prfcnt_backend_craete(){
    // TODO
    return;
}

/** Create vinstr backend if possible.
 *
 * @param inst    Instance implementation reference.
 * @param[in] period_ns    Period in nanoseconds between samples taken. Zero for manual context.
 * @param[in] config       Which counters to enable on per-block basis.
 * @param[in] config_len   Len of @p config array.
 * @return backend pointer, if created.
 */
void vinstr_backend__vinstr_backend_create(struct vinstr_backend_args* args, struct instance* inst, uint64_t period_ns, struct configuration* config, size_t config_len){
    vinstr_setup(args, inst, period_ns, config, config+config_len);

    return;
}


void vinstr_backend__init(struct vinstr_backend* b, struct vinstr_backend_args* args){
    memset(b, 0, sizeof(struct vinstr_backend));    
    b->fd_ = args->base_args.fd;
    b->features_ = args->base_args.features_v;
    b->block_extents_ = args->base_args.extents;
    b->period_ns_ = args->base_args.period_us;
    mapped_memory__move(&b->memory_, &args->base_args.memory);

    b->reader_features_ = args->features;
    b->buffer_size_ = args->buffer_size;
    b->sample_layout_ = args->sample_layout_v;
    
    uint64_queue__init(&b->user_data_manual_, args->max_buffer_count);
    session_queue__init(&b->sessions_, args->max_buffer_count * 2);
    pthread_mutex_init(&(b->access_), NULL);

    return;
}

void vinstr_backend__init_with_args(struct vinstr_backend* b, struct instance* inst, struct configuration* config, size_t config_len){
    struct vinstr_backend_args args;    
    vinstr_backend_args__init(&args);
    vinstr_backend__create(&args, inst, 0, config, config_len);
    vinstr_backend__init(b, &args);

    return;
}

#endif