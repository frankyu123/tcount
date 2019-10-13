#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <limits.h>
#include "hash.h"
#include "mergesort.h"
#include "winner_tree.h"

#ifdef __APPLE__
#include "pthread_barrier.h"
#endif

#define _HASH_KEY_BUFFER "key_buffer"
#define _HASH_OFFSET "offset"
#define _TMP_FILE "tmp"
#define _NODE_TABLE_SIZE 10000000

static Hash *hash = NULL;
static int _fileNum = 0;
static uint _memUsed = 0;
static int _topNodeIdx;
pthread_mutex_t _hashLock = PTHREAD_MUTEX_INITIALIZER;

typedef struct ThreadArgs {
    HashConfig *config;
    int idx;
} ThreadArgs;

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

HashConfig *initHashConfig(int argc, char *argv[])
{
    HashConfig *config = malloc(sizeof(HashConfig));
    config->hashTabSize = 3000;
    config->totalMem = 100000000; // Default Using MEM : approx. 100MB
    config->keyBufferSize = 2048;
    config->thread = 4;
    config->chunk = 4;

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
        } else if (strcmp(argv[i], "-chunk") == 0) {
            config->chunk = atoi(argv[i+1]);
            i += 1;
        }
    }

    if (config->thread < 2) {
        config->thread = 2;
    }

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
    _memUsed = sizeof(Hash) + config->hashTabSize * sizeof(int) + sizeof(HashConfig);
    _topNodeIdx = 1;
}

void clearHash()
{
    free(hash->hashTable);
    for (int i = 1; i < _topNodeIdx; i++) {
        free(hash->nodeTable[i].term);
        pthread_mutex_destroy(&hash->nodeTable[i].lock);
    }
    free(hash->nodeTable);
    free(hash);
    hash = NULL;
}

static void writeExternalBucket(HashConfig *config)
{
    ++_fileNum;

    int *idx = (int *) malloc(_topNodeIdx * sizeof(int));
    for (int i = 0; i < _topNodeIdx; i++) {
        idx[i] = i;
    }
    mergeSort(&hash->nodeTable, &idx, _topNodeIdx, config->hashTabSize, config->thread);

    char splitFile[31], offsetFile[31];
    sprintf(splitFile, "%s_%d.rec", _HASH_KEY_BUFFER, _fileNum);
    sprintf(offsetFile, "%s_%d.rec", _HASH_OFFSET, _fileNum);
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

static bool insertHash(char *term, HashConfig *config)
{
    pthread_mutex_lock(&(_hashLock));
    if (hash == NULL) {
        initHash(config);
    } else if (_topNodeIdx == _NODE_TABLE_SIZE) {
        fprintf(stderr, "Mem usage > node table size\n");
        exit(0);
    }
    pthread_mutex_unlock(&(_hashLock));

    int hashVal = hash65(term) % config->hashTabSize;
    if (hash->hashTable[hashVal] == 0) {
        pthread_mutex_lock(&(_hashLock));
        hash->hashTable[hashVal] = _topNodeIdx;
        hash->nodeTable[_topNodeIdx].term = strdup(term);
        hash->nodeTable[_topNodeIdx].cnt = 1;
        hash->nodeTable[_topNodeIdx].next = 0;
        pthread_mutex_init(&(hash->nodeTable[_topNodeIdx].lock), NULL);
        _memUsed += strlen(term) + sizeof(HashNodeTable);
        ++_topNodeIdx;
        pthread_mutex_unlock(&(_hashLock));
    } else {
        int nodeIdx = hash->hashTable[hashVal];
        bool find = false;
        pthread_mutex_lock(&(hash->nodeTable[nodeIdx].lock));
        while (hash->nodeTable[nodeIdx].next != 0) {
            if (strcmp(term, hash->nodeTable[nodeIdx].term) == 0) {
                ++hash->nodeTable[nodeIdx].cnt;
                find = true;
                break;
            } else {
                pthread_mutex_unlock(&(hash->nodeTable[nodeIdx].lock));
                nodeIdx = hash->nodeTable[nodeIdx].next;
                pthread_mutex_lock(&(hash->nodeTable[nodeIdx].lock));
            }
        }

        if (!find) {
            if (strcmp(term, hash->nodeTable[nodeIdx].term) == 0) {
                ++hash->nodeTable[nodeIdx].cnt;
            } else {
                pthread_mutex_lock(&(_hashLock));
                hash->nodeTable[nodeIdx].next = _topNodeIdx;
                hash->nodeTable[_topNodeIdx].term = strdup(term);
                hash->nodeTable[_topNodeIdx].cnt = 1;
                hash->nodeTable[_topNodeIdx].next = 0;
                pthread_mutex_init(&(hash->nodeTable[_topNodeIdx].lock), NULL);
                _memUsed += strlen(term) + sizeof(HashNodeTable);
                ++_topNodeIdx;
                pthread_mutex_unlock(&(_hashLock));
            }
        }
        pthread_mutex_unlock(&(hash->nodeTable[nodeIdx].lock));
    }

    pthread_mutex_lock(&(_hashLock));
    if (_memUsed + (_topNodeIdx * sizeof(int)) >= config->totalMem) {
        writeExternalBucket(config);
        clearHash();
        initHash(config);
    }
    pthread_mutex_unlock(&(_hashLock));

    return true;
}

static void *job(void *argv)
{
    ThreadArgs *args = (ThreadArgs *) argv;

    char filename[31];
    sprintf(filename, "%s%02d", _TMP_FILE, args->idx);
    FILE *fin = fopen(filename, "r");

    char *inputBuffer = (char *)malloc(args->config->keyBufferSize);
    while (fgets(inputBuffer, args->config->keyBufferSize, fin) != NULL) {
        if (inputBuffer[strlen(inputBuffer)-1] == '\n') {
            inputBuffer[strlen(inputBuffer)-1] = '\0';
        }
        insertHash(inputBuffer, args->config);
    }    

    free(inputBuffer);
    fclose(fin);
    remove(filename);
    return NULL;
}

void batchInsertHash(HashConfig *config)
{
    pthread_t tids[config->thread];
    for (int i = 0; i < config->thread; i++) {
        ThreadArgs *args = malloc(sizeof(ThreadArgs));
        args->config = config;
        args->idx = i + 1;
        pthread_create(&tids[i], NULL, job, args);
    }

    for (int i = 0; i < config->thread; i++) {
        pthread_join(tids[i], NULL);
    }

    writeExternalBucket(config);
    clearHash();
    pthread_mutex_destroy(&_hashLock);
    mergeKFile(_fileNum, config);
}