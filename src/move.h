#ifndef _MOVE_H_
#define _MOVE_H_

#include "bitboard.h"
#include <bits/stdint-uintn.h>

typedef uint32_t Move;

/*
    Move -> encoded in uint32 (4 bytes)
    
    Source square       ->   first byte
    Destination square  ->   second byte
    Piece               ->   first 4 bits of 3rd byte
    Promoted piece      ->   second 4 bits of 3rd byte
    Flags               ->   first 4 bits of last byte
        (CAPTURE, CASTLE, ENPASSANT, UNCASTLE)
    Score               ->   last 4 bits of last byte
        (used for move ordering)
    
                    Binary                    Field             Mask

    0000 0000 0000 0000 0000 0000 1111 1111   src               0xff
    0000 0000 0000 0000 1111 1111 0000 0000   dest              0xff00
    0000 0000 0000 1111 0000 0000 0000 0000   piece             0xf0000
    0000 0000 1111 0000 0000 0000 0000 0000   promoted          0xf00000
    0000 0001 0000 0000 0000 0000 0000 0000   capture flag      0x1000000
    0000 0010 0000 0000 0000 0000 0000 0000   castle flag       0x2000000
    0000 0100 0000 0000 0000 0000 0000 0000   enpassant flag    0x4000000
    0000 1000 0000 0000 0000 0000 0000 0000   uncastle flag     0x8000000
    1111 0000 0000 0000 0000 0000 0000 0000   score             0xf0000000

*/

enum {
    // Flag overlap: CAPTURE can also be ENPASSANT or UNCASTLE
    NO_FLAGS = 0,       // Quiet move
    CAPTURE = 1,        // Capture move, remove opponent piece
    CASTLE = 2,         // Castleing move, also move relevant rook
    ENPASSANT = 4,      // En Passant move, set or capture en passant square
    UNCASTLE = 8        // Just remove castle permisions
};

// table for looking up value of capture
constexpr int capture_score_table[6][5] = {
    // pawn takes
    {
        4, // pawn
        5, // bishop
        6, // knight 
        7, // rook 
        8, // queen 
    },
    // bishop takes
    {
        3, // pawn
        4, // bishop
        5, // knight 
        6, // rook 
        7, // queen 
    },
    // knight takes
    {
        2, // pawn
        3, // bishop
        4, // knight 
        5, // rook 
        6, // queen 
    },
    // rook takes
    {
        1, // pawn
        2, // bishop
        3, // knight 
        4, // rook 
        5, // queen 
    },
    // queen takes
    {
        0, // pawn
        1, // bishop
        2, // knight 
        3, // rook 
        4, // queen 
    },
    // king takes
    {
        0, // pawn
        0, // bishop
        0, // knight 
        1, // rook 
        2, // queen 
    }
};

inline Move encode(square src, square dest, piece p, piece promoted, uint8_t flags) {
    return 0u | src | (dest << 8) | (p << 16) | (promoted << 20) | (flags << 24);
}

inline Move encode(square src, square dest, piece p, piece promoted, uint8_t flags, int score) {
    return 0u |
        src | (dest << 8) | (p << 16) | (promoted << 20) | (flags << 24) | (score << 28);
}

inline square get_src(Move m) {
    return m & 0xff;
}

inline square get_dest(Move m) {
    return (m & 0xff00) >> 8;
}

inline piece get_piece(Move m) {
    return (m & 0xf0000) >> 16;
}

inline piece get_promoted(Move m) {
    return (m & 0xf00000) >> 20;
}

inline uint8_t get_flags(Move m) {
    return (m & 0xf000000) >> 24;
}

inline uint8_t get_score(Move m) {
    return (m & 0xf0000000) >> 28;
}

inline bool compare_scores(Move x, Move y) {
    return get_score(x) > get_score(y);
}

#endif
