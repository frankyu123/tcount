/**
 * mergesort.c - implement internal mergesort
 * 
 * Author: Frank Yu <frank85515@gmail.com>
 * 
 * (C) Copyright 2019 Frank Yu
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <mergesort.h>

#ifdef __linux__
#include <pthread.h>
#endif

#ifdef __APPLE__
#include <pthread_barrier.h>
#endif

/**
 * @low: begin idx for each unsorted segment
 * @hight: last idx for each unsorted segment
 */
typedef struct ThreadArgs {
    int low;
    int high;
} ThreadArgs;

static pthread_barrier_t _pbt;
static HashNodeTable *result;
static int *idx;

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

static void *job(void *data)
{
    ThreadArgs *args = (ThreadArgs *) data;
    int low = args->low;
    int high = args->high;
    qsort(idx + low, high - low + 1, sizeof(int), qcmp);
    pthread_barrier_wait(&_pbt);
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

void mergeSort(HashNodeTable **data, int **originIdx, int size, int thread)
{
    result = *data;
    idx = (*originIdx);

    // Partition qsort
    pthread_t tids[thread];
    pthread_barrier_init(&_pbt, NULL, thread + 1);
    for (int i = 0; i < thread; i++) {
        ThreadArgs *args = (ThreadArgs *) malloc(sizeof(ThreadArgs));
        args->low = (i * (size - 1) / thread) + 1;
        args->high = (i + 1) * (size - 1) / thread;
        pthread_create(&tids[i], NULL, job, args);
    }
    pthread_barrier_wait(&_pbt);

    for (int i = 0; i < thread; i++) {
        pthread_join(tids[i], NULL);
    }

    pthread_barrier_destroy(&_pbt);

    // Merge
    for (int i = thread / 2; i > 0; i /= 2) {
        for (int j = 0; j < i; j++) {
            int low, mid, high;
            low = (j * (size - 1) / i) + 1;
            high = (j + 1)  * (size - 1) / i;
            mid = (j * 2 + 1) * (size - 1) / (i * 2);
            merge(low, mid, high);
        }
    }

    result = NULL;
    idx = NULL;
    free(result);
    free(idx);
}