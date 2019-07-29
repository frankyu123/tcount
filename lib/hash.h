#ifndef _HASH_H_
#define _HASH_H_

#include <stdbool.h>
#define _DEFAULT_BUFFER_SIZE 1024

typedef struct HashConfig {
    int hashTabSize;
    unsigned long totalMem;
    int keyBufferSize;
} HashConfig;

typedef struct HashNodeTable {
    char *word;
    int cnt;
    bool isWriteExternal;
    unsigned long next;
} HashNodeTable;

typedef struct HashInfo {
    unsigned long topNodeIdx;
    unsigned long memUsed;
    unsigned long nodeTableSize;
} HashInfo;

typedef struct Hash {
    unsigned long *hashTable;
    HashNodeTable *nodeTable;
    HashInfo *info;
} Hash;

extern HashConfig *initHashConfig(int, char *[]);
extern void writeExternalHash(HashConfig *);
extern bool insertHash(char *, HashConfig *);
extern void clearHash();
extern bool mergeExternalBucket(HashConfig *);

#endif