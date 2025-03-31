#ifndef NUM_EXEC_ENGINES_H
#define NUM_EXEC_ENGINES_H

// backend/device/src/device/num_exec_engines.hpp

#include <stdint.h>
#include <stdio.h>
#include "product_id.h"

/** Argument pack for \c get_num_exec_engines. */
struct get_num_exec_engines_args {
    struct product_id id;            //!< The GPU product to query.
    uint8_t core_count;       //!< The number of cores of the GPU.
    uint32_t core_features;   //!< The raw value of the CORE_FEATURES register.
    uint32_t thread_features; //!< The raw value of the THREAD_FEATURES register.
};

void get_num_exec_engines_args__init(struct get_num_exec_engines_args* args){
    memset(args, 0, sizeof(struct get_num_exec_engines_args));
    product_id__init(&(args->id));
}

uint32_t max_register(struct product_id* pid, uint32_t raw_thread_features){
    static struct product_id product_id_g31;
    static struct product_id product_id_g51;

    product_id__init_with_major(&product_id_g31, 7, 3);
    product_id__init_with_major(&product_id_g51, 7, 0);

    if(pid->product_id_ == product_id_g31.product_id_ || pid->product_id_ == product_id_g51.product_id_){
        printf("[ERROR] Unsupported product_id for max_reigisters()");
    }

    return raw_thread_features & 0xFFFF;
}

uint8_t get_num_exec_engines(struct get_num_exec_engines_args* args){
    uint16_t g31_g51_max_registers_small_core = 0x2000;

    static struct product_id product_id_t830;
    static struct product_id product_id_t860;
    static struct product_id product_id_t880;
    static struct product_id product_id_g31;
    static struct product_id product_id_g51;
    static struct product_id product_id_g52;
    static struct product_id product_id_g57;
    static struct product_id product_id_g57_2;
    static struct product_id product_id_g68;
    static struct product_id product_id_g71;
    static struct product_id product_id_g72;
    static struct product_id product_id_g76;
    static struct product_id product_id_g77;
    static struct product_id product_id_g78;
    static struct product_id product_id_g78ae;
    static struct product_id product_id_g310;
    static struct product_id product_id_g510;
    static struct product_id product_id_g610;
    static struct product_id product_id_g710;
    static struct product_id product_id_g615;
    static struct product_id product_id_g715;
    static struct product_id product_id_g720;
    static struct product_id product_id_g720_2;
    static struct product_id product_id_g720_3;

    product_id__init_with_raw_product_id(&product_id_t830, 0x0830);
    product_id__init_with_raw_product_id(&product_id_t860, 0x0860);
    product_id__init_with_raw_product_id(&product_id_t880, 0x0880);
    product_id__init_with_major(&product_id_g31, 7, 3);
    product_id__init_with_major(&product_id_g51, 7, 0);
    product_id__init_with_major(&product_id_g52, 7, 2);
    product_id__init_with_major(&product_id_g57, 9, 1);
    product_id__init_with_major(&product_id_g57_2, 9, 3);
    product_id__init_with_major(&product_id_g68, 9, 4);
    product_id__init_with_major(&product_id_g71, 6, 0);
    product_id__init_with_major(&product_id_g72, 6, 1);
    product_id__init_with_major(&product_id_g76, 7, 1);
    product_id__init_with_major(&product_id_g77, 9, 0);
    product_id__init_with_major(&product_id_g78, 9, 2);
    product_id__init_with_major(&product_id_g78ae, 9, 5);
    product_id__init_with_major(&product_id_g310, 10, 4);
    product_id__init_with_major(&product_id_g510, 10, 3);
    product_id__init_with_major(&product_id_g610, 10, 7);
    product_id__init_with_major(&product_id_g710, 10, 2);
    product_id__init_with_major(&product_id_g615, 11, 3);
    product_id__init_with_major(&product_id_g715, 11, 2);
    product_id__init_with_major(&product_id_g720, 12, 0);
    product_id__init_with_major(&product_id_g720_2, 12, 2);
    product_id__init_with_major(&product_id_g720_3, 12, 2);

    struct product_id id = args->id;
    uint8_t core_count = args->core_count;
    uint32_t core_features = args->core_features;
    uint32_t thread_features = args->thread_features;

    uint8_t core_variant = 0;
    
    if(id.product_id_ == product_id_t830.product_id_) return 2;
    else if(id.product_id_ == product_id_t860.product_id_) return 2;
    else if(id.product_id_ == product_id_t880.product_id_) return 3;
    else if(id.product_id_ == product_id_g31.product_id_){
        if(core_count == 1 && max_register(&product_id_g31, thread_features) == g31_g51_max_registers_small_core)
            return 1;
        return 2;
    }
    else if(id.product_id_ == product_id_g51.product_id_){
        if(core_count == 1 && max_register(&product_id_g51, thread_features) == g31_g51_max_registers_small_core)
            return 1;
        return 3;
    }
    else if(id.product_id_ == product_id_g52.product_id_){
        uint8_t ee_count = (uint8_t)(core_features & 0xF);
        return ee_count;
    }
    else if(id.product_id_ == product_id_g57.product_id_) return 1;
    else if(id.product_id_ == product_id_g57_2.product_id_) return 1;
    else if(id.product_id_ == product_id_g68.product_id_) return 1;
    else if(id.product_id_ == product_id_g71.product_id_) return 3;
    else if(id.product_id_ == product_id_g72.product_id_) return 3;
    else if(id.product_id_ == product_id_g76.product_id_) return 3;
    else if(id.product_id_ == product_id_g77.product_id_) return 1;
    else if(id.product_id_ == product_id_g78.product_id_) return 1;
    else if(id.product_id_ == product_id_g78ae.product_id_) return 1;
    else if(id.product_id_ == product_id_g310.product_id_){
        core_variant = (uint8_t)(core_features & 0xF);
        switch (core_variant) {
            case 0:
            case 1:
                return 1;
            case 2:
            case 3:
            case 4:
                return 2;
        }
        return 0;
    }
    else if(id.product_id_ == product_id_g510.product_id_){
        core_variant = (uint8_t)(core_features & 0xF);
        switch (core_variant) {
            case 0:
            case 1:
                return 1;
            case 2:
            case 3:
            case 4:
                return 2;
        }
        return 0;
    }
    else if(id.product_id_ == product_id_g610.product_id_) return 2;
    else if(id.product_id_ == product_id_g710.product_id_) return 2;
    else if(id.product_id_ == product_id_g615.product_id_){
        core_variant = (uint8_t)(core_features & 0xF);
        switch (core_variant) {
            case 0:
            case 1:
                return 1;
            case 2:
            case 3:
            case 4:
                return 2;
        }
        return 0;
    }
    else if(id.product_id_ == product_id_g715.product_id_){
        core_variant = (uint8_t)(core_features & 0xF);
        switch (core_variant) {
            case 0:
            case 1:
                return 1;
            case 2:
            case 3:
            case 4:
                return 2;
        }
        return 0;
    }
    else if(id.product_id_ == product_id_g720.product_id_){
        core_variant = (uint8_t)(core_features & 0xF);
        switch (core_variant) {
            case 1:
                return 1;
            case 4:
                return 2;
        }
        return 0;
    }
    else if(id.product_id_ == product_id_g720_2.product_id_){
        core_variant = (uint8_t)(core_features & 0xF);
        switch (core_variant) {
            case 1:
                return 1;
            case 4:
                return 2;
        }
        return 0;
    }
    else if(id.product_id_ == product_id_g720_3.product_id_){
        core_variant = (uint8_t)(core_features & 0xF);
        switch (core_variant) {
            case 1:
                return 1;
            case 4:
                return 2;
        }
        return 0;
    }

    if(product_id__get_gpu_family(&id) == gpu_family_midgard) return 1;

    return 0;
}


#endif