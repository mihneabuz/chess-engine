#include "zobrist.h"
#include "boardstate.h"
#include <bits/stdint-uintn.h>

// random number generation
struct ranctx {
    uint64_t a;
    uint64_t b;
    uint64_t c;
    uint64_t d;
};

inline uint64_t rot(uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

uint64_t RKISS(ranctx& x) {
    uint64_t e = x.a - rot(x.b, 7);
    x.a = x.b ^ rot(x.c, 13);
    x.b = x.c + rot(x.d, 37);
    x.c = x.d + e;
    x.d = e + x.a;

    return x.d;
}

// hashing tables
uint64_t hash_table[2][6][64];
uint64_t check_hash_table[16];
uint64_t castle_rights_hash_table[16];
uint64_t enpass_square_hash_table[65];

void init_zobrist_table(uint64_t seed) {
    
    ranctx x;
    x.a = 0xf1ea5eed;
    x.b = x.c = x.d = seed;

    for (int i = 0; i < 30; i++)
        RKISS(x);

    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 6; j++)
            for (int k = 0; k < 64; k++)
                hash_table[i][j][k] = RKISS(x);

    for (int i = 0; i < 16; i++) {
        check_hash_table[i] = RKISS(x);
        castle_rights_hash_table[i] = RKISS(x);
    }
    
    for (int i = 0; i < 65; i++)
        enpass_square_hash_table[i] = RKISS(x);
}

uint64_t hash_state(const Boardstate& B) {
    uint64_t h = 0;

    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 6; j++)
            for (int k = 0; k < 64; k++)
                if (B.pieces[i][j] & (1ull << k))
                    h ^= hash_table[i][j][k];
    
    h ^= check_hash_table[B.flags.to_byte() >> 4];
    h ^= castle_rights_hash_table[B.flags.to_byte() & 0xf];
    h ^= enpass_square_hash_table[B.enpassant]; 
    return h;
}
