#include "alarm.c"

int stop = FALSE;
//LinkLayer linkLayer;
int success = 2;
struct termios oldtio;


/*int setLinkLayerStruct(){
    linkLayer.sequenceNumber = 0;
    linkLayer.baudRate = BAUDRATE;
}*/


int sendMessageTransmitter(int fd, int type){
    char message[5];
    char receiver_message[5];
    alarmSetup();
    do{

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
      
      if(type == SET) printf("SET: %s\n", message);
      else if(type == DISC) printf("DISC: %s\n", message);

      if(write(fd, message, 5)==-1){
          perror("Error writing SET message");
          continue;
      }

      alarm(ALARM_TIME);
      if(read(fd, receiver_message, 5)==-1){
          perror("Error reading UA from fd");
          continue;
      }

      if(verifyFrame(receiver_message, UA)){
          continue;
      }
      else if(verifyFrame(receiver_message, DISC)){
          continue;
      }
      else{
          alarmFlag=FALSE;
          sendTries=0;
          alarm(0);
      }

    } while(sendTries < MAX_TRIES && alarmFlag);
    
    if(sendTries == MAX_TRIES){
      sendTries = 0;
      alarmFlag = FALSE;
      perror("Timed out too many times");
      return 1;
    }

    return 0;
}

int sendMessageReceiver(int fd, int type){
    char message[5];

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
        perror("Writing UA message");
        exit(-1);
    }

    if(type==UA) printf("UA: %s\n", message);
    else if(type == DISC) printf("DISC: %s\n", message);

    return result;
}

int verifyFrame(char *message, int type){
  
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
      printf("SET received: %s\n", message);
      break;
    case UA:
      if(!(message[2]==UA_CTRL && message[3]==(A_ADRESS ^ UA_CTRL))){
        perror("Error in verifyFrame (UA)");
        return 1;
      }
      printf("UA received: %s\n", message);
      break;
    case DISC:
      if(!(message[2]==DISC_CTRL && message[3]==(A_ADRESS ^ DISC_CTRL))){
        perror("Error in verifyFrame (DISC)");
        return 1;
      }
      printf("DISC received: %s\n", message);
      break;
    case DATA_CTRL:     //RR or REJ
      if(message[2] != 0x85 && message[2] != 0x05){
        perror("Error in verifyFrame (RR -> Control)");
        return 1;
      }
      else success = 3;

      if(message[2] != 0x81 && message[2] != 0x01){
        perror("Error in verifyFrame (REJ -> Control)");
        return 1;
      }
      else success = 4;

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
/*
int stateMachine(int numChars, char *value){
    size_t state = 0;
    while(state < numChars){
        switch (state)
        {
            case 0:
                if(value[state] == FLAG)
                    state = 1;
                else
                    state = 0;
                break;

            case 1:
                if(value[state] == A_ADRESS)
                    state = 2;
                else if(value[state] == FLAG)
                    state = 1;
                else
                    state = 0;
                break;

            case 2:
                if(value[state] == UA_CTRL || value[state] == SET_CTRL)
                    state = 3;
                else if (value[state] == FLAG)
                    state = 1;
                else
                    state = 0;
                break;

            case 3:
                if(value[state] == (A_ADRESS ^ UA_CTRL))
                    state = 4;
                else if (value[state] == FLAG)
                    state = 1;
                else
                    state = 0;
                break;

            case 4:
                if(value[state] == FLAG){
                    stop = TRUE;
                    //alarm(0);
                    printf("UA/SET received.\n");
                    state = 5;
                }
                else
                    state = 0;
                break;

            default:
                break;
        }
    }

    return state;
}
*/

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
    return size;
}

int byteDestuffing(char* buf, int len, char* result){
    result[0] = buf[0];
    result[1] = buf[1];
    result[2] = buf[2];
    result[3] = buf[3];
    int j = 4;
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

    result[j] = buf[i+1];
    return j+1;
}

int openPort(char *port, struct termios *oldtio){
    struct termios newtio;

    int fd = open(port, O_RDWR | O_NOCTTY);
    if(fd == -1){
        perror("Port error");
        exit(-1);
    }

    if ( tcgetattr(fd,oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 1;   /* inter-character timer unused */
    newtio.c_cc[VMIN] = 5;   /* blocking read until 5 chars received */

/*
  VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
  leitura do(s) pr�ximo(s) caracter(es)
*/

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

    char set_message[5];
    while(1) {
        if (read(fd, set_message, 5) == -1) {
            perror("Error reading SET from fd");
            exit(-1);
        }

        if(verifyFrame(set_message, SET)==0)
            break;
    }

    int result = sendMessageReceiver(fd, UA);
    printf("UA result: %d\n", result);

    return fd;
}

int writeFrameI(int fd, unsigned char *buffer, int length){
    int max_length = 2 * length + 6;
    unsigned char frame[max_length];

    frame[0] = FLAG;
    frame[1] = A_ADRESS;
    
    if(linkLayer.sequenceNumber == 0) frame[2] = FI_CTRL0;
    else if(linkLayer.sequenceNumber == 1) frame[2] = FI_CTRL1;

    frame[3] = (frame[1] ^ frame[2]);
    
    int size = byteStuffing(buffer, length, frame);
    if(write(fd, frame, size) == -1){
        perror("Error writing to fd");
        exit(-1);
    }

    return size;

}

int llwrite(int fd, unsigned char *buffer, int length){
    printf("llwrite starting\n");
    int counter;
    alarmSetup();
    do
    {
        counter = writeFrameI(fd, buffer, length);
        alarm(ALARM_TIME);
        
        char answer[5];
        if(read(fd, answer, 5) == -1){
            perror("Error reading receptor answer");
            continue;
        }

        int result = verifyFrame(answer, DATA_CTRL);
        if(result != 3){
            continue;
        }
        else{
          alarmFlag=FALSE;
          sendTries=0;
          alarm(0);
          printf("RR received: %s\n", answer);
        }
        
    } while (sendTries < MAX_TRIES && alarmFlag);

    
    if(sendTries == MAX_TRIES){
      sendTries = 0;
      alarmFlag = FALSE; 

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
            if (byte == FLAG) *state = END; //sucess
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
        if(state==END)break;
    }
    return len;
}

int llread(int fd, char *buffer){

}

int llclose_transmitter(int fd){
    
    sendMessageTransmitter(fd, DISC);
    
    sendMessageReceiver(fd, UA);

    closePort(fd, &oldtio);

    return fd;
}

int llclose_receiver(int fd){

    char disc_message[5];
    while(1) {
        if (read(fd, disc_message, 5) == -1) {
            perror("Error reading SET from fd");
            exit(-1);
        }

        if(verifyFrame(disc_message, DISC)==0)
            break;
    }

    sendMessageReceiver(fd, DISC);

    char ua_message[5];
    while(1) {
        if (read(fd, ua_message, 5) == -1) {
            perror("Error reading SET from fd");
            exit(-1);
        }

        if(verifyFrame(ua_message, UA)==0)
            break;
    }

    closePort(fd, &oldtio);
    return fd;
}
