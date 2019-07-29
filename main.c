#include "main.h"

const char* DICT_PATH = "./lib/cjieba/dict/jieba.dict.utf8";
const char* HMM_PATH = "./lib/cjieba/dict/hmm_model.utf8";
const char* USER_DICT = "./lib/cjieba/dict/user.dict.utf8";
const char* IDF_PATH = "./lib/cjieba/dict/idf.utf8";
const char* STOP_WORDS_PATH = "./lib/cjieba/dict/stop_words.utf8";

int main(int argc, char *argv[]) 
{
    struct timeval start, end, exeStart, exeEnd;
    long long startusec, endusec;
    double duration, total;

    gettimeofday(&exeStart, NULL);

    HashConfig *config = initHashConfig(argc, argv);

    FILE *fin = fopen(argv[argc-1], "r");
    if (fin == NULL) {
        perror("Error fopen input file ");
        exit(0);
    }

    size_t maxInputBufferSize = _DEFAULT_BUFFER_SIZE;
    char *inputBuffer = (char *)malloc(maxInputBufferSize);
    char *tmp = (char *)malloc(maxInputBufferSize);
    memset(tmp, '\0', maxInputBufferSize);

    // Insert terms in hash
    gettimeofday(&start, NULL);
    Jieba handle = NewJieba(DICT_PATH, HMM_PATH, USER_DICT, IDF_PATH, STOP_WORDS_PATH);
    while (fgets(inputBuffer, _DEFAULT_BUFFER_SIZE, fin) != NULL) {
        if (maxInputBufferSize < strlen(tmp) + strlen(inputBuffer)) {
            maxInputBufferSize = strlen(tmp) + strlen(inputBuffer) + _DEFAULT_BUFFER_SIZE;
            tmp = (char *) realloc(tmp, maxInputBufferSize);
        }
        strcat(tmp, inputBuffer);

        if (tmp[strlen(tmp)-1] == '\n') {
            size_t len = strlen(tmp);
            CJiebaWord* words = Cut(handle, tmp, len); 
            CJiebaWord* x;
            for (x = words; x && x->word; x++) {
                char term[config->keyBufferSize];
                sprintf(term, "%*.*s", (int) x->len, (int) x->len, x->word);
                if (strcmp(term, "\n") != 0 && strcmp(term, " ") != 0) {
                    insertHash(term, config);
                }
            }
            FreeWords(words);
            memset(tmp, '\0', maxInputBufferSize);
        }
    }
    writeExternalHash(config);
    printf("Batch insert %d: already insert %d terms\n", getBatchInsertCnt(), getTotalTermCnt());
    clearHash();
    free(tmp);
    free(inputBuffer);
    fclose(fin);
    gettimeofday(&end, NULL);

    startusec = start.tv_sec * 1000000 + start.tv_usec;
    endusec = end.tv_sec * 1000000 + end.tv_usec;
    duration = (double) (endusec - startusec) / 1000000.0;
    printf("Insert spends %lf sec\n", duration); 

    // Merge
    gettimeofday(&start, NULL);
    mergeExternalBucket(config);
    gettimeofday(&end, NULL);

    startusec = start.tv_sec * 1000000 + start.tv_usec;
    endusec = end.tv_sec * 1000000 + end.tv_usec;
    duration = (double) (endusec - startusec) / 1000000.0;
    printf("Merge split files spends %lf sec\n", duration);

    free(config);
    gettimeofday(&exeEnd, NULL);
    startusec = exeStart.tv_sec * 1000000 + exeStart.tv_usec;
    endusec = exeEnd.tv_sec * 1000000 + exeEnd.tv_usec;
    duration = (double) (endusec - startusec) / 1000000.0;
    printf("Tcount spends %lf sec\n", duration);

    return 0;
}