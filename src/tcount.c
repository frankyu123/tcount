/**
 * tcount.c - main driver file for tcount
 * 
 * Author: Frank Yu <frank85515@gmail.com>
 * 
 * (C) Copyright 2019 Frank Yu
 * 
 * External hash table
 * 1) concurrent insert terms in N threads
 * 2) write external key buffer when current mem usage >= limit mem usage
 * 3) merge M files with K chunks (winner tree) 
 */

#include <tcount.h>

int main(int argc, char *argv[]) 
{
    HashConfig *config = initHashConfig(argc, argv);
    initHash(config);
    batchInsertHash(config);

    if (config->output != NULL) {
        free(config->output);
    }

    if (config->input != NULL) {
        free(config->input);
    }

    free(config);
    return 0;
}