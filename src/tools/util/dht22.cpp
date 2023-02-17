#include "../include/dht22.h"

#include <wiringPi.h>
#include <pcf8574.h>

#include <lcd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>

#define MAXTIMINGS 85


static uint8_t sizecvt(const int read)
{
	/* digitalRead() and friends from wiringpi are defined as returning a value
	< 256. However, they are returned as int() types. This is a safety function */

	if (read > 255 || read < 0)
	{
		printf("Invalid data from wiringPi library\n");
		exit(EXIT_FAILURE);
	}
	return (uint8_t)read;
}


bool readDHT22Data(int DHT22PIN, float *humidity, float *temperature)
{
	int dht22_dat[5] = { 0,0,0,0,0 };

	uint8_t laststate = HIGH;
	uint8_t counter = 0;
	uint8_t j = 0, i;

	dht22_dat[0] = dht22_dat[1] = dht22_dat[2] = dht22_dat[3] = dht22_dat[4] = 0;

	// pull pin down for 18 milliseconds
	pinMode(DHT22PIN, OUTPUT);
	digitalWrite(DHT22PIN, HIGH);
	delay(10);
	digitalWrite(DHT22PIN, LOW);
	delay(18);
	// then pull it up for 40 microseconds
	digitalWrite(DHT22PIN, HIGH);
	delayMicroseconds(40);
	// prepare to read the pin
	pinMode(DHT22PIN, INPUT);

	// detect change and read data
	for (i = 0; i < MAXTIMINGS; i++) {
		counter = 0;
		while (sizecvt(digitalRead(DHT22PIN)) == laststate) {
			counter++;
			delayMicroseconds(2);
			if (counter == 255) {
				break;
			}
		}
		laststate = sizecvt(digitalRead(DHT22PIN));

		if (counter == 255) break;

		// ignore first 3 transitions
		if ((i >= 4) && (i % 2 == 0)) {
			// shove each bit into the storage bytes
			dht22_dat[j / 8] <<= 1;
			if (counter > 16)
				dht22_dat[j / 8] |= 1;
			j++;
		}
	}

	// check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
	// print it out if data is good
	if ((j >= 40) &&
		(dht22_dat[4] == ((dht22_dat[0] + dht22_dat[1] + dht22_dat[2] + dht22_dat[3]) & 0xFF))) {
		float t, h;
		h = (float)dht22_dat[0] * 256 + (float)dht22_dat[1];
		h /= 10;
		t = (float)(dht22_dat[2] & 0x7F) * 256 + (float)dht22_dat[3];
		t /= 10.0;
		if ((dht22_dat[2] & 0x80) != 0)  t *= -1;

		piLock(0);
		*humidity = h;
		*temperature = t * 9 / 5 + 32;
		piUnlock(0);

		return true;
	}
	else
	{
		//printf("Data not good, skip\n");
		return false;
	}
}
