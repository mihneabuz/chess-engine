#include <iostream>
#include <string>
#include <chrono>
#include "boardstate.h"
#include "evaluate.h"
#include "move.h"
#include "move_gen.h"
#include "search.h"
#include "transpositions.h"
#include "zobrist.h"

using namespace std;

const string square_map[] = {
    "h1", "g1", "f1", "e1", "d1", "c1", "b1", "a1",
    "h2", "g2", "f2", "e2", "d2", "c2", "b2", "a2",
    "h3", "g3", "f3", "e3", "d3", "c3", "b3", "a3",
    "h4", "g4", "f4", "e4", "d4", "c4", "b4", "a4",
    "h5", "g5", "f5", "e5", "d5", "c5", "b5", "a5",
    "h6", "g6", "f6", "e6", "d6", "c6", "b6", "a6",
    "h7", "g7", "f7", "e7", "d7", "c7", "b7", "a7",
    "h8", "g8", "f8", "e8", "d8", "c8", "b8", "a8",
};

int count_moves(Boardstate B, Move mov, int depth) {
    if(!B.make_move(mov))
        return 0;
    
    if (depth == 0)
        return 1;

    move_list moves;
    generate_all_moves(B, moves);

    int count = 0;
    for (auto m : moves.captures) {
        count += count_moves(B, m, depth - 1);
    }

    for (auto m : moves.quiet) {
        count += count_moves(B, m, depth - 1);
    }
    
    return count + 1;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: " + string(argv[0]) + " [TEST]\n";
        cout << "Tests: [movegen] [search [DEPTH]]\n";
        return 0; 
    }

    init_move_tables();
    init_eval_tables();
    init_zobrist_table(0xdeadbeef);

    if (string(argv[1]) == "movegen") {
        cout << "Testing just move generation, no prunning!\n";
        cout << "Searching depth: 5\n\n"; 

        Boardstate B;
        B.reset();
        cout << B.get_state() << '\n';

        auto start = chrono::high_resolution_clock::now();
        move_list moves;
        generate_all_moves(B, moves);

        for (auto m : moves.quiet) {
            int x = count_moves(B, m, 5);
            cout << "\t" << square_map[get_src(m)] << square_map[get_dest(m)] << ": " << x << "\n";
        }

        auto stop = chrono::high_resolution_clock::now();
        int milis = chrono::duration_cast<chrono::milliseconds>(stop - start).count();

        cout << "\nTime: " << (float)milis / 1000 << "ms\n";
    }
    else if (string(argv[1]) == "search") {
        cout << "Testing searching with prunning and other goodies!\n";
        int depth = atoi(argv[2]);
        set_search_depth(depth);
        cout << "Searching depth: " << depth << "!\n\n"; 

        Boardstate B;
        B.reset();
        B.make_move(encode(d2, d4, PAWN, PAWN, NO_FLAGS));
        B.make_move(encode(e7, e5, PAWN, PAWN, NO_FLAGS));
        B.make_move(encode(b2, b7, PAWN, PAWN, CAPTURE));
        B.make_move(encode(c7, c5, PAWN, PAWN, NO_FLAGS));
        B.make_move(encode(g1, f3, KNIGHT, KNIGHT, NO_FLAGS));
        B.make_move(encode(h7, h4, PAWN, PAWN, NO_FLAGS));
        B.make_move(encode(g2, g4, PAWN, PAWN, ENPASSANT));
        B.make_move(encode(f7, f6, PAWN, PAWN, NO_FLAGS));
        cout << B.get_state() << '\n';

        auto start = chrono::high_resolution_clock::now();

        for (auto i = 0; i < 20; i++) {
//            std::cout << "\t" << "TT size: " << get_trans_table_size() << '\n';
            std::cout << "\t" << B.engine_move(1000000, depth);
//            std::cout << B.get_state() << '\n';
        }

        auto stop = chrono::high_resolution_clock::now();
        int milis = chrono::duration_cast<chrono::milliseconds>(stop - start).count();

        cout << "\nTime: " << (float)milis / 1000 << "s\n";
    }

    return 0;
}
