#ifndef SERVERA_H
#define SERVERA_H

#include "common.h"

// UDP port for the main server to connect to this backend server
static const int SERVERA_UDP_PORT = 21407;

// Block file to store transaction records for this backend server
static const char *SERVERA_BLOCK_FILE = "block1.txt";

#endif
