#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <stdlib.h> 
#include <errno.h> 
#include <sys/types.h>
#include <netinet/in.h> 
#include<arpa/inet.h>
#include<sys/stat.h>
#include<fcntl.h> 

#define h_addr h_addr_list[0]	//The first address in h_addr_list. 

typedef struct{
    char user[100];
    char password[100];
    char host[100];
    char path[100];
} url_info;

int openTCP();

struct hostent *getHostName(url_info *info);

int createSocket(char *ip, int port);

int parseURL(char *url, url_info *info);
