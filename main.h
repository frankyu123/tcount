#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include "lib/hash.h"
#include "lib/winner_tree.h"

#ifdef __APPLE__
#include "lib/pthread_barrier.h"
#endif

void *job(void *);

#endif