#include "download.h"


int parseURL(char *url, url_info *info){

    if(strncmp(url, "ftp://", 6) != 0){
        printf("error trying to get initial url..\n");
        return 1;
    }

    int it = 6;
    int aux = 0;

    while (url[it] != ':')
    {
        info->user[aux] = url[it];
        it++;
        aux++;    
    }
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

    it++;
    aux = 0;
    while(url[it] != '/'){
        info->host[aux] = url[it];
        aux++;
        it++;
    }
    info->host[aux] = '\0';
    printf("host: %s\n", info->host);

    it++;
    aux = 0;
    while(url[it] != '\0'){
        info->path[aux] = url[it];
        it++;
        aux++;
    }
    info->path[aux] = '\0';
    printf("url_path: %s\n", info->path);

    printf("url parsed\n");
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
		return 1;
    }
	
	/*connect to the server*/
    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
		perror("connect()");
		return 1;
	}

    return sockfd;
}

int main(int argc, char*argv[]){

    FILE *file;
    
    url_info info;
    char *url = argv[1];
    printf("url: %s\n", url);

    parseURL(url, &info);

	char buf[2000] = ""; 
    char read_buf[2000] = "";

    struct hostent *h = getHostName(&info);

    char *ip = inet_ntoa(*((struct in_addr *)h->h_addr));
    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n",ip);

    int sockfd = createSocket(ip, 21);

    file = fdopen(sockfd, "r");
    fgets(read_buf, 1024, file);

    printf("\nresponse: %s\n", read_buf);

    //sprintf(buf, "telnet %s 21\n", info.host);
    sprintf(buf, "user %s\n", info.user);
    printf("buf: %s\n", buf);

	/*send a string to the server*/
	write(sockfd, buf, strlen(buf));

    fgets(read_buf, 1024, file);

    printf("response: %s\n", read_buf);

    sprintf(buf, "pass %s\n", info.password);
    printf("buf: %s\n", buf);

    write(sockfd, buf, strlen(buf));

    fgets(read_buf, 1024, file);

    printf("response: %s\n", read_buf);

    sprintf(buf, "pasv\n");
    printf("buf: %s\n", buf);

    write(sockfd, buf, strlen(buf));

    fgets(read_buf, 1024, file);

    printf("response: %s\n", read_buf);

    int ip1, ip2, ip3, ip4, port1, port2;
    sscanf(read_buf, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).", &ip1, &ip2, &ip3, &ip4, &port1, &port2);

    char ip_server[16]="";
    sprintf(ip_server, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
    printf("ip_server: %s\n", ip_server);

    int port = port1*256 + port2;
    printf("port: %d\n", port);

    int sockfd2 = createSocket(ip_server, port);
    
    sprintf(buf, "retr %s\n", info.path);
    write(sockfd, buf, strlen(buf));

    fgets(read_buf, 1024, file);
    printf("response: %s\n", read_buf);
    
    int code;

    sscanf(read_buf, "%d", &code);
    //printf("message: %d\n", code);

    if(code != 150){
        printf("error opening file\n");
        fclose(file);
        close(sockfd);
        close(sockfd2);
        return 1;
    }

    char response[256];
    char path[100];
    int size;
    memcpy(response, &read_buf[44], (strlen(read_buf)-44+1));
    //printf("response: %s\n", response);
    sscanf(response, "%s (%d bytes)", path, &size);
    //printf("size: %d\n", size);
    int new_fd = open(info.path, O_CREAT | O_WRONLY, 0664);
    
    double progress = 0.0;
    int i = 0;
    while(read(sockfd2, read_buf, 1) > 0){
        write(new_fd, read_buf, 1);    
        i++;
        progress = ((double) i/size ) * 100;
        printf("PROGRESS: %f %%\n", progress);
    }

    struct stat st;
    stat(path, &st);
    int fsize = st.st_size;
    if(fsize != size){
        printf("File size is incorrect. Possible data loss/corruption.\n");
    }
    else printf("File size is correct.\n");

    fgets(read_buf, 1024, file);
    printf("response: %s\n", read_buf);
    
    fclose(file);
    close(sockfd);
    close(sockfd2);  

    return 0;
}

