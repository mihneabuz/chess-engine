#ifndef _TRANSPOSITIONS_H_
#define _TRANSPOSITIONS_H_
#include <bits/stdint-uintn.h>
#include <string>

#define HASH_TABLE_SIZE 32768

struct hash_entry {
    uint64_t zobrist;
    int depth;
    int best_move;
    int flag;
};

enum {
    IGNORE = 0,
    BEST_MOVE = 1,
    GOOD_MOVE = 2,
};

hash_entry& get_entry(uint64_t zobrist);

// clears tt if necessary
void update_trans_table();

void clear_trans_table();
int get_trans_table_size();

#endif 
