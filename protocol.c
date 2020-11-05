#include "alarm.c"

LinkLayer linkLayer;
ProtocolStatistics Protstatistics;
int success = FALSE;
struct termios oldtio;

void setLinkLayerStruct(speed_t baudRate){
    linkLayer.sequenceNumber = 0;
    linkLayer.baudRate = baudRate;
}

void setProtocolStats(){
    
    Protstatistics.numOfInfoFramesReceived = 0;
    Protstatistics.numOfInfoFramesSent = 0;
    Protstatistics.numOfREJsReceived =0;
    Protstatistics.numOfREJsSent = 0;
    Protstatistics.numOfRRsReceived = 0;
    Protstatistics.numOfRRsSent = 0;

}

void displayProtocolStatistics(int type){
    switch (type)
    {
        case TRANSMITTER:
            printf("Number of Info Frames Sent: %d\n", Protstatistics.numOfInfoFramesSent);
            printf("Number of RR Frames Received: %d\n", Protstatistics.numOfRRsReceived);
            printf("Number of REJ Frames Received: %d\n", Protstatistics.numOfREJsReceived);
            break;
        case RECEIVER:
            printf("Number of Info Frames Received: %d\n", Protstatistics.numOfInfoFramesReceived);
            printf("Number of RR Frames Sent: %d\n", Protstatistics.numOfRRsSent);
            printf("Number of REJ Frames Sent: %d\n", Protstatistics.numOfREJsSent);
            break;
    }
    
}

int llopen(char *port, int status){
    int fd;
    setProtocolStats();
    if(status == TRANSMITTER)
        fd = llopen_transmitter(port);
    else
        fd = llopen_receiver(port);

    return fd;
}

int sendMessageTransmitter(int fd, int type){
    unsigned char byte;
    unsigned char message[5];
    unsigned char receiver_message[5];
    sendTries = 0;
    do{
      alarmSetup();
      int flag = FALSE;
      message[0] = FLAG;
      message[1] = A_ADRESS;
      
      if(type == SET){
        message[2] = SET_CTRL;
        message[3] = (A_ADRESS ^ SET_CTRL);
      }
      else if(type == DISC){
        message[2] = DISC_CTRL;
        message[3] = (A_ADRESS ^ DISC_CTRL);
      }

      message[4] = FLAG;
      printf("message: %s\n", message);
      
      if(type == SET) printf("SET sent\n");
      else if(type == DISC) printf("DISC sent\n");

      if(write(fd, message, 5)==-1){
          perror("Error writing SET message");
          continue;
      }

      alarm(ALARM_TIME);
      
      for(int i = 0; i < 5; i++){
            if (read(fd, &byte, 1) == -1) {
                perror("Error reading UA/DISC from fd");
                flag = TRUE;
                break;
                //exit(-1);
            }
            receiver_message[i] = byte;
      }
      if(!flag) {
          if (verifyFrame(receiver_message, DISC)) {
              if (verifyFrame(receiver_message, UA)) {
                  continue;
              }
              else {
                  break;
              }
          } else {
              break;
          }
      }
      else {
          //alarmFlag = FALSE;
          //sendTries=0;
          alarm(0);
      }
      

    } while(sendTries < MAX_TRIES && alarmFlag);
    
    if(sendTries == MAX_TRIES){
      sendTries = 0;
      alarmFlag = FALSE;
      //Protstatistics.timeouts++;

      perror("Timed out too many times");
      return 1;
    }

    return 0;
}

int sendMessageReceiver(int fd, int type){
    unsigned char message[5];

    message[0] = FLAG;
    message[1] = A_ADRESS;


    if(type == UA){
        message[2] = UA_CTRL;
        message[3] = (A_ADRESS ^ UA_CTRL);
    }
    else if(type == DISC){
        message[2] = DISC_CTRL;
        message[3] = (A_ADRESS ^ DISC_CTRL);
    }
    
    message[4] = FLAG;

    int result = write(fd, message, 5);
    if(result == -1){
        perror("Error writing UA message");
        exit(-1);
    }

    if(type==UA) printf("UA sent\n");
    else if(type == DISC) printf("DISC sent\n");

    return result;
}

int verifyFrame(unsigned char *message, int type){

  if(!(message[0] == FLAG && message[1] == A_ADRESS && message[4]==FLAG)){
    perror("Error in verifyFrame");
    return 1;
  }

  switch (type)
  {
    case SET:
      if(!(message[2]==SET_CTRL && message[3]==(A_ADRESS ^ SET_CTRL))){
        perror("Error in verifyFrame (SET)");
        return 1;
      }
      printf("SET received\n");
      break;
    case UA:
      if(!(message[2]==UA_CTRL && message[3]==(A_ADRESS ^ UA_CTRL))){
        perror("Error in verifyFrame (UA)");
        return 1;
      }
      printf("UA received\n");
      break;
    case DISC:
      if(!(message[2]==DISC_CTRL && message[3]==(A_ADRESS ^ DISC_CTRL))){
        return 1;
      }
      printf("DISC received\n");
      break;
    case DATA_CTRL:     //RR or REJ
      if(message[2] != RR1 && message[2] != RR0){
        if(message[2] == REJ1 || message[2] == REJ0){
            printf("NACK\n");
            Protstatistics.numOfREJsReceived++;
            return 1;
        }
      }
      else{
          printf("ACK\n");
          success = 0;
      }

      if(message[3] != (A_ADRESS ^ message[2])){
        perror("Error in verifyFrame (RR / REJ -> XOR)");
        return 1;
      } 
      
      return success;
    default:
      break;
  }

  return 0;
}

int byteStuffing(unsigned char* buf, int len, unsigned char* result){
    unsigned char bcc2 = 0x00;

    for(int j = 0; j < len; j++){
        bcc2 ^= buf[j];
    }
    int size = 4;
    for(size_t i=0; i < len; i++){
        if(buf[i] == FLAG || buf[i] == ESC){
            result[size] = ESC;
            result[size+1] = buf[i] ^ STUFF;
            size++;
        }
        else{
            result[size] = buf[i];
        }
        size++;
    }

    if(bcc2 == FLAG || bcc2 == ESC){
        result[size] = ESC;
        result[size+1] = bcc2 ^ STUFF;
        size+=2;
    }
    else{
        result[size] = bcc2;
        size++;
    }

    result[size] = FLAG;
    size++;
    return size;
}

int byteDestuffing(char* buf, int len, char* result){
    
    int j = 0;
    int i;
    for (i = 4; i < len - 1; i++) {
        if (buf[i] == ESC) {
            i++;
            if (buf[i] == (FLAG ^ STUFF)){
                result[j] = FLAG;
                j++;
            }
            else if (buf[i] == (ESC ^ STUFF)){
                result[j] = ESC;
                j++;
            }
        }
        else {
            result[j] = buf[i];
            j++;
        }
    }

    //result[j] = buf[i+1];
    return j;
}

int openPort(char *port, struct termios *oldtio){
    struct termios newtio;

    int fd = open(port, O_RDWR | O_NOCTTY);
    if(fd == -1){
        perror("Port error");
        exit(-1);
    }

    if ( tcgetattr(fd,oldtio) == -1) { 
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = linkLayer.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 10;   /* inter-character timer unused */
    newtio.c_cc[VMIN] = 1;   /* blocking read until 5 chars received */

/*
  VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
  leitura do(s) pr�ximo(s) caracter(es)
*/
    cfsetspeed(&newtio,linkLayer.baudRate);
    
    tcflush(fd, TCIOFLUSH);
    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    return fd;
}

int closePort(int fd, struct termios *oldtio){
    tcflush(fd, TCIOFLUSH);

    if(tcsetattr(fd, TCSANOW, oldtio)==-1){
        perror("tcsetattr");
        exit(-1);
    }
    close(fd);
    return 0;
}

int llopen_transmitter(char * port){

    int fd = openPort(port, &oldtio);

    if(sendMessageTransmitter(fd, SET)) exit(-1);

    return fd;
}

int llopen_receiver(char * port){
    int fd = openPort(port, &oldtio);

    unsigned char set_message[5], byte;
    while(1) {
        for(int i = 0; i < 5; i++){
            if (read(fd, &byte, 1) == -1) {
                perror("Error reading SET from fd");
                exit(-1);
            }
            set_message[i] = byte;
        }

        if(verifyFrame(set_message, SET)==0)
            break;
    }

    sendMessageReceiver(fd, UA);

    return fd;
}


int writeFrameI(int fd, unsigned char *buffer, int length){
    int max_length = 2 * length + 7;
    unsigned char frame[max_length];

    struct timeval start, end;
    double time_used;

    frame[0] = FLAG;
    frame[1] = A_ADRESS;

    
    if(linkLayer.sequenceNumber == 0) {
        frame[2] = FI_CTRL0;
        linkLayer.sequenceNumber = 1;
    }
    else if(linkLayer.sequenceNumber == 1) {
        frame[2] = FI_CTRL1;
        linkLayer.sequenceNumber = 0;
    }
    frame[3] = (frame[1] ^ frame[2]);
    
    int size = byteStuffing(buffer, length, frame);
    
    //gettimeofday(&start, NULL);
    if(write(fd, frame, size) == -1){
        perror("Error writing to fd");
        exit(-1);
    }

    Protstatistics.numOfInfoFramesSent++;
    //gettimeofday(&end, NULL);
    
    //time_used = (end.tv_sec + end.tv_usec / 1e6) - (start.tv_sec - start.tv_usec / 1e6); // in seconds

    printf("Write Infor Frame Time: %f seconds\n", time_used);
    return size;
}

int llwrite(int fd, unsigned char *buffer, int length){
    printf("Sending packet...\n");
    int counter;
    unsigned char byte;
    int flag;
    sendTries = 0;
    alarmSetup();
    do
    {
        flag = FALSE;
        counter = writeFrameI(fd, buffer, length);
        printf("packet sent...\n");
        alarm(ALARM_TIME);  
        
        unsigned char answer[5];

        for(int i = 0; i < 5; i++){
            if (read(fd, &byte, 1) == -1) {
                perror("Error reading RR/REJ from fd");
                flag = TRUE;
                break;
                //exit(-1);
            }
            answer[i] = byte;
        }
        if(flag){
            alarm(0);
            continue;
        }
        int result = verifyFrame(answer, DATA_CTRL);
        if(result == 1){
            continue;
        }
        if(!flag){
            if(result == 0){
                printf("RR received\n");
                break;
            }
        }
        /*else{
          //alarmFlag=FALSE;
          //sendTries=0;
          alarm(0);
        }*/

    } while (sendTries < MAX_TRIES && alarmFlag);

    
    if(sendTries == MAX_TRIES){
      sendTries = 0;
      alarmFlag = FALSE; 
      //Protstatistics.timeouts++;

      perror("Timed out too many times");
      exit(-1);
    }

    return counter;
}

int stateMachine(enumStates* state, unsigned char byte){
    switch (*state)
    {
        case START:
            if (byte == FLAG) *state = FLAG_R;
            break;

            case FLAG_R:
            if (byte == A_ADRESS) *state = A_R;
            else if(byte == FLAG){
                break;
            }
            else {
                *state = START;
            }
            break;
        case A_R:
            if (byte == 0x00|| byte == 0x40) *state = C_R;
            else if(byte == FLAG) *state = FLAG_R;
            else *state = START;
            break;
        case C_R:
            if (byte == (A_ADRESS ^ 0x00) || byte == (A_ADRESS ^ 0x40)){
            *state = BCC1_R;
            }
            else if(byte == FLAG) *state = FLAG_R;
            else *state = START;
            break;
        case BCC1_R:
            if (byte != FLAG) *state = DATA_R;
            break;
        case DATA_R:
            if (byte == FLAG) *state = END;
            break;

        case END:
            break;
        default:
            break;
    }
    return 0;
}

int readFrameI(int fd, char *buffer){
    int len = 0;
    unsigned char currentByte;
    enumStates state = START;
    while(read(fd,&currentByte,1)>0){
        stateMachine(&state, currentByte);
        buffer[len] = currentByte;
        len++;
        if(state==END) {
            Protstatistics.numOfInfoFramesReceived++;   
            return len;
        }
    }

    return 0;
}

int checkBCCs(unsigned char *buffer, int length, unsigned char *frame, int size){
    int bcc2 = 0x00;
    if(frame[3]!=(frame[1]^frame[2])){
        printf("Error in BCC1\n");
        return 1;
    }
    else{
        for(int i = 0; i < length-1; i++){
            bcc2 ^= buffer[i];
        }
        if(bcc2 == buffer[length-1]){
            return 0;
        }
        else return 1;
    }
    return 0;
}


int llread(int fd, unsigned char *buffer){
    printf("Receiving packet...\n");
    unsigned char frame[131078], aux[65537];

    int framelen = readFrameI(fd, frame);
    
    char response[5];
    
    response[0] = FLAG;
    response[1] = A_ADRESS;
    response[4] = FLAG;
    
    if(framelen>0){
        int aux_length = byteDestuffing(frame, framelen, aux);
        if(checkBCCs(aux, aux_length, frame, framelen)==0){
            for(int i = 0; i < aux_length-1; i++){
                buffer[i] = aux[i];
            }
            //printf("buffer: %s\n", buffer);
            if(frame[2]==FI_CTRL1){
                response[2] = (char)RR0;
                response[3] = (char)(A_ADRESS ^ RR0);
                Protstatistics.numOfRRsSent++;
            }
            else if(frame[2]==FI_CTRL0){
                response[2] = (char)RR1;
                response[3] = (char)(A_ADRESS ^ RR1);
                Protstatistics.numOfRRsSent++;
            }
        }
        else{
            if(frame[2]==FI_CTRL0){
                response[2] = (char)REJ0;
                response[3] = (char)(A_ADRESS ^ REJ0);
                Protstatistics.numOfREJsSent++;
            }
            else if(frame[2]==FI_CTRL1){
                response[2] = (char)REJ1;
                response[3] = (char)(A_ADRESS ^ REJ1);
                Protstatistics.numOfREJsSent++;
            }
        }
    }
    else{
        if(frame[2]==FI_CTRL0){
            response[2] = (char)REJ0;
            response[3] = (char)(A_ADRESS ^ REJ0);
            Protstatistics.numOfREJsSent++;
        }
        else if(frame[2]==FI_CTRL1){
            response[2] = (char)REJ1;
            response[3] = (char)(A_ADRESS ^ REJ1);
            Protstatistics.numOfREJsSent++;
        }
    }
    
    
    write(fd,response,5);
    return framelen;
}

int llclose_transmitter(int fd){
    
    sendMessageTransmitter(fd, DISC);
    
    sendMessageReceiver(fd, UA);

    sleep(1);
    closePort(fd, &oldtio);

    return fd;
}

int llclose_receiver(int fd){
    unsigned char disc_message[5], byte;
    while(1) {
        
        for(int i = 0; i < 5; i++){
            if (read(fd, &byte, 1) == -1) {
                perror("Error reading DISC from fd");
                exit(-1);
            }
            disc_message[i] = byte;
        }

        if(verifyFrame(disc_message, DISC)==0)
            break;
    }

    sendMessageReceiver(fd, DISC);

    unsigned char ua_message[5], byte2;
    while(1) {
        
        for(int i = 0; i < 5; i++){
            if (read(fd, &byte2, 1) == -1) {
                perror("Error reading UA from fd");
                exit(-1);
            }

            ua_message[i] = byte2;

        }

        if(verifyFrame(ua_message, UA)==0)
            break;
    }

    closePort(fd, &oldtio);
    return fd;
}
