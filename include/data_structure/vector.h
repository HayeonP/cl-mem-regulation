#ifndef DATA_STRUCTURE_VECTOR_H
#define DATA_STRUCTURE_VECTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct vector {
    void** data;
    size_t size;
    size_t capacity;
};

void vector__init(struct vector *vector) {
    vector->capacity = 4; // Initial capacity
    vector->size = 0;
    vector->data = (void**)malloc(sizeof(void*) * vector->capacity);
    if (!vector->data) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    memset(vector->data, 0, sizeof(void*) * vector->capacity);
}

void vector__init_with_size(struct vector *vector, int size) {
    vector->capacity = size + 10; // Initial capacity
    vector->size = size;
    vector->data = (void**)malloc(sizeof(void*) * vector->capacity);
    if (!vector->data) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    memset(vector->data, 0, sizeof(void*) * vector->capacity);
}

void vector__free(struct vector *vector) {
    for (int i = 0; i < vector->size; i++) {
        free(vector->data[i]); // 각 요소의 메모리 해제
    }
    free(vector->data); // 데이터 배열의 메모리 해제
}

void vector__resize(struct vector *vector) {
    vector->capacity *= 2;
    vector->data = (void**)realloc(vector->data, sizeof(void*) * vector->capacity);
    if (!vector->data) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
}

void vector__push_back(struct vector *vector, void *element, size_t element_size) {
    if (vector->size == vector->capacity) {
        vector__resize(vector);
    }
    vector->data[vector->size] = malloc(element_size);
    if (!vector->data[vector->size]) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(vector->data[vector->size], element, element_size);
    vector->size++;
}

void *vector__get(struct vector *vector, int index) {
    if (index >= 0 && index < vector->size) {
        return vector->data[index];
    }
    return NULL; // 인덱스가 범위를 벗어나면 NULL 반환
}

void vector__update(struct vector *vector, int index, void *element, size_t element_size) {
    if (index < 0 || index >= vector->size) return;

    memcpy(vector->data[index], element, element_size);
}

void vector__remove(struct vector *vector, int index) {
    if (index < 0 || index >= vector->size) return;

    free(vector->data[index]); // 요소의 메모리 해제
    for (int i = index; i < vector->size - 1; i++) {
        vector->data[i] = vector->data[i + 1];
    }
    vector->size--;
}

void vector__clear(struct vector *vector) {
    for (int i = 0; i < vector->size; i++) {
        free(vector->data[i]); // 각 요소의 메모리 해제
    }
    free(vector->data); // 데이터 배열의 메모리 해제
    vector->data = NULL;
    vector->size = 0;
    vector->capacity = 0;
}

void vector__destroy(struct vector *vector) {
    vector__clear(vector);
}


void vector__example() {
    struct vector vector;
    vector__init(&vector);

    int value1 = 100;
    int value2 = 200;
    int value3 = 300;

    vector__push_back(&vector, &value1, sizeof(value1));
    vector__push_back(&vector, &value2, sizeof(value2));
    vector__push_back(&vector, &value3, sizeof(value3));

    printf("int: %d\n", *(int*)vector__get(&vector, 0));
    printf("int: %d\n", *(int*)vector__get(&vector, 1));
    printf("int: %d\n", *(int*)vector__get(&vector, 2));

    vector__remove(&vector, 1); // 2번째 요소 요소 삭제
    printf("int: %d\n", *(int*)vector__get(&vector, 0));
    printf("int: %d\n", *(int*)vector__get(&vector, 1));

    vector__free(&vector);
}

#endif
