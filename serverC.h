#ifndef SERVERC_H
#define SERVERC_H

#include "common.h"

// Server C's UDP port to listen on
static const int SERVERC_UDP_PORT = 23407;

// Block file to store transaction records for this backend server
static const char *SERVERC_BLOCK_FILE = "block3.txt";

#endif
