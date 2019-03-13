#ifndef SHAREDFILEDEFS
#define SHAREDFILEDEFS

#include "c150debug.h"
#include "c150grading.h"
#include "c150nastydgmsocket.h"
#include "c150nastyfile.h"
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <openssl/sha.h>

using namespace C150NETWORK;

#define NAME_LENGTH 50
#define DATAGRAM_LENGTH 256
#define HASH_LENGTH 20

void compute_hash(const char* filename, unsigned char hash[]);
bool areDigits(char* networkNastiness, char* fileNastiness);
void printHash(unsigned char hash[]);

typedef char Type;
const Type FILE_PACKET = 'A';
const Type E2E_CHECK = 'B';
const Type FILE_ACK = 'E';
const Type E2E_ACK = 'C';
const Type E2E_FAIL = 'D';
const Type E2E_CONF = 'F';
const Type E2E_COMPLETE = 'G';

struct client_packet {
    Type type;
    char filename[NAME_LENGTH];
    int sessionNumber;
    int num;
    int size;
    char datagram[DATAGRAM_LENGTH];
};

struct server_packet {
    Type type;
    char filename[NAME_LENGTH];
    int sessionNumber;
    unsigned char hash[HASH_LENGTH];
};

#endif 