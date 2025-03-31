#ifndef ARM_GPU_MAPPED_MEMORY_H
#define ARM_GPU_MAPPED_MEMORY_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

// backend/device/src/device/hwcnt/sampler/mapped_memory.hpp

struct mapped_memory {
    void *data_;
    size_t size_;
};

void mapped_memory__init_with_fd(struct mapped_memory* mm, int fd, size_t size) {    
    memset(mm, 0, sizeof(struct mapped_memory));
    mm->data_ = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    
    if (mm->data_ == MAP_FAILED) {
        perror("mmap");
        mm->data_ = NULL;
    } else {
        mm->size_ = size;
    }

    return;
}

void mapped_memory__init_with_data(struct mapped_memory* mm, void *data, size_t size){
    memset(mm, 0, sizeof(struct mapped_memory));
    mm->data_ = data;
    mm->size_ = size;

    return;
}

void mapped_memory__assign(struct mapped_memory *dest, struct mapped_memory* src) {
    dest->data_ = src->data_;
    dest->size_ = src->size_;
}

void mapped_memory__move(struct mapped_memory *dest, struct mapped_memory *src) {
    // Copy the data and size members from src to dest
    dest->data_ = src->data_;
    dest->size_ = src->size_;
    
    // Invalidate the src struct
    src->data_ = NULL;
    src->size_ = 0;
}

void mapped_memory__delete(struct mapped_memory* mm){
    if(mm->data_ != NULL){
        munmap(mm->data_, mm->size_);
    }
}

#endif