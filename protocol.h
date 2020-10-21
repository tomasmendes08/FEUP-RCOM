#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define SET_CTRL        0x03
#define A_ADRESS        0x03
#define FLAG            0x7E
#define UA_CTRL         0x07
#define TRANSMITTER     1
#define RECEIVER        0


int sendMessageSET(int fd);

int sendMessageUA(int fd);

int stateMachine(int numChars, char *value);

int llopen_transmitter(char * port);

int llopen_receiver(char * port);
