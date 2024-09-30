/* inet_str_client.c: Internet stream sockets client */
#include <stdio.h>
#include <sys/types.h>	     /* sockets */
#include <sys/socket.h>	     /* sockets */
#include <netinet/in.h>	     /* internet sockets */
#include <unistd.h>          /* read, write, close */
#include <netdb.h>	         /* gethostbyaddr */
#include <stdlib.h>	         /* exit */
#include <string.h>	         /* strlen */

void perror_exit(char *message);

int main(int argc, char *argv[]) {
    int             port, sock;

    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr*)&server;
    struct hostent *rem;
    if (argc < 4) {

	
    	printf("Please give host name and port number\n");
       	exit(1);}
	/* Create socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    	perror_exit("socket");
        
	/* Find server address */
    if ((rem = gethostbyname(argv[1])) == NULL) {	
	   herror("gethostbyname"); exit(1);
    }

    //server setup
    port = atoi(argv[2]); /*Convert port number to integer*/
    server.sin_family = AF_INET;       /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(port);         /* Server port */

    /* Initiate connection */
    if (connect(sock, serverptr, sizeof(server)) < 0)
	   perror_exit("connect");
    printf("Connecting to %s port %d\n", argv[1], port);

    //copying command line arguments into string array which we are gonna send
    int total_length=0;
    for(int i=3;i<argc;i++)
    {
      total_length+=strlen(argv[i]);
    }
    int index=0;
    char* buffer=malloc((total_length+argc)*sizeof(char));
    if(buffer==0)
    {
      perror("malloc");
      exit(-1);
    }
    for(int i=3;i<argc;i++){
      if(index>0)
      {
        buffer[index]=' ';
        index++;
      }
      strcpy(buffer+index,argv[i]);
      index+=strlen(argv[i]);
    }
    int nwrite;
    int baseline=strlen(buffer);
    if ((nwrite=write(sock, &baseline, sizeof(baseline))) == -1)

		{ perror("Error in Writing");free(buffer); exit(2); }
			
    if ((nwrite=write(sock,buffer ,baseline)) == -1) //on the other side we're sending the keyword and the word right after. So if we did "./jobCommander issueJob ls"
		{ perror("Error in Writing"); free(buffer); exit(2); }
    
    if (read(sock, &baseline, sizeof(int)) < 0) 
    {
				perror("problem in reading message size");
				close(sock);
				exit(-2);
		}

    buffer=malloc(baseline*sizeof(char));
    if(buffer==0)
    {
      perror("malloc fail in Commander reading");
      exit(-1);
    }

    if (read(sock, buffer, baseline) < 0) 
    {
				perror("problem in reading message value");
				close(sock);
				exit(3);
		}
      printf("\n %s\n",buffer);
                  

    if(strcmp(argv[3],"issueJob")==0){
      if (read(sock, &baseline, sizeof(int)) < 0) {
				perror("problem in reading message size");
				close(sock);
				exit(-2);
			}

    buffer=malloc(baseline*sizeof(char));

    if(buffer==0){
      perror("malloc fail in Commander reading");
      exit(-1);
    }

    if (read(sock, buffer, baseline) < 0) {
				perror("problem in reading message value");
				close(sock);
				exit(3);
			}
      printf("\n %s",buffer);
    }
     /* Close socket and exit */
    close(sock); 
    free(buffer); 
}			     



void perror_exit(char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}