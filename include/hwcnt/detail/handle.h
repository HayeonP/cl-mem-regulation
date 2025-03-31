#ifndef ARM_GPU_HWCNT_HANDLE_H
#define ARM_GPU_HWCNT_HANDLE_H

// backend/device/include/device/hwcnt/detail/handle.hpp

#define SAMPLE_HANDLE_SIZE 16
#define SAMPLE_HANDLE_ALIGNMENT 8
#define BLOCK_HANDLE_SIZE 8
#define BLOCK_HANDLE_ALIGNMENT 8

#include <stdlib.h>
#include <stdint.h>
#include <stdalign.h>  
#include <string.h>

struct sample_handle {
    uint8_t data_[SAMPLE_HANDLE_SIZE];
};

void sample_handle__init(struct sample_handle* handle){
    memset(handle->data_, 0, SAMPLE_HANDLE_SIZE);
}

// Generic get function, mimics the template behavior of C++
void *sample_handle__get(struct sample_handle *handle, size_t size, size_t alignment) {
    if (size > SAMPLE_HANDLE_SIZE) {
        return NULL; // Size overflow
    }
    if (SAMPLE_HANDLE_ALIGNMENT % alignment != 0) {
        return NULL; // Alignment error
    }
    return (void *)handle->data_;
}

// Define functions to compare two handles
int sample_handle__compare(const struct sample_handle *lhs, const struct sample_handle *rhs) {
    printf("sample handle compare\n");
    return memcmp(lhs->data_, rhs->data_, SAMPLE_HANDLE_SIZE) == 0;
}




struct block_handle {
    uint8_t data_[BLOCK_HANDLE_SIZE];
};

void block_handle__init(struct block_handle* handle){
    memset(handle->data_, 0, BLOCK_HANDLE_SIZE);
}

// Generic get function, mimics the template behavior of C++
void *block_handle__get(struct block_handle *handle, size_t size, size_t alignment) { 
    if (size > BLOCK_HANDLE_SIZE) {
        return NULL; // Size overflow
    }
    if (BLOCK_HANDLE_ALIGNMENT % alignment != 0) {
        return NULL; // Alignment error
    }
    return (void *)handle->data_;
}

// Define functions to compare two handles
int block_handle__compare(const struct block_handle *lhs, const struct block_handle *rhs) {
    printf("block handle compare");
    return memcmp(lhs->data_, rhs->data_, BLOCK_HANDLE_SIZE) == 0;
}

#endif