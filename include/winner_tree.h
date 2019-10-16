#ifndef _WINNE_TREE_H_
#define _WINNE_TREE_H_

#include <stdbool.h>

#include <hash.h>

typedef struct TermInfo {
    char *term;
    int cnt;
} TermInfo;

typedef struct WinnerTree {
    int *nodeList;
    TermInfo *nodeValue;
} WinnerTree;

extern void mergeKFile(int, HashConfig *);

#endif