#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "mergesort.h"

#ifdef __linux__
#include <pthread.h>
#endif

#ifdef __APPLE__
#include "pthread_barrier.h"
#endif

typedef struct ThreadArgs {
    int low;
    int high;
} ThreadArgs;

static pthread_barrier_t pbt;
static HashNodeTable *result;
static int *idx = NULL;
static int _hashTableSize;

static int qcmp(const void *a,const void *b)
{
    int leftIdx = *(int *) a;
    int rightIdx = *(int *) b;
    return strcmp(result[leftIdx].term, result[rightIdx].term);
}

static bool mcmp(int leftPos, int rightPos)
{
    return (strcmp(result[idx[leftPos]].term, result[idx[rightPos]].term) <= 0) ? true : false;
}

static void *thread(void *data)
{
    ThreadArgs *args = (ThreadArgs *) data;
    int low = args->low;
    int high = args->high;
    qsort(idx + low, high - low + 1, sizeof(int), qcmp);
    pthread_barrier_wait(&pbt);
    return NULL;
}

static void merge(int low, int mid, int high)
{
    int size = high - low + 1;
    int *tmp = (int *) malloc(size * sizeof(int));
    int mainPos = 0, leftPos = low, rightPos = mid + 1;

    for (int i = 0; i < size; i++) {
        if (leftPos <= mid && rightPos <= high) {
            if (mcmp(leftPos, rightPos)) {
                tmp[i] = idx[leftPos++];
            } else {
                tmp[i] = idx[rightPos++];
            }
        } else {
            if (leftPos <= mid) {
                tmp[i] = idx[leftPos++];
            } else if (rightPos <= high) {
                tmp[i] = idx[rightPos++];
            }
        }
    }

    for (int i = 0; i < size; i++) {
        idx[low+i] = tmp[i];
    }

    free(tmp);
}

int *mergeSort(HashNodeTable **data, int **originIdx, unsigned long size, int hashTableSize, int threadNum)
{
    result = *data;
    idx = (*originIdx);
    _hashTableSize = hashTableSize;

    // Partition qsort
    pthread_t tid;
    pthread_barrier_init(&pbt, NULL, threadNum);
    for (int i = 0; i < threadNum - 1; i++) {
        ThreadArgs *args = (ThreadArgs *) malloc(sizeof(ThreadArgs));
        args->low = (i * (size - 1) / (threadNum - 1)) + 1;
        args->high = (i + 1) * (size - 1) / (threadNum - 1);
        pthread_create(&tid, NULL, thread, args);
    }
    pthread_barrier_wait(&pbt);

    // Merge
    for (int i = (threadNum - 1) / 2; i > 0; i /= 2) {
        for (int j = 0; j < i; j++) {
            int low, mid, high;
            low = (j * (size - 1) / i) + 1;
            high = (j + 1)  * (size - 1) / i;
            mid = (j * 2 + 1) * (size - 1) / (i * 2);
            merge(low, mid, high);
        }
    }

    result = NULL;
    free(result);
    return idx;
}