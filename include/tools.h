#ifndef TOOLS_H
#define TOOLS_H

#include <stdio.h>
#include <time.h>
#include "configuration.h"
#include "hwcpipe_counter.h"
#include "internal_types.h"
#include "external_types.h"
#include "data_structure/vector.h"
#include "data_structure/set.h"

void print_bits(char c) {
    // char 타입의 비트 수
    int bit_count = sizeof(char) * 8;

    // 각 비트를 출력
    for (int i = bit_count - 1; i >= 0; i--) {
        // 해당 비트가 1인지 확인하여 출력
        if (c & (1 << i))
            printf("1");
        else
            printf("0");

        // 8비트마다 공백으로 구분
        if (i % 8 == 0)
            printf(" ");
    }
}

int popcount_uint64(uint64_t n) {
    int count = 0;
    while (n) {
        count += n & 1;
        n >>= 1;
    }
    return count;
}

uint64_t timespec_to_uint64(struct timespec ts) {
    uint64_t nanoseconds = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    return nanoseconds;
}

void configuration_vector_to_array(struct vector* v, struct configuration** ary){
    if (v == NULL || ary == NULL) {
        printf("[ERROR] (configuration_vector_to_array) Null pointer argument\n");
        return;
    }
    
    *ary = (struct configuration*)malloc(sizeof(struct configuration) * v->size);
    if (*ary == NULL) {
        printf("[ERROR] (configuration_vector_to_array) Memory allocation failed\n");
        exit(0);
    }
    
    for (size_t i = 0; i < v->size; i++) {
        (*ary)[i] = *(struct configuration*)v->data[i];
    }
}

int set_contains_counter(struct set *set, enum hwcpipe_counter target_counter) {
    struct set_node *current = set->head;

    while (current != NULL) {
        struct registered_counter* rc;
        rc = (struct registered_counter*)current->data;

        if (rc->counter == target_counter) {
            return 1;
        }

        current = current->next;
    }
    return 0;
}


#endif