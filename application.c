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

void readFileData(char *filename){

    int fd = open(filename, O_RDONLY);
    struct stat stat_buff;
    fstat(fd, &stat_buff);

    printf("fd: %d\n", fd);
    applicationLayer.fileDes = fd;
    applicationLayer.fileSize = stat_buff.st_size;
    applicationLayer.fileName = filename;
}

int createControlPacket(int fd, int type){
    unsigned char controlPacket[256];
    
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

    int length = 9*sizeof(unsigned char) + strlen(applicationLayer.fileName);

    llwrite(fd, controlPacket, length);

    return length;
}

int readControlPacket(int fd){
    unsigned char packet[256];
    int i = 1, file_size, filename_size;
    char *filename;
    llread(fd, packet);
    
    if(!(packet[0]==(char)0x02 || packet[0]==(char)0x03)){
        perror("Initial packet flag");
        exit(-1);
    }
    else{
        if(packet[i] == (char)0x00){
            i+=2;
            file_size += packet[i] << 24;
            i++;
            file_size += packet[i] << 16;
            i++;
            file_size += packet[i] << 8;
            i++;
            file_size += packet[i];
        }

        if(packet[i] == (char)0x01){
            i++;
            filename_size = (int) packet[i];
            i++;

            filename = (char *) malloc(filename_size+1);
            for (size_t j = 0; j < filename_size; j++)
            {
                filename[j] = packet[i];
                i++;
                if(j = filename_size -1){
                    filename[j] = '\0';
                    i++;
                } 
            }
            
        }
    }

    applicationLayer.fileDesNewFile = open(applicationLayer.fileDestName, O_CREAT | O_WRONLY | O_APPEND, 0664);
    
    return 0;
}

int createDataPacket(){
    char buffer[applicationLayer.packetSize];
    int packets_to_send = applicationLayer.fileSize/applicationLayer.packetSize;
    int read_bytes = 0, packets_sent = 0;
    int lastbyte = applicationLayer.fileSize % applicationLayer.packetSize, flag = FALSE;
    if(lastbyte != 0){
        flag = TRUE;
    }
    //printf("packets_to_send: %d\n", flag?packets_to_send+1:packets_to_send);
    //printf("Packets Sent: %d\n Packets to send: %d\n Lastbyte: %d\n Packet Size: %d\n", packets_sent, packets_to_send, lastbyte, applicationLayer.packetSize);
    while(packets_sent <= packets_to_send){
        int count = 0;
        if((packets_sent == packets_to_send) && flag){
            while(count < lastbyte){
                if(read(applicationLayer.fileDes, buffer, 1) == -1){
                    perror("Error reading");
                    read_bytes++;
                }
                count++;
            }
        }
        else{

            while(count < applicationLayer.packetSize){
                if(read(applicationLayer.fileDes, buffer, 1) == -1){
                    perror("Error reading");
                    read_bytes++;
                }
                count++;
            }
        }

        unsigned char packet[((packets_sent == packets_to_send) && flag)?lastbyte+4:applicationLayer.packetSize+4];
        packet[0] = DATA;
        packet[1] = packets_sent;
        packets_sent++;
        packet[2] = read_bytes/256;
        packet[3] = read_bytes%256;

        for(int i = 4; i < ((packets_sent == packets_to_send) && flag)?lastbyte+4:applicationLayer.packetSize+4; i++){
            packet[i] = buffer[i-4];
        }

        llwrite(applicationLayer.porta_serie, packet, read_bytes+4);
        printf("packets_sent: %d\n", packets_sent);
    }

    return 0;
}

int writeDataPackets(unsigned char *packet){
    int nmr_de_octetos = 256 * packet[2] + packet[3];
    if(write(applicationLayer.fileDesNewFile, packet+4, nmr_de_octetos) == -1){
        perror("Error writing to new fd");
        return 1;
    }
    return 0;
}

int sendFile(int fd){
    applicationLayer.porta_serie = fd;

    if(createControlPacket(applicationLayer.porta_serie, START) < 0){
        perror("Error createControlPacket (START)");
        exit(-1);
    }

    if(createDataPacket() != 0){
        perror("Error in createDataPacket");
        exit(-1);
    }

    if(createControlPacket(applicationLayer.porta_serie, END) < 0){
        perror("Error createControlPacket (END)");
        exit(-1);
    }

    return 0;
}

int readFile(int fd){
    applicationLayer.porta_serie = fd;
    unsigned char buffer[65537];

    readControlPacket(applicationLayer.porta_serie);

    while(1){
        if(llread(applicationLayer.porta_serie, buffer) > 0){
            if(buffer[0] == 0x03){
                break;
            }
            else if(buffer[0] == 0x01){
                writeDataPackets(buffer);
            }
        
        }
    }

    close(applicationLayer.fileDesNewFile);
    return 0;
}

int main(int argc, char** argv){
    int fd, c, res;
    int i, sum = 0, speed = 0;

    if ( (argc < 4) ||
  	     ((strcmp("/dev/ttyS10", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS11", argv[1])!=0) &&
          (strcmp("/dev/ttyS1", argv[1])!=0) &&
          (strcmp("/dev/ttyS0", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort TRANSMITTER(1)|RECEIVER(0) Filename (PacketSize) \n\tex: nserial /dev/ttyS1 1 filename.jpg 1024\n");
        exit(-1);
    }
    if(argc > 4) applicationLayer.packetSize = atoi(argv[4]);
    else applicationLayer.packetSize = 1024;
    int arg = atoi(argv[2]);
    //int data_packet_size = atoi(argv[3]); //passar para a struct

    if(arg == TRANSMITTER){
        readFileData(argv[3]);
        fd = llopen(argv[1], TRANSMITTER);
        sendFile(fd);
        llclose_transmitter(fd);
    }
    else if(arg == RECEIVER){
        fd = llopen(argv[1], RECEIVER);
        applicationLayer.fileDestName = argv[3];
        readFile(fd);
        llclose_receiver(fd);
    }
    
    return 0;
}
