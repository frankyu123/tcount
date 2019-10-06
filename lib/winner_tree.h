#ifndef _WINNE_TREE_H_
#define _WINNE_TREE_H_

#include "hash.h"
#include <stdbool.h>

typedef struct TermInfo {
    char *term;
    int cnt;
} TermInfo;

extern void mergeKFile(int, HashConfig *);

#endif