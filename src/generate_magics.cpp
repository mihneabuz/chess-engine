#include <iostream>
#include "bitboard.h"

///////////////////////////////////////////////////////////
//            * This stuff is actual magic *             //
//  https://www.chessprogramming.org/Looking_for_Magics  //
///////////////////////////////////////////////////////////

enum {BISHOP, ROOK};

void printBitboard(bitboard b)
{
  bitboard mask = 0x8000000000000000; // square a8
  for (int i = 7; i >= 0; --i)
  {
    for (int j = 0; j < 8; ++j)
    {
      std::cout << (b & mask ? "1 " : "0 "); mask >>= 1;
    }
    std::cout << '\n'; 
  }
}

// xor rand generator for consistent results
static bitboard seed = 8714580285;
bitboard get_random() {
    seed ^= seed << 13;
    seed ^= seed >> 7;
    seed ^= seed << 5;

    return seed;
}

// generate random number with low number of set bits (1s)
bitboard generate_magic_candidate() {
    return get_random() & get_random() & get_random();
}

bitboard get_all_bishop_attacks(square sq) {
    bitboard attacks = 0;

    int r, f;
    int tr = sq / 8;
    int tf = sq % 8;

    for (r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++)
        attacks |= (1ull << (r * 8 + f));

    for (r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++)
        attacks |= (1ull << (r * 8 + f));

    for (r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--)
        attacks |= (1ull << (r * 8 + f));

    for (r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--)
        attacks |= (1ull << (r * 8 + f));

    return attacks;
}

bitboard get_occup_bishop_attacks(square sq, const bitboard block) {
    bitboard attacks = 0;

    int r, f;
    int tr = sq / 8;
    int tf = sq % 8;

    for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++) {
        attacks |= (1ull << (r * 8 + f));
        if (block & (1ull << (r * 8 + f)))
            break;
    }

    for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++) {
        attacks |= (1ull << (r * 8 + f));
        if (block & (1ull << (r * 8 + f)))
            break;
    }

    for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--) {
        attacks |= (1ull << (r * 8 + f));
        if (block & (1ull << (r * 8 + f)))
            break;
    }

    for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--) {
        attacks |= (1ull << (r * 8 + f));
        if (block & (1ull << (r * 8 + f)))
            break;
    }

    return attacks;
}

bitboard get_all_rook_attacks(square sq) {
    bitboard attacks = 0;

    int r, f;
    int tr = sq / 8;
    int tf = sq % 8;

    for (r = tr + 1, f = tf; r <= 6; r++)
        attacks |= (1ull << (r * 8 + f));
    
    for (r = tr - 1, f = tf; r >= 1; r--)
        attacks |= (1ull << (r * 8 + f));

    for (r = tr, f = tf + 1; f <= 6; f++)
        attacks |= (1ull << (r * 8 + f));
    
    for (r = tr, f = tf - 1; f >= 1; f--)
        attacks |= (1ull << (r * 8 + f));
    
    return attacks;
}

bitboard get_occup_rook_attacks(square sq, const bitboard block) {
    bitboard attacks = 0;

    int r, f;
    int tr = sq / 8;
    int tf = sq % 8;

    for (r = tr + 1, f = tf; r <= 7; r++) {
        attacks |= (1ull << (r * 8 + f));
        if (block & (1ull << (r * 8 + f)))
            break;
    }
    
    for (r = tr - 1, f = tf; r >= 0; r--) {
        attacks |= (1ull << (r * 8 + f));
        if (block & (1ull << (r * 8 + f)))
            break;
    }

    for (r = tr, f = tf + 1; f <= 7; f++) {
        attacks |= (1ull << (r * 8 + f));
        if (block & (1ull << (r * 8 + f)))
            break;
    }
    
    for (r = tr, f = tf - 1; f >= 0; f--) {
        attacks |= (1ull << (r * 8 + f));
        if (block & (1ull << (r * 8 + f)))
            break;
    }
    
    return attacks;
}

// generate occupancy based on index
bitboard gen_occupancy(int index, bitboard attacks) {
    int count = count_bits(attacks);

    bitboard occupancy = 0ull;

    for (int i = 0; i < count; i++) {
        square sq = get_and_clear_lsb(attacks);

        if (index & (1 << i))
            occupancy |= (1ull << sq);
    }

    return occupancy;
}

bitboard find_magic_bitboard(piece p, square sq) {
    
    // all possible occupancies
    bitboard occupancies[4096];
    
    // real attacks generated on the fly
    bitboard real_attacks[4096];

    // get attack bitboard
    bitboard attack = (p == BISHOP ? get_all_bishop_attacks(sq) :
                                       get_all_rook_attacks(sq));

    // find number of relevant occupancy bits
    int bits = count_bits(attack);
    int max_index = 1 << bits;

    // fill occupancy and real attacks arrays
    for (int index = 0; index < max_index; index++) {
        occupancies[index] = gen_occupancy(index, attack);

        real_attacks[index] = (p == BISHOP ?
            get_occup_bishop_attacks(sq, occupancies[index]) :
              get_occup_rook_attacks(sq, occupancies[index]));
    }

    // Monte Carlo search for magic numbers
    for (int i = 0; i < 10000000; i++) {
      
        // generate candidate
        bitboard magic = generate_magic_candidate();

        // skip bad candidates
        if (count_bits((attack * magic) & 0xff00000000000000ull) < 6) continue;

        // init magic attacks to 0
        bitboard magic_attacks[4096] = {0};

        bool failed = false;
        for (int index = 0; index < max_index && !failed; index++) {
            int magic_index = (occupancies[index] * magic) >> (64 - bits);

            if (magic_attacks[magic_index] == 0)
                magic_attacks[magic_index] = real_attacks[index];

            else if (magic_attacks[magic_index] != real_attacks[index])
                failed = true;
        }

        // we found it!
        if (!failed)
          return magic;
    }
    return 0;
}

int main() {

    // print findings to stdout

    std::cout << "const bitboard bishop_magics[] = {\n";
    for (square x = 0; x < 64; x++)
        std::cout << find_magic_bitboard(BISHOP, x) << "ull,\n";
    std::cout << "}\n\n";

    std::cout << "const bitboard rook_magics[] = {\n";
    for (square x = 0; x < 64; x++)
        std::cout << find_magic_bitboard(ROOK, x) << "ull,\n";
    std::cout << "}\n\n";

    return 0;
}
