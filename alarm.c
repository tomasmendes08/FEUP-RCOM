#include <signal.h>
#include "protocol.h"

#define ALARM_TIME   3
#define MAX_TRIES    3

int sendTries = 0;
int alarmFlag = FALSE;

void alarmHandler(){
    printf("alarm was called.\n");
    sendTries++;
    alarmFlag = TRUE;
}

void alarmSetup(){
    struct sigaction action;
    memset(&action,0,sizeof action);
    action.sa_handler = alarmHandler;
    action.sa_flags = 0;
    sigaction(SIGALRM, &action, NULL);
}