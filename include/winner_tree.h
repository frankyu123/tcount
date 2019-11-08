#ifndef _WINNE_TREE_H_
#define _WINNE_TREE_H_

typedef struct WinnerTreeNode {
    char *term;
    int cnt;
} WinnerTreeNode;

typedef struct WinnerTree {
    int *nodeList;
    WinnerTreeNode *nodeValue;
} WinnerTree;

extern void mergeKFile(int, int, int, char *);

#endif