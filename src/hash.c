/**
 * hash.c - implement open addressing hash table
 * 
 * Author: Frank Yu <frank85515@gmail.com>
 * 
 * (C) Copyright 2019 Frank Yu
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <hash.h>
#include <mergesort.h>

#define _HASH_KEY_BUFFER "_buffer"
#define _HASH_OFFSET "_offset"
#define _NODE_TABLE_SIZE 10000000

static Hash *hash = NULL;
static int _fileNum = 0;
static int _topNodeIdx;
static uint _memUsed = 0;

static uint hash65(char *term)
{
    uint hashVal = 61;
    int i;
    char *ptr = term;

    while ((i = (*ptr++))) {
        hashVal = (hashVal << 6) + hashVal + i;
    }

    ptr = NULL;
    free(ptr);
    return hashVal;
}

int getExternalKeyBufferNum()
{
    return _fileNum;
}

int getTopNodeIdx()
{
    return _topNodeIdx;
}

HashConfig *initHashConfig()
{
    HashConfig *config = malloc(sizeof(HashConfig));
    config->hashTabSize = 500000;
    config->keyBufferSize = 2048;
    config->totalLimitMem = 400000000;
    return config;
}

void initHash(HashConfig *config)
{
    hash = malloc(sizeof(Hash));
    hash->nodeTable = (HashNodeTable *) malloc(_NODE_TABLE_SIZE * sizeof(HashNodeTable));
    hash->hashTable = (int *) malloc(config->hashTabSize * sizeof(int));
    for (int i = 0; i < config->hashTabSize; i++) {
        hash->hashTable[i] = 0;
    }
    _topNodeIdx = 1;
    _memUsed = sizeof(Hash) + config->hashTabSize * sizeof(int);
}

void clearHash()
{
    free(hash->hashTable);
    for (int i = 1; i < _topNodeIdx; i++) {
        free(hash->nodeTable[i].term);
    }
    free(hash->nodeTable);
    free(hash);
    hash = NULL;
}

void writeExternalBucket(int thread, HashConfig *config)
{
    ++_fileNum;

    int *idx = (int *) malloc(_topNodeIdx * sizeof(int));
    for (int i = 0; i < _topNodeIdx; i++) {
        idx[i] = i;
    }
    mergeSort(&hash->nodeTable, &idx, _topNodeIdx, thread);

    char splitFile[31], offsetFile[31];
    sprintf(splitFile, "%s_%d", _HASH_KEY_BUFFER, _fileNum);
    sprintf(offsetFile, "%s_%d", _HASH_OFFSET, _fileNum);
    FILE *fout = fopen(splitFile, "w");
    FILE *fmap = fopen(offsetFile, "w");

    char entry[config->keyBufferSize + 10];
    for (int i = 1; i < _topNodeIdx; i++) {
        memset(entry, '\0', config->keyBufferSize + 10);
        sprintf(entry, "%s %d\n", hash->nodeTable[idx[i]].term, hash->nodeTable[idx[i]].cnt);
        fwrite(entry, strlen(entry), sizeof(char), fout);
        fprintf(fmap, "%d\n", (int) strlen(entry));
    }

    fclose(fout);
    fclose(fmap);
    free(idx);
}

void insertHash(char *term, int thread, HashConfig *config)
{
    if (hash == NULL) {
        initHash(config);
    }

    int hashVal = hash65(term) % config->hashTabSize;
    if (hash->hashTable[hashVal] == 0) {
        hash->hashTable[hashVal] = _topNodeIdx;
        hash->nodeTable[_topNodeIdx].term = strdup(term);
        hash->nodeTable[_topNodeIdx].cnt = 1;
        hash->nodeTable[_topNodeIdx].next = 0;
        ++_topNodeIdx;
        _memUsed += strlen(term) + sizeof(HashNodeTable);
    } else {
        int nodeIdx = hash->hashTable[hashVal];
        bool find = false;
        while (hash->nodeTable[nodeIdx].next != 0) {
            if (strcmp(term, hash->nodeTable[nodeIdx].term) == 0) {
                ++hash->nodeTable[nodeIdx].cnt;
                find = true;
                break;
            } else {
                nodeIdx = hash->nodeTable[nodeIdx].next;
            }
        }

        if (!find) {
            if (strcmp(term, hash->nodeTable[nodeIdx].term) == 0) {
                ++hash->nodeTable[nodeIdx].cnt;
            } else {
                hash->nodeTable[nodeIdx].next = _topNodeIdx;
                hash->nodeTable[_topNodeIdx].term = strdup(term);
                hash->nodeTable[_topNodeIdx].cnt = 1;
                hash->nodeTable[_topNodeIdx].next = 0;
                ++_topNodeIdx;
                _memUsed += strlen(term) + sizeof(HashNodeTable);
            }
        }
    }

    if (_memUsed >= config->totalLimitMem || _topNodeIdx == _NODE_TABLE_SIZE) {
        writeExternalBucket(thread, config);
        clearHash();
        initHash(config);
    }
}