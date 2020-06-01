#include "utils.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

int getNumDigits(int baseTenNum) {
    int retval = 0;
    while (baseTenNum != 0 )
    {
      retval += 1;
      baseTenNum /= 10;
    }
    return retval;
}

void extrateStrings (const char * mainStr, char * output, int * counter) {
    int fCounter = 0;
    while (mainStr[(*counter)]!= ':') {
        output[fCounter] = mainStr[(*counter)];
        fCounter++;
        (*counter)++;
    }
    output[fCounter] = '\0';
    (*counter)++;
    return;
}

void deserialize (Packet * result, const char * byteArray, int totalPacketSize) {
    char *totalFrag, *currFrag;
    totalFrag = (char*) malloc(MAX_FRAG_DIGITS * sizeof(char));
    currFrag = (char*) malloc(MAX_FRAG_DIGITS * sizeof(char));
    memset(totalFrag, '\0', MAX_FRAG_DIGITS);
    memset(currFrag, '\0', MAX_FRAG_DIGITS);
    char dataSize[MAX_DATA_SIZE_DIGITS];
    char fileName[MAX_FILE_NAME_SIZE];
    char MD5hash[MD5_DIGEST_LENGTH+1];
    memset(&MD5hash,'\0',MD5_DIGEST_LENGTH+1);
    char fileContent[MAX_DATA_SIZE];

    int i = 0, j = 0;

    extrateStrings(byteArray, totalFrag, &i );
    extrateStrings(byteArray, currFrag, &i );
    extrateStrings(byteArray, dataSize, &i );
    extrateStrings(byteArray, fileName, &i );
    //extrateStrings(byteArray, MD5hash, &i ) ;
    for (j = 0 ; j < MD5_DIGEST_LENGTH  ; j += 1)
    {
        MD5hash[j] = byteArray[i];
        i += 1;
    }
    MD5hash[j] = '\0';
    i += 1; // for the :
    int fCounter = 0;

    while (i < totalPacketSize) {
        result->filedata[fCounter] = byteArray[i];
        fCounter++;
        i++;
    }

    result->total_frag = atoi(totalFrag);
    result->frag_no = atoi(currFrag);
    result->size = atoi(dataSize);
    memset(&result->MD5hash,'\0',MD5_DIGEST_LENGTH + 1);
    strncpy(result->filename, fileName, strlen(fileName) + 1);
    strncpy(result->MD5hash, MD5hash, strlen(MD5hash) + 1);
    free(totalFrag);
    free(currFrag);
    return;
}

int serialize (char **byteArray, const Packet* packet) {
    int total_frag_digits = getNumDigits(packet->total_frag);
    int maxPacketSizeNoMsg = 2 * total_frag_digits + MAX_DATA_SIZE_DIGITS + strlen(packet->filename) + MD5_DIGEST_LENGTH + NUM_COLONS;
    *byteArray = (char*) malloc (sizeof(char)*(maxPacketSizeNoMsg + MAX_DATA_SIZE ));
    char arry = **byteArray;
    snprintf(*byteArray, maxPacketSizeNoMsg + 1, "%d:%d:%d:%s:%s:", packet->total_frag, packet->frag_no, packet->size, packet->filename, packet->MD5hash);

    //After processing the packet header data, we need to copy the actual file data byte by byte since file might contain null-terminating characters
    char *message = *byteArray + getNumDigits(packet->total_frag) + getNumDigits(packet->frag_no) + getNumDigits(packet->size) + strlen(packet->filename) + NUM_COLONS + MD5_DIGEST_LENGTH ;
    int extra_len = getNumDigits(packet->total_frag) + getNumDigits(packet->frag_no) + getNumDigits(packet->size) + strlen(packet->filename) + NUM_COLONS + MD5_DIGEST_LENGTH ;

    // message points to the position after the fourth colon in the byteArray at this point
    int j;
    for (j = 0; j < packet->size; j++) {
        message[j] = packet->filedata[j];
    }
    message[j] = '\0';
    int currentPacketTotalSize = getNumDigits(packet->total_frag) + getNumDigits(packet->size) + getNumDigits(packet->frag_no) + strlen(packet->filename) + packet->size + NUM_COLONS + MD5_DIGEST_LENGTH + 1;
    return currentPacketTotalSize;
}
