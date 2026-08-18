#ifndef SERVERM_H
#define SERVERM_H

#include "common.h"

// UDP port for the main server to listen for backend servers
static const int SERVERM_UDP_PORT = 24407;

// UDP ports for the main server to connect to backend servers
static const int SERVERM_BACKEND_A_PORT = 21407;
static const int SERVERM_BACKEND_B_PORT = 22407;
static const int SERVERM_BACKEND_C_PORT = 23407;

// TCP port for clients to connect to the main server
static const int SERVERM_CLIENT_TCP_PORT = 25407;
static const int SERVERM_MONITOR_TCP_PORT = 26407;

#endif
