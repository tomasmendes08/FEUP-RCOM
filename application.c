#include "application.h"


ApplicationLayer applicationLayer;
AppStatistics appstats;

void setAppStatistics(){
    appstats.numOfDataPacketsReceived = 0;
    appstats.numOfDataPacketsSent = 0;
}

void displayStats(int type){
    switch (type)
    {
        case TRANSMITTER:
            printf("\n\n            TRANSMITTER STATISTICS \n\n");
            printf("Filename: %s\n", applicationLayer.fileName);
            printf("File size: %d\n", applicationLayer.fileSize);
            printf("Packet size %d\n", applicationLayer.packetSize);
            if(strcmp(appstats.baudrate, "") == 0)
                strcpy(appstats.baudrate, "B38400");
            printf("Baudrate: %s\n\n", appstats.baudrate);
            displayProtocolStatistics(TRANSMITTER);
            printf("\nNumber of Data Packets Sent: %d\n\n", appstats.numOfDataPacketsSent);
            break;
    
        case RECEIVER:
            printf("\n\n            RECEIVER STATISTICS \n\n");
            printf("Filename: %s\n", applicationLayer.fileDestName);
            printf("File size: %d\n", applicationLayer.fileSize);
            if(strcmp(appstats.baudrate, "") == 0)
                strcpy(appstats.baudrate, "B38400");
            printf("Baudrate: %s\n\n", appstats.baudrate);
            displayProtocolStatistics(RECEIVER);
            printf("\nNumber of Data Packets Received: %d\n\n", appstats.numOfDataPacketsReceived);
            break;
    }
}

void readFileData(char *filename){

    int fd = open(filename, O_RDONLY);
    struct stat stat_buff;
    fstat(fd, &stat_buff);

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
    int i = 1, file_size=0, filename_size=0;
    unsigned char *filename;
    llread(fd, packet);
    
    if(!(packet[0]==(unsigned char)0x02 || packet[0]==(unsigned char)0x03)){
        perror("Initial packet flag");
        return 1;
    }
    else{
        if(packet[i] == (unsigned char)0x00){
            i+=2;
            file_size += packet[i] << 24;
            i++;
            file_size += packet[i] << 16;
            i++;
            file_size += packet[i] << 8;
            i++;
            file_size += packet[i];
            i++;
        }

        if(packet[i] == (unsigned char)0x01){
            i++;
            filename_size = (int) packet[i];
            i++;

            filename = (unsigned char *) malloc(filename_size+1);
            for (size_t j = 0; j < filename_size; j++)
            {
                filename[j] = packet[i];
                i++;
            }
            
        }
    }
    applicationLayer.fileSize = file_size;
    
    if(strcmp(applicationLayer.fileDestName,"none")==0) applicationLayer.fileDestName = filename;
    
    applicationLayer.fileDesNewFile = open(applicationLayer.fileDestName, O_CREAT | O_WRONLY | O_APPEND, 0664);
    
    return 0;
}

int createDataPacket(){
    char buffer[applicationLayer.packetSize];
    int packets_to_send = applicationLayer.fileSize/applicationLayer.packetSize;
    int read_bytes = 0, packets_sent = 0;
    int lastbyte = applicationLayer.fileSize % applicationLayer.packetSize, flag = FALSE;
    int size;
    if(lastbyte != 0){
        flag = TRUE;
    }
    
    while(packets_sent <= packets_to_send){
        int count = 0;
        if((packets_sent == packets_to_send) && flag){
            size = lastbyte+4;
        
                if((read_bytes=read(applicationLayer.fileDes, buffer, lastbyte)) == -1){
                    perror("Error reading");
                }
                
        }
        else{
            size = applicationLayer.packetSize+4;
            
                if((read_bytes = read(applicationLayer.fileDes, buffer, applicationLayer.packetSize)) == -1){
                    perror("Error reading");
                }
        }

        unsigned char packet[size];
        packet[0] = DATA;
        packet[1] = packets_sent;
        packets_sent++;
        packet[2] = read_bytes/256;
        packet[3] = read_bytes%256;
        int j = 0;
        for(int i = 4; i < size; i++){
            packet[i] = buffer[j];
            j++;
        }

        llwrite(applicationLayer.porta_serie, packet, read_bytes+4);
        appstats.numOfDataPacketsSent = packets_sent;
        if(packets_to_send == 0){
            printf("Progress ...... 100.000000%%\n");
        }
        else printf("Progress ...... %f %%\n", ((double)(packets_sent-1)/packets_to_send) * 100);
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
                appstats.numOfDataPacketsReceived++;
            }
        
        }
    }

    struct stat stat_buff;
    fstat(applicationLayer.fileDesNewFile, &stat_buff);
    
    if(stat_buff.st_size == applicationLayer.fileSize){
        printf("The received file size is the same as the sent file. No data loss\n");
    }
    else{
        printf("The received file size is not the same as the sent file. Possible data loss\n");
    }
    
    close(applicationLayer.fileDesNewFile);
    return 0;
}

speed_t checkBaudrate(long br){
    switch (br){
        case 0xB0:
            return B0;
        case 0xB50:
            return B50;
        case 0xB75:
            return B75;
        case 0xB110:
            return B110;
        case 0xB134:
            return B134;
        case 0xB150:
            return B150;
        case 0xB200:
            return B200;
        case 0xB300:
            return B300;
        case 0xB600:
            return B600;
        case 0xB1200:
            return B1200;
        case 0xB1800:
            return B1800;
        case 0xB2400:
            return B2400;       
        case 0xB4800:
            return B4800;
        case 0xB9600:
            return B9600;
        case 0xB19200:
            return B19200;
        case 0xB38400:
            return B38400;
        case 0xB57600:
            return B57600;
        case 0xB115200:
            return B115200;
        case 0xB230400:
            return B230400;
        default:
            printf("Bad baudrate value. Using default (B38400)");
            return B38400;
    }
}

int main(int argc, char** argv){

    int fd;
    int ps = 1024;
    int brflag = 0;
    long br = 0xB38400;
    char auxps[100];
    char auxbr[100];
    struct timeval start, end;
    double time_used;

    gettimeofday(&start, NULL);

    if (argc < 3 || ((strcmp("/dev/ttyS", argv[1])!=0) && strcmp(argv[2],"1") && strcmp(argv[2],"0"))) {
        printf("Usage:\tnserial SerialPort TRANSMITTER(1)|RECEIVER(0) Filename (ps=PacketSize) (br=Baudrate(HEX)) \n\tex: nserial /dev/ttyS1 1 filename.jpg\n");
        exit(-1);   
    }

    int arg = atoi(argv[2]);

    if(argc > 3){
        for(size_t i = 3; i < argc; i++){
            if(sizeof(argv[i])/sizeof(argv[i][0])>=3){
                if(argv[i][0]=='p' && argv[i][1]=='s' && argv[i][2]=='='){
                    memcpy(auxps, &argv[i][3], (strlen(argv[i])-3)*sizeof(*argv[i]));
                    
                    ps = atoi(auxps);
                }
                if(argv[i][0]=='b' && argv[i][1]=='r' && argv[i][2]=='='){
                    memcpy(auxbr, &argv[i][3], (strlen(argv[i])-3)*sizeof(*argv[i]));
                    
                    if(strcmp(auxbr, "")!=0) {
                        for(int i = 0; i < strlen(auxbr); i++){
                            appstats.baudrate[i] = auxbr[i];
                        }
                    }

                    br = strtol(auxbr,NULL,16);
                    if(arg == RECEIVER && i == 3) brflag=1;
                    //printf("BR Long: %ld\n", br);
                }
            }
        }
    }

    if(arg == TRANSMITTER){
        if(argc < 4){
            printf("Usage:\tnserial SerialPort TRANSMITTER(1)|RECEIVER(0) Filename (ps=PacketSize) (br=Baudrate(HEX)) \n\tex: nserial /dev/ttyS1 1 filename.jpg\n");
            exit(-1);
        }
        
        applicationLayer.packetSize = ps;
        setLinkLayerStruct(checkBaudrate(br));
        readFileData(argv[3]);
        fd = llopen(argv[1], TRANSMITTER);
        sendFile(fd);
        llclose_transmitter(fd);
        displayStats(TRANSMITTER);
    }
    else if(arg == RECEIVER){
        setLinkLayerStruct(checkBaudrate(br));
        fd = llopen(argv[1], RECEIVER);
        if(argc >= 4 && !brflag){
            applicationLayer.fileDestName = argv[3];
        }
        else applicationLayer.fileDestName = "none";
        readFile(fd);
        llclose_receiver(fd);
        displayStats(RECEIVER);
    }

    gettimeofday(&end, NULL);
    
    time_used = (end.tv_sec + end.tv_usec / 1e6) - (start.tv_sec - start.tv_usec / 1e6); // in seconds

    printf("Execution Time: %f seconds\n", time_used);

    
    return 0;
}
