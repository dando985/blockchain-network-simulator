#ifndef SERVERB_H
#define SERVERB_H

#include "common.h"

// UDP port for the main server to connect to this backend server
static const int SERVERB_UDP_PORT = 22407;

// Block file to store transaction records for this backend server
static const char *SERVERB_BLOCK_FILE = "block2.txt";

#endif
