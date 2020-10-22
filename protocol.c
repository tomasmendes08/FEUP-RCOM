/*Non-Canonical Input Processing*/
#include "alarm.c"

int stop = FALSE;
struct termios oldtio, newtio;

int sendMessageSET(int fd){
    char message[5];

    do{
      startAlarm();

      message[0] = FLAG;
      message[1] = A_ADRESS;
      message[2] = SET_CTRL;
      message[3] = (A_ADRESS ^ SET_CTRL);
      message[4] = FLAG;
    
      printf("SET: %s\n", message);

      int result = write(fd, message, 5);
      if(result == -1){
          perror("Error writing SET message");
          continue;
      }

      char ua_message[5];
      if(read(fd, ua_message, 5)==-1){
          perror("Error reading UA from fd");
          continue;
      }

      if(verifyFrame(ua_message, UA)){
          continue;
      }
      else{
          alarmFlag=FALSE;
          sendTries=0;
          alarm(0);
          printf("UA received: %s\n", ua_message);
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

int sendMessageUA(int fd){
    char message[5];

    message[0] = FLAG;
    message[1] = A_ADRESS;
    message[2] = UA_CTRL;
    message[3] = (A_ADRESS ^ UA_CTRL);
    message[4] = FLAG;

    int result = write(fd, message, 5);
    if(result == -1){
        perror("Writing UA message");
        exit(-1);
    }

    printf("UA: %s\n", message);
    return result;
}

int verifyFrame(char *message, int type){
  if(strlen(message) != 5){
      perror("Error in verifyFrame (MSG Size)");
      return 1;
  }
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
    default:
      break;
  }

  return 0;
}

/*int stateMachine(int numChars, char *value){
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
}*/

int llopen_transmitter(char * port){

    int fd = open(port, O_RDWR | O_NOCTTY);
    if(fd == -1){
        perror("Port error");
        exit(-1);
    }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0;   /* inter-character timer unused */
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

    if(sendMessageSET(fd)) exit(-1);

    return fd;
}

int llopen_receiver(char * port){

    int fd = open(port, O_RDWR | O_NOCTTY);
    if(fd == -1){
        perror("Port error");
        exit(-1);
    }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0;   /* inter-character timer unused */
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

    char set_message[5];

    if(read(fd, set_message, 5)==-1){
        perror("Error reading SET from fd");
        exit(-1);
    }

    int set_received = verifyFrame(set_message, SET);

    int result = sendMessageUA(fd);
    printf("UA result: %d\n", result);

    return fd;
}

