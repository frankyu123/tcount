#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/time.h>
#include "hash.h"
#include "mergesort.h"
#include "winner_tree.h"

#define _HASH_KEY_BUFFER "key_buffer"
#define _HASH_RESULT "result.rec"

static Hash *hash = NULL;
static int _batchInsertCnt = 0;
static int _totalTermCnt = 0;
static int _fileNum = 1;

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

static void initHashInfo(HashConfig *config)
{
    hash->info->topNodeIdx = 1;
    hash->info->nodeTableSize = (int) config->totalMem / config->keyBufferSize;
}

static void newHash(HashConfig *config)
{
    hash = malloc(sizeof(Hash));

    hash->hashTable = (unsigned long *) malloc(config->hashTabSize * sizeof(unsigned long));
    memset(hash->hashTable, 0, config->hashTabSize * sizeof(unsigned long));

    hash->info = (HashInfo *) malloc(sizeof(HashInfo));
    initHashInfo(config);

    hash->nodeTable = (HashNodeTable *) malloc(hash->info->nodeTableSize * sizeof(HashNodeTable));
    for (int i = 1; i < hash->info->nodeTableSize; i++) {
        hash->nodeTable[i].term = (char *) malloc(config->keyBufferSize);
        memset(hash->nodeTable[i].term, '\0', config->keyBufferSize);
        hash->nodeTable[i].cnt = 0;
        hash->nodeTable[i].next = 0;
    }
}

void clearHash()
{
    free(hash->hashTable);
    for (int i = 1; i < hash->info->nodeTableSize; i++) {
        free(hash->nodeTable[i].term);
    }
    free(hash->nodeTable);
    free(hash->info);
    free(hash);
    hash = NULL;
}

static void setHashNode(char *term, HashConfig *config)
{
    memset(hash->nodeTable[hash->info->topNodeIdx].term, '\0', config->keyBufferSize);
    strcpy(hash->nodeTable[hash->info->topNodeIdx].term, term);
    hash->nodeTable[hash->info->topNodeIdx].cnt = 1;
    ++hash->info->topNodeIdx;
}

static bool checkTermExist(char *term, unsigned long nodeIdx, bool createNewNode, HashConfig *config)
{
    if (strcmp(term, hash->nodeTable[nodeIdx].term) == 0) {
        ++hash->nodeTable[nodeIdx].cnt;
        return true;
    } else {
        if (createNewNode) {
            hash->nodeTable[nodeIdx].next = hash->info->topNodeIdx;
            setHashNode(term, config);
            return true;
        } else {
            return false;
        }
    }
}

static void writeExternalEntry(char *term, int cnt, FILE *fout, HashConfig *config)
{
    char entry[config->keyBufferSize];
    memset(entry, '\0', config->keyBufferSize);
    sprintf(entry, "%s %d\n", term, cnt);
    fwrite(entry, strlen(entry), sizeof(char), fout);
}

void writeExternalBucket(HashConfig *config)
{
    int *idx = (int *) malloc(hash->info->topNodeIdx * sizeof(int));
    for (int i = 0; i < hash->info->topNodeIdx; i++) {
        idx[i] = i;
    }
    mergeSort(&hash->nodeTable, &idx, hash->info->topNodeIdx, config->hashTabSize, config->thread);

    char splitFile[100];
    sprintf(splitFile, "%s_%d.rec", _HASH_KEY_BUFFER, _fileNum);
    FILE *fout = fopen(splitFile, "w");
    for (int i = 1; i < hash->info->topNodeIdx; i++) {
        writeExternalEntry(hash->nodeTable[idx[i]].term, hash->nodeTable[idx[i]].cnt, fout, config);
    }
    fclose(fout);
    ++_fileNum;
    ++_batchInsertCnt;
}

bool insertHash(char *term, HashConfig *config)
{
    if (hash == NULL) {
        newHash(config);
    }

    int hashVal = hash65(term) % config->hashTabSize;
    if (hash->hashTable[hashVal] == 0) {
        hash->hashTable[hashVal] = hash->info->topNodeIdx;
        setHashNode(term, config);
    } else {
        unsigned long nodeIdx = hash->hashTable[hashVal];
        bool find = false;

        while (hash->nodeTable[nodeIdx].next != 0) {
            find = checkTermExist(term, nodeIdx, false, config);
            if (find) {
                break;
            }
            nodeIdx = hash->nodeTable[nodeIdx].next;
        }

        if (!find) {
            checkTermExist(term, nodeIdx, true, config);
        } 
    }

    if (hash->info->topNodeIdx == hash->info->nodeTableSize) {
        writeExternalBucket(config);
        printf("Batch insert %d: already insert %d terms\n", getBatchInsertCnt(), getTotalTermCnt());
        clearHash();
    }

    ++_totalTermCnt;
    return true;
}

HashConfig *initHashConfig(int argc, char *argv[])
{
    HashConfig *config = malloc(sizeof(HashConfig));
    config->hashTabSize = 3000;
    config->totalMem = 500000000; // Default : approx. 500MB
    config->keyBufferSize = 2048;
    config->thread = 5;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0) {
            config->totalMem = atol(argv[i+1]);
            i += 1;
        } else if (strcmp(argv[i], "-s") == 0) {
            config->keyBufferSize = atoi(argv[i+1]);
            i += 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            config->hashTabSize = atoi(argv[i+1]);
            i += 1;
        } else if (strcmp(argv[i], "-parallel") == 0) {
            config->thread = atoi(argv[i+1]);
            i += 1;
        }
    }

    if (config->thread < 2) {
        config->thread = 2;
    }

    return config;
}

int getBatchInsertCnt()
{
    return _batchInsertCnt;
}

int getTotalTermCnt()
{
    return _totalTermCnt;
}

void mergeKBucket(HashConfig *config)
{
    initWinnerTree(_fileNum-1, config->keyBufferSize);

    while (true) {
        if (checkWinnerTreeEmpty()) {
            break;
        }
        winnerTreePop();
    }
}