#ifndef ARM_GPU_SESSION_H
#define ARM_GPU_SESSION_H

// backend/device/src/device/hwcnt/sampler/vinstr/session.hpp

#include <stdint.h>
#include <string.h>
#include <stdio.h>

/** Vinstr profiling session state class. */
struct session {
    /** Either session start timestamp or last sample's timestamp. */
    uint64_t last_ts_ns_;
    /** User data for periodic samples of this session. */
    uint64_t user_data_periodic_;
    /** True stop() was called for this session, but some samples
     * might not have been parsed yet.
     */
    int pending_stop_;
    /** Number of the manual sample that was taken when this
     * session was stopped.
     */
    uint64_t stop_sample_nr_;
};

void session__init(struct session* s){
    memset(s, 0, sizeof(struct session));
    return;
}

void session__init_with_args(struct session* s, uint64_t start_ts_ns, uint64_t user_data_periodic){
    session__init(s);
    s->last_ts_ns_ = start_ts_ns;
    s->user_data_periodic_ = user_data_periodic;
    return;
};

void session__copy(struct session* dest, struct session* src){
    dest->last_ts_ns_ = src->last_ts_ns_;
    dest->user_data_periodic_ = src->user_data_periodic_;
    dest->pending_stop_ = src->pending_stop_;
    dest->stop_sample_nr_ = src->stop_sample_nr_;
    return;
}

uint64_t session__update_ts(struct session* s, uint64_t ts){
    if (s->last_ts_ns_ < ts){
        printf("[ERROR] Last timestamp > current timestamp\n");
        exit(0);
    }
    uint64_t last_ts_ns = s->last_ts_ns_;
    s->last_ts_ns_ = ts;
    return last_ts_ns;
}

/**
 * Track session stop.
 *
 * @param[in] stop_sample_nr    Number of the manual sample that corresponds to
 *                              this session stop.
 */
void session__stop(struct session* s, uint64_t stop_sample_nr){
    s->pending_stop_ = 1;
    s->stop_sample_nr_ = stop_sample_nr;
    return;
}

/**
 * Check if this session state can be erased.
 *
 * @param[in] manual_sample_nr    Number of the manual sample being parsed.
 * @return if this session can be removed.
 */
int session__can_erase(struct session* s, uint64_t manual_sample_nr){
    if(!s->pending_stop_) return 0;
    return manual_sample_nr == s->stop_sample_nr_;
}



#endif