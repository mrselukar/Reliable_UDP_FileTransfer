/* deliver.c
 * This file implements the client program that sends a specified file to the server. The file to be sent, server address, server port and client list port
 * are accepted as command line arguments. Make sure to run the client and server in different directories.
 */

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "utils.h"
#include "utils.c"

/*
4 Arguments 
1. File to be transmitted 
2. server ip
3. server port
4. self port
*/
int main (int argc, char **argv) {
    struct addrinfo hints, *server_info, *my_info;
    int socketFD;
    const char *inputFileName = argv[1];
    FILE *inputFile;

    // Ensure that the correct number of command line arguments were provided
    if (argc != 5) {
        printf("Usage: deliver <server address> <server port number> <client listen port> <file name>\n");
        return 1;
    }

    memset (&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    // Get IP address of the server
    getaddrinfo(argv[2], argv[3], &hints, &server_info);

    // Get client's IP address
    getaddrinfo(NULL, argv[4], &hints, &my_info);

    if (server_info != NULL) {

        // Create socket
        socketFD = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

        struct timeval tv;

        // 1-second timeout on recv calls with socket
        tv.tv_sec = 1;

        // Not initialzingthis can cause strange errors
        tv.tv_usec = 0;
        setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

        bind(socketFD, my_info->ai_addr, my_info->ai_addrlen);

        inputFile = fopen(inputFileName, "rb");

        struct stat inputFileInfo;

        // Get filesize to calculate total number of fragments and total number of fragment digits
        stat(inputFileName, &inputFileInfo);

        int total_frag = ((inputFileInfo.st_size) / MAX_DATA_SIZE) + 1;
        int current_frag_no = 1;

        char *byteArrayPacket;
        Packet currentPacket;
        currentPacket.total_frag = total_frag;
        strncpy (currentPacket.filename, inputFileName, strlen(inputFileName) + 1);

        while (current_frag_no <= total_frag) {
            currentPacket.frag_no = current_frag_no;

            bool endOfFile = feof(inputFile);
            char nextChar;

            int i;
            for (i = 0; i < MAX_DATA_SIZE && !endOfFile; i++) {
                nextChar = getc (inputFile);
                if (feof(inputFile)) {
                    endOfFile = true;
                    i--;
                } else {
                    currentPacket.filedata[i] = nextChar;
                }
            }

            currentPacket.filedata[i] = '\0';
            currentPacket.size = i;
            memset(&currentPacket.MD5hash,'\0',MD5_DIGEST_LENGTH + 1);

            MD5((char*) currentPacket.filedata, currentPacket.size , currentPacket.MD5hash);

            int currentPacketTotalSize = serialize (&byteArrayPacket, &currentPacket);
            Packet temp;
            deserialize(&temp,byteArrayPacket ,currentPacketTotalSize);
            
            MD5((char*) temp.filedata, temp.size, temp.MD5hash);

            char new_MD5hash[MD5_DIGEST_LENGTH + 1];
            memset(&new_MD5hash,'\0',MD5_DIGEST_LENGTH + 1);
            MD5((char*) temp.filedata, temp.size, new_MD5hash);
            
            if (strcmp(currentPacket.filedata,temp.filedata) != 0)
            {
                printf("ERROR STRING MISMATCH AR SOURCE \n");
            }
            if ( strcmp(new_MD5hash,currentPacket.MD5hash) != 0 )
            {
                printf("HASH MISMATCH AT SOURCE \n");
            }

            char buf[BUFLEN];
            int ackReceived = 0;
            Packet ackPacket;

            do {
                //Transmit the packet that was just created
                int bytesSent = sendto(socketFD, byteArrayPacket, currentPacketTotalSize, 0, server_info->ai_addr, sizeof(struct sockaddr_storage));

                struct sockaddr_storage dummyVar;
                int dummyVar_len = sizeof(dummyVar);

                // Wait 1 second for ACK packet from server
                int bytesReceived = recvfrom(socketFD, buf, BUFLEN, 0, (struct sockaddr *) &dummyVar, &dummyVar_len);

                if (bytesReceived != -1) {
                    ackReceived = 1;
                    memset(&ackPacket,0, sizeof(Packet));
                    deserialize(&ackPacket, buf, bytesReceived);
                    current_frag_no = ackPacket.frag_no;
                }
                else
                {
                    printf("Timeout while sending frag %d of %d ", currentPacket.frag_no, currentPacket.total_frag);
                    Packet temp1;
                    deserialize(&temp1,byteArrayPacket ,bytesSent);

                    char new_MD5hash1[MD5_DIGEST_LENGTH + 1];
                    
                    memset(&new_MD5hash1,'\0',MD5_DIGEST_LENGTH + 1);
                    
                    MD5((char*) temp1.filedata, temp1.size, new_MD5hash1);
            
                    if (strcmp(currentPacket.filedata,temp1.filedata) != 0)
                    {
                        printf("ERROR STRING MISMATCH AR SOURCE 1 \n ");
                    }
                    if ( strcmp(new_MD5hash,currentPacket.MD5hash) != 0 )
                    {
                        printf("HASH MISMATCH AT SOURCE 1 \n ");
                    }

                }
                

            } while(ackReceived == 0 && current_frag_no <= total_frag);

            free(byteArrayPacket);
        }

        //Free the server_info and my_info linked lists from getaddrinfo calls
        freeaddrinfo(server_info);
        freeaddrinfo(my_info);
    }

    return 0;
}
