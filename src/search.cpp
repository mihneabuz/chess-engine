#include "search.h"
#include "boardstate.h"
#include "evaluate.h"
#include "move_gen.h"
#include "transpositions.h"
#include <cstdint>

static int depth = 6;

void set_search_depth(const int x) {
    depth = x;
}

constexpr int illegal_move[] = {
    INT32_MIN + 1, INT32_MAX - 1
};

constexpr int win[] = {
    INT32_MAX - 2, INT32_MIN + 2
};

int quiescence(const Boardstate& B, int alpha, int beta);

///////////////////////////////////////////////////////////
//          Minimax with Alpha/Beta prunning             //
//           @ <= Performance Critical => @              //
///////////////////////////////////////////////////////////

int search(Boardstate B, const Move m, const int depth, int alpha, int beta) {
    // make move
    if (!B.make_move(m))
        return illegal_move[B.to_move];

    auto result = B.get_result();
    if (result != 0)
        return win[result - 1];
    
    // static evaluation
    if (depth == 0)
        return quiescence(B, alpha, beta);
    
    move_list moves;
    generate_all_moves(B, moves);

    // if there are no moves or 50 move rule -> stalemate
    if ((moves.captures.count == 0 && moves.quiet.count == 0) || B.no_capture_count >= 50)
        return 0;

    if (B.to_move == WHITE) {
        // search captures
        for (auto next_move : moves.captures) {
            auto curr_eval = search(B, next_move, depth - 1, alpha, beta);
            if (curr_eval > alpha) {
                alpha = curr_eval;
                if (beta <= alpha) {
                    return beta;
                }
            }
        }
        // search quiet moves
        for (auto next_move : moves.quiet) {
            auto curr_eval = search(B, next_move, depth - 1, alpha, beta);
            if (curr_eval > alpha) {
                alpha = curr_eval;
                if (beta <= alpha) {
                    return beta;
                }
            }
        }
       
        return alpha;
    
    } else {

        // search captures
        for (auto next_move : moves.captures) {
            auto curr_eval = search(B, next_move, depth - 1, alpha, beta);
            if (curr_eval < beta) {
                beta = curr_eval;
                if (beta <= alpha) {
                    return alpha;
                }
            }
        }
        // search quiet moves
        for (auto next_move : moves.quiet) {
            auto curr_eval = search(B, next_move, depth - 1, alpha, beta);
            if (curr_eval < beta) {
                beta = curr_eval;
                if (beta <= alpha) {
                    return alpha;
                }
            }
            
        }
        
        return beta;
    }
}

///////////////////////////////////////////////////////////
//       Initial search part, returns best Move          //
///////////////////////////////////////////////////////////

Move search(const Boardstate& B) { 
    // generate possible moves
    move_list moves;
    generate_all_moves(B, moves);

    // return null move if there are no moves
    if (moves.captures.count == 0 && moves.quiet.count == 0)
        return 0;

    // initialize alpha beta
    int alpha = INT32_MIN;
    int beta = INT32_MAX;

    Move best_move = 0;
    if (B.to_move == WHITE) {
        // search captures
        for (auto i = 0; i < moves.captures.count; i++) {
            int curr_eval = search(B, moves.captures.arr[i], depth - 1, alpha, beta);
            if (curr_eval > alpha) {
                alpha = curr_eval;
                best_move = moves.captures.arr[i];
            }
        }
        // search quiet moves
        for (auto i = 0; i < moves.quiet.count; i++) {
            int curr_eval = search(B, moves.quiet.arr[i], depth - 1, alpha, beta);
            if (curr_eval > alpha) {
                alpha = curr_eval;
                best_move = moves.quiet.arr[i];
            }
        }
        
        if (best_move == 0) {
            for (auto i = 0; i < moves.captures.count; i++) {
                Boardstate B_cpy = B;
                auto m = moves.captures.arr[i];
                if (B_cpy.make_move(m))
                    return m;
            }
            for (auto i = 0; i < moves.quiet.count; i++) {
                Boardstate B_cpy = B;
                auto m = moves.quiet.arr[i];
                if (B_cpy.make_move(m))
                    return m;
            }
        }
        return best_move;
    
    } else {

        // search captures
        for (auto i = 0; i < moves.captures.count; i++) {
            int curr_eval = search(B, moves.captures.arr[i], depth - 1, alpha, beta);
            if (curr_eval < beta) {
                beta = curr_eval;
                best_move = moves.captures.arr[i];
            }
        }
        // search quiet moves
        for (auto i = 0; i < moves.quiet.count; i++) {
            int curr_eval = search(B, moves.quiet.arr[i], depth - 1, alpha, beta);
            if (curr_eval < beta) {
                beta = curr_eval;
                best_move = moves.quiet.arr[i];
            }
        }
        if (best_move == 0) {
            for (auto i = 0; i < moves.captures.count; i++) {
                Boardstate B_cpy = B;
                auto m = moves.captures.arr[i];
                if (B_cpy.make_move(m))
                    return m;
            }
            for (auto i = 0; i < moves.quiet.count; i++) {
                Boardstate B_cpy = B;
                auto m = moves.quiet.arr[i];
                if (B_cpy.make_move(m))
                    return m;
            }
        }
        return best_move;
    }
}

////////////////////////////////////////////////////////////
//     Quiescence Search until no capture moves left      //
//   https://www.chessprogramming.org/Quiescence_Search   //
////////////////////////////////////////////////////////////

int q_search(Boardstate B, const Move m, int alpha, int beta) {
    if (!B.make_move(m))
        return illegal_move[B.to_move];
    
    move_array<64> moves;
    generate_capture_moves(B, moves);

    auto result = B.get_result();
    if (result != 0 || moves.count == 0)
        return win[result - 1];

    if (B.to_move == WHITE) {
    
        int stand_pat = evaluate(B);
        if (stand_pat >= beta)
            return beta;
        if (alpha < stand_pat)
            alpha = stand_pat;

        int curr_eval = 0;
        for (auto m : moves) {
            curr_eval = q_search(B, m, alpha, beta);
            alpha = std::max(alpha, curr_eval);
            if (beta <= alpha)
                return beta;
        }
        return alpha;
    
    } else {
    
        int stand_pat = evaluate(B);
        if (alpha >= stand_pat)
            return alpha;
        if (beta > stand_pat)
            beta = stand_pat;

        int curr_eval = 0;
        for (auto m : moves) {
            curr_eval = q_search(B, m, alpha, beta);
            beta = std::min(beta, curr_eval);
            if (beta <= alpha)
                return alpha;
        }
        return beta;
    }
}

int quiescence(const Boardstate& B, int alpha, int beta){
    move_array<64> moves;
    generate_capture_moves(B, moves);

    if (moves.count == 0)
        return evaluate(B);
    
    if (B.to_move == WHITE) {
    
        int stand_pat = evaluate(B);
        if (stand_pat >= beta)
            return beta;
        if (alpha < stand_pat)
            alpha = stand_pat;

        int curr_eval = 0;
        for (auto m : moves) {
            curr_eval = q_search(B, m, alpha, beta);
            alpha = std::max(alpha, curr_eval);
            if (beta <= alpha)
                return beta;
        }
        return alpha;
   
    } else {
    
        int stand_pat = evaluate(B);
        if (alpha >= stand_pat)
            return alpha;
        if (beta > stand_pat)
            beta = stand_pat;

        int curr_eval = 0;
        for (auto m : moves) {
            curr_eval = q_search(B, m, alpha, beta);
            beta = std::min(beta, curr_eval);
            if (beta <= alpha)
                return alpha;
        }
        return beta;
    }
}
