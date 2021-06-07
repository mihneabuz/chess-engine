#include "boardstate.h"
#include "bitboard.h"
#include "move_gen.h"
#include "search.h"
#include "evaluate.h"
#include "transpositions.h"
#include "zobrist.h"
#include "logger.h"
#include <chrono>
#include <string>
#include <unordered_map>

///////////////////////////////////////////////////////////
//                     Constructors                      //
///////////////////////////////////////////////////////////

Boardstate::Boardstate() {
    to_move = WHITE;
    flags = 0;
    enpassant = no_sq;

    board = 0;
    occupancies = {0, 0};
    pieces[WHITE] = {0, 0, 0, 0, 0, 0};
    pieces[BLACK] = {0, 0, 0, 0, 0, 0};

    hash = 0;

    endgame = 0;
    midgame = 0;
    gamestage = 0;

    no_capture_count = 0;
}

Boardstate::Boardstate(const Boardstate& c):
    to_move(c.to_move), board(c.board), pieces(c.pieces),
    occupancies(c.occupancies), flags(c.flags), enpassant(c.enpassant),
    midgame(c.midgame), endgame(c.endgame), gamestage(c.gamestage),
    no_capture_count(c.no_capture_count), hash(c.hash) {}


///////////////////////////////////////////////////////////
//           Make-move and helper functions              //
//           @ <= Performance Critical => @              //
///////////////////////////////////////////////////////////

inline void Boardstate::swap_to_move() noexcept {
    to_move = (1 - to_move);
}

inline void Boardstate::set_piece(const piece p, const color c, const square i) noexcept {
    bitboard b = 1ull << i;

    board          |= b;
    occupancies[c] |= b;
    pieces[c][p]   |= b;
}

inline void Boardstate::pop_piece(const piece p, const color c, const square i) noexcept {
    bitboard b = ~(1ull << i);

    board          &= b;
    occupancies[c] &= b;
    pieces[c][p]   &= b;
}

// rook position maps for castling
const std::unordered_map<square, square> castle_rook_begin {
    {g1, h1}, {c1, a1}, {g8, h8}, {c8, a8}
};

const std::unordered_map<square, square> castle_rook_end {
    {g1, f1}, {c1, d1}, {g8, f8}, {c8, d8}
};

const std::unordered_map<square, int> castle_id {
    {a1, 0}, {h1, 1}, {a8, 2}, {h8, 3}
};

// offsets for calculating enpassant square
constexpr int enpassant_offset[] = {-8, 8};

bool Boardstate::make_move(const Move m) noexcept {
    // parse move information
    square src = get_src(m);
    square dest = get_dest(m);
    piece p = get_piece(m);
    piece promotion = get_promoted(m);
    uint8_t move_flags = get_flags(m);

    // clear from bit
    pop_piece(p, to_move, src);
    midgame -= midgame_value_map[to_move][p][src];
    endgame -= endgame_value_map[to_move][p][src];
    //hash ^= hash_table[to_move][p][src];

    // set to bit
    set_piece(promotion, to_move, dest);
    midgame += midgame_value_map[to_move][promotion][dest];
    endgame += endgame_value_map[to_move][promotion][dest];
    //hash ^= hash_table[to_move][promotion][dest];

    no_capture_count += 1;
    //hash ^= enpass_square_hash_table[enpassant];

    // if capture -> clear other bitboards
    if (move_flags & CAPTURE) {
        no_capture_count = 0;

        bitboard b;

        // if en passant capture
        if (move_flags & ENPASSANT) {
            b = 1ull << (dest + enpassant_offset[to_move]);
            board ^= b;
            occupancies[1 - to_move] ^= b;
            pieces[1 - to_move][PAWN] ^= b;

            midgame -= midgame_value_map[1 - to_move][PAWN][dest + enpassant_offset[to_move]];
            endgame -= endgame_value_map[1 - to_move][PAWN][dest + enpassant_offset[to_move]];
            //hash ^= hash_table[1 - to_move][PAWN][dest + enpassant_offset[to_move]];

            gamestage += gamestage_value_map[PAWN];
        }
        else {
            b = 1ull << dest;
            occupancies[1 - to_move] ^= b;

            for (piece p = PAWN; p <= QUEEN; p++) {
                if (pieces[1 - to_move][p] & b) {
                    pieces[1 - to_move][p] ^= b;

                    midgame -= midgame_value_map[1 - to_move][p][dest];
                    endgame -= endgame_value_map[1 - to_move][p][dest];
                    //hash ^= hash_table[1 - to_move][p][dest];

                    gamestage += gamestage_value_map[p];
                    break;
                }
            }
        }
        enpassant = no_sq;

        if (move_flags & UNCASTLE) {
            //hash ^= castle_rights_hash_table[flags.to_byte() & 0xf];
            if (p == KING) {
                flags.set(2 * to_move);
                flags.set(2 * to_move + 1);
            }
            else
                flags.set(castle_id.at(src));
            //hash ^= castle_rights_hash_table[flags.to_byte() & 0xf];
        }
    }

    // if castle -> move rook and set flags
    else if (move_flags & CASTLE) {
        square from = castle_rook_begin.at(dest);
        pop_piece(ROOK, to_move, from);
        midgame -= midgame_value_map[to_move][ROOK][from];
        endgame -= endgame_value_map[to_move][ROOK][from];
        //hash ^= hash_table[to_move][ROOK][from];

        square to = castle_rook_end.at(dest);
        set_piece(ROOK, to_move, to);
        midgame += midgame_value_map[to_move][ROOK][to];
        endgame += endgame_value_map[to_move][ROOK][to];
        //hash ^= hash_table[to_move][ROOK][to];

        enpassant = no_sq;

        //hash ^= castle_rights_hash_table[flags.to_byte() & 0xf];
        flags.set(2 * to_move);
        flags.set(2 * to_move + 1);
        //hash ^= castle_rights_hash_table[flags.to_byte() & 0xf];
    }

    // if uncastle -> remove castle permisions
    else if (move_flags & UNCASTLE) {
        //hash ^= castle_rights_hash_table[flags.to_byte() & 0xf];
        if (p == KING) {
            flags.set(2 * to_move);
            flags.set(2 * to_move + 1);
        }
        else
            flags.set(castle_id.at(src));
        //hash ^= castle_rights_hash_table[flags.to_byte() & 0xf];
    }

    // if en passant -> set en passant square
    else if (move_flags & ENPASSANT)
        enpassant = dest + enpassant_offset[to_move];
    else
        enpassant = no_sq;
    
    //hash ^= enpass_square_hash_table[enpassant];

    // check if move was legal
    if (is_attacked(*this, lsb(pieces[to_move][KING])))
        return false;

    swap_to_move();

    // check if enemy king is in check
    if (is_attacked(*this, lsb(pieces[to_move][KING]))) {
        //hash ^= check_hash_table[flags.to_byte() >> 4];
        flags.add(6 - 2 * to_move);
        //hash ^= check_hash_table[flags.to_byte() >> 4];
    }

    return true;
}

///////////////////////////////////////////////////////////
//           @ <= Performance Critical => @              //
///////////////////////////////////////////////////////////

int status_map[] = {
          //  W    B
     0,   // 0 0  0 0
     0,   // 1 0  0 0
     0,   // 0 1  0 0
     1,   // 1 1  0 0 -> white wins
     0,   // 0 0  1 0
     0,   // 1 0  1 0
     0,   // 0 1  1 0
     1,   // 1 1  1 0 -> white wins
     0,   // 0 0  0 1
     0,   // 1 0  0 1
     0,   // 0 1  0 1
     1,   // 1 1  0 1 -> white wins
     2,   // 0 0  1 1 -> black wins
     2,   // 1 0  1 1 -> black wins
     2,   // 0 1  1 1 -> black wins
     0    // 1 1  1 1
};

int Boardstate::get_result() const {
    return status_map[flags.to_byte() >> 4];
}

///////////////////////////////////////////////////////////
//           Reset to initial chess position             //
///////////////////////////////////////////////////////////

void Boardstate::reset() {
    to_move = WHITE;
    flags = 0;

    board = 0;
    occupancies = {0, 0};
    pieces[WHITE] = {0, 0, 0, 0, 0, 0};
    pieces[BLACK] = {0, 0, 0, 0, 0, 0};

    endgame = 0;
    midgame = 0;
    gamestage = 0;

    no_capture_count = 0;

    // setting white pieces
    set_piece(ROOK, WHITE, a1);
    set_piece(ROOK, WHITE, h1);

    set_piece(KNIGHT, WHITE, b1);
    set_piece(KNIGHT, WHITE, g1);

    set_piece(BISHOP, WHITE, c1);
    set_piece(BISHOP, WHITE, f1);

    set_piece(KING, WHITE, e1);
    set_piece(QUEEN, WHITE, d1);
    for (uint8_t i = h2; i <= a2; i++) set_piece(PAWN, WHITE, i);

    // setting black pieces
    set_piece(ROOK, BLACK, a8);
    set_piece(ROOK, BLACK, h8);

    set_piece(KNIGHT, BLACK, b8);
    set_piece(KNIGHT, BLACK, g8);

    set_piece(BISHOP, BLACK, c8);
    set_piece(BISHOP, BLACK, f8);

    set_piece(KING, BLACK, e8);
    set_piece(QUEEN, BLACK, d8);
    for (uint8_t i = h7; i <= a7; i++) set_piece(PAWN, BLACK, i);

    hash = hash_state(*this);
}


///////////////////////////////////////////////////////////
//      These should only be used by the interface       //
///////////////////////////////////////////////////////////

bool Boardstate::player_move(Move m, bool forcing) {
    // TODO: check if move is valid
    //       xboard doesn't track castles and enpassant
    if (forcing) {
        make_move(m);
        return true;
    } else {
        square src = get_src(m);
        square dest = get_dest(m);
        uint8_t move_flags = get_flags(m);
        
        if((move_flags & CASTLE) && is_attacked(*this, (src + dest) / 2))
            return false;

        return make_move(m);
    }
}

std::string Boardstate::engine_move(uint32_t time, int max_depth) {
    update_trans_table();
    
    Move m;
    if (max_depth <= 6) {
        set_search_depth(max_depth);
        m = search(*this); 
    } else {
        uint32_t allocated_time = time / 20;
        auto start = std::chrono::system_clock::now();
        
        for (int depth = 6; depth <= max_depth; depth++) {
            set_search_depth(depth);
            m = search(*this);

            auto stop = std::chrono::system_clock::now();
            uint32_t centis = std::chrono::duration_cast<std::chrono::milliseconds>
                            (stop - start).count() / 10;
    
            log("Searched to depth " + std::to_string(depth) + " in " + 
                 std::to_string(centis * 10) + "ms");
            if (centis * 5 > allocated_time)
                break;
        }
    }

    if (m == 0)
        return "1/2-1/2 {Stalemate}\n";

    if (!make_move(m))
        return to_move == WHITE ?
               "0-1 {Black Mates}\n":
               "1-0 {White Mates}\n";

    if (no_capture_count >= 50)
        return "1/2-1/2 {Stalemate}\n";

    square from = get_src(m);
    square to = get_dest(m);

    return "move "
        + std::string(1, (char)(7 - (from % 8) + 'a')) + std::to_string(from / 8 + 1)
        + std::string(1, (char)(7 - (to % 8) + 'a')) + std::to_string(to / 8 + 1) + '\n';
}

piece Boardstate::get_piece(square i) const {
    bitboard b = 1ull << i;
    for (piece p = PAWN; p <= KING; p++)
        if ((pieces[WHITE][p] & b) || (pieces[BLACK][p] & b))
            return p;
    return NULL_PIECE;
}

bool Boardstate::is_castle(square old, square new_poz, piece p) const {
	// Castling moves:	e1g1, e1c1, e8g8, e8c8
    return p == KING && (
        (old == e1 && new_poz == g1 && !flags.test(WhiteKingSideCastle)) ||
        (old == e1 && new_poz == c1 && !flags.test(WhiteQueenSideCastle)) ||
        (old == e8 && new_poz == g8 && !flags.test(BlackKingSideCastle)) ||
        (old == e8 && new_poz == c8 && !flags.test(BlackQueenSideCastle))
    );
}

bool Boardstate::is_rk_init_sq(square pos) const {
    if (to_move == WHITE && (pos == a1 || pos == h1))
        return true;
    if (to_move == BLACK && (pos == a8 || pos == h8))
        return true;
    return false;
}

bool Boardstate::is_enpass(square old, square new_poz, piece p) const {
    if (p != PAWN)
        return false;

    if (to_move == WHITE) {
        if (new_poz - old != 16)
            return false;

        // TODO board bounds check
        if (!((1ull << (new_poz + 1) | 1ull << (new_poz - 1)) & pieces[BLACK][PAWN]))
            return false;
    }
    else {
        if (old - new_poz != 16)
            return false;

        // TODO board bounds check
        if (!((1ull << (new_poz + 1) | 1ull << (new_poz - 1)) & pieces[WHITE][PAWN]))
            return false;
    }

    return true;
}


///////////////////////////////////////////////////////////
//       This is just for pretty logging/debugging       //
///////////////////////////////////////////////////////////

static std::unordered_map<uint8_t, char> char_map = {
    {PAWN, 'P'}, {ROOK, 'R'}, {BISHOP, 'B'}, {KNIGHT, 'N'}, {KING, 'K'}, {QUEEN, 'Q'}
};

std::string Boardstate::get_state() const {
    std::string output;

    // side to play
    output = output + "To play: " + (to_move ? "BLACK\n" : "WHITE\n");
    // flags
    output += "Flags: "; for (auto i = 0; i < 8; i++) output += flags.test(i) ? "1 " : "0 ";
    // enpassant square
    output += "\nEn Passant square: " + (enpassant > 100 ? "None" : std::to_string(enpassant));
    // files
    output += "\nX a  b  c  d  e  f  g  h\n8 ";
    // board
    for (auto i = 63; i >= 0; i--) {
        if (board & 1ull << i) {
            for (piece p = PAWN; p <= KING; p++) {
                if (pieces[WHITE][p] & 1ull << i)
                    output = output + char_map[p] + "w ";
                else if (pieces[BLACK][p] & 1ull << i)
                    output = output + char_map[p] + "b ";
            }
        }
        else
            output += ".  ";

        // ranks
        if (i % 8 == 0)
            output = output + '\n' + (i > 0 ? std::to_string(i / 8) + ' ' : "");
    }
    output += "Game result: " + std::to_string(get_result()) + '\n';
    output += "Game stage: " + std::to_string(gamestage) + '\n';
    // these two should always be equal
    output += "Static  Evaluation: " + std::to_string(static_evaluate(*this)) + '\n';
    output += "Rolling Evaluation: " + std::to_string(midgame + endgame) + '\n';
    output += "Static  Hash: ";
    uint64_t h = hash_state(*this);
    for (int i = 0; i < 64; i++)
        output += (h & (1ull << i) ? "1" : "0");
    output += '\n';
    output += "Rolling Hash: ";
    for (int i = 0; i < 64; i++)
        output += (hash & (1ull << i) ? "1" : "0");
    output += '\n';

    return output;
}
