#pragma once
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include <ctype.h>
#include <errno.h>
#include <unistd.h>


pthread_t threadCreate(void *(*method)(void *), const char *description);

