
#ifndef UTILS_H
#define UTILS_H

#define MAX_DATA_SIZE 1000		// Number of bytes in the data field of a packet
#define MAX_DATA_SIZE_DIGITS 4  // Number of digits in MAX_DATA_SIZE
#define NUM_COLONS 5			// Number of colons in a packet when represented as a byte array
#define MAX_FRAG_DIGITS 10      // Number of digits in the largest fragment number
#define MAX_FILE_NAME_SIZE 100  // Maximum file name length
#define BUFLEN 5000			    // Generic buffer size 
#include <openssl/md5.h>


typedef struct packet {
    int total_frag;
    int frag_no;
    int size;
    char filename[MAX_FILE_NAME_SIZE];
    char MD5hash[MD5_DIGEST_LENGTH + 1];
    char filedata[MAX_DATA_SIZE + 1];
} Packet;

//Gets the number of digits in a given base-10 integer
int getNumDigits(int baseTenNum);


//Extracts one colon-delimited string from mainStr and writes it to output
void extrateStrings (const char * mainStr, char * output, int * counter);

//Extracts information from the byte-array form of a packet.
void deserialize (Packet * result, const char * byteArray, int totalPacketSize);

//Creates a byte-array version of the given packet that can be transmitted using a socket. 
int serialize (char **byteArray, const Packet* packet);

#endif

