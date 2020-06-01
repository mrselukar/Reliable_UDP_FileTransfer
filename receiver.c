#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <openssl/md5.h>

#include "../utils.h"
#include "../utils.c"

#define ACK_CONTENT "ACK"
#define ACK_CONTENT_LEN 3

int main (int argc, char **argv) {
    struct addrinfo hints, *server_info;
    struct sockaddr_storage their_addr;
    int their_addrlen = sizeof(their_addr);
    int socketFD;
    char buf[BUFLEN];
    FILE * outputFile;

    if (argc != 2) {
        printf ("Usage: server <UDP listen port> ");
        return 1;
    }

    memset (&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, argv[1], &hints, &server_info);

    if (server_info != NULL) {
        socketFD = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
        bind(socketFD, server_info->ai_addr, server_info->ai_addrlen);

        freeaddrinfo(server_info);

        while (1) {
            int bytesReceived = recvfrom(socketFD, buf, BUFLEN, 0, (struct sockaddr *)&their_addr, &their_addrlen);

            Packet incomingPacket;
            memset(&incomingPacket,0, sizeof(Packet));

            deserialize(&incomingPacket, buf, bytesReceived);

            char new_MD5hash[MD5_DIGEST_LENGTH + 1];
            memset(&new_MD5hash,'\0',MD5_DIGEST_LENGTH + 1);
            MD5((char*) incomingPacket.filedata, incomingPacket.size, new_MD5hash);
            if ( strcmp(new_MD5hash,incomingPacket.MD5hash) != 0 )
            {
                printf("HASH MISMATCH at ");
                printf("Frag %d of %d \n", incomingPacket.frag_no,incomingPacket.total_frag);
                continue;
            }
            else
            {
                printf("REceived Frag %d of %d \n", incomingPacket.frag_no,incomingPacket.total_frag);
            }

            outputFile = fopen(incomingPacket.filename, "ab");

            int i;
            for(i = 0; i < incomingPacket.size; ++i) {
                fputc(incomingPacket.filedata[i], outputFile);
            }

            char *byteArrayPacket;
            Packet ackPacket;

            // Set up the ACK packet with all data
            ackPacket.total_frag = incomingPacket.total_frag;
            ackPacket.frag_no = incomingPacket.frag_no + 1;
            ackPacket.size = ACK_CONTENT_LEN;
            memset(ackPacket.MD5hash,'\0',MD5_DIGEST_LENGTH+1);
            strncpy (ackPacket.filename, incomingPacket.filename, strlen(incomingPacket.filename) + 1);
            strncpy (ackPacket.filedata, ACK_CONTENT, ACK_CONTENT_LEN + 1);

            // Get byte-array form of packet
            int currentPacketTotalSize = serialize (&byteArrayPacket, &ackPacket);

            // Send ACK packet
            int bytesSent = sendto(socketFD, byteArrayPacket, currentPacketTotalSize, 0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr_storage));

            free (byteArrayPacket);
            fclose(outputFile);
        }

    }

    return 0;
}
