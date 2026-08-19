#ifndef SERVERM_H
#define SERVERM_H

#include "common.h"

// Main server's UDP port
static const int SERVERM_UDP_PORT = 24407;

// Backend server's UDP port
static const int SERVERM_BACKEND_A_PORT = 21407;
static const int SERVERM_BACKEND_B_PORT = 22407;
static const int SERVERM_BACKEND_C_PORT = 23407;

// Main server's TCP ports to listen for client and monitor
static const int SERVERM_CLIENT_TCP_PORT = 25407;
static const int SERVERM_MONITOR_TCP_PORT = 26407;

#endif
