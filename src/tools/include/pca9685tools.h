#pragma once


#define PCA9685_PIN_BASE 100
#define PCA9865_MIN_FREQ 0
#define PCA9865_MAX_FREQ 4046


bool setupPCA9685();
void setFrequency(int pin, int speed);