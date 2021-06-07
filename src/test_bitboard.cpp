#include "bitboard.h"
#include "boardstate.h"
#include "evaluate.h"
#include "move.h"
#include "move_gen.h"
#include "search.h"
#include "zobrist.h"
#include "transpositions.h"
#include <iostream> 
#include <string>

void printBitboard(bitboard b)
{
  bitboard mask = 0x8000000000000000; // square a8
  for (int i = 7; i >= 0; --i)
  {
    for (int j = 0; j < 8; ++j)
    { std::cout << (b & mask ? "1 " : "0 "); mask >>= 1;
    }
    std::cout << '\n'; 
  }
}

std::string move_to_string(Move m) {
    return std::to_string(get_src(m)) + " " + std::to_string(get_dest(m)) + " "
         + std::to_string(get_piece(m)) + " " +std::to_string(get_promoted(m)) + " " 
         + std::to_string(get_flags(m)) + " " +std::to_string(get_score(m));
}

int main()
{
  init_move_tables();
  init_eval_tables();
  init_zobrist_table(0xdeadbeef);
  set_search_depth(1);

  Boardstate B;
  B.reset();
  std::cout << B.get_state();

  square x = f3;
  bitboard b = 1ull << x;
  
  std::cout << "\nPiece:\n";
  printBitboard(b);
  std::cout << "\nPawn attacks:\n";
  printBitboard(test_attack_tables(PAWN, WHITE, x, 0));
  std::cout << "\nKnight attacks:\n";
  printBitboard(test_attack_tables(KNIGHT, WHITE, x, 0));
  std::cout << "\nBishop attacks:\n";
  printBitboard(test_attack_tables(BISHOP, WHITE, x, 0));
  std::cout << "\nRook attacks:\n";
  printBitboard(test_attack_tables(ROOK, WHITE, x, 0));
  std::cout << "\nQueen attacks:\n";
  printBitboard(test_attack_tables(QUEEN, WHITE, x, 0));
  std::cout << "\nKing attacks:\n";
  printBitboard(test_attack_tables(KING, WHITE, x, 0));

  std::cout << "\n < Attacks on the fly >\n";

  bitboard block = 0;
  block |= 1ull << b3;
  block |= 1ull << g6;
  block |= 1ull << f4;
  block |= 1ull << b6;
  block |= 1ull << e6;

  square t = e3;
  std::cout << "\n  <Occupancy>\n";
  printBitboard(block);
  std::cout << "\n    <Piece>\n";
  printBitboard(1ull << t);

  std::cout << "\n<Bishop attacks>\n";
  printBitboard(test_attack_tables(BISHOP, 0, t, block));
  
  std::cout << "\n<Rook attacks>\n";
  printBitboard(test_attack_tables(ROOK, 0, t, block));
  
  std::cout << "\n<Queen attacks>\n";
  printBitboard(test_attack_tables(QUEEN, 0, t, block));
  
  std::cout << "\n < Boardstate and move gen >\n";
  B.make_move(encode(d2, d4, PAWN, PAWN, NO_FLAGS));
  B.make_move(encode(e7, e5, PAWN, PAWN, NO_FLAGS));
  B.make_move(encode(b2, b7, PAWN, PAWN, CAPTURE));
  B.make_move(encode(c7, c5, PAWN, PAWN, NO_FLAGS));
  B.make_move(encode(g1, f3, KNIGHT, KNIGHT, NO_FLAGS));
  B.make_move(encode(h7, h4, PAWN, PAWN, NO_FLAGS));
  B.make_move(encode(g2, g4, PAWN, PAWN, ENPASSANT));

  std::cout << B.get_state();
  std::cout << "\n<   Move Generation   >\n";
  move_list moves;
  generate_all_moves(B, moves);

  for (auto m : moves.captures)
    std::cout << move_to_string(m) << "\n";
  for (auto m : moves.quiet)
    std::cout << move_to_string(m) << "\n";

  B.make_move(encode(h8, h6, ROOK, ROOK, NO_FLAGS));
  B.make_move(encode(f1, g2, BISHOP, BISHOP, NO_FLAGS));
  B.make_move(encode(d8, a5, QUEEN, QUEEN, NO_FLAGS));

  std::cout << '\n' << B.get_state() << '\n';
  move_array<64> moves2;
  generate_capture_moves(B, moves2);

  for (auto m : moves2)
    std::cout << move_to_string(m) << "\n";

  std::cout << "\n<   Move execution   >\n";
  Move m = search(B);
    std::cout << "Best move: " << move_to_string(m) << "\n";
  B.make_move(m);
  std::cout << B.get_state();

  std::cout << "\n< Evaluation tables >\n";
  for (piece p = PAWN; p <= KING; p++) {
      std::cout << "\nPIECE: " << (int)p << "\n";
      for (int i = 63; i >= 0; i--) {
          if (midgame_value_map[WHITE][p][i] < 100) std::cout << " ";
          std::cout << midgame_value_map[WHITE][p][i] << ' '; 
          if (i % 8 == 0)
            std::cout << '\n';
      }
      std::cout << '\n';
      for (int i = 63; i >= 0; i--) {
          if (midgame_value_map[BLACK][p][i] > -100) std::cout << " ";
          std::cout << midgame_value_map[BLACK][p][i] << ' '; 
          if (i % 8 == 0)
            std::cout << '\n';
      }
  }

  std::cout << "\nHashing array\n";
  for (int i = 0; i < 2; i++)
      for (int j = 0; j < 6; j++)
          for (int k = 0; k < 5; k++) {
              for (int c = 0; c < 64; c++)
                std::cout << (hash_table[i][j][k] & (1ull << c) ? 1 : 0);
              std::cout << '\n';
          }
  std::cout << ".....\n\n";

  std::cout << B.get_state() << '\n';
  set_search_depth(1);
  m = search(B);
  B.make_move(m);
  m = search(B);

  move_list moves3;
  generate_all_moves(B, moves3);

  for (auto m : moves3.captures) {
    Boardstate B2(B);
  
    B2.make_move(m);

    std::cout << move_to_string(m) << "\n";
    std::cout << B2.get_state();
    auto& entry = get_entry(B2.hash);
    std::cout << B2.hash << '\n';
    std::cout << entry.zobrist << " " << entry.flag << " " << entry.depth << " " << move_to_string(entry.best_move)<< "\n";
    std::cout << evaluate(B2) << "\n\n";
  }

  return 0;
}
