/**
 * hash.c - implement concurrent hash table
 * 
 * Author: Frank Yu <frank85515@gmail.com>
 * 
 * (C) Copyright 2019 Frank Yu
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>

#include <hash.h>
#include <mergesort.h>
#include <winner_tree.h>

#ifdef __APPLE__
#include <pthread_barrier.h>
#endif

#define _HASH_KEY_BUFFER "key_buffer"
#define _HASH_OFFSET "offset"
#define _NODE_TABLE_SIZE 10000000

static Hash *hash = NULL;
static int _fileNum = 0; // total external key buffers
static uint _memUsed = 0; // current memory used
static int _topNodeIdx; // last idx for node table
pthread_mutex_t _hashLock = PTHREAD_MUTEX_INITIALIZER; // global hash mutux lock

typedef struct ThreadArgs {
    HashConfig *config;
    int idx;
    char *filename;
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

static void usage()
{
    printf("Usage: tcount [OPTION]... [FILE]...\n");
    printf("Counting terms / sentences from giving FILE by OPTION.\n");
    printf("Example: ./tcount -m 400000000 text.txt\n\nCommand:\n");
    printf ("\
  -m                limit memory usage for tcount\n\
  -s                expected key buffer size\n\
  -h                hash table size\n\
  -chunk            number of external chunks using in merge\n\
  -parallel         number of threads needed\n\
  -o                output in specific file\n\
  --help            show tcount information\n\n");

  exit(0);
}

HashConfig *initHashConfig(int argc, char *argv[])
{
    HashConfig *config = malloc(sizeof(HashConfig));
    config->hashTabSize = 3000;
    config->totalMem = 100000000; // Default Using MEM : approx. 100MB
    config->keyBufferSize = 2048;
    config->thread = 4;
    config->chunk = 4;
    config->output = NULL;

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
        } else if (strcmp(argv[i], "--help") == 0) {
            usage();
        } else if (strcmp(argv[i], "-o") == 0) {
            config->output = strdup(argv[i+1]);
            i += 1;
        }
    }

    if (config->thread < 1) {
        config->thread = 1;
    }

    if (config->chunk < 2) {
        config->chunk = 2;
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
    mergeSort(&hash->nodeTable, &idx, _topNodeIdx, config->thread);

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

/**
 * job - pthread job for inserting terms
 * @config: hash config
 * @idx: no. of pthread
 * @filename: input file
 */
static void *job(void *argv)
{
    ThreadArgs *args = (ThreadArgs *) argv;

    struct stat st;
    stat(args->filename, &st);
    uint size = st.st_size / args->config->thread;
    uint start = (args->idx - 1) * size;

    FILE *fin = fopen(args->filename, "r");
    fseek(fin, start, SEEK_SET);

    char inputBuffer[args->config->keyBufferSize];
    bool flag = (args->idx == 1) ? true : false;
    uint buffer = 0;
    while (buffer <= size) {
        memset(inputBuffer, '\0', args->config->keyBufferSize);
        char *res = fgets(inputBuffer, args->config->keyBufferSize, fin);
        if (res == NULL) {
            break;
        }

        buffer += strlen(inputBuffer);
        if (flag) {
            if (inputBuffer[strlen(inputBuffer)-1] == '\n') {
                inputBuffer[strlen(inputBuffer)-1] = '\0';
            }
            insertHash(inputBuffer, args->config);
        } else {
            flag = true;
        }
    }
    fclose(fin);

    return NULL;
}

/**
 * batchInsertHash - main function for concurrent insert hash table
 * @filename: input file 
 * @config: hash config
 */
void batchInsertHash(char *filename, HashConfig *config)
{
    pthread_t tids[config->thread];
    for (int i = 0; i < config->thread; i++) {
        ThreadArgs *args = malloc(sizeof(ThreadArgs));
        args->config = config;
        args->idx = i + 1;
        args->filename = (char *) malloc(31);
        memset(args->filename, '\0', 31);
        strcpy(args->filename, filename);
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