#ifndef ARM_GPU_SAMPLER_H
#define ARM_GPU_SAMPLER_H

// hwcpipe/include/hwcpipe/sampler.hpp

#include <stdint.h>
#include <string.h>
#include "debug.h"
#include "configuration.h"
#include "hwcpipe_counter.h"
#include "internal_types.h"
#include "gpu.h"
#include "hwcnt/block_extents.h"
#include "hwcnt/block_metadata.h"
#include "internal_types.h"
#include "handle.h"
#include "instance.h"
#include "backend.h"
#include "vinstr_sample.h"
#include "hwcpipe_counter.h"
#include "internal_types.h"
#include "external_types.h"
#include "tools.h"
#include "data_structure/hashmap.h"
#include "data_structure/vector.h"
#include "data_structure/set.h"

typedef struct configuration backend_cfg_type;

/** @brief Type of the sampled data. */
enum counter_sample_type {
    counter_sample_type_uint64,
    counter_sample_type_float64,
};

/** @brief Value of the sample. */
union counter_sample_value {
    uint64_t uint64;
    double float64;
};

/**
 * @brief a counter_sample object holds the sampled information from a counter at
 * a specific timestamp.
 */
struct counter_sample {
    /** The counter id. */
    enum hwcpipe_counter counter;
    /** The timestamp of the sample. */
    uint64_t timestamp;
    /** The sample value (variant is defined by the type field). */
    union counter_sample_value value;
    /** The data type of the sampled value. */
    enum counter_sample_type type;
};

/**
 * @brief A sampler_config object holds the list of counters that were selected
 * by the user and builds the state that is needed by the sampler to set up the
 * GPU.
 */
struct sampler_config {
    uint64_t gpu_id_;
    int device_number_;

    // counter_database db_; // TODO
    struct set counters_;
    struct hashmap backend_config_; // block_type, configuration
};

struct offset_to_buffer_pos {
    size_t block_offset;
    size_t buffer_pos;
    size_t shift;
};

struct sample_iterator{

};

struct sampler{
    // Handles to the hwcpipe backend
    struct handle* handle_;
    struct instance* instance_;
    struct vinstr_backend* sampler_;
    struct constants constants_;

    // Sample buffer and index mappings
    struct hashmap counter_to_buffer_pos_; // hwcpipe_counter, size_t
    struct hashmap counters_by_block_map_; // block_type, vector <offset_to_buffer_pos>
    struct vector sample_buffer_; // uint64_t
    int valid_sample_buffer_;
    // struct hashmap counter_to_evaluator; // hwcpipe_counter, dexpression // TODO

    // Sampler state
    int values_are_64_bit_;
    uint64_t last_collection_timestamp_;
    int sampling_in_progress_;
};


void counter_sample__init(struct counter_sample* cs);
void counter_sample__init_uint64(struct counter_sample* cs, enum hwcpipe_counter counter, uint64_t timestamp, uint64_t value);
void counter_sample__init_float64(struct counter_sample* cs, enum hwcpipe_counter counter, uint64_t timestamp, double value);
void register_counter__init(struct registered_counter* rc, enum hwcpipe_counter counter);
void register_counter__init_with_definition(struct registered_counter* rc, enum hwcpipe_counter counter, struct counter_definition definition);
int registered_counter__equals(const struct registered_counter *lhs, const struct registered_counter *rhs);
int registered_counter__not_equals(const struct registered_counter *lhs, const struct registered_counter *rhs);
int registered_counter__less_than(const struct registered_counter *lhs, const struct registered_counter *rhs);
void sampler_config__init(struct sampler_config* sc, struct gpu* gpu);
void sampler_config__init_with_args(struct sampler_config* sc, uint64_t gpu_id, int device_number);
void sampler_config__add_counter(struct sampler_config* sc, enum hwcpipe_counter counter);
void sampler_config__build_backend_config_list(struct sampler_config* sc, struct vector* config_list);

void sampler__init(struct sampler* s, struct sampler_config* config);
void sampler__start_sampling(struct sampler* s);
void sampler__stop_sampling(struct sampler* s);
void sampler__sample_now(struct sampler* s);
void sampler__get_counter_value_with_sample(struct sampler* s, enum hwcpipe_counter counter, struct counter_sample* sample);
void sampler__get_expression_counter_value();
// double sampler__get_counter_value(struct sampler* s, enum hwcpipe_counter counter);
void sampler__get_counter_value(struct sampler* sampler, enum hwcpipe_counter counter, struct counter_sample* sample);
double sampler__get_mali_config_ext_bus_byte_size();
double sampler__get_mali_config_shader_core_count();
double sampler__get_mali_config_l2_cache_count();
void sampler__build_sample_buffer_mappings(struct sampler* s, struct set* counters);
void sampler__fill_sample_buffer(struct sampler* s, struct vinstr_sample* backend_sample);

void counter_sample__init(struct counter_sample* cs){
    counter_sample__init_uint64(cs, MaliGPUActiveCy, 0, 0UL);
}

void counter_sample__init_uint64(struct counter_sample* cs, enum hwcpipe_counter counter, uint64_t timestamp, uint64_t value){
    memset(cs, 0, sizeof(struct counter_sample));
    cs->counter = counter;
    cs->timestamp = timestamp;
    cs->value.uint64 = value;
    cs->type = counter_sample_type_uint64;
}

void counter_sample__init_float64(struct counter_sample* cs, enum hwcpipe_counter counter, uint64_t timestamp, double value){
    memset(cs, 0, sizeof(struct counter_sample));
    cs->counter = counter;
    cs->timestamp = timestamp;
    cs->value.float64 = value;
    cs->type = counter_sample_type_float64;
}

void register_counter__init(struct registered_counter* rc, enum hwcpipe_counter counter){
    rc->counter = counter;
}

void register_counter__init_with_definition(struct registered_counter* rc, enum hwcpipe_counter counter, struct counter_definition definition){
    rc->counter = counter;
    rc->definition = definition;
}

// Equality operator
int registered_counter__equals(const struct registered_counter *lhs, const struct registered_counter *rhs) {
    return lhs->counter == rhs->counter;
}

// Inequality operator
int registered_counter__not_equals(const struct registered_counter *lhs, const struct registered_counter *rhs) {
    return lhs->counter != rhs->counter;
}

// Less-than operator
int registered_counter__less_than(const struct registered_counter *lhs, const struct registered_counter *rhs) {
    return lhs->counter < rhs->counter;
}

/**
 * @brief Construct a sampler configuration for a GPU.
 */
void sampler_config__init(struct sampler_config* sc, struct gpu* gpu){
    struct product_id pid;
    int device_number;
    memset(&pid, 0, sizeof(struct product_id));
    gpu__get_product_id(gpu, &pid);
    device_number = gpu->device_number_;

    sampler_config__init_with_args(sc, pid.product_id_, device_number);
}

/**
 * @brief Construct a sampler configuration from the provided GPU ID and
 * device device.
 */
void sampler_config__init_with_args(struct sampler_config* sc, uint64_t gpu_id, int device_number){
    size_t num_block_types = block_type_last + 1;

    memset(sc, 0, sizeof(struct sampler_config));
    set__init(&sc->counters_);
    hashmap__init(&sc->backend_config_);
    enum prfcnt_set prfcnt_set = prfcnt_set_primary;


    // preallocate the backend enable maps for each possible block type
    for(size_t i = 0; i != num_block_types; ++i){
        struct configuration config;
        enum block_type type;

        type = (enum block_type)(i);
        configuration__init(&config, type, prfcnt_set);
        hashmap__put(&sc->backend_config_, &type, sizeof(type), &config, sizeof(config));
    }

    sc->device_number_ = device_number;

    return;
}


/**
 * @brief Requests that a counter is collected by the sampler. The counter
 * is validated to make sure that it's supported by the GPU.
 *
 * @param [in] counter  The counter to sample.
 * @return Returns hwcpipe::errc::invalid_counter_for_device if the counter
 *                 is not supported by the current GPU.
 */
void sampler_config__add_counter(struct sampler_config* sc, enum hwcpipe_counter counter){
    if(set_contains_counter(&sc->counters_, (uint64_t)counter)){
        return;
    }

    // TODO: Following is hardcoded

    struct counter_definition definition;
    memset(&definition, 0, sizeof(struct counter_definition));
    if(counter == MaliExtBusRd){
        definition.tag = hardware;
        definition.address.offset = 29;
        definition.address.shift = 0;
        definition.address.block_type = block_type_memory;
    }
    else if(counter == MaliExtBusWr){
        definition.tag = hardware;
        definition.address.offset = 42;
        definition.address.shift = 0;
        definition.address.block_type = block_type_memory;
    }
    else if(counter == MaliExtBusRdBt){
        definition.tag = hardware;
        definition.address.offset = 32; // 32
        definition.address.shift = 0;
        definition.address.block_type = block_type_memory;
    }
    else if(counter == MaliExtBusWrBt){
        definition.tag = hardware;
        definition.address.offset = 47; // 47
        definition.address.shift = 0;
        definition.address.block_type = block_type_memory;
    }
    
    struct registered_counter rc;
    rc.counter = counter;
    rc.definition = definition;
    set__insert_registered_counter(&sc->counters_, &rc);

    struct configuration* config = (struct configuration*)hashmap__get(&sc->backend_config_, &(definition.address.block_type), sizeof(enum block_type));
    bitset__set_bit(&config->enable_map, definition.address.offset);

#ifdef ENABLE_DEBUG
    printf("(in add counter) enable map\n");
    for(int i = 0; i < 128; i++){
        struct configuration* temp_config = (struct configuration*)hashmap__get(&sc->backend_config_, &(definition.address.block_type), sizeof(enum block_type));
        if(bitset__get_bit(&temp_config->enable_map, i)){
            printf("\t i: %d / value: 1\n", i);
        }
    }
#endif

}

/**
 * @brief Constructs a list of enable maps, specific to the selected GPU,
 * that can be used by the sampler backend to enable the performance
 * counters.
 */
void sampler_config__build_backend_config_list(struct sampler_config* sc, struct vector* config_list){
    vector__init(config_list);

    for (int block_type = 0; block_type < sc->backend_config_.capacity; block_type++) {
        struct node *node = sc->backend_config_.nodes[block_type];
        while (node) {
            if (node->value == NULL) {
                node = node->next;
                continue;
            }
            
            struct configuration* config = (struct configuration*)node->value;
            if(bitset__any(&config->enable_map)){
                vector__push_back(config_list, config, sizeof(struct configuration));
            }

            node = node->next;
        }
    }

    return;
}

void sampler__init(struct sampler* s, struct sampler_config* config){
    memset(s, 0, sizeof(struct sampler));

    s->handle_ = (struct handle*)malloc(sizeof(struct handle));
    s->instance_ =(struct instance*)malloc(sizeof(struct instance));
    s->sampler_ = (struct vinstr_backend*)malloc(sizeof(struct vinstr_backend));
    
    handle__create(s->handle_, config->device_number_);
    instance__create(s->instance_, s->handle_);
    s->constants_ = s->instance_->constants_;
    s->values_are_64_bit_ = 0; // TODO: hardcoding

    // reserve the sample list buffer and map counters to posisions within
    // the buffer
    struct set* valid_counters = &config->counters_;
    sampler__build_sample_buffer_mappings(s, valid_counters);

    // finally, try to create the backend sampler using the config array
    struct vector config_array_vector;
    sampler_config__build_backend_config_list(config, &config_array_vector);    

#ifdef ENABLE_DEBUG
    printf("(sampler init) config len: %lu\n", config_array_vector.size);
#endif

    struct configuration* config_array = NULL;
    configuration_vector_to_array(&config_array_vector, &config_array);
    vinstr_backend__init_with_args(s->sampler_, s->instance_, config_array, config_array_vector.size);    
    if(!s->sampler_){
        printf("[ERROR] Backend sampler failure.\n");
        exit(0);
    }

    free(config_array);
    vector__destroy(&config_array_vector);

    return;
}

/**
 * @brief Starts counter accumulation.
 *
 * @return Returns an error if the sampler backend could not be started or
 * if sampling was already in progress, otherwise returns a default
 * constructed error_code.
 */
void sampler__start_sampling(struct sampler* s) {
    // if (s->sampling_in_progress_) {
    //     printf("[ERROR] sampling already started\n");
    //     exit(0);
    // }

    vinstr_backend__start(s->sampler_, 0); // accumulation start
    s-> sampling_in_progress_ = 1;

    return;
}


/**
 * @brief Requests that counter accumulation is stopped. If the request
 * fails an error is returned, the sampler should be considered to be in an
 * undefined state and should not be used further.
 *
 * @return A default constructed error_code on success, or an error if the
 * request failed.
 */
void sampler__stop_sampling(struct sampler* s){
    vinstr_backend__stop(s->sampler_, 0); // accumulation stop
    s->sampling_in_progress_ = 0;

    return;
}


/**
 * @brief Updates the sample buffer with the most recent counter values.
 * The buffer can then be queried via get_counter_value(). If an error
 * occurs while trying to read the counters then the contents of the sample
 * buffer are unchanged and further queries will return the old values.
 *
 * @return An error if sampling has not been started, or if an error
 * occurred while reading counters from the GPU.
 */
void sampler__sample_now(struct sampler* s){
#ifdef ENABLE_DEBUG
    printf("(sample_now)\n");
#endif

    if(!s->sampling_in_progress_){
        printf("[ERROR] (sample_now) sampling not started");
        exit(0);
    }

    vinstr_backend__request_sample(s->sampler_, 0);
    // vinstr_backend__get_reader(&s->sampler_); // sampler_ == reader
    struct vinstr_sample backend_sample;
    vinstr_sample__init(&backend_sample, s->sampler_);
    
    struct sample_metadata* metadata = &backend_sample.metadata_;
    if(metadata->flags.error || metadata->flags.stretched){
        s->valid_sample_buffer_ = 0;
        printf("[ERROR] (sample_now) metadata flag error: %d / metadata flag stretched: %d\n", metadata->flags.error, metadata->flags.stretched);
    }    
    s->last_collection_timestamp_ = metadata->timestamp_ns_begin;    

    // clear out any samples from the previous poll
    for (int i = 0; i < s->sample_buffer_.size; i++) {
        if (s->sample_buffer_.data[i] == NULL) {
            continue;
        }
        int zero = 0;
        vector__update(&s->sample_buffer_, i, &zero, sizeof(zero));
    }

    // loop over the counter blocks returned by the reader and fetch any
    // samples that were requested
    // TODO: 64bit
    sampler__fill_sample_buffer(s, &backend_sample);

    s->valid_sample_buffer_ = 1;

    vinstr_sample__destroy(&backend_sample, s->sampler_);

    return;
}


/** @@NOTE@@
 * @brief Fetches the last sampled value for a counter.
 *
 * @param [in]  counter  The counter to read.
 * @param [out] sample   Populated with data from the counter.
 * @return Returns hwcpipe::errc::unknown_counter if the counter was not
 * configured for sampling, otherwise an empty error_code.
 */
void sampler__get_counter_value_with_sample(struct sampler* s, enum hwcpipe_counter counter, struct counter_sample* sample){
    // if the data wasn't captured during the last poll flag an error for
    // the caller    

    if(!s->valid_sample_buffer_){
        printf("[ERROR] Sample collection failure.\n");
        exit(0);
    }

    // TODO: Expression

    size_t buffer_pos = *(size_t*)hashmap__get(&s->counter_to_buffer_pos_, &counter, sizeof(counter));

    // TODO: For double    ;
    uint64_t value = *(uint64_t*)vector__get(&s->sample_buffer_, buffer_pos);
    counter_sample__init_uint64(sample, counter, s->last_collection_timestamp_, value);

    return;
}


// Don't need
// uint64_t get_hardware_counter_value(struct sampler* s, int it){
// }

void sampler__get_expression_counter_value(){ // TODO
    return;
} 

// double sampler__get_counter_value(struct sampler* s, enum hwcpipe_counter counter){
//     size_t buffer_pos = *(size_t*)hashmap__get(&s->counter_to_buffer_pos_, &counter, sizeof(counter));
//     double output;
//     output = *(double*)vector__get(&s->sample_buffer_, buffer_pos);
//     return output;
// }

void sampler__get_counter_value(struct sampler* sampler, enum hwcpipe_counter counter, struct counter_sample* sample){
    if(!sampler->valid_sample_buffer_){
        printf("[ERROR] (get_counter_value) sample collsection failure\n");
        exit(0);
    }

    if(!hashmap__contains(&sampler->counter_to_buffer_pos_, &counter, sizeof(enum hwcpipe_counter))){
        printf("[ERROR] (get_counter_value) Unknown counter.\n");
        exit(0);
    }

    // TODO: It only works for uint64
    int index;
    uint64_t value;
    index  = *(int*)hashmap__get(&sampler->counter_to_buffer_pos_, &counter, sizeof(enum hwcpipe_counter));
    value = *(uint64_t*)vector__get(&sampler->sample_buffer_, index);

    sample->counter = counter;
    sample->timestamp = sampler->last_collection_timestamp_;
    sample->value.uint64 = value;
    sample->type = counter_sample_type_uint64;

    return;
}

// TODO
double sampler__get_mali_config_ext_bus_byte_size(){
    return 0.0;
}

// TODO
double sampler__get_mali_config_shader_core_count(){
    return 0.0;
}

// TODO
double sampler__get_mali_config_l2_cache_count(){
    return 0.0;
}

/**
 * Reserves memory for the samples and sets up the various mappings that are
 * needed to convert between counter names & positions in the buffer.
 */

void sampler__build_sample_buffer_mappings(struct sampler* s, struct set* counters){ // set for registerd_counter
    // reserve space    
    int num_counters = counters->size;
    hashmap__init(&s->counter_to_buffer_pos_);
    hashmap__reserve(&s->counter_to_buffer_pos_, num_counters);
    hashmap__init(&s->counters_by_block_map_);
    vector__init(&s->sample_buffer_);

    // set up the index needed by the sample writer to convert from
    // block/offset to sample buffer position
    for(size_t i = 0; i != (size_t) block_type_last + 1; ++i){
        enum block_type block_type;
        struct vector* value;
        block_type = (enum block_type)i;
        value = (struct vector*)malloc(sizeof(struct vector));
        if (!value) {
            fprintf(stderr, "(build_sample_buffer_mappings) Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
        vector__init(value);
        hashmap__put(&s->counters_by_block_map_, &block_type, sizeof(enum block_type), value, sizeof(struct vector));

    }

#ifdef ENABLE_DEBUG
    printf("(build_sample_buffer_mappings) num_counters: %d\n", (int)num_counters);
    printf("(build_sample_buffer_mappings) sample_buffer_.size(): %d\n", (int)s->sample_buffer_.size);
#endif

    struct set_node *cur_node = counters->head;
    while (cur_node != NULL){
        struct registered_counter* counter;
        size_t buffer_pos;
        counter = (struct registered_counter *)cur_node->data;
        buffer_pos = s->sample_buffer_.size;

        uint64_t initial_sample_value = 0;
        struct vector* block_pos_list;
        struct offset_to_buffer_pos block_pos_entry;
        switch(counter->definition.tag){
            case hardware:
                // build the index needed when the caller reads a counter by name               
                hashmap__put(&s->counter_to_buffer_pos_, &counter->counter, sizeof(enum hwcpipe_counter), &buffer_pos, sizeof(size_t));
                vector__push_back(&s->sample_buffer_, &initial_sample_value, sizeof(uint64_t));

                block_pos_list = (struct vector*)hashmap__get(&s->counters_by_block_map_, &counter->definition.address.block_type, sizeof(counter->definition.address.block_type));                
                block_pos_entry.block_offset = counter->definition.address.offset;
                block_pos_entry.buffer_pos = buffer_pos;
                block_pos_entry.shift = counter->definition.address.shift;
                vector__push_back(block_pos_list, &block_pos_entry, sizeof(struct offset_to_buffer_pos));
                break;
            default:
                // TODO
                break;
        }
        cur_node = cur_node->next;
    }
}


/**
 * Reads samples from the kinstr/vinstr reader and collects them into the
 * sample buffer. Templated because the void* we get from the reader is
 * either uint32_t* or uint64_t*, depending on the GPU.
 */
// TODO: need to exapnsion for 64bit
void sampler__fill_sample_buffer(struct sampler* s, struct vinstr_sample* backend_sample) {
    struct vinstr_blocks_view bv;
    blocks_view__init(&bv, s->sampler_, &backend_sample->sample_hndl_);
    struct vinstr_block_iterator it = blocks_view__begin(&bv);
    struct vinstr_block_iterator end = blocks_view__end(&bv);

    while(!block_iterator__equals(&it, &end)) {
        if(it.reader_ == NULL) printf("it is NULL\n");
        enum block_type block_type = it.metadata_.type;
        struct vector* counters_in_block = (struct vector*)hashmap__get(&s->counters_by_block_map_, &block_type, sizeof(block_type));

        // Access the block's data values assuming it's of type values_type_t
        const uint32_t* block_buffer = (const uint32_t *)(it.metadata_.values);

        for (size_t i = 0; i < (*counters_in_block).size; ++i) {
            struct offset_to_buffer_pos* mapping = (struct offset_to_buffer_pos*)vector__get(counters_in_block, i);
            uint64_t* sample = (uint64_t*)vector__get(&s->sample_buffer_, mapping->buffer_pos);
            *sample += block_buffer[mapping->block_offset] << mapping->shift;
        }
        block_iterator__increment(&it);
    }
}



// handle_type -> struct handle
// instance_type -> instance
// sampler_type -> vinstr_backend?
// sample_type -> vinstr_sample
// block_type -> enum block_type

#endif