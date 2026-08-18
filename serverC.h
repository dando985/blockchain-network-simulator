#ifndef SERVERC_H
#define SERVERC_H

#include "common.h"

// UDP port for the main server to connect to this backend server
static const int SERVERC_UDP_PORT = 23407;

// Block file to store transaction records for this backend server
static const char *SERVERC_BLOCK_FILE = "block3.txt";

#endif
