#ifndef ARM_GPU_HWCNT_BLOCK_ITERATOR_H
#define ARM_GPU_HWCNT_BLOCK_ITERATOR_H

// backend/device/include/device/hwcnt/block_iterator.hpp

#include <stdint.h>
#include "backend.h"
#include "hwcnt/detail/handle.h"
#include "hwcnt/block_metadata.h"

struct vinstr_block_iterator {
    struct vinstr_backend* reader_;
    struct sample_handle sample_hndl_;
    struct block_handle block_hndl_;
    struct block_metadata metadata_;
};

void block_iterator__init(struct vinstr_block_iterator* iterator) {    
    memset(&iterator->reader_, 0, sizeof(struct vinstr_backend));
    iterator->reader_ = NULL;
    sample_handle__init(&iterator->sample_hndl_);
    block_handle__init(&iterator->block_hndl_);
    memset(&iterator->metadata_, 0, sizeof(struct block_metadata));

    return;
}

void block_iterator__init_with_args(struct vinstr_block_iterator* iterator, struct vinstr_backend *r, struct sample_handle* sample_hndl) {        
    iterator->sample_hndl_ = *sample_hndl;
    block_handle__init(&iterator->block_hndl_);
    memset(&iterator->metadata_, 0, sizeof(struct block_metadata));
    
    return;
}

void block_iterator__increment(struct vinstr_block_iterator *iterator) {
    int ret;
    if(iterator->reader_ != NULL) // Ensure reader is not NULL
    {
        ret = vinstr_backend__next(iterator->reader_, &iterator->sample_hndl_, &iterator->metadata_, &iterator->block_hndl_);
            
        if (ret < 0) {
            iterator->reader_ = NULL; // End of iteration
        }
    }
    else{
        printf("reader_ is NULL!\n");
        exit(0);
    }
}

int block_iterator__equals(const struct vinstr_block_iterator *lhs, const struct vinstr_block_iterator *rhs) {    
    // Both iterators being NULL is considered equal
    if (lhs == NULL && rhs == NULL) {
        return 1;
    }
    // If only one of them is NULL, they are not equal
    if (lhs == NULL || rhs == NULL) {
        return 0;
    }
    // If both readers are NULL, they are equal
    if (lhs->reader_ == NULL && rhs->reader_ == NULL) {
        return 1;
    }
    // If only one reader is NULL, they are not equal
    if (lhs->reader_ == NULL || rhs->reader_ == NULL) {
        return 0;
    }
    // Compare the remaining members
    return lhs->reader_ == rhs->reader_
        && sample_handle__compare(&lhs->sample_hndl_, &rhs->sample_hndl_)
        && block_handle__compare(&lhs->block_hndl_, &rhs->block_hndl_);
}

int block_iterator_not_equals(const struct vinstr_block_iterator *lhs, const struct vinstr_block_iterator *rhs) {
    return !block_iterator__equals(lhs, rhs);
}


#endif