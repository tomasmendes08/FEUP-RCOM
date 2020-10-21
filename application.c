#include "application.h"

int llopen(char *port, int status){
    int fd;
    if(status == TRANSMITTER)
        fd = llopen_transmitter(port);
    else
        fd = llopen_receiver(port);

    return fd;
}


int main(int argc, char** argv)
{
    int fd, c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;
    
    if ( (argc < 3) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort TRANSMITTER(1)|RECEIVER(0)\n\tex: nserial /dev/ttyS1 1\n");      
        exit(-1);
    }

    int arg = atoi(argv[2]);
    if(arg == TRANSMITTER)
        fd = llopen(argv[1], TRANSMITTER);
    else if(arg == RECEIVER)
        fd = llopen(argv[1], RECEIVER);

    /*
      O ciclo FOR e as instru��es seguintes devem ser alterados de modo a respeitar
      o indicado no gui�o
    */

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);
    return 0;
}
