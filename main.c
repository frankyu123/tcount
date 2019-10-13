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

#define _TMP_FILE "tmp"

typedef struct ThreadArgs {
    int idx;
    uint size;
    char *filename;
    int keyBuffer;
} ThreadArgs;

int main(int argc, char *argv[]) 
{
    HashConfig *config = initHashConfig(argc, argv);
    initHash(config);

    struct stat st;
    stat(argv[argc-1], &st);
    uint size = st.st_size / config->thread;

    pthread_t tids[config->thread];
    for (int i = 0; i < config->thread; i++) {
        ThreadArgs *args = malloc(sizeof(ThreadArgs));
        args->idx = i + 1;
        args->size = size;
        args->keyBuffer = config->keyBufferSize;
        args->filename = (char *) malloc(31);
        memset(args->filename, '\0', 31);
        strcpy(args->filename, argv[argc-1]);
        pthread_create(&tids[i], NULL, job, args);
    }

    for (int i = 0; i < config->thread; i++) {
        pthread_join(tids[i], NULL);
    }

    batchInsertHash(config);
    free(config);

    return 0;
}

void *job(void *argv)
{
    ThreadArgs *args = (ThreadArgs *) argv;
    uint start = (args->idx - 1) * args->size;

    FILE *fin = fopen(args->filename, "r");
    fseek(fin, start, SEEK_SET);

    char foutName[31];
    sprintf(foutName, "%s%02d", _TMP_FILE, args->idx);
    FILE *fout = fopen(foutName, "w");

    char line[args->keyBuffer];
    bool flag = (args->idx == 1) ? true : false;
    uint buffer = 0;
    while (buffer < args->size) {
        memset(line, '\0', args->keyBuffer);
        fgets(line, args->keyBuffer, fin);
        if (flag) {
            fwrite(line, strlen(line), sizeof(char), fout);
        } else {
            flag = true;
        }
        buffer += strlen(line);
    }

    fclose(fin);
    fclose(fout);
    return NULL;
}