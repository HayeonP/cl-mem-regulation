#ifndef DATA_STRUCTURE_BITSET_H
#define DATA_STRUCTURE_BITSET_H

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

struct bitset {
    uint32_t* bits;
    int size;
};

void bitset__init(struct bitset* bitset, int size);
void bitset__delete(struct bitset* bitset);
void bitset__set_bit(struct bitset* bitset, int index);
void bitset__clear_bit(struct bitset* bitset, int index);
int bitset__get_bit(struct bitset* bitset, int index);
int bitset__any(struct bitset* bitset);
void bitset__set_bits_from_number(struct bitset* bitset, uint32_t number);
void bitset__shift_left(struct bitset* bitset, int positions);
void bitset__shift_right(struct bitset* bitset, int positions);
uint32_t bitset__to_uint32(struct bitset* bitset);
void uint64_to_bitset(uint64_t value, struct bitset* bitset);
struct bitset bitset__and(struct bitset* bitset1, struct bitset* bitset2);
unsigned int bitset__popcount(struct bitset* bitset);
void bitset__print(struct bitset* bitset);
void bitset__set_bits_from_binary(struct bitset* bitset, const char* binary_str);

void bitset__init(struct bitset* bitset, int size) {
    memset(bitset, 0, sizeof(struct bitset));
    bitset->bits = (uint32_t *)calloc((size + 31) / 32, sizeof(uint32_t));
    bitset->size = size;
}

void bitset__delete(struct bitset* bitset) {
    free(bitset->bits);
}

void bitset__set_bit(struct bitset* bitset, int index) {
    bitset->bits[index / 32] |= 1U << (index % 32);
}

void bitset__clear_bit(struct bitset* bitset, int index) {
    bitset->bits[index / 32] &= ~(1U << (index % 32));
}

int bitset__get_bit(struct bitset* bitset, int index) {
    return (bitset->bits[index / 32] >> (index % 32)) & 1;
}

int bitset__any(struct bitset* bitset) {
    for (int i = 0; i < (bitset->size + 31) / 32; ++i) {
        if (bitset->bits[i] != 0) {
            return 1;
        }
    }
    return 0;
}

void bitset__set_bits_from_number(struct bitset* bitset, uint32_t number) {
    for (int i = 0; i < bitset->size && i < 32; i++) {
        if (number & (1U << i)) {
            bitset__set_bit(bitset, i);
        } else {
            bitset__clear_bit(bitset, i);
        }
    }
}

void bitset__shift_left(struct bitset* bitset, int positions) {
    int shift_blocks = positions / 32;
    int shift_bits = positions % 32;
    int num_blocks = (bitset->size + 31) / 32;

    for (int i = num_blocks - 1; i >= 0; i--) {
        if (i >= shift_blocks) {
            bitset->bits[i] = bitset->bits[i - shift_blocks];
        } else {
            bitset->bits[i] = 0;
        }

        if (i > shift_blocks) {
            bitset->bits[i] |= bitset->bits[i - shift_blocks - 1] >> (32 - shift_bits);
        }

        bitset->bits[i] <<= shift_bits;
    }
}

void bitset__shift_right(struct bitset* bitset, int positions) {
    int shift_blocks = positions / 32;
    int shift_bits = positions % 32;
    int num_blocks = (bitset->size + 31) / 32;

    if (shift_blocks >= num_blocks) {
        for (int i = 0; i < num_blocks; i++) {
            bitset->bits[i] = 0;
        }
        return;
    }

    for (int i = 0; i < num_blocks; i++) {
        int src_index = i + shift_blocks;
        uint32_t lower_part = src_index < num_blocks ? bitset->bits[src_index] : 0;

        uint32_t upper_part = 0;
        if (shift_bits != 0 && src_index + 1 < num_blocks) {
            upper_part = bitset->bits[src_index + 1] << (32 - shift_bits);
        }

        bitset->bits[i] = (lower_part >> shift_bits) | upper_part;
    }

    for (int i = num_blocks - shift_blocks; i < num_blocks; i++) {
        bitset->bits[i] = 0;
    }
}

uint32_t bitset__to_uint32(struct bitset* bitset) {
    if (bitset->size < 32) {
        printf("Error: struct bitset size is smaller than 32.\n");
        return 0;
    }
    return bitset->bits[0];
}

void uint64_to_bitset(uint64_t value, struct bitset* bitset) {
    bitset__init(bitset, 64);
    for (int i = 0; i < 64; i++) {
        if (value & (1ULL << i)) {
            bitset->bits[i / 32] |= (1U << (i % 32));
        }
    }
}

struct bitset bitset__and(struct bitset* bitset1, struct bitset* bitset2) {
    struct bitset result;
    if (bitset1->size != bitset2->size) {
        printf("[ERROR] struct bitsets are not the same size. bitset1: %d / bitset2: %d\n", bitset1->size, bitset2->size);
        exit(1);
    }

    result.bits = (uint32_t *)calloc((bitset1->size + 31) / 32, sizeof(uint32_t));
    result.size = bitset1->size;

    for (int i = 0; i < (bitset1->size + 31) / 32; ++i) {
        result.bits[i] = bitset1->bits[i] & bitset2->bits[i];
    }

    return result;
}

unsigned int simple__popcount(uint32_t x) {
    unsigned int count = 0;
    while (x) {
        count += x & 1;
        x >>= 1;
    }
    return count;
}

unsigned int bitset__popcount(struct bitset* bitset) {
    unsigned int total_count = 0;
    int num_blocks = (bitset->size + 31) / 32;
    for (int i = 0; i < num_blocks; i++) {
        total_count += simple__popcount(bitset->bits[i]);
    }
    return total_count;
}

void bitset__print(struct bitset* bitset) {
    for (int i = 0; i < bitset->size; ++i) {
        printf("%d", bitset__get_bit(bitset, i));
        if ((i + 1) % 8 == 0) {
            printf(" ");
        }
    }
    printf("\n");
}

void bitset__set_bits_from_binary(struct bitset* bitset, const char* binary_str) {
    int len = strlen(binary_str);
    if (len > bitset->size) {
        printf("[ERROR] Binary string length is greater than struct bitset size.\n");
        exit(1);
    }

    for (int i = 0; i < len; i++) {
        if (binary_str[len - 1 - i] == '1') {
            bitset__set_bit(bitset, i);
        } else if (binary_str[len - 1 - i] == '0') {
            bitset__clear_bit(bitset, i);
        } else {
            printf("[ERROR] Invalid character in binary string: %c\n", binary_str[len - 1 - i]);
            exit(1);
        }
    }
}

#endif
