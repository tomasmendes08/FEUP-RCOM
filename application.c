#include "application.h"

ApplicationLayer applicationLayer;

int llopen(char *port, int status){
    int fd;
    if(status == TRANSMITTER)
        fd = llopen_transmitter(port);
    else
        fd = llopen_receiver(port);

    return fd;
}

int createControlPacket(char *filename, int type){
    unsigned char controlPacket[1024]="";
    
    int fd = open(filename, O_RDONLY);
    struct stat stat_buff;
    fstat(fd, &stat_buff);

    stat(filename, &stat_buff);
    applicationLayer.fileSize = stat_buff.st_size;
    applicationLayer.fileName = filename;
    
    if(type == START) controlPacket[0] = 0x02;
    else if(type == END) controlPacket[0] = 0x03;
    controlPacket[1] = 0x00;
    controlPacket[2] = sizeof(applicationLayer.fileSize);
    controlPacket[3] = (applicationLayer.fileSize >> 24) & 0xFF;
    controlPacket[4] = (applicationLayer.fileSize >> 16) & 0xFF;
    controlPacket[5] = (applicationLayer.fileSize >> 8) & 0xFF;
    controlPacket[6] = applicationLayer.fileSize & 0xFF;

    controlPacket[7] = 0x01;
    controlPacket[8] = strlen(applicationLayer.fileName);
    for(int i = 0; i < strlen(applicationLayer.fileName); i++){
        controlPacket[i+9] = applicationLayer.fileName[i];
    }

    int length = 9*sizeof(unsigned char) + applicationLayer.fileName;

    llwrite(fd, controlPacket, length);

    return 0;
}

int readControlPacket(int fd){
    char string[1024], *filename;
    int i = 1, size, name_size;
    llread(fd, string);

    char *packet = string + 4;
    
    if(packet[0]==0x02 || packet[0]==0x03) continue;
    else {
        perror("Initial packet flag");
        exit(-1);
    }
    
    if(packet[i] == (char)0x00){
        i++;
        size = (int) packet[i];
        i++;

        int j = 0;
        char *octetos = malloc(size*sizeof(char));
        for(i; i < 3 + size; i++){
            octetos[j++] += packet[i];
        } 
    }
    else{
        perror("Reading T1");
        exit(-1);
    }

    if(packet[i] == (char)0x01){
        i++;
        name_size = (int) packet[i];
        i++;

        int j = 0;
        for(i; i < size+name_size+5; i++){
            filename[j++] = packet[i];
        }
    }

    return 0;
}

int main(int argc, char** argv)
{
    int fd, c, res;
    char buf[255];
    int i, sum = 0, speed = 0;
    
    if ( (argc < 3) || 
  	     ((strcmp("/dev/ttyS10", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS11", argv[1])!=0) )) {
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

    return 0;
}
