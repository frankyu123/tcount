#ifndef _HASH_H_
#define _HASH_H_

#include <stdbool.h>
#include <sys/types.h>

typedef struct HashConfig {
    int hashTabSize;
    uint totalMem;
    int keyBufferSize;
    int thread;
    int chunk;
} HashConfig;

typedef struct HashNodeTable {
    char *term;
    int cnt;
    int next;
    pthread_mutex_t lock;
} HashNodeTable;

typedef struct Hash {
    int *hashTable;
    HashNodeTable *nodeTable;
} Hash;

extern HashConfig *initHashConfig(int, char *[]);
extern void initHash(HashConfig *);
extern void batchInsertHash(HashConfig *);

#endif