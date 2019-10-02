#ifndef _HASH_H_
#define _HASH_H_

#include <stdbool.h>
#include <sys/types.h>

typedef struct HashConfig {
    int hashTabSize;
    uint totalMem;
    int keyBufferSize;
    int thread;
} HashConfig;

typedef struct HashNodeTable {
    char *term;
    int cnt;
    int next;
} HashNodeTable;

typedef struct HashInfo {
    int topNodeIdx;
    int nodeTableSize;
} HashInfo;

typedef struct Hash {
    int *hashTable;
    HashNodeTable *nodeTable;
    HashInfo *info;
} Hash;

extern HashConfig *initHashConfig(int, char *[]);
extern void writeExternalBucket(HashConfig *);
extern bool insertHash(char *, HashConfig *);
extern void clearHash();
extern int getBatchInsertCnt();
extern int getTotalTermCnt();
extern void mergeKBucket(HashConfig *);

#endif