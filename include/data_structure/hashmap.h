#ifndef DATA_STRUCTURE_HASHMAP_H
#define DATA_STRUCTURE_HASHMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
    void *key;
    void *value;
    struct node *next;
    size_t key_size; // 추가: 키의 크기를 저장
};

struct hashmap {
    struct node **nodes;
    int capacity;
    int size;
    float load_factor;
};

unsigned int hashmap__hash(struct hashmap *map, void *key, size_t key_size) {
    unsigned char *p = (unsigned char *)key;
    unsigned int h = 0;
    for (size_t i = 0; i < key_size; i++) {
        h = 31 * h + p[i];
    }
    return h % map->capacity;
}

void hashmap__init(struct hashmap *map) {
    map->capacity = 10; // Fixed initial capacity
    map->size = 0;
    map->load_factor = 0.75; // Fixed load factor
    map->nodes = (struct node**)malloc(sizeof(struct node*) * map->capacity);
    if (!map->nodes) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < map->capacity; i++) {
        map->nodes[i] = NULL;
    }
}

void hashmap__reserve(struct hashmap *map, int new_capacity) {
    // Return if current capacity is bigger
    if (new_capacity <= map->capacity) {
        return;
    }

    // Reallocate node array memory
    struct node **new_nodes = (struct node**)malloc(sizeof(struct node*) * new_capacity);
    if (!new_nodes) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Initialize node array
    for (int i = 0; i < new_capacity; i++) {
        new_nodes[i] = NULL;
    }

    // Move to new node.
    for (int i = 0; i < map->capacity; i++) {
        struct node *node = map->nodes[i];
        while (node) {
            struct node *next = node->next;
            unsigned int index = hashmap__hash(map, node->key, node->key_size);
            node->next = new_nodes[index];
            new_nodes[index] = node;
            node = next;
        }
    }

    free(map->nodes);
    map->nodes = new_nodes;
    map->capacity = new_capacity;
}

void hashmap__rehash(struct hashmap *map) {
    int old_capacity = map->capacity;
    struct node **old_nodes = map->nodes;

    map->capacity *= 2;
    map->nodes = (struct node**)malloc(sizeof(struct node*) * map->capacity);
    if (!map->nodes) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < map->capacity; i++) {
        map->nodes[i] = NULL;
    }

    for (int i = 0; i < old_capacity; i++) {
        struct node *node = old_nodes[i];
        while (node) {
            struct node *next = node->next;
            unsigned int index = hashmap__hash(map, node->key, node->key_size);
            node->next = map->nodes[index];
            map->nodes[index] = node;
            node = next;
        }
    }
    free(old_nodes);
}

void hashmap__destroy(struct hashmap *map) {
    for (int i = 0; i < map->capacity; i++) {
        struct node *node = map->nodes[i];
        while (node) {
            struct node *next = node->next;
            free(node->key);
            free(node->value);
            free(node);
            node = next;
        }
    }
    free(map->nodes);
}

void hashmap__put(struct hashmap *map, void *key, size_t key_size, void *value, size_t value_size) {
    if ((float)(map->size + 1) / map->capacity >= map->load_factor) {
        hashmap__rehash(map);
    }

    unsigned int index = hashmap__hash(map, key, key_size);
    struct node *node = (struct node *)malloc(sizeof(struct node));
    if (!node) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    node->key = malloc(key_size);
    if (!node->key) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    node->value = malloc(value_size);
    if (!node->value) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(node->key, key, key_size);
    memcpy(node->value, value, value_size);
    node->next = map->nodes[index];
    node->key_size = key_size; // 키 크기 저장
    map->nodes[index] = node;
    map->size++;
}

void *hashmap__get(struct hashmap *map, void *key, size_t key_size) {
    unsigned int index = hashmap__hash(map, key, key_size);
    struct node *node = map->nodes[index];
    while (node) {
        if (node->key_size == key_size && memcmp(key, node->key, key_size) == 0) {
            return node->value;
        }
        node = node->next;
    }
    return NULL;
}

int hashmap__contains(struct hashmap *map, void *key, size_t key_size) {
    unsigned int index = hashmap__hash(map, key, key_size);
    struct node *node = map->nodes[index];
    while (node) {
        if (node->key_size == key_size && memcmp(key, node->key, key_size) == 0) {
            return 1;
        }
        node = node->next;
    }
    return 0;
}

void hashmap__example() {
    struct hashmap map;
    hashmap__init(&map);

    int key1 = 1; // 정수 키 예시
    double value1 = 123.456;

    hashmap__put(&map, &key1, sizeof(key1), &value1, sizeof(value1));

    double *retrieved_value1 = (double *)hashmap__get(&map, &key1, sizeof(key1));
    if (retrieved_value1) {
        printf("Key %d: %f\n", key1, *retrieved_value1);
    }

    if (hashmap__contains(&map, &key1, sizeof(key1))) {
        printf("Key %d exists in the map.\n", key1);
    } else {
        printf("Key %d does not exist in the map.\n", key1);
    }

    hashmap__destroy(&map);
}

#endif
