/* 20190650 Gwanho Kim */

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cachelab.h"

struct Block {
    bool is_valid;
    unsigned int tag;
    unsigned int placed_time;
};

unsigned int s, E, b, hit_count, miss_count, eviction_count, time_count;
struct Block **cache;
void operate(unsigned int address);
bool find(unsigned int set_index, unsigned int tag);
bool is_full(unsigned int set_index);
void evict(unsigned int set_index, unsigned int tag);
void place(unsigned int set_index, unsigned int tag);

void operate(unsigned int address) {
    unsigned int tag = address >> (s + b);
    unsigned int set_index = (address >> b) & ((1 << s) - 1);
    if (find(set_index, tag)) {
        hit_count++;
    } else {
        miss_count++;
        if (is_full(set_index)) {
            eviction_count++;
            evict(set_index, tag);
        } else {
            place(set_index, tag);
        }
    }
}

bool find(unsigned int set_index, unsigned int tag) {
    for (int i = 0; i < E; i++) {
        // Find the block which is valid and has the same tag as the tag of the
        // address
        if (cache[set_index][i].is_valid && cache[set_index][i].tag == tag) {
            cache[set_index][i].placed_time = time_count;
            return true;
        }
    }
    return false;
}

bool is_full(unsigned int set_index) {
    for (int i = 0; i < E; i++) {
        if (!cache[set_index][i].is_valid) {
            return false;
        }
    }
    return true;
}

void evict(unsigned int set_index, unsigned int tag) {
    int lru_index = 0;
    // Find the lru(Least Recently Used) block
    for (int i = 0; i < E; i++) {
        lru_index = cache[set_index][i].placed_time <
                            cache[set_index][lru_index].placed_time
                        ? i
                        : lru_index;
    }
    cache[set_index][lru_index].tag = tag;
    cache[set_index][lru_index].placed_time = time_count;
}

void place(unsigned int set_index, unsigned int tag) {
    for (int i = 0; i < E; i++) {
        // If there is an empty block, place the block
        if (!cache[set_index][i].is_valid) {
            cache[set_index][i].is_valid = true;
            cache[set_index][i].tag = tag;
            cache[set_index][i].placed_time = time_count;
            return;
        }
    }
}

int main(int argc, char *argv[]) {
    // Parse s, E and b which are in terminal command like "./csim -s 1 -E 1 -b
    // 1 -t traces/yi2.trace"
    FILE *trace_file;
    for (int arg; (arg = getopt(argc, argv, "s:E:b:t:")) != -1;) {
        switch (arg) {
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            trace_file = fopen(optarg, "r");
            break;
            // default:
            //     printf("wrong input\n");
            //     break;
        }
    }

    // Initialize cache
    int S = 1 << s;
    cache = (struct Block **)malloc(sizeof(struct Block *) * S);
    for (int i = 0; i < S; i++) {
        cache[i] = (struct Block *)malloc(sizeof(struct Block) * E);
        for (int j = 0; j < E; j++) {
            cache[i][j].is_valid = false;
            cache[i][j].tag = 0;
            cache[i][j].placed_time = time_count;
        }
    }

    // Read trace file
    char operation;
    unsigned int address, size;
    // Input type of address is hexadecimal
    while (fscanf(trace_file, " %c %x,%u", &operation, &address, &size) !=
           EOF) {
        if (operation == 'I') {
            continue;
        }
        time_count++;
        operate(address);
        if (operation == 'M') {
            operate(address);
        }
    }

    // Print result
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
