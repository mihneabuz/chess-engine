#include "transpositions.h"
#include <array>
#include <string>
#include <vector>

static std::array<std::vector<hash_entry>, HASH_TABLE_SIZE> hash_table;

void clear_trans_table() {
    for (auto& bucket : hash_table)
        bucket.clear();
}

int get_trans_table_size() {
    int acc = 0;
    for (auto& bucket : hash_table)
        acc += bucket.size();
   
    return acc;
}

void update_trans_table() {
    if (get_trans_table_size() > HASH_TABLE_SIZE * 10)
        clear_trans_table();
}

hash_entry& get_entry(uint64_t zobrist) {
    int key = zobrist % HASH_TABLE_SIZE;

    auto& bucket = hash_table[key];
    for (auto& a : bucket)
        if (a.zobrist == zobrist)
            return a;

    bucket.push_back({zobrist, 0, 0, IGNORE});
    return bucket.back();
}
