#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "winner_tree.h"

#ifdef __linux__
#include <pthread.h>
#endif

#ifdef __APPLE__
#include "pthread_barrier.h"
#endif

#define _WINNER_TREE_SPLIT_FILE "key_buffer"
#define _WINNER_TREE_OFFSET_FILE "offset"

typedef struct ThreadArgs {
    int endIdx;
    int newFileNo;
} ThreadArgs;

typedef struct WinnerTree {
    int *nodeList;
    TermInfo *nodeValue;
} WinnerTree;

static int _fileNum;
static int _nodeNum = 1;
static HashConfig *config;
static pthread_barrier_t pbt;

static TermInfo *getTermInfo(FILE *fin, FILE *fmap)
{
    TermInfo *data = NULL;
    int size;

    if (fmap == NULL) {
        return NULL;
    }

    if (fscanf(fmap, "%d\n", &size) != EOF) {
        char *inputBuffer = malloc(size + 2);
        memset(inputBuffer, '\0', size + 2);
        fread(inputBuffer, size, sizeof(char), fin);

        data = (TermInfo *) malloc(sizeof(TermInfo));
        char *ret = strrchr(inputBuffer, ' ');
        data->cnt = atoi(ret);
        data->term = (char *) malloc(size);
        memset(data->term, '\0', size);
        strncpy(data->term, inputBuffer, strlen(inputBuffer) - strlen(ret));

        free(inputBuffer);
    }
    return data;
}

static bool nodeCmp(TermInfo *left, TermInfo *right) 
{
    return (strcmp(left->term, right->term) <= 0) ? true : false;
}

static void initWinnerTree(WinnerTree *tree, int nodeIdx, FILE **fin, FILE **fmap)
{
    if (nodeIdx >= _nodeNum) {
        return;
    }

    int leftIdx = 2 * nodeIdx + 1;
    int rightIdx = 2 * nodeIdx + 2;
    initWinnerTree(tree, leftIdx, fin, fmap);
    initWinnerTree(tree, rightIdx, fin, fmap);

    if (leftIdx >= _nodeNum && rightIdx >= _nodeNum) {
        int idx = nodeIdx - (_nodeNum - config->chunk);
        tree->nodeList[nodeIdx] = idx;
        TermInfo *data = getTermInfo(fin[idx], fmap[idx]);
        if (data == NULL) {
            tree->nodeList[nodeIdx] = -1;
        } else {
            tree->nodeValue[idx].term = (char *) malloc(strlen(data->term) + 1);
            strcpy(tree->nodeValue[idx].term, data->term);
            tree->nodeValue[idx].cnt = data->cnt;
            free(data->term);
        }
        free(data);
    } else {
		if (tree->nodeList[leftIdx] == -1 && tree->nodeList[rightIdx] == -1) {
			tree->nodeList[nodeIdx] = -1;
		} else if (tree->nodeList[leftIdx] != -1 && tree->nodeList[rightIdx] == -1) {
			tree->nodeList[nodeIdx] = tree->nodeList[leftIdx];
		} else if (tree->nodeList[leftIdx] == -1 && tree->nodeList[rightIdx] != -1) {
			tree->nodeList[nodeIdx] = tree->nodeList[rightIdx];
		} else {
			if (nodeCmp(&tree->nodeValue[tree->nodeList[leftIdx]], &tree->nodeValue[tree->nodeList[rightIdx]])) {
				tree->nodeList[nodeIdx] = tree->nodeList[leftIdx];
			} else {
				tree->nodeList[nodeIdx] = tree->nodeList[rightIdx];
			}
		}
    }
}

static void updateWinnerTree(WinnerTree *tree, int nodeIdx, int updateIdx, FILE **fin, FILE **fmap)
{
    if (nodeIdx >= _nodeNum) {
        return;
    }

    int leftIdx = 2 * nodeIdx + 1;
    int rightIdx = 2 * nodeIdx + 2;
    updateWinnerTree(tree, leftIdx, updateIdx, fin, fmap);
    updateWinnerTree(tree, rightIdx, updateIdx, fin, fmap);

	if (leftIdx >= _nodeNum &&  rightIdx >= _nodeNum && tree->nodeList[nodeIdx] == updateIdx) {
		TermInfo *newData = getTermInfo(fin[updateIdx], fmap[updateIdx]);
		if (newData != NULL) {
            free(tree->nodeValue[updateIdx].term);
            tree->nodeValue[updateIdx].term = (char *) malloc(strlen(newData->term) + 1);
			strcpy(tree->nodeValue[updateIdx].term, newData->term);
            tree->nodeValue[updateIdx].cnt = newData->cnt;
            free(newData->term);
		} else {
            free(tree->nodeValue[updateIdx].term);
			tree->nodeList[nodeIdx] = -1;
		}
        free(newData);
	} else if (leftIdx < _nodeNum &&  rightIdx < _nodeNum && tree->nodeList[nodeIdx] != -1) {
		if (tree->nodeList[leftIdx] == -1 && tree->nodeList[rightIdx] == -1) {
			tree->nodeList[nodeIdx] = -1;
		} else if (tree->nodeList[leftIdx] != -1 && tree->nodeList[rightIdx] == -1) {
			tree->nodeList[nodeIdx] = tree->nodeList[leftIdx];
		} else if (tree->nodeList[leftIdx] == -1 && tree->nodeList[rightIdx] != -1) {
			tree->nodeList[nodeIdx] = tree->nodeList[rightIdx];
		} else {
			if (nodeCmp(&tree->nodeValue[tree->nodeList[leftIdx]], &tree->nodeValue[tree->nodeList[rightIdx]])) {
				tree->nodeList[nodeIdx] = tree->nodeList[leftIdx];
			} else {
				tree->nodeList[nodeIdx] = tree->nodeList[rightIdx];
			}
		}
	}
}

static void *job(void *argv)
{
    ThreadArgs *args = (ThreadArgs *) argv;
    TermInfo *lastOutputTerm = malloc(sizeof(TermInfo));
    lastOutputTerm->term = (char *) malloc(config->keyBufferSize);
    memset(lastOutputTerm->term, '\0', config->keyBufferSize);

    FILE **fin = malloc((config->chunk + 1) * sizeof(FILE *));
    FILE **fmap = malloc((config->chunk + 1) * sizeof(FILE *));
    char splitFile[31], offsetFile[31], newFile[31], newMap[31];
    for (int i = 0; i < config->chunk; i++) {
        sprintf(splitFile, "%s_%d.rec", _WINNER_TREE_SPLIT_FILE, args->endIdx - (config->chunk - 1) + i);
        sprintf(offsetFile, "%s_%d.rec", _WINNER_TREE_OFFSET_FILE, args->endIdx - (config->chunk - 1) + i);
        fin[i] = fopen(splitFile, "r");
        fmap[i] = fopen(offsetFile, "r");
    }
    sprintf(newFile, "%s_%d.rec", _WINNER_TREE_SPLIT_FILE, args->newFileNo);
    sprintf(newMap, "%s_%d.rec", _WINNER_TREE_OFFSET_FILE, args->newFileNo);
    FILE *fout = fopen(newFile, "w");
    FILE *fnmap = fopen(newMap, "w");

    WinnerTree *tree = malloc(sizeof(WinnerTree));
    tree->nodeValue = (TermInfo *) malloc(config->chunk * sizeof(TermInfo));
    tree->nodeList = (int *) malloc(_nodeNum * sizeof(int));
    for (int i = 0; i < _nodeNum; i++) {
        tree->nodeList[i] = -1;
    }

    initWinnerTree(tree, 0, fin, fmap);
    while (true) {
        if (tree->nodeList[0] == -1) {
            char entry[config->keyBufferSize + 10];
            sprintf(entry, "%s %d\n", lastOutputTerm->term, lastOutputTerm->cnt);
            fwrite(entry, strlen(entry), sizeof(char), fout);
            fprintf(fnmap, "%d\n", (int) strlen(entry));

            free(lastOutputTerm->term);
            free(lastOutputTerm);
            free(tree->nodeList);
            free(tree->nodeValue);
            free(tree);
            break;
        }

        if (strcmp(lastOutputTerm->term, "\0") == 0) {
            strcpy(lastOutputTerm->term, tree->nodeValue[tree->nodeList[0]].term);
            lastOutputTerm->cnt = tree->nodeValue[tree->nodeList[0]].cnt;
        } else if (strcmp(lastOutputTerm->term, tree->nodeValue[tree->nodeList[0]].term) == 0) {
            lastOutputTerm->cnt += tree->nodeValue[tree->nodeList[0]].cnt;
        } else {
            char entry[config->keyBufferSize + 10];
            sprintf(entry, "%s %d\n", lastOutputTerm->term, lastOutputTerm->cnt);
            fwrite(entry, strlen(entry), sizeof(char), fout);
            fprintf(fnmap, "%d\n", (int) strlen(entry));

            memset(lastOutputTerm->term, '\0', config->keyBufferSize);
            strcpy(lastOutputTerm->term, tree->nodeValue[tree->nodeList[0]].term);
            lastOutputTerm->cnt = tree->nodeValue[tree->nodeList[0]].cnt;
        }

        updateWinnerTree(tree, 0, tree->nodeList[0], fin, fmap);
    }

    for (int i = 0; i < config->chunk; i++) {
        sprintf(splitFile, "%s_%d.rec", _WINNER_TREE_SPLIT_FILE, args->endIdx - (config->chunk - 1) + i);
        sprintf(offsetFile, "%s_%d.rec", _WINNER_TREE_OFFSET_FILE, args->endIdx - (config->chunk - 1) + i);
        fclose(fin[i]);
        fclose(fmap[i]);
        remove(splitFile);
        remove(offsetFile);
    }
    fclose(fout);
    fclose(fnmap);

    return NULL;
}

void mergeKFile(int fileNum, HashConfig *conf)
{
    config = conf;
    while (_nodeNum < config->chunk) {
        _nodeNum <<= 1;
    }
    _nodeNum = 2 * _nodeNum - 1;

    int cnt = 0;
    while (fileNum - cnt > config->chunk) {
        int threadNum = (fileNum - cnt) / config->chunk;

        if (threadNum > config->thread) {
            threadNum = config->thread;
        }

        pthread_t tids[threadNum];
        pthread_barrier_init(&pbt, NULL, threadNum + 1);
        for (int i = 0; i < threadNum; i++) {
            ThreadArgs *args = (ThreadArgs *) malloc(sizeof(ThreadArgs));
            args->endIdx = cnt + config->chunk * (i + 1);
            args->newFileNo = fileNum + (i + 1);
            pthread_create(&tids[i], NULL, job, args);
        }

        for (int i = 0; i < threadNum; i++) {
            pthread_join(tids[i], NULL);
        }

        pthread_barrier_destroy(&pbt);
        cnt += threadNum * config->chunk;
        fileNum += threadNum;
    }

    FILE **fin = malloc((config->chunk + 1) * sizeof(FILE *));
    FILE **fmap = malloc((config->chunk + 1) * sizeof(FILE *));
    char splitFile[31], offsetFile[31];
    for (int i = 0; i < config->chunk; i++) {
        sprintf(splitFile, "%s_%d.rec", _WINNER_TREE_SPLIT_FILE, cnt + i + 1);
        sprintf(offsetFile, "%s_%d.rec", _WINNER_TREE_OFFSET_FILE, cnt + i + 1);
        fin[i] = fopen(splitFile, "r");
        fmap[i] = fopen(offsetFile, "r");
    }
    FILE *fout = stdout;

    TermInfo *lastOutputTerm = malloc(sizeof(TermInfo));
    lastOutputTerm->term = (char *) malloc(config->keyBufferSize);
    memset(lastOutputTerm->term, '\0', config->keyBufferSize);

    WinnerTree *tree = malloc(sizeof(WinnerTree));
    tree->nodeValue = (TermInfo *) malloc(config->chunk * sizeof(TermInfo));
    tree->nodeList = (int *) malloc(_nodeNum * sizeof(int));
    for (int i = 0; i < _nodeNum; i++) {
        tree->nodeList[i] = -1;
    }

    initWinnerTree(tree, 0, fin, fmap);
    while (true) {
        if (tree->nodeList[0] == -1) {
            fprintf(fout, "%s %d\n", lastOutputTerm->term, lastOutputTerm->cnt);
            free(lastOutputTerm->term);
            free(lastOutputTerm);
            free(tree->nodeList);
            free(tree->nodeValue);
            free(tree);
            break;
        }

        if (strcmp(lastOutputTerm->term, "\0") == 0) {
            strcpy(lastOutputTerm->term, tree->nodeValue[tree->nodeList[0]].term);
            lastOutputTerm->cnt = tree->nodeValue[tree->nodeList[0]].cnt;
        } else if (strcmp(lastOutputTerm->term, tree->nodeValue[tree->nodeList[0]].term) == 0) {
            lastOutputTerm->cnt += tree->nodeValue[tree->nodeList[0]].cnt;
        } else {
            fprintf(fout, "%s %d\n", lastOutputTerm->term, lastOutputTerm->cnt);
            memset(lastOutputTerm->term, '\0', config->keyBufferSize);
            strcpy(lastOutputTerm->term, tree->nodeValue[tree->nodeList[0]].term);
            lastOutputTerm->cnt = tree->nodeValue[tree->nodeList[0]].cnt;
        }

        updateWinnerTree(tree, 0, tree->nodeList[0], fin, fmap);
    }

    for (int i = 0; i < config->chunk; i++) {
        sprintf(splitFile, "%s_%d.rec", _WINNER_TREE_SPLIT_FILE, cnt + i + 1);
        sprintf(offsetFile, "%s_%d.rec", _WINNER_TREE_OFFSET_FILE, cnt + i + 1);
        fclose(fin[i]);
        fclose(fmap[i]);
        remove(splitFile);
        remove(offsetFile);
    }
    fclose(fout);
}