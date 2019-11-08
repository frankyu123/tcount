/**
 * winner_tree.c - implement winner tree for merging multiple split files
 * 
 * Author: Frank Yu <frank85515@gmail.com>
 * 
 * (C) Copyright 2019 Frank Yu
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <winner_tree.h>

#ifdef __linux__
#include <pthread.h>
#endif

#ifdef __APPLE__
#include <pthread_barrier.h>
#endif

#define _WINNER_TREE_SPLIT_FILE "_buffer"
#define _WINNER_TREE_OFFSET_FILE "_offset"

/**
 * @threadIdx: idx of current thread
 * @lastFileIdx: last idx of split file in each thread task
 * @newFileIdx: file idx of new split file & offset file
 */
typedef struct ThreadArgs {
    int threadIdx;
    int lastFileIdx;
    int newFileIdx;
} ThreadArgs;

static int _chunk, _thread;
static int _totalNodeNum = 1;
static int _childrenNum = 0;
static pthread_barrier_t _pbt;

static WinnerTreeNode *setWinnerTreeNode(FILE *fin, FILE *fmap)
{
    WinnerTreeNode *node = NULL;
    int size;

    if (fmap == NULL) {
        return NULL;
    }

    if (fscanf(fmap, "%d\n", &size) != EOF) {
        char *line = malloc(size + 1);
        memset(line, '\0', size + 1);
        fread(line, size, sizeof(char), fin);

        node = (WinnerTreeNode *) malloc(sizeof(WinnerTreeNode));
        char *ret = strrchr(line, ' ');
        node->cnt = atoi(ret);
        node->term = (char *) malloc(size);
        memset(node->term, '\0', size);
        strncpy(node->term, line, strlen(line) - strlen(ret));

        free(line);
    }
    return node;
}

static bool cmp(WinnerTreeNode *left, WinnerTreeNode *right) 
{
    return (strcmp(left->term, right->term) <= 0) ? true : false;
}

static void initWinnerTree(WinnerTree *tree, int nodeIdx, FILE **fin, FILE **fmap)
{
    if (nodeIdx >= _totalNodeNum) {
        return;
    }

    int leftIdx = 2 * nodeIdx + 1;
    int rightIdx = 2 * nodeIdx + 2;
    initWinnerTree(tree, leftIdx, fin, fmap);
    initWinnerTree(tree, rightIdx, fin, fmap);

    if (leftIdx >= _totalNodeNum && rightIdx >= _totalNodeNum) {
        int idx = nodeIdx - (_totalNodeNum - _childrenNum);
        if (idx >= _chunk) {
            tree->nodeList[nodeIdx] = -1;
        } else {
            tree->nodeList[nodeIdx] = idx;
            WinnerTreeNode *node = setWinnerTreeNode(fin[idx], fmap[idx]);
            if (node == NULL) {
                tree->nodeList[nodeIdx] = -1;
            } else {
                tree->nodeValue[idx].term = strdup(node->term);
                tree->nodeValue[idx].cnt = node->cnt;
                free(node->term);
            }
            free(node);
        }
    } else {
		if (tree->nodeList[leftIdx] == -1 && tree->nodeList[rightIdx] == -1) {
			tree->nodeList[nodeIdx] = -1;
		} else if (tree->nodeList[leftIdx] != -1 && tree->nodeList[rightIdx] == -1) {
			tree->nodeList[nodeIdx] = tree->nodeList[leftIdx];
		} else if (tree->nodeList[leftIdx] == -1 && tree->nodeList[rightIdx] != -1) {
			tree->nodeList[nodeIdx] = tree->nodeList[rightIdx];
		} else {
			if (cmp(&tree->nodeValue[tree->nodeList[leftIdx]], &tree->nodeValue[tree->nodeList[rightIdx]])) {
				tree->nodeList[nodeIdx] = tree->nodeList[leftIdx];
			} else {
				tree->nodeList[nodeIdx] = tree->nodeList[rightIdx];
			}
		}
    }
}

static void updateWinnerTree(WinnerTree *tree, int nodeIdx, int updateIdx, FILE **fin, FILE **fmap)
{
    if (nodeIdx >= _totalNodeNum) {
        return;
    }

    int leftIdx = 2 * nodeIdx + 1;
    int rightIdx = 2 * nodeIdx + 2;
    updateWinnerTree(tree, leftIdx, updateIdx, fin, fmap);
    updateWinnerTree(tree, rightIdx, updateIdx, fin, fmap);

	if (leftIdx >= _totalNodeNum &&  rightIdx >= _totalNodeNum && tree->nodeList[nodeIdx] == updateIdx) {
		WinnerTreeNode *newNode = setWinnerTreeNode(fin[updateIdx], fmap[updateIdx]);
		if (newNode != NULL) {
            free(tree->nodeValue[updateIdx].term);
            tree->nodeValue[updateIdx].term = strdup(newNode->term);
            tree->nodeValue[updateIdx].cnt = newNode->cnt;
            free(newNode->term);
		} else {
            free(tree->nodeValue[updateIdx].term);
			tree->nodeList[nodeIdx] = -1;
		}
        free(newNode);
	} else if (leftIdx < _totalNodeNum &&  rightIdx < _totalNodeNum && tree->nodeList[nodeIdx] != -1) {
		if (tree->nodeList[leftIdx] == -1 && tree->nodeList[rightIdx] == -1) {
			tree->nodeList[nodeIdx] = -1;
		} else if (tree->nodeList[leftIdx] != -1 && tree->nodeList[rightIdx] == -1) {
			tree->nodeList[nodeIdx] = tree->nodeList[leftIdx];
		} else if (tree->nodeList[leftIdx] == -1 && tree->nodeList[rightIdx] != -1) {
			tree->nodeList[nodeIdx] = tree->nodeList[rightIdx];
		} else {
			if (cmp(&tree->nodeValue[tree->nodeList[leftIdx]], &tree->nodeValue[tree->nodeList[rightIdx]])) {
				tree->nodeList[nodeIdx] = tree->nodeList[leftIdx];
			} else {
				tree->nodeList[nodeIdx] = tree->nodeList[rightIdx];
			}
		}
	}
}

static void merge(int baseFileIdx, int lastFileIdx, FILE *fout, FILE *fnmap)
{
    FILE **fin = malloc((_chunk + 1) * sizeof(FILE *));
    FILE **fmap = malloc((_chunk + 1) * sizeof(FILE *));
    char splitFile[31], offsetFile[31], newFile[31], newMap[31];
    for (int i = 0; i < _chunk; i++) {
        if (baseFileIdx + i <= lastFileIdx) {
            sprintf(splitFile, "%s_%d", _WINNER_TREE_SPLIT_FILE, baseFileIdx + i);
            sprintf(offsetFile, "%s_%d", _WINNER_TREE_OFFSET_FILE, baseFileIdx + i);
            fin[i] = fopen(splitFile, "r");
            fmap[i] = fopen(offsetFile, "r");
        } else {
            fin[i] = fmap[i] = NULL;
        }
    }

    WinnerTree *tree = malloc(sizeof(WinnerTree));
    tree->nodeValue = (WinnerTreeNode *) malloc(_chunk * sizeof(WinnerTreeNode));
    tree->nodeList = (int *) malloc(_totalNodeNum * sizeof(int));
    for (int i = 0; i < _totalNodeNum; i++) {
        tree->nodeList[i] = -1;
    }

    WinnerTreeNode *lastOutputNode = malloc(sizeof(WinnerTreeNode));
    lastOutputNode->term = NULL;
    
    initWinnerTree(tree, 0, fin, fmap);
    while (true) {
        if (tree->nodeList[0] == -1) {
            char entry[strlen(lastOutputNode->term) + 10];
            sprintf(entry, "%s %d\n", lastOutputNode->term, lastOutputNode->cnt);
            fwrite(entry, strlen(entry), sizeof(char), fout);
            if (fnmap != NULL) {
                fprintf(fnmap, "%d\n", (int) strlen(entry));
            }
            free(lastOutputNode->term);
            free(lastOutputNode);
            free(tree->nodeList);
            free(tree->nodeValue);
            free(tree);
            break;
        }

        if (lastOutputNode->term == NULL) {
            lastOutputNode->term = strdup(tree->nodeValue[tree->nodeList[0]].term);
            lastOutputNode->cnt = tree->nodeValue[tree->nodeList[0]].cnt;
        } else if (strcmp(lastOutputNode->term, tree->nodeValue[tree->nodeList[0]].term) == 0) {
            lastOutputNode->cnt += tree->nodeValue[tree->nodeList[0]].cnt;
        } else {
            char entry[strlen(lastOutputNode->term) + 10];
            sprintf(entry, "%s %d\n", lastOutputNode->term, lastOutputNode->cnt);
            fwrite(entry, strlen(entry), sizeof(char), fout);
            if (fnmap != NULL) {
                fprintf(fnmap, "%d\n", (int) strlen(entry));
            }
            free(lastOutputNode->term);
            lastOutputNode->term = strdup(tree->nodeValue[tree->nodeList[0]].term);
            lastOutputNode->cnt = tree->nodeValue[tree->nodeList[0]].cnt;
        }

        updateWinnerTree(tree, 0, tree->nodeList[0], fin, fmap);
    }

    for (int i = 0; i < _chunk && baseFileIdx + i <= lastFileIdx; i++) {
        sprintf(splitFile, "%s_%d", _WINNER_TREE_SPLIT_FILE, baseFileIdx + i);
        sprintf(offsetFile, "%s_%d", _WINNER_TREE_OFFSET_FILE, baseFileIdx + i);
        fclose(fin[i]);
        fclose(fmap[i]);
        remove(splitFile);
        remove(offsetFile);
    }
}

static void *job(void *argv)
{
    ThreadArgs *args = (ThreadArgs *) argv;
    char newFile[31], newMap[31];
    sprintf(newFile, "%s_%d", _WINNER_TREE_SPLIT_FILE, args->newFileIdx);
    sprintf(newMap, "%s_%d", _WINNER_TREE_OFFSET_FILE, args->newFileIdx);
    FILE *fout = fopen(newFile, "w");
    FILE *fnmap = fopen(newMap, "w");
    int baseFileIdx = args->lastFileIdx - (args->lastFileIdx - _chunk * (args->threadIdx - 1)) + 1;
    merge(baseFileIdx, args->lastFileIdx, fout, fnmap);
    fclose(fout);
    fclose(fnmap);
    return NULL;
}

void mergeKFile(int fileNum, int chunk, int thread, char *output)
{
    _chunk = chunk;
    _thread = thread;

    int nodeNum = 1;
    while (nodeNum < _chunk) {
        nodeNum <<= 1;
    }
    _childrenNum = nodeNum;
    _totalNodeNum = 2 * nodeNum - 1;

    int cnt = 0;
    while (fileNum - cnt > _chunk) {
        int threadNum = (fileNum - cnt) / _chunk;

        if (threadNum > _thread) {
            threadNum = _thread;
        }

        pthread_t tids[threadNum];
        pthread_barrier_init(&_pbt, NULL, threadNum + 1);
        int tmp = 0;
        for (int i = 0; i < threadNum; i++) {
            ThreadArgs *args = (ThreadArgs *) malloc(sizeof(ThreadArgs));
            args->threadIdx = i + 1;

            args->lastFileIdx = cnt + _chunk * (i + 1);
            if (args->lastFileIdx > fileNum) {
                args->lastFileIdx = fileNum;
            }

            args->newFileIdx = fileNum + (i + 1);
            tmp += args->lastFileIdx - _chunk * (args->threadIdx - 1);
            pthread_create(&tids[i], NULL, job, args);
        }

        for (int i = 0; i < threadNum; i++) {
            pthread_join(tids[i], NULL);
        }

        pthread_barrier_destroy(&_pbt);
        cnt += tmp;
        fileNum += threadNum;
    }

    FILE *fout;
    if (output != NULL) {
        fout = fopen(output, "w");
    } else {
        fout = stdout;
    }
    merge(cnt + 1, fileNum, fout, NULL);
    fclose(fout);
}