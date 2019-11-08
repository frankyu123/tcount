#ifndef _HASH_H_
#define _HASH_H_

typedef struct HashConfig {
    int hashTabSize;
    int keyBufferSize;
    int totalLimitMem;
} HashConfig;

typedef struct HashNodeTable {
    char *term;
    int cnt;
    int next;
} HashNodeTable;

typedef struct Hash {
    int *hashTable;
    HashNodeTable *nodeTable;
} Hash;

extern int getExternalKeyBufferNum();
extern int getTopNodeIdx();
extern void writeExternalBucket(int, HashConfig *);
extern HashConfig *initHashConfig();
extern void initHash(HashConfig *);
extern void clearHash();
extern void insertHash(char *, int, HashConfig *);

#endif