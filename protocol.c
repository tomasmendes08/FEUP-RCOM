/*Non-Canonical Input Processing*/
#include "protocol.h"

int stop = FALSE;
alarm_flag = FALSE;
int counter = 1;
struct termios oldtio, newtio;

/*void alarm_handler(){
  printf("Alarm Counter = %d\n", counter);
  counter++;
  alarm_flag = TRUE;
}*/

int sendMessageSET(int fd){
  char message[5];

  printf("cheguei SET\n");
  message[0] = FLAG;
  message[1] = A_ADRESS;
  message[2] = SET_CTRL;
  message[3] = (A_ADRESS ^ SET_CTRL);
  message[4] = FLAG;

  int result = write(fd, message, 5);
  if(result == -1){
    perror("Writing SET message");
    exit(-1);
  }

  printf("SET: %s\n", message);
  return result;
}

int sendMessageUA(int fd){
  char message[5];

  printf("cheguei UA\n");

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

int stateMachine(int numChars, char *value){
  size_t state;
  for ( state = 0; state < numChars; state++)
  {
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
          return 6;
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

  newtio.c_cc[VTIME] = 2;   /* inter-character timer unused */
  newtio.c_cc[VMIN] = 1;   /* blocking read until 5 chars received */

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

  int result = sendMessageSET(fd);
  printf("SET result: %d\n", result);

  char ua_message[6];
  if(read(fd, ua_message, 6)==-1){
    perror("Error reading UA from fd");
    exit(-1);
  }

  int ua_received = stateMachine(6, ua_message);

  printf("UA: %s\n", ua_message);
  
  if(ua_received != 6){
    perror("UA message must have length equal to 5");
    exit(-1); 
  }

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

  newtio.c_cc[VTIME] = 2;   /* inter-character timer unused */
  newtio.c_cc[VMIN] = 1;   /* blocking read until 5 chars received */

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

  char set_message[6];
  
  if(read(fd, set_message, 6)==-1){
    perror("Error reading SET from fd");
    exit(-1);
  }

  int set_received = UAstateMachine(6, set_message);

  printf("SET: %s\n", set_message);
  
  if(set_received != 6){
    perror("SET message must have length equal to 5");
    exit(-1); 
  }

  int result = sendMessageUA(fd);
  printf("UA result: %d\n", result);

  return fd;
}

