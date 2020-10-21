#include <signal.h>
#include "protocol.h"

#define ALARM_TIME   3   
#define MAX_TRIES    3

int sendTries = 0;
int alarmFlag = FALSE;


void alarmHandler(int signal){
    printf("Timed out, retrying...");
    alarmFlag = TRUE;
    sendTries++;
}

void startAlarm(){
    signal(SIGALRM, alarmHandler);
    alarm(ALARM_TIME);
}
