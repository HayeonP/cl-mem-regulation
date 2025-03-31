#ifndef DATA_STRUCTURE_SET_H
#define DATA_STRUCTURE_SET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "internal_types.h"
#include "external_types.h"
#include "hwcpipe_counter.h"

struct set_node {
    void *data;
    struct set_node *next;
};

struct set {
    struct set_node *head;
    size_t size;
};

void set__init(struct set *set) {
    set->head = NULL;
    set->size = 0;
}

void set__insert_registered_counter(struct set *set, struct registered_counter* data) {
    // Check duplication
    struct set_node *current = set->head;
    while (current != NULL) {
        struct registered_counter rc;
        rc = *(struct registered_counter*)current->data;
        if (rc.counter == data->counter) {
            return;
        }
        current = current->next;
    }

    // Create new node
    struct set_node *new_node = (struct set_node *)malloc(sizeof(struct set_node));
    if (new_node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Duplicate data to avoid the issue of temporary scope
    struct registered_counter *new_data = (struct registered_counter*)malloc(sizeof(struct registered_counter));
    memcpy(new_data, data, sizeof(struct registered_counter));

    new_node->data = new_data;
    new_node->next = set->head;

    // Update head
    set->head = new_node;
    set->size++;
}

void set__remove(struct set *set, void *data) {
    struct set_node *prev = NULL;
    struct set_node *current = set->head;

    while (current != NULL) {
        if (current->data == data) {
            if (prev == NULL) {
                // First node
                set->head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
    set->size--;
}

int set__contains(struct set *set, void *data) {
    struct set_node *current = set->head;
    while (current != NULL) {
        if (current->data == data) {
            return 1;
        }

        current = current->next;
    }
    return 0;
}

void set__clear(struct set *set) {
    struct set_node *current = set->head;
    while (current != NULL) {
        struct set_node *next = current->next;
        free(current);
        current = next;
    }
    set->head = NULL;
}

void set__destroy(struct set *set) {
    set__clear(set);
    // No need to free set itself if it's not dynamically allocated
}

void set__print(struct set *set) {
    struct set_node *current = set->head;
    printf("set contents: ");
    while (current != NULL) {
        printf("%p ", current->data);
        current = current->next;
    }
    printf("\n");
}

#endif