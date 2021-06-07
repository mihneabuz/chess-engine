#ifndef _BOARDSTATE_H_
#define _BOARDSTATE_H_

#include "bitboard.h"
#include "move.h"
#include "bitarray.h"
#include <array>
#include <string>

// Definitions of internal board structure

enum {
  WhiteQueenSideCastle = 0,
  WhiteKingSideCastle = 1,
  BlackQueenSideCastle = 2,
  BlackKingSideCastle = 3,
  WhiteCheck1 = 4,
  WhiteCheck2 = 5,
  BlackCheck1 = 6,
  BlackCheck2 = 7
};

enum {
  WHITE,
  BLACK
};

enum {
  PAWN = 0,
  BISHOP = 1,
  KNIGHT = 2,
  ROOK = 3,
  QUEEN = 4,
  KING = 5,
  NULL_PIECE
};

class Boardstate
{
  public:

    ///////////////////////////////////
    /*             Data              */
    ///////////////////////////////////

    // side to move
    int to_move;

    // all pieces on the board
    bitboard board;

    // pieces[COLOR][PIECE] -> bitboard of pieces PIECE and color COLOR
    std::array<std::array<bitboard, 6>, 2> pieces;

    // occupancies[COLOR] -> all pieces of color COLOR
    std::array<bitboard, 2> occupancies;

    // castling rights and check count
    bitarray flags;

    // enpassant square
    square enpassant;

    // evaluation cache
    int midgame, endgame;

    // gamestage euristic for evaluation
    int gamestage;

    // no capture moves count, for 50 move rule
    int no_capture_count;

    // zobrist hash
    uint64_t hash;

    ///////////////////////////////////
    /*            Methods            */
    ///////////////////////////////////

    // default constructor
    Boardstate();

    // copy-constructor for recursion (copy-move)
    Boardstate(const Boardstate& c);

    // get string of board state for logging and debugging
    std::string get_state() const;

    // reset board to starting position
    void reset();

    // applies pseudo-legal move m to boardstate
    // if m is not legal, returns false
    // non-reversible (copy-make)
    bool make_move(Move m) noexcept;

    // returns 0 if game is still going,
    //         1 if white 3-checked
    //         2 if black 3-checked
    int get_result() const;

    // methods used only by interface
    bool player_move(Move m, bool forcing);
    std::string engine_move(uint32_t time, int max_depth);
    piece get_piece(square i) const;
    bool is_castle(square old, square new_poz, piece p) const;
    bool is_enpass(square old, square new_poz, piece p) const;
    bool is_rk_init_sq(square pos) const;


  private:

    void set_piece(piece p, color c, square i) noexcept;
    void pop_piece(piece p, color c, square i) noexcept;
    void swap_to_move() noexcept;

};
#endif
