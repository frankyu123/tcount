/**
 * tcount.c - main driver file for tcount
 * 
 * Author: Frank Yu <frank85515@gmail.com>
 * 
 * (C) Copyright 2019 Frank Yu
 * 
 * External hash table
 * 1) insert terms in hash ( open addressing )
 * 2) write external files when memory usage >= limit memory
 *    or node table is full ( default size : 10000000 )
 * 3) merge M files with K chunks ( winner tree ) 
 */

#include <tcount.h>

int main(int argc, char *argv[]) 
{
    TcountConfig *config = initTcountConfig(argc, argv);
    initHash(config->hash);

    FILE *fin;
    if (config->input != NULL) {
        fin = fopen(config->input, "r");
        if (fin == NULL) {
            fprintf(stderr, "Error: file not found\n");
            exit(2);
        }
    } else {
        fin = stdin;
    }

    char inputBuffer[config->hash->keyBufferSize];
    while (fgets(inputBuffer, config->hash->keyBufferSize, fin) != NULL) {
        if (inputBuffer[strlen(inputBuffer)-1] == '\n') {
            inputBuffer[strlen(inputBuffer)-1] = '\0';
        } 
        insertHash(inputBuffer, config->thread, config->hash);
        memset(inputBuffer, '\0', config->hash->keyBufferSize);
    }

    if (getTopNodeIdx() != 1) {
        writeExternalBucket(config->thread, config->hash);
    }
    clearHash();

    mergeKFile(getExternalKeyBufferNum(), config->chunk, config->thread, config->output);

    if (config->output != NULL) {
        free(config->output);
    }

    if (config->input != NULL) {
        free(config->input);
    }

    free(config->hash);
    free(config);
    return 0;
}

void usage()
{
    printf("Usage: tcount [OPTION]... [FILE]...\n");
    printf("Counting terms / sentences from giving FILE by OPTION.\n");
    printf("Example: ./tcount -m 400000000 text.txt\n\nCommand:\n");
    printf ("\
  -m                total memory limit\n\
  -s                expected key buffer size\n\
  -h                hash table size\n\
  -chunk            number of external chunks using in merge\n\
  -parallel         number of threads needed\n\
  -o                output in specific file\n\
  --help            show tcount information\n\n");

  exit(0);
}

TcountConfig *initTcountConfig(int argc, char *argv[])
{
    if (argc == 1) {
        fprintf(stderr, "Error: missing arguments\n");
        exit(2);
    }

    TcountConfig *config = malloc(sizeof(TcountConfig));
    config->thread = 4;
    config->chunk = 4;
    config->output = config->input = NULL;
    config->hash = initHashConfig();

    for (int i = 0; i < argc; i++) {
        bool flag = false;

        if (strcmp(argv[i], "-m") == 0) {
            config->hash->totalLimitMem = atoi(argv[i+1]);
            i += 1;
            flag = true;
        } else if (strcmp(argv[i], "-s") == 0) {
            config->hash->keyBufferSize = atoi(argv[i+1]);
            i += 1;
            flag = true;
        } else if (strcmp(argv[i], "-h") == 0) {
            config->hash->hashTabSize = atoi(argv[i+1]);
            i += 1;
            flag = true;
        } else if (strcmp(argv[i], "-parallel") == 0) {
            config->thread = atoi(argv[i+1]);
            i += 1;
            flag = true;
        } else if (strcmp(argv[i], "-chunk") == 0) {
            config->chunk = atoi(argv[i+1]);
            i += 1;
            flag = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            usage();
        } else if (strcmp(argv[i], "-o") == 0) {
            config->output = strdup(argv[i+1]);
            i += 1;
            flag = true;
        }

        if (!flag && i == argc - 1) {
            config->input = strdup(argv[i]);
        }
    }

    if (config->thread < 1) {
        fprintf(stderr, "Error: thread can not smaller than 1\n");
        exit(2);
    }

    if (config->chunk < 2) {
        fprintf(stderr, "Error: chunk can not smaller than 2\n");
        exit(2);
    }

    return config;
}