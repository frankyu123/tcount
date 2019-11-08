#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <hash.h>
#include <winner_tree.h>

typedef struct TcountConfig {
    int thread;
    int chunk;
    char *output;
    char *input;
    HashConfig *hash;
} TcountConfig;

void usage();
TcountConfig *initTcountConfig(int, char *[]);

#endif