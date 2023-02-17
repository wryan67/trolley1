#include <stdio.h>

#include <wiringPi.h>
#include <pca9685.h>


#include "../include/pca9685tools.h"

static int PCA9865_CAP = 50;
static int PCA9865_ADDRESS = 0x40;

static int pca9685fd = -1;

bool setupPCA9685() {

	if ((pca9685fd = pca9685Setup(PCA9685_PIN_BASE, PCA9865_ADDRESS, PCA9865_CAP)) <= 0) {
		printf("pca9685 setup failed!\n");
		return false;
	}
	return true;
}

void setFrequency(int pin, int frequency) {
	if (frequency < 1) {
		frequency = 0;
	}
	else if (frequency >= 4096) {
		frequency = 4096;
	}

	pwmWrite(PCA9685_PIN_BASE + pin, frequency);
}