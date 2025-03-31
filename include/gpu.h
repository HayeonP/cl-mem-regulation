#ifndef GPU_H
#define GPU_H

#include <string.h>
#include "handle.h"
#include "instance.h"
#include "constants.h"
#include "product_id.h"


struct gpu {
    int device_number_;
    int valid_;
    struct constants constants_;
};

void gpu__get_product_id(struct gpu* gpu, struct product_id* pid){
    product_id__from_raw_gpu_id(pid, gpu->constants_.gpu_id);
}

void gpu__fetch_device_info(struct gpu* gpu, struct handle* handle){
    struct instance instance;
    instance__create(&instance, handle);
    gpu->constants_ = instance.constants_;

}

void gpu__init(struct gpu* gpu, int device_number){
    int ret;
    memset(gpu, 0, sizeof(struct gpu));
    gpu->device_number_ = device_number;
    printf("gpu->device_number: %d\n", gpu->device_number_);
    
    struct handle handle;
    ret = handle__create(&handle, device_number);

    if(ret < 0){
        gpu->valid_ = 0;
    }
    else{
        gpu->valid_ = 1;
        gpu__fetch_device_info(gpu, &handle);
    }

}

#endif