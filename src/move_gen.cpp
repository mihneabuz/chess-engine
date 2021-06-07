#include "move_gen.h"
#include "bitboard.h"
#include "boardstate.h"
#include "magics.h"
#include "move.h"
#include "algorithm"

typedef bitboard (*bitboard_func) (const bitboard b);
typedef int (*square_func) (const square to);

///////////////////////////////////////////////////////////
//                         Pawns                         //
///////////////////////////////////////////////////////////

// pre-calculated attack tables
bitboard pawn_attack_table[2][64];

inline bitboard get_white_pawn_single_pushes(const bitboard pawns) {
    return northShiftOne(pawns);
}

inline bitboard get_white_pawn_double_pushes(const bitboard pawns) {
    constexpr bitboard rank2 = 0xff00ull;
    return northShiftOne(northShiftOne(pawns & rank2));
}

inline bitboard get_black_pawn_single_pushes(const bitboard pawns) {
    return southShiftOne(pawns);
}

inline bitboard get_black_pawn_double_pushes(const bitboard pawns) {
    constexpr bitboard rank7 = 0xff000000000000ull;
    return southShiftOne(southShiftOne(pawns & rank7));
}

inline bitboard get_white_pawn_attacks(const bitboard pawns) {
    return northwestShiftOne(pawns) | northeastShiftOne(pawns);
}

inline bitboard get_black_pawn_attacks(const bitboard pawns) {
    return southwestShiftOne(pawns) | southeastShiftOne(pawns);
}

///////////////////////////////////////////////////////////
//                        Knights                        //
///////////////////////////////////////////////////////////

// pre-calculated attack tables
bitboard knight_attack_table[64];

/*
moves are added in clockwise order, starting from +15
___ +17 ___ +15 ___
+10 ___ ___ ___ +6
___ ___ _N_ ___ ___
-6  ___ ___ ___ -10
___ -15 ___ -17 ___
*/

constexpr bitboard knightmasks[] = {
    0xfefefefefefeull,
    0xfcfcfcfcfcfcfcull,
    0xfcfcfcfcfcfcfc00ull,
    0xfefefefefefe0000ull,
    0x7f7f7f7f7f7f0000ull,
    0x3f3f3f3f3f3f3f00ull,
    0x3f3f3f3f3f3f3full,
    0x7f7f7f7f7f7full
};

const bitboard_func knightmoves[] = {
    [] (const bitboard b) {return b << 15;},
    [] (const bitboard b) {return b <<  6;},
    [] (const bitboard b) {return b >> 10;},
    [] (const bitboard b) {return b >> 17;},
    [] (const bitboard b) {return b >> 15;},
    [] (const bitboard b) {return b >>  6;},
    [] (const bitboard b) {return b << 10;},
    [] (const bitboard b) {return b << 17;},
};

bitboard get_knight_attacks(const bitboard knights) {
    bitboard attacks = 0;
    for (int i = 0; i < 8; ++i)
        attacks |= knightmoves[i](knights & knightmasks[i]);
    return attacks;
}

///////////////////////////////////////////////////////////
//                         Kings                         //
///////////////////////////////////////////////////////////

// pre-calculated attack tables
bitboard king_attack_table[64];

bitboard get_king_attacks(const bitboard kings) {
    bitboard attacks = 0;
    attacks |= southShiftOne(kings);
    attacks |= southeastShiftOne(kings);
    attacks |= southwestShiftOne(kings);
    attacks |= eastShiftOne(kings);
    attacks |= westShiftOne(kings);
    attacks |= northShiftOne(kings);
    attacks |= northeastShiftOne(kings);
    attacks |= northwestShiftOne(kings);
    return attacks;
}

///////////////////////////////////////////////////////////
//                        Bishop                         //
///////////////////////////////////////////////////////////

// pre-calculated attack tables
bitboard bishop_attack_masks[64];
bitboard bishop_attack_table[64][512];

constexpr int bishop_relevant_bits[] = {
  6, 5, 5, 5, 5, 5, 5, 6, 
  5, 5, 5, 5, 5, 5, 5, 5, 
  5, 5, 7, 7, 7, 7, 5, 5, 
  5, 5, 7, 9, 9, 7, 5, 5, 
  5, 5, 7, 9, 9, 7, 5, 5, 
  5, 5, 7, 7, 7, 7, 5, 5, 
  5, 5, 5, 5, 5, 5, 5, 5, 
  6, 5, 5, 5, 5, 5, 5, 6
};

inline bitboard get_bishop_attacks(const square sq, const bitboard occupancy) {
    bitboard attacks = occupancy & bishop_attack_masks[sq];

    int index = (attacks * bishop_magics[sq]) >> (64 - bishop_relevant_bits[sq]);
    return bishop_attack_table[sq][index];
}

bitboard get_bishop_masks(const bitboard bishops) {
    square sq = lsb(bishops);

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

bitboard get_bishop_occup_masks(const bitboard bishops, const bitboard block) {
    square sq = lsb(bishops);

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

///////////////////////////////////////////////////////////
//                         Rook                          //
///////////////////////////////////////////////////////////

// pre-calculated attack tables
bitboard rook_attack_masks[64];
bitboard rook_attack_table[64][4096];

constexpr int rook_relevant_bits[] = {
  12, 11, 11, 11, 11, 11, 11, 12, 
  11, 10, 10, 10, 10, 10, 10, 11, 
  11, 10, 10, 10, 10, 10, 10, 11, 
  11, 10, 10, 10, 10, 10, 10, 11, 
  11, 10, 10, 10, 10, 10, 10, 11, 
  11, 10, 10, 10, 10, 10, 10, 11, 
  11, 10, 10, 10, 10, 10, 10, 11, 
  12, 11, 11, 11, 11, 11, 11, 12
};

inline bitboard get_rook_attacks(const square sq, const bitboard occupancy) {
    bitboard attacks = occupancy & rook_attack_masks[sq];

    int index = (attacks * rook_magics[sq]) >> (64 - rook_relevant_bits[sq]);
    return rook_attack_table[sq][index];
}

bitboard get_rook_masks(const bitboard rooks) {
    square sq = lsb(rooks);

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

bitboard get_rook_occup_masks(const bitboard rooks, const bitboard block) {
    square sq = lsb(rooks);

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

///////////////////////////////////////////////////////////
//                         Queen                         //
///////////////////////////////////////////////////////////

// Queen is just rook + bishop
inline bitboard get_queen_attacks(const square sq, const bitboard occupancy) {
    return get_bishop_attacks(sq, occupancy) | get_rook_attacks(sq, occupancy);
}

///////////////////////////////////////////////////////////

bitboard gen_occupancy(const int index, bitboard attacks);

void init_move_tables() {
    for (square i = 0; i < 64; i++) {
        bitboard piece = 1ull << i;

        ///////////////////////
        //   leaper pieces   //
        ///////////////////////

        // pawn tables
        pawn_attack_table[WHITE][i] = get_white_pawn_attacks(piece);
        pawn_attack_table[BLACK][i] = get_black_pawn_attacks(piece);

        // knight table
        knight_attack_table[i] = get_knight_attacks(piece);

        // king table
        king_attack_table[i] = get_king_attacks(piece);

        
        ////////////////////////
        //   sliding pieces   //
        ////////////////////////
        int max_index;

        // rook tables
        bitboard rook_attacks = get_rook_masks(piece);
        rook_attack_masks[i] = rook_attacks;

        max_index = 1 << count_bits(rook_attacks);
        for (int index = 0; index < max_index; index++) {
            bitboard occupancy = gen_occupancy(index, rook_attacks);
            int magic_index = (occupancy * rook_magics[i]) >> (64 - rook_relevant_bits[i]);

            rook_attack_table[i][magic_index] = get_rook_occup_masks(piece, occupancy);
        }

        // bishops tables
        bitboard bishop_attacks = get_bishop_masks(piece);
        bishop_attack_masks[i] = bishop_attacks;

        max_index = 1 << count_bits(bishop_attacks);
        for (int index = 0; index < max_index; index++) {
            bitboard occupancy = gen_occupancy(index, bishop_attacks);
            int magic_index = (occupancy * bishop_magics[i]) >> (64 - bishop_relevant_bits[i]);

            bishop_attack_table[i][magic_index] = get_bishop_occup_masks(piece, occupancy);
        }
    }
}

///////////////////////////////////////////////////////////
//           @ <= Performance Critical => @              //
///////////////////////////////////////////////////////////

const square_func pawn_promotion[] = {
    [] (const square to) {return QUEEN * (to >= 56);},
    [] (const square to) {return QUEEN * (to <= 7);}
};

const square_func pawn_push[] = {
    [] (const square to) {return to + 8;},
    [] (const square to) {return to - 8;}
};

square_func is_starting_rook_poz[] = {
    [] (const square poz) {return static_cast<int>(poz == a1 || poz == h1);},
    [] (const square poz) {return static_cast<int>(poz == a8 || poz == h8);}
};

constexpr bitboard pawn_double_push_mask[] = {
    0xff000000ull,
    0xff00000000ull
};
 
constexpr square king_start_poz_square[] = {
    e1, e8
};

constexpr bitboard rook_start_king_side_mask[] = {
    0x1ull,
    0x100000000000000ull
};

constexpr bitboard rook_start_queen_side_mask[] = {
    0x80ull,
    0x8000000000000000ull
};

constexpr bitboard king_side_castle_blocking[] = {
    0x6ull,
    0x600000000000000ull
};

constexpr bitboard queen_side_castle_blocking[] = {
    0x70ull,
    0x7000000000000000ull
};

inline int king_check(const bitboard attacks, const Boardstate& B) {
    return (attacks & B.pieces[1 - B.to_move][KING]) != 0;
}

///////////////////////////////////////////////////////////
//           @ <= Performance Critical => @              //
///////////////////////////////////////////////////////////

void generate_all_moves(const Boardstate& B, move_list& moves) {
    // push all possible pseudo-legal moves in moves list

    bitboard pieces;
    square to;
    square from;
    bitboard attacks, captures;

    ////////////////////////
    //        pawns       //
    ////////////////////////
    pieces = B.pieces[B.to_move][PAWN];

    while (pieces) {
        from = get_and_clear_lsb(pieces);

        // generate pawn captures
        attacks = pawn_attack_table[B.to_move][from];

        // check for enpassant capture
        if (attacks & (1ull << B.enpassant))
            moves.captures.push(encode(from, B.enpassant, PAWN, PAWN, CAPTURE | ENPASSANT));

        // check for other captures
        for (piece p = PAWN; p < KING; p++) {
            captures = attacks & B.pieces[1 - B.to_move][p];
            while (captures) {
                to = get_and_clear_lsb(captures);
                moves.captures.push(encode(from, to, PAWN, pawn_promotion[B.to_move](to), CAPTURE, capture_score_table[PAWN][p]));
            }
        }

        // generate pawn single pushes
        to = pawn_push[B.to_move](from);

        if (1ull << to & ~B.board) {
            moves.quiet.push(encode(from, to, PAWN, pawn_promotion[B.to_move](to), NO_FLAGS));

            // generate pawn double pushes
            to = pawn_push[B.to_move](to);

            if (1ull << to & ~B.board & pawn_double_push_mask[B.to_move]) {
                // this ugly
                bool enpas = pawn_attack_table[B.to_move][pawn_push[1 - B.to_move](to)] & 
                             B.occupancies[1 - B.to_move];
                moves.quiet.push(encode(from, to, PAWN, PAWN, ENPASSANT * enpas));
            }
        }
    }

    ////////////////////////
    //       knights      //
    ////////////////////////
    pieces = B.pieces[B.to_move][KNIGHT];

    while (pieces) {
        from = get_and_clear_lsb(pieces);
 
        // generate knight captures
        attacks = knight_attack_table[from];
        for (piece p = PAWN; p < KING; p++) {
            captures = attacks & B.pieces[1 - B.to_move][p];
            while (captures) {
                to = get_and_clear_lsb(captures);
                moves.captures.push(encode(from, to, KNIGHT, KNIGHT, CAPTURE, capture_score_table[KNIGHT][p]));
            }
        }
 
        // generate knight attacks
        attacks = knight_attack_table[from] & ~B.board;

        while (attacks) {
            to = get_and_clear_lsb(attacks);
            moves.quiet.push(encode(from, to, KNIGHT, KNIGHT, NO_FLAGS,
                                    king_check(knight_attack_table[to], B)));
        }
    }

    ////////////////////////
    //        kings       //
    ////////////////////////
    pieces = B.pieces[B.to_move][KING];

    from = lsb(pieces);

    int uncastle = UNCASTLE * (from == king_start_poz_square[B.to_move]);

    // generate castles
    if (from == king_start_poz_square[B.to_move]) {
        // Castling moves:	e1g1, e1c1, e8g8, e8c8
      
        // king side castle
        if (!B.flags.test(2 * B.to_move + 1) && // no castle flag
            !(B.board & king_side_castle_blocking[B.to_move]) && // no blocking pieces
            B.pieces[B.to_move][ROOK] & rook_start_king_side_mask[B.to_move] && // rook poz
            !is_attacked(B, from) &&   // king is not in check
            !is_attacked(B, from - 1)) // moving square is not attacked

            moves.quiet.push(encode(from, from - 2, KING, KING, CASTLE, 1));

        // queen side castle
        if (!B.flags.test(2 * B.to_move) && // no castle flag
            !(B.board & queen_side_castle_blocking[B.to_move]) && // no blocking pieces
            B.pieces[B.to_move][ROOK] & rook_start_queen_side_mask[B.to_move] && // rook poz
            !is_attacked(B, from) &&   // king is not in check
            !is_attacked(B, from + 1)) // moving square is not attacked

            moves.quiet.push(encode(from, from + 2, KING, KING, CASTLE, 1));
    }

    // generate king captures
    attacks = king_attack_table[from];
    for (piece p = PAWN; p < KING; p++) {
        captures = attacks & B.pieces[1 - B.to_move][p];
        while (captures) {
            to = get_and_clear_lsb(captures);
            moves.captures.push(encode(from, to, KING, KING, CAPTURE, capture_score_table[KING][p]));
        }
    }
    
    // generate king attacks
    attacks = king_attack_table[from] & ~B.board;

    while (attacks) {
        to = get_and_clear_lsb(attacks);
        moves.quiet.push(encode(from, to, KING, KING, uncastle));
    }

    ////////////////////////
    //       bishops      //
    ////////////////////////
    pieces = B.pieces[B.to_move][BISHOP];

    while (pieces) {
        from = get_and_clear_lsb(pieces);

        // generate bishop captures
        attacks = get_bishop_attacks(from, B.board);
        for (piece p = PAWN; p < KING; p++) {
            captures = attacks & B.pieces[1 - B.to_move][p];
            while (captures) {
                to = get_and_clear_lsb(captures);
                moves.captures.push(encode(from, to, BISHOP, BISHOP, CAPTURE, capture_score_table[KING][p]));
            }
        }

        // generate bishop attacks
        attacks &= ~B.board;
        while (attacks) {
            to = get_and_clear_lsb(attacks);
            moves.quiet.push(encode(from, to, BISHOP, BISHOP, NO_FLAGS,
                                    king_check(get_bishop_attacks(to, B.board), B)));
        }
    }

    ////////////////////////
    //       rooks        //
    ////////////////////////
    pieces = B.pieces[B.to_move][ROOK];

    while (pieces) {
        from = get_and_clear_lsb(pieces);

        int uncastle = UNCASTLE * is_starting_rook_poz[B.to_move](from);

        // generate rook captures
        attacks = get_rook_attacks(from, B.board);
        for (piece p = PAWN; p < KING; p++) {
            captures = attacks & B.pieces[1 - B.to_move][p];
            while (captures) {
                to = get_and_clear_lsb(captures);
                moves.captures.push(encode(from, to, ROOK, ROOK, CAPTURE, capture_score_table[KING][p]));
            }
        }

        // generate rook attacks
        attacks &= ~B.board;
        while (attacks) {
            to = get_and_clear_lsb(attacks);
            moves.quiet.push(encode(from, to, ROOK, ROOK, uncastle,
                                    king_check(get_rook_attacks(to, B.board), B)));
        }
    }
    
    ////////////////////////
    //       queens       //
    ////////////////////////
    pieces = B.pieces[B.to_move][QUEEN];

    while (pieces) {
        from = get_and_clear_lsb(pieces);

        // generate queen captures
        attacks = get_queen_attacks(from, B.board);
        for (piece p = PAWN; p < KING; p++) {
            captures = attacks & B.pieces[1 - B.to_move][p];
            while (captures) {
                to = get_and_clear_lsb(captures);
                moves.captures.push(encode(from, to, QUEEN, QUEEN, CAPTURE, capture_score_table[KING][p]));
            }
        }

        // generate queen attacks
        attacks &= ~B.board;
        while (attacks) {
            to = get_and_clear_lsb(attacks);
            moves.quiet.push(encode(from, to, QUEEN, QUEEN, NO_FLAGS, 
                                    king_check(get_queen_attacks(to, B.board), B)));
        }
    }
    
    std::sort(moves.captures.begin(), moves.captures.end(), compare_scores);
    std::sort(moves.quiet.begin(), moves.quiet.end(), compare_scores);
}

///////////////////////////////////////////////////////////
//           @ <= Performance Critical => @              //
///////////////////////////////////////////////////////////

void generate_capture_moves(const Boardstate& B, move_array<64>& moves) {
    bitboard pieces;
    square to;
    square from;
    bitboard attacks, captures;

    ////////////////////////
    //        pawns       //
    ////////////////////////
    pieces = B.pieces[B.to_move][PAWN];

    while (pieces) {
        from = get_and_clear_lsb(pieces);

        // generate pawn captures
        attacks = pawn_attack_table[B.to_move][from];

        // check for enpassant capture
        if (attacks & (1ull << B.enpassant))
            moves.push(encode(from, B.enpassant, PAWN, PAWN, CAPTURE | ENPASSANT));

        // check for other captures
        for (piece p = PAWN; p < KING; p++) {
            captures = attacks & B.pieces[1 - B.to_move][p];
            while (captures) {
                to = get_and_clear_lsb(captures);
                moves.push(encode(from, to, PAWN, pawn_promotion[B.to_move](to), CAPTURE, capture_score_table[PAWN][p]));
            }
        }
    }

    ////////////////////////
    //       knights      //
    ////////////////////////
    pieces = B.pieces[B.to_move][KNIGHT];

    while (pieces) {
        from = get_and_clear_lsb(pieces);
 
        // generate knight captures
        attacks = knight_attack_table[from];
        for (piece p = PAWN; p < KING; p++) {
            captures = attacks & B.pieces[1 - B.to_move][p];
            while (captures) {
                to = get_and_clear_lsb(captures);
                moves.push(encode(from, to, KNIGHT, KNIGHT, CAPTURE, capture_score_table[KNIGHT][p]));
            }
        }
    }

    ////////////////////////
    //        kings       //
    ////////////////////////
    pieces = B.pieces[B.to_move][KING];

    from = lsb(pieces);

    // generate king captures
    attacks = king_attack_table[from];
    for (piece p = PAWN; p < KING; p++) {
        captures = attacks & B.pieces[1 - B.to_move][p];
        while (captures) {
            to = get_and_clear_lsb(captures);
            moves.push(encode(from, to, KING, KING, CAPTURE, capture_score_table[KING][p]));
        }
    }
    
    ////////////////////////
    //       bishops      //
    ////////////////////////
    pieces = B.pieces[B.to_move][BISHOP];

    while (pieces) {
        from = get_and_clear_lsb(pieces);

        // generate bishop captures
        attacks = get_bishop_attacks(from, B.board);
        for (piece p = PAWN; p < KING; p++) {
            captures = attacks & B.pieces[1 - B.to_move][p];
            while (captures) {
                to = get_and_clear_lsb(captures);
                moves.push(encode(from, to, BISHOP, BISHOP, CAPTURE, capture_score_table[KING][p]));
            }
        }
    }

    ////////////////////////
    //       rooks        //
    ////////////////////////
    pieces = B.pieces[B.to_move][ROOK];

    while (pieces) {
        from = get_and_clear_lsb(pieces);

        // generate rook captures
        attacks = get_rook_attacks(from, B.board);
        for (piece p = PAWN; p < KING; p++) {
            captures = attacks & B.pieces[1 - B.to_move][p];
            while (captures) {
                to = get_and_clear_lsb(captures);
                moves.push(encode(from, to, ROOK, ROOK, CAPTURE, capture_score_table[KING][p]));
            }
        }
    }
    
    ////////////////////////
    //       queens       //
    ////////////////////////
    pieces = B.pieces[B.to_move][QUEEN];

    while (pieces) {
        from = get_and_clear_lsb(pieces);

        // generate queen captures
        attacks = get_queen_attacks(from, B.board);
        for (piece p = PAWN; p < KING; p++) {
            captures = attacks & B.pieces[1 - B.to_move][p];
            while (captures) {
                to = get_and_clear_lsb(captures);
                moves.push(encode(from, to, QUEEN, QUEEN, CAPTURE, capture_score_table[KING][p]));
            }
        }
    }
    
    std::sort(moves.begin(), moves.end(), compare_scores);
}

///////////////////////////////////////////////////////////
//           @ <= Performance Critical => @              //
///////////////////////////////////////////////////////////

bool is_attacked(const Boardstate& B, const square poz) {
    // check is square is currently under attack
    int side = 1 - B.to_move;

    bitboard attacks = 0;

    attacks |= pawn_attack_table[B.to_move][poz] & B.pieces[side][PAWN];
    attacks |= knight_attack_table[poz] & B.pieces[side][KNIGHT];
    attacks |= king_attack_table[poz] & B.pieces[side][KING];
    
    bitboard bishops = get_bishop_attacks(poz, B.board);
    attacks |= bishops & B.pieces[side][BISHOP];

    bitboard rooks = get_rook_attacks(poz, B.board);
    attacks |= rooks & B.pieces[side][ROOK];

    attacks |= (bishops | rooks) & B.pieces[side][QUEEN];

    return attacks;
}

///////////////////////////////////////////////////////////

// helper function for slinding pieces pre-calculated tables
bitboard gen_occupancy(const int index, bitboard attacks) {
    int count = count_bits(attacks);
    bitboard occupancy = 0ull;

    for (int i = 0; i < count; i++) {
        square sq = get_and_clear_lsb(attacks);

        if (index & (1 << i))
            occupancy |= (1ull << sq);
    }
    return occupancy;
}

// for debugging
bitboard test_attack_tables(piece p, color c, square poz, bitboard occupancy) {
    switch (p) {
        case PAWN:   return pawn_attack_table[c][poz];
        case KNIGHT: return knight_attack_table[poz]; 
        case KING:   return king_attack_table[poz]; 
        case BISHOP: return get_bishop_attacks(poz, occupancy);
        case ROOK:   return get_rook_attacks(poz, occupancy);
        case QUEEN:  return get_queen_attacks(poz, occupancy);
    }
    return 0;
}
///////////////////////////////////////////////////////////
