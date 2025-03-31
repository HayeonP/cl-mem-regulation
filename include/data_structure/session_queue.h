#ifndef DATA_STRUCTURE_SESSION_QUEUE_H
#define DATA_STRUCTURE_SESSION_QUEUE_H

#include <stdio.h>
#include <stdlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "session.h"

struct session_queue{
    unsigned long long push_idx;
    unsigned long long pop_idx;
    struct session* elements;
    int max_size;
};

void session_queue__init(struct session_queue* q, int n) {
    q->push_idx = 0;
    q->pop_idx = 0;
    q->max_size = n;
    q->elements = (struct session*)malloc(sizeof(struct session)*n);
    for(int i = 0; i < n; i++){
        session__init_with_args(&(q->elements[i]), 0, 0);
    }

    return;
}

void session_queue__destroy(struct session_queue* q){
    free(q->elements);

    return;
}

int session_queue__is_empty(const struct session_queue* q) {
    return q->push_idx == q->pop_idx;
}

int session_queue__is_full(const struct session_queue* q) {
    return (q->push_idx - q->pop_idx) == q->max_size;
}

int session_queue__size(const struct session_queue* q) {
    return q->push_idx - q->pop_idx;
}

void session_queue__push(struct session_queue* q, struct session value) {
    assert(!session_queue__is_full(q));

    unsigned long long index = q->push_idx % q->max_size;
    q->elements[index] = value;
    q->push_idx++;
}

struct session session_queue__pop(struct session_queue* q) {
    assert(!session_queue__is_empty(q));

    unsigned long long index = q->pop_idx % q->max_size;
    struct session result = q->elements[index];
    q->pop_idx++;
    return result;
}

struct session session_queue__front(const struct session_queue* q) {
    assert(!session_queue__is_empty(q));

    unsigned long long index = q->pop_idx % q->max_size;
    return q->elements[index];
}

struct session session_queue__back(const struct session_queue* q) {
    assert(!session_queue__is_empty(q));

    unsigned long long index = (q->push_idx - 1) % q->max_size;
    return q->elements[index];
}

struct session session_queue__at(const struct session_queue* q, unsigned int index) {
    assert(index < session_queue__size(q));

    unsigned long long real_index = (q->pop_idx + index) % q->max_size;
    return q->elements[real_index];
}

#endif