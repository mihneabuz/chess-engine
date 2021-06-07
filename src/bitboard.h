#ifndef _BITBOARD_H_
#define _BITBOARD_H_
#include <stdint.h>

typedef uint64_t bitboard;
typedef uint8_t square;
typedef uint8_t piece;
typedef uint8_t color;

enum {
  h1, g1, f1, e1, d1, c1, b1, a1,
  h2, g2, f2, e2, d2, c2, b2, a2,
  h3, g3, f3, e3, d3, c3, b3, a3,
  h4, g4, f4, e4, d4, c4, b4, a4,
  h5, g5, f5, e5, d5, c5, b5, a5,
  h6, g6, f6, e6, d6, c6, b6, a6,
  h7, g7, f7, e7, d7, c7, b7, a7,
  h8, g8, f8, e8, d8, c8, b8, a8, 
  no_sq
};

/* shift amounts:
  northwest    north   northeast
  noWe         nort         noEa
          +9    +8    +7
              \  |  /
  west    +1 <-  0 -> -1    east
              /  |  \
          -7    -8    -9
          
  soWe         sout         soEa
  southwest    south   southeast

(https://www.chessprogramming.org/General_Setwise_Operations#Shifting_Bitboards)
modified to new reversed file encoding
*/

// masks to avoid wrapping
constexpr bitboard notAFile = 0x7f7f7f7f7f7f7f7f;
constexpr bitboard notHFile = 0xfefefefefefefefe;

// shifting the board
inline bitboard northShiftOne(bitboard b) {
    return b << 8;
}
inline bitboard southShiftOne(bitboard b) {
    return b >> 8;
}
inline bitboard eastShiftOne(bitboard b) {
    return (b & notHFile) >> 1;
}
inline bitboard westShiftOne(bitboard b) {
    return (b & notAFile) << 1;
}
inline bitboard northeastShiftOne(bitboard b) {
    return (b & notHFile) << 7;
}
inline bitboard northwestShiftOne(bitboard b) {
    return (b & notAFile) << 9;
}
inline bitboard southeastShiftOne(bitboard b) {
    return (b & notHFile) >> 9;
}
inline bitboard southwestShiftOne(bitboard b) {
    return (b & notAFile) >> 7;
}

// index of least significant bit, using de Bruijn Sequences:
constexpr uint8_t debruijn_index64[64] = {
    0,  1, 48,  2, 57, 49, 28,  3,
   61, 58, 50, 42, 38, 29, 17,  4,
   62, 55, 59, 36, 53, 51, 43, 22,
   45, 39, 33, 30, 24, 18, 12,  5,
   63, 47, 56, 27, 60, 41, 37, 16,
   54, 35, 52, 21, 44, 32, 23, 11,
   46, 26, 40, 15, 34, 20, 31, 10,
   25, 14, 19,  9, 13,  8,  7,  6
};

constexpr bitboard debruijn64 = 0x03f79d71b4cb0a89ull;

inline square lsb(bitboard b) {
    return debruijn_index64[((b & -b) * debruijn64) >> 58];
}

inline square get_and_clear_lsb(bitboard& b) {
    bitboard result = debruijn_index64[((b & -b) * debruijn64) >> 58];
    b &= (b - 1);
    return result;
}

inline int count_bits(bitboard b) {
    int count = 0;
    while (b != 0) {
        get_and_clear_lsb(b);
        count ++;
    }
    return count;
}
#endif
