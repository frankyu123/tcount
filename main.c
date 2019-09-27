#include "main.h"

int main(int argc, char *argv[]) 
{
    struct timeval start, end, exeStart, exeEnd;
    long long startusec, endusec;
    double duration, total;
    gettimeofday(&start, NULL);
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
    printf("Batch insert %d: already insert %d terms\n", getBatchInsertCnt(), getTotalTermCnt());
    clearHash();
    free(inputBuffer);
    fclose(fin);

    if (getBatchInsertCnt() > 1) {
        mergeKBucket(config);
    } else {
        rename("key_buffer_1.rec", "result.rec");
    }
    free(config);

    gettimeofday(&end, NULL);
    startusec = start.tv_sec * 1000000 + start.tv_usec;
    endusec = end.tv_sec * 1000000 + end.tv_usec;
    duration = (double) (endusec - startusec) / 1000000.0;

    if (duration > 60.0) {
        printf("Tcount spends %d min %lf sec\n", (int) duration / 60, fmod(duration, 60.0));
    } else {
        printf("Tcount spends %lf sec\n", duration);
    }
    return 0;
}