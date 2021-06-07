#ifndef _MOVE_GEN_H_
#define _MOVE_GEN_H_

#include "bitboard.h"
#include "move.h"
#include "boardstate.h"

template<int T>
struct move_array {
    std::array<Move, T> arr;
    uint8_t count;

    // base constructor
    move_array(): count(0) {};

    // pushes a move to the list
    inline void push(Move m) {
        arr[count++] = m;
    }

    // iterators for range for loops
    inline Move* begin() {
        return arr.begin();
    }

    inline Move* end() {
        return arr.begin() + count;
    }
};

struct move_list {
    move_array<128> captures;
    move_array<128> quiet;
};

void init_move_tables();
void generate_all_moves(const Boardstate& B, move_list& moves);
void generate_capture_moves(const Boardstate& B, move_array<64>& moves);
bool is_attacked(const Boardstate& B, const square poz);

// for debugging
bitboard test_attack_tables(piece p, color c, square poz, bitboard occupancy);
#endif
