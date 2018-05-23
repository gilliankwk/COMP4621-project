#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>

#include <sys/stat.h>
#include <fcntl.h>

#define MAXLINE 2048
#define SERVER_PORT 12345
#define LISTENNQ 5
#define MAXTHREAD 99

#define OKAY_MSG "HTTP/1.1 200 OK\\r\\n\n"
#define ERROR_MSG "HTTP/1.1 404 Not Found\\r\\n\n"
#define ERROR_PAGE "<!DOCTYPE html><html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\n<p>The requested URL was not found on this server.</p>\n</body></html>\n"
#define OKAY_HTML "Content-Type: text/html\\r\\n\n"
#define OKAY_IMG "Content-Type: image/jpeg\\r\\n\n"
#define OKAY_PDF "Content-Type: application/pdf\\r\\n\n"
#define OKAY_CSS "Content-Type: text/css\\r\\n\n"
#define CONNTECTION "Connection: Keep-Alive\\r\\n\n\\r\\n\n"

void* request_func(void *args);

int main(int argc, char **argv)
{
        int listenfd, connfd;
        
	struct sockaddr_in servaddr, cliaddr;
        socklen_t len = sizeof(struct sockaddr_in);
        char buff[MAXLINE] = {0};
        
	char ip_str[INET_ADDRSTRLEN] = {0};
        
	time_t ticks;
	int threads_count = 0;
	pthread_t threads[MAXTHREAD];

		
        /* initialize server socket */
        listenfd = socket(AF_INET, SOCK_STREAM, 0); /* SOCK_STREAM : TCP */
        if (listenfd < 0) {
                printf("Error: init socket\n");
                return 0;
        }

        /* initialize server address (IP:port) */
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY; /* IP address: 0.0.0.0 */
        servaddr.sin_port = htons(SERVER_PORT); /* port number */

        /* bind the socket to the server address */
        if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) < 0) {
                printf("Error: bind\n");
                return 0;
        }

        if (listen(listenfd, LISTENNQ) < 0) {
                printf("Error: listen\n");
                return 0;
        }
		
		
	/* keep processing incoming requests */
        while (1) {
                /* accept an incoming connection from the remote side */
                connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len);
                if (connfd < 0) {
                        printf("Error: accept\n");
                        return 0;
                }

                 /* print client (remote side) address (IP : port) */
                inet_ntop(AF_INET, &(cliaddr.sin_addr), ip_str, INET_ADDRSTRLEN);
                printf("Incoming connection from %s : %hu with fd: %d\n", ip_str, ntohs(cliaddr.sin_port), connfd);

		/* create dedicate thread to process the request */
		if (pthread_create(&threads[threads_count], NULL, request_func, (void *)connfd) != 0) {
			printf("Error when creating thread %d\n", threads_count);
			return 0;
		}

		if (++threads_count >= MAXTHREAD) {
			break;
		}
        }
        printf("Max thread number reached, wait for all threads to finish and exit...\n");
	//for (int i = 0; i < MAXTHREAD; ++i) {
	int i;
	for(i = 0; i < MAXTHREAD; i++) {
		pthread_join(threads[i], NULL);
	}

        return 0;
}

void* request_func(void *args)
{
	/* get the connection fd */
	int connfd = (int)args;
        char buff[MAXLINE] = {0};
	char out_buff[MAXLINE] = {0};
	char temp[MAXLINE] = {0};
	int fdimg = 0;
	
	FILE *fp = NULL;
	int file_len = 0;
	
	memset(buff, 0, 2048);
	//read(connfd, buff, 2047);
	if(read(connfd, buff, 2047) < 0) {
	  printf("An error occurred in the read.\n\n");
	}
	printf("%s\n\n", buff);

	
	//printf("heavy computation\n");
	//sleep(10);
	
	
	if(!strncmp(buff, "GET /img.jpg", 10)) {
	  //printf("Loading image...\n\n");
	  
	  /* get file information */
	  fp = fopen("img.jpg", "r");
	  if(fp == NULL) {
	    printf("Error opening file...\n\n");
	  }
	  fseek(fp, 0, SEEK_END);
	  file_len = ftell(fp);
	  
	  
	  fclose(fp);
	  
	  /* send the file to client */
	  fdimg = open("img.jpg", O_RDONLY);
	  sendfile(connfd, fdimg, NULL, file_len + 200);
	  
	  
	  /* construct HTTP response */
	  char out_len[MAXLINE] = {0};
	  strcpy(out_len, "Content-Length: ");
	  char int_to_str[MAXLINE] = {0};
	  sprintf(int_to_str, "%d", file_len);
	  strcat(out_len, int_to_str);
	  strcat(out_len, "\\r\\n\n");
	    
	  strcpy(temp, OKAY_MSG);
	  strcat(temp, OKAY_IMG);
	  strcat(temp, out_len);
	  strcat(temp, CONNTECTION);
	  strcpy(out_buff, temp);
	  printf(out_buff);

	  if(close(fdimg) < 0) {
	    printf("Error in closing file...\n\n");
	  }
	}
	else if(!strncmp(buff, "GET /style.css", 10)) {
	  //printf("Loading CSS...\n\n");
	  
	  /* get file information */
	  fp = fopen("style.css", "r");
	  if(fp == NULL) {
	    printf("Error opening file...\n\n");
	  }
	  fseek(fp, 0, SEEK_END);
	  file_len = ftell(fp);
	  
	  
	  char fbuf[4096];
	  fseek(fp, 0, SEEK_SET);
	  fread(fbuf, file_len+1, 1, fp);  
	   
	  
	  fclose(fp);
	  
	  /* send  the file to client */
	  fdimg = open("style.css", O_RDONLY);
	  sendfile(connfd, fdimg, NULL, file_len + 200);
	  
	  /* construct HTTP reponse */
	  char out_len[MAXLINE] = {0};
	  strcpy(out_len, "Content-Length: ");
	  char int_to_str[MAXLINE] = {0};
	  sprintf(int_to_str, "%d", file_len);
	  strcat(out_len, int_to_str);
	  strcat(out_len, "\\r\\n\n");
	    
	  strcpy(temp, OKAY_MSG);
	  strcat(temp, OKAY_CSS);
	  strcat(temp, out_len);
	  strcat(temp, CONNTECTION);
	  strcpy(out_buff, temp);
	  printf(out_buff);
	  printf("\nstyle.css file content:\n%s\n", fbuf);

	  if(close(fdimg) < 0) {
	    printf("Error in closing file...\n");
	  }
	}
	else if(!strncmp(buff, "GET /doc.pdf", 10)) {
	  //printf("Loading pdf...\n\n");
	  
	  /* get file information */
	  fp = fopen("doc.pdf", "r");
	  if(fp == NULL) {
	    printf("Error opening file...\n\n");
	  }
	  fseek(fp, 0, SEEK_END);
	  file_len = ftell(fp);
	  
	  fclose(fp);
	  
	  fdimg = open("doc.pdf",O_RDONLY);
	  sendfile(connfd, fdimg, NULL, file_len + 200);
	  
	  
	  /* construct HTTP response */
	  char out_len[MAXLINE] = {0};
	  strcpy(out_len, "Content-Length: ");
	  char int_to_str[MAXLINE] = {0};
	  sprintf(int_to_str, "%d", file_len);
	  strcat(out_len, int_to_str);
	  strcat(out_len, "\\r\\n\n");
	    
	  strcpy(temp, OKAY_MSG);
	  strcat(temp, OKAY_PDF);
	  strcat(temp, out_len);
	  strcat(temp, CONNTECTION);
	  strcpy(out_buff, temp);
	  printf(out_buff);

	  if(close(fdimg) < 0) {
	    printf("Error in closing file...\n\n");
	  }
	}
	else {
	  if(!strncmp(buff, "GET /webpage.html", 10)) {
	    //printf("Loading HTML1...\n");
	    
	    /* get file information */	    
	    fp = fopen("webpage.html", "r");
	    if(fp == NULL) {
	      printf("Error opening file...\n\n");
	    }
	    fseek(fp, 0, SEEK_END);
	    file_len = ftell(fp);
	    
	    char fbuf[4096];
	    fseek(fp, 0, SEEK_SET);
	    fread(fbuf, file_len+1, 1, fp);
	    
	    fclose(fp);
	    
	    /* send the file to client */
	    fdimg = open("webpage.html",O_RDONLY);
	    sendfile(connfd, fdimg, NULL, file_len + 200);
	    
	    /* construct HTTP response */
	    char out_len[MAXLINE] = {0};
	    strcpy(out_len, "Content-Length: ");
	    char int_to_str[MAXLINE] = {0};
	    sprintf(int_to_str, "%d", file_len);
	    strcat(out_len, int_to_str);
	    strcat(out_len, "\\r\\n\n");
	    
	    strcpy(temp, OKAY_MSG);
	    strcat(temp, OKAY_HTML);
	    strcat(temp, out_len);
	    strcat(temp, CONNTECTION);
	    strcpy(out_buff, temp);
	    
	    printf(out_buff);
	    printf("\nCSS file content:\n%s\n", fbuf);
	    

	    if(close(fdimg) < 0) {
	      printf("Error in closing file...\n\n");
	    }
	  }
	  else if(!strncmp(buff, "GET /testpage.html", 10)) {
	  //printf("Loading HTML2...\n");
	  
	  /* get file information */
	  fp = fopen("testpage.html", "r");
	  if(fp == NULL) {
	    printf("Error opening file...\n\n");    
	 }
	  fseek(fp, 0, SEEK_END);
	  file_len = ftell(fp);
	  
	  char fbuf[4096];
	  fseek(fp, 0, SEEK_SET);
	  fread(fbuf, file_len+1, 1, fp);
	  
	  fclose(fp);
	  
	  /* send the file to client */
	  fdimg = open("testpage.html",O_RDONLY);
	  sendfile(connfd, fdimg, NULL, file_len + 200);
	    
	  /* construct HTTP response */
	  char out_len[MAXLINE] = {0};
	  strcpy(out_len, "Content-Length: ");
	  char int_to_str[MAXLINE] = {0};
	  sprintf(int_to_str, "%d", file_len);
	  strcat(out_len, int_to_str);
	  strcat(out_len, "\\r\\n\n");
	   
	  strcpy(temp, OKAY_MSG);
	  strcat(temp, OKAY_HTML);
	  strcat(temp, out_len);
	  strcat(temp, CONNTECTION);
	  strcpy(out_buff, temp);
	  printf(out_buff);
	  printf("\ntestpage.html file content:\n%s\n", fbuf);

	  if(close(fdimg) < 0) {
	    printf("Error in closing file...\n\n");
	  }
	}
	  else {
	    //printf("Loading error page...\n");

	    int error_len = strlen(ERROR_PAGE);
	    
	    /* construct HTTP reponse */
	    strcpy(temp, ERROR_MSG);
	    strcat(temp, OKAY_HTML);
	    strcat(temp, "Content-Length:");
	    char int_to_str[MAXLINE] = {0};
	    sprintf(int_to_str, "%d", error_len);
	    strcat(temp, int_to_str);
	    strcat(temp, "\\r\\n\n");
	    strcpy(out_buff, temp);
	    printf(out_buff);
	    printf("\nError page content:\n%s\n", ERROR_PAGE);
	    
	    /* send the file to client */
	    write(connfd, ERROR_PAGE, strlen(ERROR_PAGE));
	    
	  }
	}
	
       
  	/* prepare for the send buffer */ 
        //snprintf(buff, sizeof(buff) - 1, "This is the content sent to connection %d\r\n", connfd);

	//strcpy(buff, error_page);
	//printf("Hello");
	
	/* write the buffer to the connection */
	//write(connfd, buff, strlen(buff));
	
	close(connfd);
	printf("\n---Finished one request!---\n\n");
}