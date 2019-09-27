#ifndef _WINNE_TREE_H_
#define _WINNE_TREE_H_

#include <stdbool.h>

typedef struct TermInfo {
    char *term;
    int cnt;
} TermInfo;

extern void initWinnerTree(int, int);
extern bool checkWinnerTreeEmpty();
extern void winnerTreePop();

#endif