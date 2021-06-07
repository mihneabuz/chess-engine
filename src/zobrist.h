#ifndef _ZOBRIST_H_
#define _ZOBRIST_H_

#include <unordered_map>
#include "boardstate.h"

// bitstring table used for hashing
extern uint64_t hash_table[2][6][64];
extern uint64_t check_hash_table[16];
extern uint64_t castle_rights_hash_table[16];
extern uint64_t enpass_square_hash_table[65];

// init hash table
void init_zobrist_table(uint64_t seed);

// static hash, used for verifying rolling hash
uint64_t hash_state(const Boardstate& B);

#endif
