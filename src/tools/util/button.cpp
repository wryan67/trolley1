#include <wiringPi.h>
#include <sys/time.h>
#include <stddef.h>
#include "../include/button.h"

bool steadyState(int pin, int state, int microseconds) {
	struct timeval tv;
	gettimeofday(&tv, NULL);

	long int start = tv.tv_usec;
	long int elapsed = 0;

	while (digitalRead(pin) == state && elapsed <= microseconds) {
		gettimeofday(&tv, NULL);
		elapsed = tv.tv_usec - start;
	}

	if (elapsed < microseconds) {
		return false;
	}
	else {
		return true;
	}
}