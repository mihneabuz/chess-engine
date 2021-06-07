#ifndef _EVALUATE_H_
#define _EVALUATE_H_

#include "boardstate.h"

// 2 colors, 6 pieces, 64 squares
extern int midgame_value_map[2][6][64];
extern int endgame_value_map[2][6][64];

constexpr int gamestage_value_map[] = {
    0, 1, 1, 2, 4, 0
};

void init_eval_tables();
int evaluate(const Boardstate& B);
int static_evaluate(const Boardstate& B);

#endif
