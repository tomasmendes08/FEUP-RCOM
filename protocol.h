#pragma once
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define DISC_CTRL       0x11
#define SET_CTRL        0x03
#define A_ADRESS        0x03
#define FLAG            0x7E
#define UA_CTRL         0x07
#define TRANSMITTER     1
#define RECEIVER        0
#define UA              0
#define SET             1
#define DISC            2
#define DATA_CTRL       3
#define FI_CTRL0        0x00
#define FI_CTRL1        0x40
#define ESC             0x7D
#define STUFF           0x20

typedef struct{

    int baudRate;                   /*Velocidade de transmissão*/
    unsigned int sequenceNumber;

}LinkLayer;

int setLinkLayerStruct();

int sendMessageSET(int fd);

int sendMessageUA(int fd);

int verifyFrame(char *message, int type);

int stateMachine(int numChars, char *value);

int llopen_transmitter(char * port);

int llopen_receiver(char * port);

int llwrite(int fd, unsigned char *buffer, int length);

int writeFrameI(int fd, unsigned char *buffer, int length);
