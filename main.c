//***************************************************************************
// Tcount
// @Author : Frank Yu
// @Command : 
// case -m              : limit total memory usage
// case -s              : expected key buffer size
// case -h              : hash table size
// case -parallel       : number of threads
// case -chunk          : number of external chunk
//***************************************************************************

#include "main.h"

int main(int argc, char *argv[]) 
{
    HashConfig *config = initHashConfig(argc, argv);
    initHash(config);
    batchInsertHash(argv[argc-1], config);
    free(config);
    return 0;
}