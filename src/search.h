#ifndef __SEARCH_H_
#define __SEARCH_H_

#include "move_gen.h"

void set_search_depth(const int depth);
Move search(const Boardstate& B);

#endif
