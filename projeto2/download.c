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
	printf("It: %d\n Aux: %d\n", it, aux);
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

int readResponse(FILE* file, char* response){
	do{
		if(fgets(response, 1024, file) == NULL) break;
		printf("%s\n", response);
		
	} while(response[3] != ' ');
	char aux[3];
	strncpy(aux, response, 3);
	return atoi(aux);
}

int main(int argc, char*argv[]){

    FILE *file;
    
    url_info info;
    char *url = argv[1];
    printf("url: %s\n", url);

    if(parseURL(url, &info)) return 1;

	char buf[2000] = ""; 
    char read_buf[2000] = "";

    struct hostent *h = getHostName(&info);

    char *ip = inet_ntoa(*((struct in_addr *)h->h_addr));
    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n",ip);

    int sockfd = createSocket(ip, 21);

    file = fdopen(sockfd, "r");
	readResponse(file, read_buf);
    

    sprintf(buf, "user %s\n", info.user);
    printf("buf: %s\n", buf);

	    /*send a string to the server*/
	write(sockfd, buf, strlen(buf));
	readResponse(file, read_buf);
    

    sprintf(buf, "pass %s\n", info.password);
    printf("buf: %s\n", buf);

    write(sockfd, buf, strlen(buf));
	if(readResponse(file, read_buf) != 230){
		printf("Login Failed\n");
		return 1;
	}
    

    sprintf(buf, "pasv\n");
    printf("buf: %s\n", buf);

    write(sockfd, buf, strlen(buf));

	if(readResponse(file, read_buf) != 227){
		printf("Error entering Passive Mode\n");
		return 1;
	}
	

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

	readResponse(file, read_buf);

    
    int code;

    sscanf(read_buf, "%d", &code);


    if(code != 150){
        printf("Error opening file\n");
        fclose(file);
        close(sockfd);
        close(sockfd2);
        return 1;
    }

    char response[256];
    char path[100];
    long int size;
    memcpy(response, &read_buf[44], (strlen(read_buf)-44+1));

    sscanf(response, "%s (%ld bytes)", path, &size);

	char* filename;
	filename = strrchr(info.path, '/')+1;
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
	
	readResponse(file, read_buf);
    
    fclose(file);
    close(sockfd);
    close(sockfd2);  

    return 0;
}

