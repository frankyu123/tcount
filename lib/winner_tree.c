#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "winner_tree.h"

#define _WINNER_TREE_SPLIT_FILE "key_buffer"
#define _WINNER_TREE_RESULT_FILE "result.rec"

typedef struct WinnerTreeNode {
    TermInfo *value;
    int fileIdx;
} WinnerTreeNode;

// File variables
static FILE **fin;
static FILE *fout;
static int _fileNum;
static int _keyBufferSize;
static int _currentFileIdx = 0;
static TermInfo *lastOutputTerm;

// Tree variables
static int *nodeList;
static WinnerTreeNode *nodeValue;

static TermInfo *getTermInfo(int fileIdx)
{
    TermInfo *data = malloc(sizeof(TermInfo));
    data->term = (char *) malloc(_keyBufferSize);
    memset(data->term, '\0', _keyBufferSize);

    char *inputBuffer = malloc(_keyBufferSize);
    if (fgets(inputBuffer, _keyBufferSize, fin[fileIdx]) == NULL) {
        return NULL;
    } 

    char *ret = strrchr(inputBuffer, ' ');
    data->cnt = atoi(ret);
    strncpy(data->term, inputBuffer, strlen(inputBuffer) - strlen(ret));

    free(inputBuffer);
    return data;
}

static bool nodeCmp(TermInfo *left, TermInfo *right) 
{
    return (strcmp(left->term, right->term) <= 0) ? true : false;
}

static void winnerTreeInsert(int nodeIdx, int totalNode)
{
    if (nodeIdx >= totalNode) {
        return;
    }

    int leftIdx = 2 * nodeIdx + 1;
    int rightIdx = 2 * nodeIdx + 2;
    winnerTreeInsert(leftIdx, totalNode);
    winnerTreeInsert(rightIdx, totalNode);

    if (leftIdx >= totalNode && rightIdx >= totalNode && _currentFileIdx < _fileNum) {
        nodeList[nodeIdx] = _currentFileIdx;
        nodeValue[_currentFileIdx].value = getTermInfo(_currentFileIdx);
        ++_currentFileIdx;
    } else if (leftIdx >= totalNode && rightIdx >= totalNode && _currentFileIdx >= _fileNum) {
        nodeList[nodeIdx] = -1;
    } else {
		if (nodeList[leftIdx] == -1 && nodeList[rightIdx] == -1) {
			nodeList[nodeIdx] = -1;
		} else if (nodeList[leftIdx] != -1 && nodeList[rightIdx] == -1) {
			nodeList[nodeIdx] = nodeList[leftIdx];
		} else if (nodeList[leftIdx] == -1 && nodeList[rightIdx] != -1) {
			nodeList[nodeIdx] = nodeList[rightIdx];
		} else {
			if (nodeCmp(nodeValue[nodeList[leftIdx]].value, nodeValue[nodeList[rightIdx]].value)) {
				nodeList[nodeIdx] = nodeList[leftIdx];
			} else {
				nodeList[nodeIdx] = nodeList[rightIdx];
			}
		}
    }
}

void initWinnerTree(int fileNum, int keyBufferSize)
{
    _fileNum = fileNum;
    _keyBufferSize = keyBufferSize;
    lastOutputTerm = (TermInfo *) malloc(sizeof(TermInfo));
    lastOutputTerm->term = (char *) malloc(_keyBufferSize);
    memset(lastOutputTerm->term, '\0', _keyBufferSize);

    fin = (FILE **) malloc((_fileNum + 1) * sizeof(FILE *));
    char filename[31]; 
    for (int i = 1; i <= _fileNum; i++) {
        sprintf(filename, "%s_%d.rec", _WINNER_TREE_SPLIT_FILE, i);
        fin[i-1] = fopen(filename, "r");
    }
    fout = fopen(_WINNER_TREE_RESULT_FILE, "w");

    int nodeNum = 1;
    while (nodeNum < _fileNum) {
        nodeNum <<= 1;
    }

    nodeList = (int *) malloc((2 * nodeNum - 1) * sizeof(int));
    for (int i = 0; i < 2 * nodeNum - 1; i++) {
        nodeList[i] = -1;
    }

    nodeValue = (WinnerTreeNode *) malloc(_fileNum * sizeof(WinnerTreeNode));
    for (int i = 0; i < _fileNum; i++) {
        nodeValue[i].fileIdx = i + 1;
    }

    winnerTreeInsert(0, 2 * nodeNum - 1);
}

bool checkWinnerTreeEmpty()
{
    if (nodeList[0] == -1) {
        fprintf(fout, "%s %d\n", lastOutputTerm->term, lastOutputTerm->cnt);
        free(lastOutputTerm->term);
        free(lastOutputTerm);
        fflush(fout);
        fclose(fout);
        free(nodeList);
        free(nodeValue);
        return true;
    } else {
        return false;
    }
}

static void winnerTreeUpdate(int nodeIdx, int updateIdx, int totalNode)
{
    if (nodeIdx >= totalNode) {
        return;
    }

    int leftIdx = 2 * nodeIdx + 1;
    int rightIdx = 2 * nodeIdx + 2;
    winnerTreeUpdate(leftIdx, updateIdx, totalNode);
    winnerTreeUpdate(rightIdx, updateIdx, totalNode);

	if (leftIdx >= totalNode &&  rightIdx >= totalNode && nodeList[nodeIdx] == updateIdx) {
		TermInfo *newData = getTermInfo(updateIdx);
		if (newData != NULL) {
            free(nodeValue[updateIdx].value->term);
            free(nodeValue[updateIdx].value);
			nodeValue[updateIdx].value = newData;
		} else {
            char filename[31]; 
            sprintf(filename, "%s_%d.rec", _WINNER_TREE_SPLIT_FILE, nodeValue[updateIdx].fileIdx);
            fclose(fin[updateIdx]);
			
            free(nodeValue[updateIdx].value->term);
            free(nodeValue[updateIdx].value);
            nodeList[nodeIdx] = -1;

            free(newData);
			remove(filename);
			printf("Sucessfully merge %s\n", filename);
		}
	} else if (leftIdx < totalNode &&  rightIdx < totalNode && nodeList[nodeIdx] != -1) {
		if (nodeList[leftIdx] == -1 && nodeList[rightIdx] == -1) {
			nodeList[nodeIdx] = -1;
		} else if (nodeList[leftIdx] != -1 && nodeList[rightIdx] == -1) {
			nodeList[nodeIdx] = nodeList[leftIdx];
		} else if (nodeList[leftIdx] == -1 && nodeList[rightIdx] != -1) {
			nodeList[nodeIdx] = nodeList[rightIdx];
		} else {
			if (nodeCmp(nodeValue[nodeList[leftIdx]].value, nodeValue[nodeList[rightIdx]].value)) {
				nodeList[nodeIdx] = nodeList[leftIdx];
			} else {
				nodeList[nodeIdx] = nodeList[rightIdx];
			}
		}
	}
}

void winnerTreePop()
{
    if (strcmp(lastOutputTerm->term, "\0") == 0) {
        strcpy(lastOutputTerm->term, nodeValue[nodeList[0]].value->term);
        lastOutputTerm->cnt = nodeValue[nodeList[0]].value->cnt;
    } else if (strcmp(lastOutputTerm->term, nodeValue[nodeList[0]].value->term) == 0) {
        lastOutputTerm->cnt += nodeValue[nodeList[0]].value->cnt;
    } else {
        fprintf(fout, "%s %d\n", lastOutputTerm->term, lastOutputTerm->cnt);
        memset(lastOutputTerm->term, '\0', _keyBufferSize);
        strcpy(lastOutputTerm->term, nodeValue[nodeList[0]].value->term);
        lastOutputTerm->cnt = nodeValue[nodeList[0]].value->cnt;
    }

    int nodeNum = 1;
    while (nodeNum < _fileNum) {
        nodeNum <<= 1;
    }

    winnerTreeUpdate(0, nodeList[0], 2 * nodeNum - 1);
}