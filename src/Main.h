#pragma once

#include <stdio.h>

#include <wiringPi.h>

#include <chrono>

#include <mcp23x17rpi.h>
#include <ads1115rpi.h>
#include <pca9635rpi.h>
#include <log4pi.h>

#include "threads.h"
#include "tcpservice.h"
#include "dht22.h"
#include "pca9685tools.h"

void updateClockMessage(const char* msg);
