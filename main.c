//***************************************************************************
// Tcount Command
// @Description : 
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

    FILE *fin = fopen(argv[argc-1], "r");
    if (argc == 1 || fin == NULL) {
        fin = stdin;
    }

    char *inputBuffer = (char *)malloc(config->keyBufferSize);
    while (fgets(inputBuffer, config->keyBufferSize, fin) != NULL) {
        if (inputBuffer[strlen(inputBuffer)-1] == '\n') {
            inputBuffer[strlen(inputBuffer)-1] = '\0';
        }
        insertHash(inputBuffer, config);
    }
    writeExternalBucket(config);
    clearHash();
    free(inputBuffer);
    fclose(fin);

    mergeKFile(getFileNum(), config);
    free(config);
    return 0;
}