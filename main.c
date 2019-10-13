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

#define _BUFFER_SIZE 1024

int main(int argc, char *argv[]) 
{
    HashConfig *config = initHashConfig(argc, argv);
    initHash(config);

    char cmd[31];
    sprintf(cmd, "wc -l %s", argv[argc-1]);
    FILE *fp = popen(cmd, "r");

    char info[_BUFFER_SIZE];
    memset(info, '\0', _BUFFER_SIZE);
    fgets(info, _BUFFER_SIZE, fp);
    int line = (atoi(info) / config->thread) + 1;
    pclose(fp);

    memset(cmd, '\0', 31);
    sprintf(cmd, "split -l %d %s", line, argv[argc-1]);
    system(cmd);

    batchInsertHash(config);
    free(config);

    return 0;
}