#include "download.h"


int parseURL(char *url, url_info *info){
    //ftp://ftp.up.pt/pub.txt
    //ftp://user:pass@ftp.up.pt/pub.txt
    //ftp://user@ftp.up.pt/pub.txt

    if(strncmp(url, "ftp://", 6) != 0){
        printf("error trying to get initial url..\n");
        return 1;
    }

    int it = 6;
    int aux = 0;
    int flag = 0;
    if(strchr(url, '@') != NULL){
        while (url[it] != ':')
        {
            info->user[aux] = url[it];
            it++;
            aux++;
            if(url[it] == '@'){
                flag = 1;
                info->user[aux] = '\0';
                printf("Enter password for user %s:\n", info->user);
                fgets(info->password, sizeof(info->password), stdin);
				char* auxptr;
				auxptr = strchr(info->password, '\n');
				*auxptr = '\0';
				printf("pw: %s\n", info->password);
				break;
            }       
        }
        if(!flag){
            info->user[aux] = '\0';
            printf("user: %s\n", info->user);

            it++;
            aux = 0;
            while(url[it] != '@'){
                info->password[aux] = url[it];
                aux++;
                it++;
            }
            info->password[aux] = '\0';
            printf("pw: %s\n", info->password);
            }
        it++;
        aux = 0;
    }
	else{
		strcpy(info->user, "anonymous");
		strcpy(info->password, "random");
	}
    while(url[it] != '/'){
        info->host[aux] = url[it];
        aux++;
        it++;
    }
    info->host[aux] = '\0';
    printf("Host: %s\n", info->host);

    it++;
    aux = 0;
    while(url[it] != '\0'){
        info->path[aux] = url[it];
        it++;
        aux++;
    }
    info->path[aux] = '\0';
    printf("File path: %s\n", info->path);

    printf("URL parsed!\n");
    return 0;
}

struct hostent* getHostName(url_info *info){

    struct hostent *h;

    if ((h=gethostbyname(info->host)) == NULL) {  
            perror("gethostbyname");
    }

    return h;
}

int createSocket(char *ip, int port){
    
    struct sockaddr_in server_addr;
    int sockfd;

    bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port);		/*server TCP port must be network byte ordered */
    
	/*open an TCP socket*/
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
		perror("socket()");
		return -1;
    }
	
	/*connect to the server*/
    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
		perror("connect()");
		return -1;
	}

    return sockfd;
}

int readResponse(FILE* file, char* response, int printflag){
	do{
		if(fgets(response, 1024, file) == NULL) break;
		if(printflag) printf("%s\n", response);
		
	} while(response[3] != ' ');
	char aux[3];
	strncpy(aux, response, 3);
	return atoi(aux);
}

void sendResponse(int sockfd, char* command, int printflag){
    if(printflag) {
        printf("Sending command: %s\n", command);
    }
    write(sockfd, command, strlen(command));
}

void terminateConnection(int sockfd, FILE* file){
    sendResponse(sockfd, "quit\n", 1);
    char buffer[1024];
    if(readResponse(file, buffer, 0) != 221){
        printf("Failed to safely terminate connection, quitting program...\n");
    }
    else printf("Connection successfully closed\n");
    fclose(file);
    close(sockfd);
}

int main(int argc, char*argv[]){

    FILE *file;
    
    url_info info;
    char *url = argv[1];

    if(parseURL(url, &info)) return 1;

	char buf[2000] = ""; 
    char read_buf[2000] = "";

    struct hostent *h = getHostName(&info);

    char *ip = inet_ntoa(*((struct in_addr *)h->h_addr));
    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n",ip);

    printf("Connecting to %s at port %d...\n",ip, 21);
    int sockfd = createSocket(ip, 21);
    if(sockfd < 0) {
        printf("Connection failed!\n");
        return 1;
    }

    printf("Connected!\n");

    file = fdopen(sockfd, "r");
	readResponse(file, read_buf, 1);
    printf("Sending login commands\n");
    sprintf(buf, "user %s\n", info.user);
    sendResponse(sockfd, buf, 0);

	readResponse(file, read_buf, 0);

    sprintf(buf, "pass %s\n", info.password);
    sendResponse(sockfd, buf, 0);

	if(readResponse(file, read_buf, 1) != 230){
		printf("Login Failed\n");
		terminateConnection(sockfd, file);
		return 1;
	}

    sprintf(buf, "pasv\n");
    sendResponse(sockfd, buf, 1);

	if(readResponse(file, read_buf, 1) != 227){
		printf("Error entering Passive Mode\n");
        terminateConnection(sockfd, file);
        return 1;
	}

    int ip1, ip2, ip3, ip4, port1, port2;
    sscanf(read_buf, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).", &ip1, &ip2, &ip3, &ip4, &port1, &port2);

    char ip_server[16]="";
    sprintf(ip_server, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
    int port = port1*256 + port2;
    printf("Connecting to %s at port %d...\n",ip_server, port);
    int sockfd2 = createSocket(ip_server, port);
    if(sockfd2 < 0) {
        printf("Connection failed!\n");
        terminateConnection(sockfd, file);
        return 1;
    }
    printf("Connected!\n");
    
    sprintf(buf, "retr %s\n", info.path);
    sendResponse(sockfd, buf, 1);

	if(readResponse(file, read_buf, 0) != 150){
        printf("Error opening file\n");
        terminateConnection(sockfd, file);
        close(sockfd2);
        return 1;
    }
    char response[256];
    char path[100];
    long int size;
    memcpy(response, &read_buf[44], (strlen(read_buf)-44+1));

    sscanf(response, "%s (%ld bytes)", path, &size);
    printf("Successfully accessed file %s with size %ld bytes, starting transfer\n", path, size);
	char* filename;
	filename = strrchr(info.path, '/')+1;
	printf("Transferring to ./%s\n", filename);
    int new_fd = open(filename, O_CREAT | O_WRONLY, 0666);

    
    int totalbytes = 0;
	int read_bytes;
	double progress = 0.0;
    while((read_bytes = read(sockfd2, read_buf, 1024)) > 0){
        write(new_fd, read_buf, read_bytes);    
        totalbytes+=read_bytes;
        progress = ((double) totalbytes/size ) * 100;
        printf("PROGRESS: %f %%\n", progress);
    }

    struct stat st;
    stat(filename, &st);
    int fsize = st.st_size;
    if(fsize != size){
        printf("File size is incorrect. Possible data loss/corruption.\n");
    }
    else printf("File size is correct.\n");
	
	readResponse(file, read_buf, 1);
    
    terminateConnection(sockfd, file);
    close(sockfd2);  

    return 0;
}

