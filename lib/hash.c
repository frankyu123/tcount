#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/time.h>
#include "hash.h"

#define _HASH_KEY_BUFFER "key_buffer"
#define _HASH_RESULT "result.rec"

static Hash *hash = NULL;

static void initHashInfo()
{
    hash->info->memUsed = 0;
    hash->info->topNodeIdx = 1;
    hash->info->nodeTableSize = _DEFAULT_BUFFER_SIZE;
}

static void newHash(HashConfig *config)
{
    hash = malloc(sizeof(Hash));

    hash->hashTable = (unsigned long *) malloc(config->hashTabSize * sizeof(unsigned long));
    memset(hash->hashTable, 0, _DEFAULT_BUFFER_SIZE * sizeof(unsigned long));

    hash->info = (HashInfo *) malloc(sizeof(HashInfo));
    initHashInfo();

    hash->nodeTable = (HashNodeTable *) malloc(hash->info->nodeTableSize * sizeof(HashNodeTable));
    for (int i = 1; i < hash->info->nodeTableSize; i++) {
        hash->nodeTable[i].word = (char *) malloc(config->keyBufferSize);
        memset(hash->nodeTable[i].word, '\0', config->keyBufferSize);
        hash->nodeTable[i].isWriteExternal = false;
        hash->nodeTable[i].cnt = 0;
        hash->nodeTable[i].next = 0;
    }
}

void clearHash()
{
    free(hash->hashTable);
    for (int i = 1; i < hash->info->nodeTableSize; i++) {
        free(hash->nodeTable[i].word);
    }
    free(hash->nodeTable);
    free(hash->info);
    free(hash);
    hash = NULL;
}

static int hash65(char *word, HashConfig *config)
{
    unsigned long value = 17;
    for (int i = 0; i < strlen(word); i++) {
        value = (value << 6) + value + word[i];
    }
    return value % config->hashTabSize;
}

static void setHashNode(char *word)
{
    strcpy(hash->nodeTable[hash->info->topNodeIdx].word, word);
    hash->nodeTable[hash->info->topNodeIdx].cnt = 1;
    hash->info->memUsed += strlen(word);
    ++hash->info->topNodeIdx;
}

static bool checkTermExist(char *word, unsigned long nodeIdx, bool createNewNode)
{
    if (strcmp(word, hash->nodeTable[nodeIdx].word) == 0) {
        ++hash->nodeTable[nodeIdx].cnt;
        return true;
    } else {
        if (createNewNode) {
            hash->nodeTable[nodeIdx].next = hash->info->topNodeIdx;
            setHashNode(word);
            return true;
        } else {
            return false;
        }
    }
}

static void writeExternalEntry(char *word, int cnt, FILE *fout, HashConfig *config)
{
    char entry[config->keyBufferSize];
    memset(entry, '\0', config->keyBufferSize);
    sprintf(entry, "%s %d\n", word, cnt);
    fwrite(entry, strlen(entry), sizeof(char), fout);
}

static void writeExternalBucket(int hashVal, HashConfig *config)
{
    FILE *fout = NULL, *fin = NULL;
    bool isNewBucket = false;

    char splitFile[100];
    sprintf(splitFile, "%s_%d.rec", _HASH_KEY_BUFFER, hashVal);
    fin = fopen(splitFile, "r");
    if (fin == NULL) {
        fout = fopen(splitFile, "a+");
        isNewBucket = true;
    } else {
        char tmpFile[100];
        sprintf(tmpFile, "%s.bak", splitFile);
        fout = fopen(tmpFile, "a+");
    }

    if (!isNewBucket) {
        char word[config->keyBufferSize];
        int cnt, total = 0, completeCnt = 0;
        bool isBegin = true;
        while (fscanf(fin, "%s %d\n", word, &cnt) != EOF) {
            if (!isBegin && total == completeCnt) {
                writeExternalEntry(word, cnt, fout, config);
            } else {
                bool find = false;

                unsigned long nodeIdx = hash->hashTable[hashVal];
                while (hash->nodeTable[nodeIdx].next != 0) {
                    if (strcmp(word, hash->nodeTable[nodeIdx].word) == 0) {
                        cnt += hash->nodeTable[nodeIdx].cnt;
                        hash->nodeTable[nodeIdx].isWriteExternal = true;
                        writeExternalEntry(word, cnt, fout, config);
                        find = true;
                        completeCnt += 1;
                        break;
                    }
                    nodeIdx = hash->nodeTable[nodeIdx].next;
                }

                if (!find) {
                    if (strcmp(word, hash->nodeTable[nodeIdx].word) == 0) {
                        cnt += hash->nodeTable[nodeIdx].cnt;
                        hash->nodeTable[nodeIdx].isWriteExternal = true;
                        completeCnt += 1;
                    }
                    writeExternalEntry(word, cnt, fout, config);
                }

                if (isBegin) {
                    nodeIdx = hash->hashTable[hashVal];
                    while (hash->nodeTable[nodeIdx].next != 0) {
                        ++total;
                        nodeIdx = hash->nodeTable[nodeIdx].next;
                    }
                    ++total;
                    isBegin = false;
                }
            }
        }
    }

    unsigned long nodeIdx = hash->hashTable[hashVal];
    while (hash->nodeTable[nodeIdx].next != 0) {
        if (!hash->nodeTable[nodeIdx].isWriteExternal) {
            writeExternalEntry(hash->nodeTable[nodeIdx].word, hash->nodeTable[nodeIdx].cnt, fout, config);
        }
        nodeIdx = hash->nodeTable[nodeIdx].next;
    }
    
    if (!hash->nodeTable[nodeIdx].isWriteExternal) {
        writeExternalEntry(hash->nodeTable[nodeIdx].word, hash->nodeTable[nodeIdx].cnt, fout, config);
    }

    fclose(fin);
    fclose(fout);

    if (!isNewBucket) {
        char tmpFile[100];
        sprintf(tmpFile, "%s.bak", splitFile);
        remove(splitFile);
        rename(tmpFile, splitFile);
    }
}

void writeExternalHash(HashConfig *config)
{
    for (int i = 0; i < config->hashTabSize; i++) {
        if (hash->hashTable[i] != 0) {
            writeExternalBucket(i, config);
        }
    }
}

bool insertHash(char *word, HashConfig *config)
{
    if (hash == NULL) {
        newHash(config);
    } else if (hash->info->topNodeIdx == hash->info->nodeTableSize) {
        hash->info->nodeTableSize *= 2;
        hash->nodeTable = (HashNodeTable *) realloc(hash->nodeTable, hash->info->nodeTableSize * sizeof(HashNodeTable));
        for (int i = hash->info->topNodeIdx; i < hash->info->nodeTableSize; i++) {
            hash->nodeTable[i].word = (char *) malloc(config->keyBufferSize);
            memset(hash->nodeTable[i].word, '\0', config->keyBufferSize);
            hash->nodeTable[i].isWriteExternal = false;
            hash->nodeTable[i].cnt = 0;
            hash->nodeTable[i].next = 0;
        }
    }

    int hashVal = hash65(word, config);
    if (hash->hashTable[hashVal] == 0) {
        hash->hashTable[hashVal] = hash->info->topNodeIdx;
        setHashNode(word);
    } else {
        unsigned long nodeIdx = hash->hashTable[hashVal];
        bool find = false;

        while (hash->nodeTable[nodeIdx].next != 0) {
            find = checkTermExist(word, nodeIdx, false);
            if (find) {
                break;
            }
            nodeIdx = hash->nodeTable[nodeIdx].next;
        }

        if (!find) {
            checkTermExist(word, nodeIdx, true);
        } 
    }

    if (hash->info->memUsed >= config->totalMem) {
        writeExternalHash(config);
        clearHash();
    }

    return true;
}

bool mergeExternalBucket(HashConfig *config)
{
    char cmd[500], removeFile[100];

    sprintf(removeFile, "%s*", _HASH_KEY_BUFFER);
    sprintf(cmd, "cat %s > %s", removeFile, _HASH_RESULT);
    system(cmd);

    memset(cmd, '\0', 500);
    sprintf(cmd, "rm -rf %s", removeFile);
    system(cmd);

    return true;
}

HashConfig *initHashConfig(int argc, char *argv[])
{
    HashConfig *config = malloc(sizeof(HashConfig));
    config->hashTabSize = 1000;
    config->totalMem = 2000000;
    config->keyBufferSize = 300;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0) {
            config->totalMem = atol(argv[i+1]);
        } else if (strcmp(argv[i], "-s") == 0) {
            config->keyBufferSize = atoi(argv[i+1]);
        } else if (strcmp(argv[i], "-h") == 0) {
            config->hashTabSize = atoi(argv[i+1]);
        }
    }

    return config;
}