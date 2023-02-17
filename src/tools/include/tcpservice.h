#pragma once

// TCP interface stuff

static int  servicePort = 12300;
static bool serviceDebug = true;

// internal methods
void *tcpInterface(void *);
void signal_SIGPIPE_handler(int signum);
void serviceChildMethod(int socketDesc);

// public methods
void startTCPService();
const char* executeServiceCommand(const char* command);