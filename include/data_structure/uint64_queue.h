#ifndef DATA_STRUCTURE_uint64_queue_H
#define DATA_STRUCTURE_uint64_queue_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

struct uint64_queue {
    unsigned long long push_idx;
    unsigned long long pop_idx;
    int max_size;
    uint64_t* elements;
};

void uint64_queue__init(struct uint64_queue* q, uint32_t n) {
    q->push_idx = 0;
    q->pop_idx = 0;
    q->max_size = n + 1; // 실제로 사용 가능한 공간은 n개
    q->elements = (uint64_t*)malloc(sizeof(uint64_t) * q->max_size);
}

void uint64_queue__destroy(struct uint64_queue* q) {
    free(q->elements);
}

int uint64_queue__is_empty(const struct uint64_queue* q) {
    return q->push_idx == q->pop_idx;
}

int uint64_queue__is_full(const struct uint64_queue* q) {
    return ((q->push_idx + 1) % q->max_size) == q->pop_idx;
}

uint32_t uint64_queue__size(const struct uint64_queue* q) {
    if (q->push_idx >= q->pop_idx) {
        return q->push_idx - q->pop_idx;
    } else {
        return q->max_size - (q->pop_idx - q->push_idx);
    }
}

void uint64_queue__push(struct uint64_queue* q, uint64_t value) {
    q->elements[q->push_idx] = value;
    q->push_idx = (q->push_idx + 1) % q->max_size;
}

uint64_t uint64_queue__pop(struct uint64_queue* q) {
    uint64_t result = q->elements[q->pop_idx];
    q->pop_idx = (q->pop_idx + 1) % q->max_size;
    return result;
}

uint64_t uint64_queue__front(const struct uint64_queue* q) {
    assert(!uint64_queue__is_empty(q));

    return q->elements[q->pop_idx];
}

uint64_t uint64_queue__back(const struct uint64_queue* q) {
    assert(!uint64_queue__is_empty(q));

    return q->elements[(q->push_idx - 1 + q->max_size) % q->max_size];
}

uint64_t uint64_queue__at(const struct uint64_queue* q, uint32_t index) {
    assert(index < uint64_queue__size(q));

    uint32_t real_index = (q->pop_idx + index) % q->max_size;
    return q->elements[real_index];
}

uint64_t uint64_queue__pop_count(const struct uint64_queue* q) {
    return q->pop_idx;
}

int uint64_queue__example() {
    struct uint64_queue q;
    uint64_queue__init(&q, 32); // 최대 크기 32로 설정
    for (int i = 1; i <= 32; ++i) {
        uint64_queue__push(&q, i);
    }
    
    while (!uint64_queue__is_empty(&q)) {
        printf("Popped: %lu\n", uint64_queue__pop(&q));
    }

    uint64_queue__destroy(&q);
    
    return 0;
}

#endif
