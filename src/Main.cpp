#include "Main.h"
#include <lcd101rpi.h>
#include <unordered_map>
#include <thread>

using namespace std;

unordered_map<int, long long> lastActivation;

float crossingBellVolume = 10.0;
float crossingBellDuration = 3.0;
#define stationDelay 1500

// pca9685 pins
static int CrossingGate1;
static int CrossingGate2;
static int CrossingGate1Up;
static int CrossingGate1Dn;
static int CrossingGate2Up;
static int CrossingGate2Dn;

int TurnoutPin;
int TurnoutLeft;
int TurnoutRight;

int ads1115Handle = -1;
int ads1115Address;

int pca9635Handle = -1;
int pca9635Address;

int mcp23x17_x20_handle = -1;
int mcp23x17_x20_address;
int mcp23x17_x20_inta_pin;
int mcp23x17_x20_intb_pin;


MCP23x17_GPIO startStopButtonPin;
MCP23x17_GPIO trollingLockPin;

MCP23x17_GPIO eastApproach1Pin;
MCP23x17_GPIO eastStation1Pin;

MCP23x17_GPIO eastApproach2Pin;
MCP23x17_GPIO eastStation2Pin;

MCP23x17_GPIO westApproachPin;
MCP23x17_GPIO westStationPin;

MCP23x17_GPIO eastCrossingPin;
MCP23x17_GPIO westCrossingPin;

MCP23x17_GPIO accessoryLights;


static volatile int crossingGateTrigger = 0;

int turnoutActivationDelay = 100; // ms
volatile int turnoutCount = 0;

volatile bool isRunning = false;
volatile int timer = 120;
long long lastPress = 0;

static volatile int marqueeSpeed = 400;
static volatile int marqueePosition = 0;

static char clockMessage[128] = "stopped";
static char saveClockMessage[128];
static char dateFormat = '1';

std::time_t lastReading = (time_t)NULL;
static volatile float humidity = -9999;
static volatile float temperature = -9999;

// Input/Output RPi GPIO Pins
static volatile int dhtPin = 25;

// lcd 1602
static unsigned int lcdAddress = 0x27;
static volatile int lcdHandle;

static volatile bool showCrossingSignal = true;
PCA9635_TYPE crossingLED1;
PCA9635_TYPE crossingLED2;

enum DirectionType
{
    EAST,
    WEST,
    DIRECTIONS
};

#define STOP -1

enum SpeedType
{
    TROLLING,
    NORMAL,
    _SPEED_TYPES
};

enum TramEnum
{
    TRAM1,
    _TRAMS
};

volatile int tramSpeedConfiguration[_SPEED_TYPES][_TRAMS];

bool stationLock[DIRECTIONS] = {false, false};
static volatile bool isAccelerating[_TRAMS];
static volatile bool accelerateOverride[_TRAMS];
int tramCurrentSpeedType[_TRAMS];
int tramCurrentDirection[_TRAMS] = {STOP};
int tramCurrentSpeed[_TRAMS] = {0};

int tramSpeedInputChannel[_SPEED_TYPES][_TRAMS] = {1, 0};

int tramOutputSpeedControlPin[_TRAMS] = {
    10};

int tramDirectionPins[_TRAMS][DIRECTIONS] = {
    {9, 8}};

int tramLEDPin[_TRAMS][DIRECTIONS] = {
    {7, 6}};
PCA9635_COLOR tramLEDColor[_TRAMS][DIRECTIONS] = {
    {RED, YELLOW}};

void accelerate(int tram, int type, int ms)
{
    if (isAccelerating[tram])
    {
        printf("accelerate:: tram=%d requested acceleration, but was already accelerating\n", tram);
        accelerateOverride[tram] = true;
        while (isAccelerating[tram])
            usleep(100);
        return;
    }
    isAccelerating[tram] = true;

    int desiredSpeed = (type == STOP) ? 0 : tramSpeedConfiguration[type][tram];
    int vector;
    int stepDelay = 0;

    printf("accelerate:: tram=%d; cst=%d, rst=%d, currentSpeed=%d, desiredSpeed=%d, delay=%d; ",
           tram, tramCurrentSpeedType[tram], type, tramCurrentSpeed[tram], desiredSpeed, ms);

    tramCurrentSpeedType[tram] = type;

    if (desiredSpeed == tramCurrentSpeed[tram])
    {
        isAccelerating[tram] = false;
        printf("no change\n");
        return;
    }
    if (desiredSpeed > tramCurrentSpeed[tram])
    {
        vector = 1;
    }
    else
    {
        vector = -1;
    }

    if (ms > 0)
    {
        accelerateOverride[tram] = false;
        stepDelay = 1000.0 * ms / abs(tramCurrentSpeed[tram] - desiredSpeed);
    }
    else
    {
        accelerateOverride[tram] = true;
        vector = desiredSpeed - tramCurrentSpeed[tram];
    }

    printf(" vecotor = %2d, step delay=%d\n", vector, stepDelay);

    while (!accelerateOverride[tram] && tramCurrentSpeed[tram] != desiredSpeed)
    {
        tramCurrentSpeed[tram] += vector;
        wiringPiI2CWriteReg8(pca9635Handle, 0x02 + tramOutputSpeedControlPin[tram], 255 - tramCurrentSpeed[tram]);
        if (stepDelay > 0 && tramCurrentSpeed[tram] != desiredSpeed && !accelerateOverride[tram])
        {
            usleep(stepDelay);
        }
    }

    if (accelerateOverride[tram])
    {
        tramCurrentSpeed[tram] = desiredSpeed;
        wiringPiI2CWriteReg8(pca9635Handle, 0x02 + tramOutputSpeedControlPin[tram], 255 - tramCurrentSpeed[tram]);
        accelerateOverride[tram] = false;
    }

    isAccelerating[tram] = false;
}

void moveTram(int tram, int direction)
{
    int forwardPin = tramDirectionPins[tram][WEST];
    int reversePin = tramDirectionPins[tram][EAST];

    for (int d = EAST; d <= WEST; ++d)
    {
        pca9635SetBrightness(pca9635Handle, tramLEDPin[tram][d], tramLEDColor[tram][d], 0);
    }

    switch (direction)
    {
    case EAST:
        if (isRunning)
        {
            updateClockMessage("moving east");
            pca9635SetBrightness(pca9635Handle, tramLEDPin[tram][direction], tramLEDColor[tram][direction], 100);
            pca9635DigitalWrite(pca9635Handle, reversePin, LOW);
            pca9635DigitalWrite(pca9635Handle, forwardPin, HIGH);
        }
        break;
    case WEST:
        if (isRunning)
        {
            updateClockMessage("moving west");
            pca9635SetBrightness(pca9635Handle, tramLEDPin[tram][direction], tramLEDColor[tram][direction], 100);
            pca9635DigitalWrite(pca9635Handle, forwardPin, LOW);
            pca9635DigitalWrite(pca9635Handle, reversePin, HIGH);
        }
        break;
    default: // STOP
        updateClockMessage("stopped");
        pca9635DigitalWrite(pca9635Handle, forwardPin, LOW);
        pca9635DigitalWrite(pca9635Handle, reversePin, LOW);
    }
    tramCurrentDirection[tram] = direction;

    if (direction == STOP || !isRunning)
    {
        updateClockMessage("decelerating");
        tramCurrentSpeed[tram] = 0;
        accelerateOverride[tram];
        accelerate(tram, STOP, 0);
        strcpy(clockMessage, "stopped");
    }
}

long long updateLastActivation(int key)
{
    long long now = currentTimeMillis();
    long long last = 0;

    if (lastActivation.find(key) != lastActivation.end())
    {
        last = lastActivation[key];
    }
    lastActivation[key] = now;
    return last;
}

void crossingBell()
{
    char tmpstr[128];
    sprintf(tmpstr, "/home/wryan/bin/crossingbell.sh %3.2f %.3f", crossingBellVolume, crossingBellDuration);
    system(tmpstr);
}

void *crossingGate(void *)
{
    long long last = updateLastActivation(0x4000);

    if (crossingGateTrigger++ % 2 == 0)
    {
        // down
        printf("millis=%9d closing crossing gate\n", millis());
        fflush(stdout);

        showCrossingSignal = true;
        setFrequency(CrossingGate1, CrossingGate1Dn);
        setFrequency(CrossingGate2, CrossingGate2Dn);

        strcpy(saveClockMessage, clockMessage);
        updateClockMessage("crossing gate");

        crossingBell();
    }
    else
    {
        // up
        printf("millis=%9d opening crossing gate\n", millis());
        fflush(stdout);

        showCrossingSignal = false;
        setFrequency(CrossingGate1, CrossingGate1Up);
        setFrequency(CrossingGate2, CrossingGate2Up);
        updateClockMessage(saveClockMessage);

        // long long duration = currentTimeMillis() - last;
        // if (duration >= 3000)
        // {
        //     crossingBellDuration = (float)duration / 1000.0;
        // }
    }

    pthread_exit(NULL);
}

void crossingActivated(MCP23x17_GPIO gpio, int value)
{
    if (value != 0)
    {
        return;
    }

    if ((currentTimeMillis() - updateLastActivation(gpio)) < 2000)
    {
        pthread_exit(NULL);
    }

    if (gpio == eastCrossingPin && tramCurrentDirection[TRAM1] == WEST)
    {
        crossingGateTrigger = 0; // approaching
    }
    if (gpio == westCrossingPin && tramCurrentDirection[TRAM1] == EAST)
    {
        crossingGateTrigger = 0; // approaching
    }

    printf("millis=%9d crossing gate state=%04x\n", millis(), gpio);
    fflush(stdout);
    threadCreate(crossingGate, "crossing gate");
}

int envGetInteger(const char *var, const char *format)
{
    if (!var)
    {
        fprintf(stderr, "Could not locate NULL in the environment variables\n");
        exit(EXIT_FAILURE);
    }
    char *buf = getenv(var);
    if (buf)
    {
        int value;
        int offset = 0;
        if (strcmp(format, "%x") == 0 && strncmp(buf, "0x", 2) == 0)
        {
            offset += 2;
        }
        sscanf(&buf[offset], format, &value);
        return value;
    }
    else
    {
        fprintf(stderr, "Could not locate '%s' in the environment variables\n", var);
        exit(EXIT_FAILURE);
    }
}

void fullStop()
{
    isRunning = false;
    crossingGateTrigger = 0;
    showCrossingSignal = true;

    moveTram(TRAM1, STOP);
}

bool startStopButtonAction(char *message)
{
    long long now = currentTimeMillis();
    long long elapsed = now - lastPress;

    if (elapsed < 500)
    {
        return false;
    }
    lastPress = now;

    if (message)
    {
        printf("%s\n", message);
    }

    if (isRunning)
    {
        fullStop();
        return true;
    }
    timer = 120;

    isRunning = true;
    showCrossingSignal = false;
    setFrequency(CrossingGate1, CrossingGate1Up);
    setFrequency(CrossingGate2, CrossingGate2Up);
    stationLock[EAST] = false;
    stationLock[WEST] = false;

    printf("turn LCD LED lights on\n");
    delay(10);
    lcdLED(1);
    delay(100);

    int direction = tramCurrentDirection[TRAM1];

    if (direction == STOP)
    {
        direction = WEST;
    }
    if (!mcp23x17_digitalRead(eastStation1Pin) && !mcp23x17_digitalRead(westStationPin))
    {
        printf("both east and west stations are blocked, exiting\n");
        exit(9);
    }

    if (!mcp23x17_digitalRead(eastStation1Pin))
    {
        direction = WEST;
    }
    if (!mcp23x17_digitalRead(eastStation2Pin))
    {
        direction = WEST;
    }
    if (!mcp23x17_digitalRead(westStationPin))
    {
        direction = EAST;
    }

    moveTram(TRAM1, direction);
    accelerate(TRAM1, NORMAL, 100);
    //
    return true;
}

void startStopButton(MCP23x17_GPIO gpio, int value)
{
    if (value != 0)
    {
        return;
    }

    char message[1024];
    sprintf(message, "start/stop button pressed pin-%d tripped with value %d; occured at %llu ms", mcp23x17_getPin(gpio), value, currentTimeMillis());
    fflush(stdout);

    startStopButtonAction(message);
}

void approachActivated(MCP23x17_GPIO gpio, int value)
{
    if (value != 0)
    {
        return;
    }
    if ((currentTimeMillis() - updateLastActivation(gpio)) < 2000)
    {
        pthread_exit(NULL);
    }

    bool approaching = true;
    char direction[32];

    if (gpio == eastApproach2Pin && tramCurrentDirection[TRAM1] == WEST)
    {
        approaching = false;
    }
    if (gpio == eastApproach1Pin && tramCurrentDirection[TRAM1] == WEST)
    {
        approaching = false;
    }
    if (gpio == westApproachPin && tramCurrentDirection[TRAM1] == EAST)
    {
        approaching = false;
    }

    if (gpio == eastApproach1Pin && tramCurrentDirection[TRAM1] == EAST)
    {
        strcpy(direction, "east1");
    }
    if (gpio == eastApproach2Pin && tramCurrentDirection[TRAM1] == EAST)
    {
        strcpy(direction, "east2");
    }
    if (gpio == westApproachPin && tramCurrentDirection[TRAM1] == WEST)
    {
        strcpy(direction, "west");
    }

    printf("approaching %s station; activated pin-%d; occured at %llu ms; approaching=%d\n", direction, mcp23x17_getPin(gpio), currentTimeMillis(), approaching);
    fflush(stdout);

    if (approaching)
    {
        char bellCommand[256];
        updateClockMessage("approaching stn.");
        sprintf(bellCommand, "/home/wryan/bin/ringbell.sh %4.4s", direction);
        system(bellCommand);

        accelerate(TRAM1, TROLLING, 1000);
        //    } else {
        //        accelerate(TRAM1, NORMAL, 1500);
    }
}

void switchTurnout()
{

    int pin = (turnoutCount++) % 2;
    if (pin) {
        setFrequency(TurnoutPin, TurnoutLeft);
    } else {
        setFrequency(TurnoutPin, TurnoutRight);
    }
    delay(turnoutActivationDelay);
}

void stationActivated(MCP23x17_GPIO gpio, int value)
{
    if (value != 0)
    {
        return;
    }
    printf("station pin-%d tripped with value %d; occured at %llu ms\n", mcp23x17_getPin(gpio), value, currentTimeMillis());
    fflush(stdout);

    if (gpio == eastStation1Pin)
    {
        if (!stationLock[EAST])
        {
            moveTram(TRAM1, STOP);
            stationLock[EAST] = true;
            stationLock[WEST] = false;
            system("/home/wryan/bin/ringbell.sh east");
            updateClockMessage("east station-n");
            delay(stationDelay);
            moveTram(TRAM1, WEST);
            accelerate(TRAM1, NORMAL, 1500);
        }
    }
    else if (gpio == eastStation2Pin)
    {
        if (!stationLock[EAST])
        {
            moveTram(TRAM1, STOP);
            stationLock[EAST] = true;
            stationLock[WEST] = false;
            system("/home/wryan/bin/ringbell.sh east");
            updateClockMessage("east station-s");
            delay(stationDelay);
            moveTram(TRAM1, WEST);
            accelerate(TRAM1, NORMAL, 1500);
        }
    }
    else
    {
        if (!stationLock[WEST])
        {
            moveTram(TRAM1, STOP);
            stationLock[EAST] = false;
            stationLock[WEST] = true;
            system("/home/wryan/bin/ringbell.sh west");
            updateClockMessage("west station");
            delay(stationDelay);
            moveTram(TRAM1, EAST);
            thread(switchTurnout).detach();
            accelerate(TRAM1, NORMAL, 1500);
        }
    }
}

void loadSpeed()
{
    char filename[256];
    char buf[512];
    int type;
    int tram;
    int speed;

    sprintf(filename, "%s/.tramSpeeds", getenv("HOME"));

    FILE *speedFile = fopen(filename, "r");

    if (!speedFile)
    {
        speedFile = fopen(filename, "w");
        fclose(speedFile);
        speedFile = fopen(filename, "r");
    }

    while (fgets(buf, sizeof(buf), speedFile))
    {
        sscanf(buf, "%d,%d,%d", &type, &tram, &speed);

        if (type < 0 || type > _SPEED_TYPES)
        {
            fprintf(stderr, "unknonw speed type (%d) encountered while reading speed file %s\n", type, filename);
            continue;
        }
        if (tram < 0 || tram > _TRAMS - 1)
        {
            fprintf(stderr, "invalid tram number (%d) while reading speed file %s\n", tram, filename);
            continue;
        }

        tramSpeedConfiguration[type][tram] = speed;
    }

    fclose(speedFile);
}

void saveSpeed()
{
    char filename[256];

    sprintf(filename, "%s/.tramSpeeds", getenv("HOME"));

    FILE *speedFile = fopen(filename, "w");

    for (int tram = 0; tram < _TRAMS; ++tram)
    {
        for (int type = 0; type < _SPEED_TYPES; ++type)
        {
            fprintf(speedFile, "%d,%d,%d\n", type, tram, tramSpeedConfiguration[type][tram]);
        }
    }
    fclose(speedFile);
}

float getMaxVoltage()
{
    return 5.5;
    // 2*readVoltage(ads1115Handle, 3, 0);
}

int getSpeed(int channel)
{
    float maxVoltage = getMaxVoltage();

    if (channel < 0 || channel > 3)
    {
        fprintf(stderr, "getSpeed channel(%d) out of range\n", channel);
        exit(0);
    }

    float voltage = readVoltageSingleShot(ads1115Handle, channel, 0);

    static float offset = 0.10;

    if (voltage >= (maxVoltage - offset))
    {
        voltage = 0;
    }

    float duty = offset + (voltage / maxVoltage);

    int speed = (duty * 255);

    if (speed < 1)
        speed = 1;
    if (speed > 250)
        speed = 250;

    return speed;
}

void *timerControl(void *)
{
    while (true)
    {
        if (timer == 0)
        {
            fullStop();
        }
        --timer;
        delay(1000);
    }
}

void *speedController(void *)
{
    unsigned long counter = 0;

    while (true)
    {
        unsigned long long startTime = currentTimeMillis();
        bool updateDisk = false;

        for (int tram = 0; tram < _TRAMS; ++tram)
        {
            for (int type = 0; type < _SPEED_TYPES; ++type)
            {

                int channel = tramSpeedInputChannel[type][tram];
                int speed = getSpeed(channel);

                if (abs(speed - tramSpeedConfiguration[type][tram]) > 2)
                {

                    if (type == TROLLING || tramCurrentDirection[tram] == STOP)
                    {
                        if (isAccelerating[tram])
                        {
                            accelerateOverride[tram] = true;
                            while (accelerateOverride[tram] == true)
                                usleep(250);
                        }
                        int currentSpeed = tramCurrentSpeed[tram];
                        int currentType = tramCurrentSpeedType[tram];
                        updateDisk = true;
                        printf("speedController::tram=%d type=%d speed=%d\n", tram, type, speed);
                        tramSpeedConfiguration[type][tram] = speed;
                        accelerate(tram, type, 0);
                        tramCurrentSpeed[tram] = currentSpeed;
                        tramCurrentSpeedType[tram] = currentType;
                    }
                    else
                    {
                        if (tramCurrentSpeedType[tram] == NORMAL)
                        {
                            if (isAccelerating[tram])
                            {
                                accelerateOverride[tram] = true;
                                while (accelerateOverride[tram] == true)
                                    usleep(100);
                            }
                            updateDisk = true;
                            printf("speedController::tram=%d type=%d speed=%d\n", tram, type, speed);
                            tramSpeedConfiguration[type][tram] = speed;
                            accelerate(tram, type, 0);
                        }
                    }
                }
            }
        }
        if (updateDisk)
            saveSpeed();

        long elapsed = currentTimeMillis() - startTime;
        if (elapsed < 100)
        {
            delay(100 - elapsed);
        }
        else
        {
            printf("polling input speed channels took longer than 100 ms; acutal=%d\n", elapsed);
        }
    }
}

void *crossingSignal(void *)
{
    bool isRunning = false;

    while (true)
    {
        if (showCrossingSignal)
        {
            isRunning = true;
            for (int blinkCount = 0; blinkCount < 2; ++blinkCount)
            {
                pca9635DigitalWrite(pca9635Handle, crossingLED1, 1);
                pca9635DigitalWrite(pca9635Handle, crossingLED2, 0);
                delay(500);
                pca9635DigitalWrite(pca9635Handle, crossingLED1, 0);
                pca9635DigitalWrite(pca9635Handle, crossingLED2, 1);
                delay(500);
            }
        }
        else
        {
            if (isRunning)
            {
                pca9635DigitalWrite(pca9635Handle, crossingLED1, 1);
                pca9635DigitalWrite(pca9635Handle, crossingLED2, 1);
                isRunning = false;
            }
            delay(250);
        }
    }
}

void *readDHT22Loop(void *)
{
    float h;
    float t;

    while (true)
    {
        if (readDHT22Data(dhtPin, &h, &t))
        {
            auto now = std::chrono::system_clock::now();
            lastReading = std::chrono::system_clock::to_time_t(now);

            humidity = h;
            temperature = t;
            delay(2000);
        }
        else
        {
            delay(500);
        }
    }
}

bool setup()
{

    if (wiringPiSetup() != 0)
    {
        fprintf(stderr, "Wiring Pi could not be initialized\n");
        return false;
    }

    if (!setupPCA9685())
    {
        return false;
    }

    for (int i = 0; i < 16; ++i)
    {
        pwmWrite(PCA9685_PIN_BASE + i, 0);
    }

    TurnoutPin   = envGetInteger("TurnoutPin", "%d");
    TurnoutLeft  = envGetInteger("TurnoutLeft", "%d");
    TurnoutRight = envGetInteger("TurnoutRight", "%d");

    CrossingGate1 = envGetInteger("CrossingGate1", "%d");
    CrossingGate2 = envGetInteger("CrossingGate2", "%d");
    CrossingGate1Up = envGetInteger("CrossingGate1Up", "%d");
    CrossingGate2Up = envGetInteger("CrossingGate2Up", "%d");
    CrossingGate1Dn = envGetInteger("CrossingGate1Dn", "%d");
    CrossingGate2Dn = envGetInteger("CrossingGate2Dn", "%d");

    crossingLED1 = pca9635_getTypeFromENV("CROSSING_LED1");
    crossingLED2 = pca9635_getTypeFromENV("CROSSING_LED2");

    PCA9635_TYPE speedPinType = pca9635_getTypeFromENV("SPEED_PIN");
    PCA9635_TYPE eastDirectionPinType = pca9635_getTypeFromENV("MOVE_EAST");
    PCA9635_TYPE westDirectionPinType = pca9635_getTypeFromENV("MOVE_WEST");

    PCA9635_TYPE eastDirectionLEDPinType = pca9635_getTypeFromENV("MOVING_EAST_LED");
    PCA9635_TYPE westDirectionLEDPinType = pca9635_getTypeFromENV("MOVING_WEST_LED");

    tramOutputSpeedControlPin[0] = pca9635_getLED(speedPinType);
    tramDirectionPins[0][EAST] = pca9635_getLED(eastDirectionPinType);
    tramDirectionPins[0][WEST] = pca9635_getLED(westDirectionPinType);
    tramLEDPin[0][EAST] = pca9635_getLED(eastDirectionLEDPinType);
    tramLEDPin[0][WEST] = pca9635_getLED(westDirectionLEDPinType);
    tramLEDColor[0][EAST] = pca9635_getColor(eastDirectionLEDPinType);
    tramLEDColor[0][WEST] = pca9635_getColor(westDirectionLEDPinType);

    tramSpeedInputChannel[TROLLING][0] = envGetInteger("TROLLING_SPEED_KNOB", "%d");
    tramSpeedInputChannel[NORMAL][0] = envGetInteger("NORMAL_SPEED_KNOB", "%d");

    mcp23x17_x20_address = envGetInteger("MCP23017_ADDRESS", "%x");
    mcp23x17_x20_inta_pin = envGetInteger("MCP23017_INTA_PIN", "%d");
    mcp23x17_x20_intb_pin = envGetInteger("MCP23017_INTB_PIN", "%d");

    mcp23x17_x20_handle = mcp23x17_setup(0, mcp23x17_x20_address, mcp23x17_x20_inta_pin, mcp23x17_x20_intb_pin);
    if (mcp23x17_x20_handle < 0)
    {
        fprintf(stderr, "mcp23017 at address 0x20 could not be initialized\n");
        return false;
    }

    startStopButtonPin = getEnvMCP23x17_GPIO("START_STOP_BUTTON");
    trollingLockPin = getEnvMCP23x17_GPIO("TROLLING_LOCK");

    eastApproach1Pin = getEnvMCP23x17_GPIO("EAST_APPROACH1");
    eastStation1Pin = getEnvMCP23x17_GPIO("EAST_STATION1");

    eastApproach2Pin = getEnvMCP23x17_GPIO("EAST_APPROACH2");
    eastStation2Pin = getEnvMCP23x17_GPIO("EAST_STATION2");

    westApproachPin = getEnvMCP23x17_GPIO("WEST_APPROACH");
    westStationPin = getEnvMCP23x17_GPIO("WEST_STATION");

    eastCrossingPin = getEnvMCP23x17_GPIO("EAST_CROSSING");
    westCrossingPin = getEnvMCP23x17_GPIO("WEST_CROSSING");

    accessoryLights = getEnvMCP23x17_GPIO("ACCESSORY_LIGHTS");

    switchTurnout();
    delay(500);
    switchTurnout();
    delay(500);
    switchTurnout();



    mcp23x17_setPinInputMode(startStopButtonPin, TRUE, startStopButton);

    mcp23x17_setPinInputMode(eastApproach1Pin, TRUE, approachActivated);
    mcp23x17_setPinInputMode(eastStation1Pin, TRUE, stationActivated);

    mcp23x17_setPinInputMode(eastApproach2Pin, TRUE, approachActivated);
    mcp23x17_setPinInputMode(eastStation2Pin, TRUE, stationActivated);

    mcp23x17_setPinInputMode(westApproachPin, TRUE, approachActivated);
    mcp23x17_setPinInputMode(westStationPin, TRUE, stationActivated);

    mcp23x17_setPinInputMode(eastCrossingPin, TRUE, crossingActivated);
    mcp23x17_setPinInputMode(westCrossingPin, TRUE, crossingActivated);

    mcp23x17_setPinOutputMode(accessoryLights, 0);

    ads1115Address = envGetInteger("ADS1115_ADDRESS", "%x");
    ads1115Handle = wiringPiI2CSetup(ads1115Address);
    if (ads1115Handle < 0)
    {
        printf("ads1115 initialization failed for address 0x%02x\n", ads1115Address);
        return false;
    }

    pca9635Address = envGetInteger("PCA9635_ADDESS", "%x");
    pca9635Handle = pca9635Setup(pca9635Address);
    if (pca9635Handle < 0)
    {
        printf("pca9635 initialization failed for address 0x%02x\n", pca9635Address);
        return false;
    }

    loadSpeed();
    threadCreate(speedController, "tram1SpeedController");

    threadCreate(timerControl, "timer");

    threadCreate(readDHT22Loop, "read dht sensor");

    threadCreate(crossingSignal, "crossing signal");

    lcdHandle = lcdSetup(lcdAddress);
    if (lcdHandle < 0)
    {
        fprintf(stderr, "lcdInit failed\n");
        return false;
    }

    return true;
}

void line1()
{
    auto now = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t(now);
    char vtime[64];

    switch (dateFormat)
    {
    case '2':
        std::strftime(vtime, 64, "%a %e  %I:%M %p", std::localtime(&end_time));
        break;
    default:
        if (std::localtime(&end_time)->tm_mday < 10)
        {
            char xtime[64];
            std::strftime(xtime, 64, "%a %b %%d  %H:%M", std::localtime(&end_time));
            sprintf(vtime, xtime, std::localtime(&end_time)->tm_mday);
        }
        else
        {
            std::strftime(vtime, 64, "%a %b %e %H:%M", std::localtime(&end_time));
        }
    }

    lcdPosition(lcdHandle, 0, 0); // Position cursor on the first line in the first column
    if (strlen(vtime) > 16)
    {
        vtime[16] = 0;
    }

    lcdPuts(lcdHandle, vtime); // Print the text on the LCD at the current cursor postion
}

void updateClockMessage(const char *msg)
{
    strcpy(clockMessage, msg);

    piLock(1);

    line1();

    if (strlen(clockMessage) > 16)
    {
        clockMessage[16] = 0;
    }

    if (temperature > -999)
    {
        lcdPosition(lcdHandle, 0, 1);
        char humi[32];
        char temp[32];
        char tmpstr1[64];
        char tmpstr2[128];

        if (strlen(clockMessage) > 0)
        {
            sprintf(tmpstr2, "%32s", "");
            int p1 = (16 - strlen(clockMessage)) / 2;
            if (p1 < 0)
            {
                p1 = 0;
            }

            sprintf(&tmpstr2[p1], "%s", clockMessage);

            printf("lcdMessage: '%s'\n", tmpstr2);
            lcdPrintf(lcdHandle, "%-16.16s", tmpstr2);
        }
        else
        {
            lcdPrintf(lcdHandle, "%16s", "");
        }
    }

    piUnlock(1);
}

static char serviceMessage[4096];

const char *executeServiceCommand(const char *command)
{
    if (serviceDebug)
    {
        printf("remote command: %s\n", command);
    }

    if (strcasecmp(command, "quit") == 0)
    {
        return NULL;
    }

    if (strcasecmp(command, "SanityCheck") == 0)
    {
        return "online";
    }

    if (strcasecmp(command, "help") == 0)
    {
        memset(serviceMessage, 0, sizeof(serviceMessage));

        strcat(serviceMessage, "Valid Commands are: \n");
        strcat(serviceMessage, "   start       - Start train\n");
        strcat(serviceMessage, "   stop        - Bring train to full stop\n");
        strcat(serviceMessage, "   status      - Display current train status\n");
        strcat(serviceMessage, "   shutdown    - Bring the train system down\n");
        strcat(serviceMessage, "   lights %a   - Turn the crossing lights on or off\n");
        strcat(serviceMessage, "   temperature - Return last temperature reading\n");
        strcat(serviceMessage, "   quit        - Exit the command interface\n");
        // strcat(usage, "   \n");

        return serviceMessage;
    }

    if (strncasecmp(command, "stat", 4) == 0)
    {
        return clockMessage;
    }

    if (strncasecmp(command, "shut", 4) == 0)
    {
        fullStop();
        delay(1000);
        exit(0);
    }

    if (strncasecmp(command, "temp", 4) == 0)
    {
        char vtime[64];

        if ((long)lastReading == (long)NULL)
        {
            sprintf(serviceMessage, "reading is not available");
        }
        else
        {
            std::strftime(vtime, 64, "%a %e  %I:%M %p", std::localtime(&lastReading));
            sprintf(serviceMessage, "%s  H:%.0f%% T:%.0f%c%cF", vtime, humidity, temperature, 0xC2, 0xB0);
        }

        return serviceMessage;
    }

    if (strcasecmp(command, "start") == 0 ||
        strcasecmp(command, "stop") == 0)
    {

        char message[1024] = "start/stop command activated remotely";

        if (startStopButtonAction(message))
        {
            return "success: train start/stop action executed";
        }
        else
        {
            return "failed: train start/stop action failed";
        }
    }

    if (strncasecmp(command, "lig", 3) == 0)
    {
        if (strcasestr(command, "on"))
        {
            showCrossingSignal = true;
            lcdLED(1);
            mcp23x17_digitalWrite(accessoryLights, 0);
            return "crossing lights on";
        }
        else
        {
            showCrossingSignal = false;
            lcdLED(0);
            mcp23x17_digitalWrite(accessoryLights, 1);
            return "crossing lights off";
        }
    }

    return "unknown command";
}

void updateClock()
{
    if (isRunning)
    {
        return;
    }
    char msg[128];
    piLock(1);

    strcpy(msg, "Press Start");

    line1();

    if (temperature > -999)
    {
        lcdPosition(lcdHandle, 0, 1);
        char humi[32];
        char temp[32];
        char tmpstr1[64];
        char tmpstr2[128];
        sprintf(humi, "H:%.0f%%", humidity);
        sprintf(temp, "T:%.0f%cF", temperature, 0xdf);

        if (strlen(msg) > 0)
        {
            sprintf(tmpstr2, "%16s", "");
            sprintf(&tmpstr2[(16 - strlen(msg)) / 2], "%s", msg);
            sprintf(tmpstr1, "%-6.6s  %-6.6s  %-16.16s", humi, temp, tmpstr2);
            sprintf(tmpstr2, "%s%s", tmpstr1, tmpstr1);

            tmpstr2[marqueePosition + 16] = 0;

            lcdPuts(lcdHandle, &tmpstr2[marqueePosition++]);

            if (marqueePosition > strlen(tmpstr1) - 1)
            {
                marqueePosition = 0;
            }
        }
        else
        {
            lcdPrintf(lcdHandle, "%-8.8s%8.8s", humi, temp);
        }
    }
    piUnlock(1);
}

void *updateClockLoop(void *)
{
    while (true)
    {
        updateClock();
        delay(marqueeSpeed);
    }
}

int main(int argc, char **argv)
{
    mcp23x17_setDebug(false);

    if (!setup())
    {
        return 2;
    }
    /*
        for (int tram = 0; tram < _TRAMS; ++tram) {
            for (int type = 0; type < _SPEED_TYPES; ++type) {
                printf("tram=%d, type=%d, speed=%d\n", tram, type, tramSpeedConfiguration[type][tram]);
            }
        }
    */

    threadCreate(updateClockLoop, "update clock loop");
    threadCreate(tcpInterface, "tcp interface");

    PCA9635_TYPE systemReadyPinType = pca9635_getTypeFromENV("SYSTEM_READY_LED");
    pca9635SetBrightness(pca9635Handle, systemReadyPinType, WHITE, 10);

    printf("system ready\n");

    while (true)
    {

        fflush(stdout);
        delay(100);
    }
}
