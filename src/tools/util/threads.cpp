
#include "../include/threads.h"

pthread_t threadCreate(void *(*method)(void *), const char *description) {
	pthread_t threadId;
	int status= pthread_create(&threadId, NULL, method, NULL);
	if (status != 0) {
		printf("%s::thread create failed %d--%s\n", description, status, strerror(errno));
		exit(9);
	}
	pthread_detach(threadId);
	return threadId;
}

unsigned long long currentTimeMillis() {
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);

	return
		(unsigned long long)(currentTime.tv_sec) * 1000 +
		(unsigned long long)(currentTime.tv_usec) / 1000;
}