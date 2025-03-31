#ifndef ARM_GPU_VINSTR_SMPLE_H
#define ARM_GPU_VINSTR_SMPLE_H

// backend/device/include/device/hwcnt/sample.hpp

#include <stdint.h>
#include "debug.h"
#include "hwcnt/block_extents.h"
#include "hwcnt/detail/handle.h"
#include "hwcnt/features.h"
#include "backend.h"
#include "hwcnt/blocks_view.h"

/** Sample flags. */
struct sample_flags {
    /**
     * The counters sample period was not met because of the counters ring buffer
     * overflow. The sample period is stretched for this sample. The value is
     * undefined if @ref features::has_stretched_flag is false.
     */
    uint32_t stretched : 1;

    /** This sample has had an error condition for sample duration. */
    uint32_t error : 1;
};

/** Hardware counters sample meta-data. */
struct sample_metadata {
    /** User data. */
    uint64_t user_data;

    /** Sample flags. */
    struct sample_flags flags;

    /** Sample number. */
    uint64_t sample_nr;

    /** Earliest timestamp that values in this sample represent. */
    uint64_t timestamp_ns_begin;

    /** Latest timestamp that values in this sample represent. */
    uint64_t timestamp_ns_end;

    /**
     * GPU cycles elapsed since the last sample was taken.
     * The value is undefined if @ref features::has_gpu_cycle is false.
     */
    uint64_t gpu_cycle;

    /**
     * Shader cores cycles elapsed since the last sample was taken.
     * The value is undefined if @ref features::has_gpu_cycle is false.
     */
    uint64_t sc_cycle;
};


/**
 * Hardware counters sample.
 *
 * Represents an entry in the hardware counters @ref reader ring buffer.
 * After a sample was got from a reader using @ref reader::get_sample,
 * the user may extract @ref sample_metadata using @ref get_metadata method.
 * To extract the counters values, one should iterate over blocks using
 * @ref blocks.
 */
struct vinstr_sample {
    struct vinstr_backend reader_;

    /** Sample meta-data. */
    struct sample_metadata metadata_;

    /** Point in the counters sample buffer where this sample was decoded from. */
    struct sample_handle sample_hndl_;
};

void vinstr_backend__get_sample(struct vinstr_backend *b, struct sample_metadata* sm, struct sample_handle* sample_handle);
void vinstr_backend__put_sample(struct vinstr_backend* b, struct sample_handle* sample_hndl_raw);
void vinstr_sample__get(struct vinstr_sample* sample);
void vinstr_sample__destroy(struct vinstr_sample* sample, struct vinstr_backend* b);

void vinstr_sample__init(struct vinstr_sample* sample, struct vinstr_backend* reader){
    memset(sample, 0, sizeof(struct vinstr_sample));
    memset(&sample->metadata_, 0, sizeof(struct sample_metadata));
    sample_handle__init(&sample->sample_hndl_);
    sample->reader_ = *reader;
    vinstr_sample__get(sample);
}

void vinstr_sample__destroy(struct vinstr_sample* sample, struct vinstr_backend* b){
    vinstr_backend__put_sample(b, &sample->sample_hndl_);
    return;
}



void vinstr_sample__get(struct vinstr_sample* sample){
    vinstr_backend__get_sample(&sample->reader_, &sample->metadata_, &sample->sample_hndl_);
    return;
}

struct vinstr_blocks_view blocks(struct vinstr_sample* sample){
    struct vinstr_blocks_view bv;
    blocks_view__init(&bv, &sample->reader_, &sample->sample_hndl_);
    return bv;
}

void vinstr_backend__get_sample(struct vinstr_backend *b, struct sample_metadata* sm, struct sample_handle* sample_handle){
    int ret = 0;

    wait_for_sample(b->fd_);

    struct vinstr_reader_metadata_with_cycles metadata;
    memset(&metadata, 0, sizeof(struct vinstr_reader_metadata_with_cycles));
    if((!!b->reader_features_)){
        printf("[ERROR] (get_sample) Wrong!\n");
        exit(0);
    }
    else{
#ifdef ENABLE_IOCTL_DEBUG
        printf("ioctl: KBASE_HWCNT_READER_GET_BUFFER\n");
#endif
        ret = ioctl(b->fd_, KBASE_HWCNT_READER_GET_BUFFER, &metadata.metadata);
        if(ret < 0){
            perror("ioctl KBASE_HWCNT_READER_GET_BUFFER failed");
            close(b->fd_);
            exit(0);
        }
    }

    {
        pthread_mutex_lock(&b->access_);
        struct session session = session_queue__front(&b->sessions_);

        sm->user_data = uint64_queue__pop(&b->user_data_manual_);
        // TOOD: For non-manual case
        struct sample_flags flags;
        memset(&flags, 0, sizeof(struct sample_flags));
        sm->flags = flags;
        sm->sample_nr = b->sample_nr_alloc_++;
        sm->timestamp_ns_begin = session__update_ts(&session, metadata.metadata.timestamp);
        
        uint64_t manual_sample_nr = uint64_queue__pop_count(&b->user_data_manual_);
        if(session__can_erase(&session, manual_sample_nr))
            session_queue__pop(&b->sessions_);

        pthread_mutex_unlock(&b->access_);
    }

    sm->timestamp_ns_end = metadata.metadata.timestamp;

    if(!!(b->reader_features_ & cycles_top)) sm->gpu_cycle = metadata.cycles.top;

    if(!!(b->reader_features_ & cycles_shader_core)) sm->sc_cycle = metadata.cycles.shader_cores;

    if(!sm->sc_cycle && sm->gpu_cycle){
        sm->sc_cycle = sm->gpu_cycle;
    }

    struct vinstr_reader_metadata* data;
    data = (struct vinstr_reader_metadata*)sample_handle__get(sample_handle, sizeof(struct vinstr_reader_metadata), alignof(struct vinstr_reader_metadata));
    *data = metadata.metadata;

    return;
}


void vinstr_backend__put_sample(struct vinstr_backend* b, struct sample_handle* sample_hndl_raw){
    int ret;
    struct vinstr_reader_metadata* sample_hndl;
    sample_hndl = (struct vinstr_reader_metadata*)sample_handle__get(sample_hndl_raw, sizeof(struct vinstr_reader_metadata), alignof(struct vinstr_reader_metadata));

#ifdef ENABLE_IOCTL_DEBUG
    printf("ioctl: KBASE_HWCNT_READER_PUT_BUFFER\n");
#endif
    ret = ioctl(b->fd_, KBASE_HWCNT_READER_PUT_BUFFER, sample_hndl);
    if(ret < 0){
        perror("ioctl KBASE_HWCNT_READER_PUT_BUFFER failed");
        close(b->fd_);
        exit(0);
    }

    return;
}

// TODO: destructor

#endif