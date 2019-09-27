#ifndef _HASH_H_
#define _HASH_H_

#include <stdbool.h>

typedef struct HashConfig {
    int hashTabSize;
    unsigned long totalMem;
    int keyBufferSize;
    int thread;
} HashConfig;

typedef struct HashNodeTable {
    char *term;
    int cnt;
    unsigned long next;
} HashNodeTable;

typedef struct HashInfo {
    unsigned long topNodeIdx;
    unsigned long nodeTableSize;
} HashInfo;

typedef struct Hash {
    unsigned long *hashTable;
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