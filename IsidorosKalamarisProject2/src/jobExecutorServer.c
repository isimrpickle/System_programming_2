/*inet_str_server.c: Internet stream sockets server */
#define _XOPEN_SOURCE 700//necessary for sigaction, if your'e interested check https://stackoverflow.com/questions/6491019/struct-sigaction-incomplete-error
#include <stdio.h>
#include <sys/wait.h>	     /* sockets */
#include <sys/types.h>	     /* sockets */
#include <sys/socket.h>	     /* sockets */
#include <netinet/in.h>	     /* internet sockets */
#include <netdb.h>	         /* gethostbyaddr */
#include <unistd.h>	         /* fork */		
#include <stdlib.h>	         /* exit */
#include <ctype.h>	         /* toupper */
#include <signal.h>          /* signal */
#include <pthread.h>        //threads 
#include <string.h>         //strtok,strcat ...
#include <errno.h>          //perror
#include "dynamicqueue.h"   //saving jobs into the buffer(queue)
#include <sys/stat.h> //i use struct stat

//defining functions

                        //jobExecutorServer [portnum] [bufferSize] [threadPoolSize]
int counter=0;
int running_processes=0;
int concurrency =1;
int flag_variable=1;
pthread_t mainID;
int buffersize; //buffersize is global so it can be initialized in main and still be accesed in the thread function
struct Queue* jobs_to_be_executed; //it will contain the commands from issueJob
pthread_mutex_t mtx; // will be used for mutual exclusion on the above queue and will help implement the conditions on threadpoolsize/buffersize/concurrecny
pthread_mutex_t exitmtx ; //will be used for implementing the exit. We needed another mutex because using mtx was causing a deadlock
int threadpoolsize;
pthread_t* workert;
pthread_cond_t worker_condition ; //conditional variables which will be used for (waiting and) signaling the worker_thread
pthread_cond_t controller_thread;
pthread_cond_t exitcond,exit_after_commands; //both will be used for implementing the exit feature.
//pthread_cond_t buffercond;

void signal_sigchld(){
        pthread_mutex_lock(&mtx);
        running_processes--;
        pthread_cond_signal(&worker_condition);
        pthread_mutex_unlock(&mtx);        
   }

//thread functions
void* controller(void* newsock);
void* worker_thread();




int main(int argc, char *argv[]) {

//setting up the socket communication, exactly like in lecture 10 notes.

    int             port, sock;
    struct sockaddr_in server, client;
    socklen_t clientlen;

    struct sockaddr *serverptr=(struct sockaddr *)&server;
    struct sockaddr *clientptr=(struct sockaddr *)&client;
    threadpoolsize=atoi(argv[3]);
    pthread_t controllert;


    pthread_mutex_init (&mtx , NULL ) ; /* Initialize mutex */
    pthread_mutex_init(&exitmtx,NULL);
    pthread_cond_init(&exitcond,NULL);
    pthread_cond_init(&exit_after_commands,NULL);
    pthread_cond_init (&worker_condition , NULL ) ; /* and condition variable */
    pthread_cond_init(&controller_thread,NULL);


    if (argc != 4) {
        printf("Please give port number,buffer size and thread_pool size\n");exit(1);}
    port = atoi(argv[1]);

    //create signal
    struct sigaction sigchld_action = {0};
	sigchld_action.sa_flags= SA_RESTART;
	sigchld_action.sa_handler=&signal_sigchld;
	sigaction(SIGCHLD,&sigchld_action,NULL);

    /* Create socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror("socket");
    server.sin_family = AF_INET;       /* Internet domain */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);      /* The given port */
    /* Bind socket to address */

    if (bind(sock, serverptr, sizeof(server)) < 0)
        perror("bind");

    /* Listen for connections */
    if (listen(sock, 5) < 0) perror("listen");
    printf("Listening for connections to port %d\n", port);
    
    //create the command buffer
    buffersize=atoi(argv[2]);
    jobs_to_be_executed=create_queue(); //the queue where the jobs from issuejob are gonna be held.
       
    workert = malloc ( threadpoolsize * sizeof ( pthread_t ) ) ;
    for ( int i =0; i<threadpoolsize; i++) 
        {
            struct queuejob*jobtriplet=malloc(sizeof(struct queuejob)); 
            pthread_create( workert+i , NULL , &worker_thread ,(void*) jobtriplet) ;
            
        }   
    mainID=pthread_self();

    while (1) {
       struct queuejob*jobtriplet=malloc(sizeof(jobtriplet)); 
        

        if(jobtriplet==NULL){
            perror("malloc of triplet struct failed");
            exit(-1);
        }
        /* accept connection */
        int newsock;

    	if ((newsock = accept(sock, clientptr, &clientlen)) < 0) perror("accept");
        printf("Accepted connection\n");


        jobtriplet->file_descriptor=newsock;
      
    	
       
    	pthread_create(&controllert,NULL,&controller,(void*)(jobtriplet));
    	
        
        
    }
    delete_queue(jobs_to_be_executed);
    pthread_mutex_destroy(&mtx);
    pthread_mutex_destroy(&exitmtx);
    pthread_cond_destroy(&exitcond);
    pthread_cond_destroy(&exit_after_commands);
    pthread_cond_destroy(&worker_condition);
    pthread_cond_destroy(&controller_thread);
}

void* controller(void* newsock) {
    char*controller_message; //will be used to return a message back to the client(Commander)
    char* msgbuf; //It accepts the jobCommander message. Also with this we will create the array of strings ready for execvp
    int message_size=0;
    struct queuejob* ptrsock = (struct queuejob *)newsock;    	

    
    if (read(ptrsock->file_descriptor, &message_size, sizeof(int)) < 0) {
            perror("problem in reading message size");
            close(ptrsock->file_descriptor);
            pthread_exit(NULL);
            exit(3);
       	 }
    msgbuf = malloc((message_size + 1) * sizeof(char));//for adding the null terminated character
    if (msgbuf==NULL) {
        perror("memory allocation failed");
        close(ptrsock->file_descriptor);
        pthread_exit(NULL);
        exit(4);
    }

          if (read(ptrsock->file_descriptor, msgbuf, message_size) < 0) {
            perror("problem in reading message");
            free(msgbuf);
            close(ptrsock->file_descriptor);
            pthread_exit(NULL);
            exit(5);
        }
    msgbuf[message_size]='\0'; //adding null termination value.


    //getting ready for tokenization
    int spaces = 0; //indicator of how many arguments are there after the keyword_command of Commander(issuejob)
	for (int i=0; msgbuf[i]!='\0'; i++)
    {
    	if(msgbuf[i]==' ')
        	spaces++;
	}
    char ** arguments= malloc((spaces+2)*sizeof(char*)); //we need one more extra for null termination
    if (arguments==NULL)
    {
		perror("memory allocation failed");
		close(ptrsock->file_descriptor); // closing connection in case of emergency exit
        pthread_exit(NULL);
		exit(-1);
	}
    arguments[0]='\0';
    char *token=strtok(msgbuf," ");
    char *data=malloc((message_size + spaces) * sizeof(char)); //data will practically be the new msgbuf
    if(data==NULL){
        perror("something wrong with data's malloc\n");
        exit(-2);
    }
    data[0]='\0'; //If I don't do that the string starts with gibberish. More on that here: https://stackoverflow.com/questions/69851898/gibberish-data-being-stored-in-memory-while-concatenating-the-strings-using-str
    for(int i=0; i<=spaces; i++){ //we're using spaces because it's the same amount as the args.
			
			
        strcat(data,token); //strtok modifies the buffer which divides. That's why we copy it into data, so we still have access to it later.
        arguments[i] = token;
        if(i<spaces)
            strcat(data," "); //if it's not the last argument leave a space for the next one.
        
        token = strtok(NULL," ");
        
    } 
    arguments[spaces+1]=NULL;
  
        

       
    if(strcmp(arguments[0],"issueJob")==0)
    {
        ptrsock->job=malloc((strlen(data)+1)*sizeof(char));
        if(ptrsock->job==NULL){
        perror("malloc in job failed ");
        exit(-1);
        }
        //formatting the message we're gonna send back in the Commander

        int i=1;
        while(arguments[i]!=NULL){
            strcat(ptrsock->job,arguments[i]);
            if(i<spaces)
            {
                strcat(ptrsock->job," ");
            }
            i++;
        }
        

        sprintf(ptrsock->jobID,"job_%d",counter);
        ptrsock->jobID[6]='\0';
        counter++;
        controller_message=malloc(strlen(ptrsock->job)+20*sizeof(char));
        if(controller_message==NULL){
            perror("controller message problem with malloc");
        }
        sprintf(controller_message,"JOB <%s,%s>",ptrsock->jobID,ptrsock->job);
        int total_length=strlen(controller_message);

        if((write(ptrsock->file_descriptor,&total_length,sizeof(total_length)))==-1)
        {
            perror("Error in Writing"); 
		    exit(-2); 
        }

        if((write(ptrsock->file_descriptor,controller_message,total_length))==-1)
        {
            perror("Error in Writing"); 
		    exit(2); 
        }


       
        if(pthread_mutex_lock(&mtx))//returns zero if it's seccesfull,we're gonna enqueue to buffer so we need to lock the mutex
        { 
            perror("error on locking the mutex");
        }

        //waiting in order to not exceed the buffersize the user provided
        while(jobs_to_be_executed->size==buffersize || flag_variable==0) //so it wont be possible for more jobs to be enqeueued after exit
        {
            pthread_cond_wait(&controller_thread , &mtx); //signaled by worker thread when a job gets deququed.
        }   
            
            enqueue(jobs_to_be_executed,ptrsock);
            pthread_cond_signal(&worker_condition);//signaling because now the buffer(queue) is not empty
        
        
        
        
        if(pthread_mutex_unlock(&mtx)) perror("error unlocking the mutex"); //unlocking mutex now that we're done with the common buffer(queue)

        
        
    }

    else if(strcmp(arguments[0],"setConcurrency")==0)
    {
        concurrency=atoi(arguments[1]);
        controller_message=malloc(25*sizeof(char));
        if(controller_message==NULL){
            perror("controller message problem with malloc");
            exit(-2);
        }
        sprintf(controller_message,"Concurrency set at %d\n",concurrency);
        int total_length=strlen(controller_message);
        pthread_cond_broadcast(&worker_condition); //waking up every thread there is on wait in ordeer to fully take advantage of the concurrency
                                                    //When i did just signal,only 1 more thread was waken up right after the new concurrecny.

        if((write(ptrsock->file_descriptor,&total_length,sizeof(total_length)))==-1)
        {
            perror("Error in Writing"); 
		    exit(-2); 
        }

        if((write(ptrsock->file_descriptor,controller_message,total_length))==-1)
        {
            perror("Error in Writing"); 
            exit(2); 
        }

    }

    else if(strcmp(arguments[0],"stop")==0){

        char* jobID_to_stop=arguments[1];
        int if_found= stop_job(jobs_to_be_executed,jobID_to_stop); //this is a queue function inside the dynamicqueue.h
        controller_message=malloc(25*sizeof(char));
        if(controller_message==NULL){
            perror("controller message problem with malloc");
            exit(-2);
        }

        if(if_found ==1)
            sprintf(controller_message,"JOB <%s> REMOVED",arguments[1]);
        else
            sprintf(controller_message,"JOB <%s> NOT FOUND",arguments[1]);
        

        int total_length=strlen(controller_message);
        if((write(ptrsock->file_descriptor,&total_length,sizeof(total_length)))==-1)
        {
            perror("Error in Writing"); 
            exit(-2); 
        }

        if((write(ptrsock->file_descriptor,controller_message,total_length))==-1)
        {
            perror("Error in Writing"); 
            exit(2); 
        }

    }
    else if(strcmp(arguments[0],"poll")==0)
    {

        char* stringToBePrinted=malloc((jobs_to_be_executed->size+2)*sizeof(char));
        if (stringToBePrinted==NULL){
            perror("problem with mallocing stringtobeprinted");
            exit(-2);
        }
        stringToBePrinted=poll_queue(jobs_to_be_executed);
        int total_length=strlen(stringToBePrinted);

        if((write(ptrsock->file_descriptor,&total_length,sizeof(total_length)))==-1)
        {
            perror("Error in Writing"); 
            exit(-2); 
        }

        if((write(ptrsock->file_descriptor,stringToBePrinted,total_length))==-1)
        {
            perror("Error in Writing"); 
            exit(2); 
        }

        free(stringToBePrinted);

    }

    else if(strcmp(arguments[0],"exit")==0)
    { 
       
        
        flag_variable=0; //letting the other threads know that exit has been called
        // if we call exit without issueing any job first we don't have to do all of those waits
        if(jobs_to_be_executed->size>0 || running_processes>0 )
        {
            pthread_mutex_lock(&exitmtx);

            pthread_cond_wait(&exitcond,&exitmtx);
            pthread_cond_wait(&exit_after_commands,&exitmtx);
            pthread_mutex_unlock(&exitmtx);
        }
            

            controller_message=malloc(25*sizeof(char));
            if(controller_message==NULL){
                perror("controller message problem with malloc");
                exit(-2);
            }
            sprintf(controller_message,"SERVER TERMINATED");
            int total_length=strlen(controller_message);
            if((write(ptrsock->file_descriptor,&total_length,sizeof(total_length)))==-1)
                {
                    perror("Error in Writing"); 
                    exit(-2); 
                }

                if((write(ptrsock->file_descriptor,controller_message,total_length))==-1)
                {
                    perror("Error in Writing"); 
                    exit(2); 
                }
            free(controller_message);
            close(ptrsock->file_descriptor);
            free(ptrsock->job);
            free(ptrsock);
            delete_queue(jobs_to_be_executed);
            pthread_kill(mainID,SIGTERM);
        

    }

    
    free(msgbuf);
    free(arguments);
    pthread_exit(NULL);


    
}



void*worker_thread(){
    char* filename=malloc(20*sizeof(char)); //pid.output 20 bytes are more than enough
    if(filename==NULL){
        perror("something wrong in mallocing filename");
        exit(-4);
    }
    while(1){
        
       

        
        FILE *job_output;
        

        if(pthread_mutex_lock(&mtx))//returns zero if it's seccesfull
        { 
            perror("error on locking the mutex");
        }

        //Enforcing the worker_threads to run in the background. The other condition is for enforcing the concurrency limitations 
        while((jobs_to_be_executed->size==0 && flag_variable!=0) || (running_processes>=concurrency) )
        {
            pthread_cond_wait(&worker_condition,&mtx);
        }

        //If exit has been called and the buffer(queue) is empty we signal the "exit" function
        while(jobs_to_be_executed->size==0 && flag_variable==0){
            pthread_cond_signal(&exitcond);
        }

        running_processes++; // is getting reduced at sigchld signal.
        struct queuejob dequeuedJob;
        dequeuedJob=dequeue(jobs_to_be_executed);
        pthread_cond_signal(&controller_thread); //we signal the controller thread because now the buffersize has space
        
       
        if(pthread_mutex_unlock(&mtx))//returns zero if it's seccesfull
        { 
            perror("error on locking the mutex");
        }


        //dup2 and executing the command in the child. We use wait that waits(NO WNOHANG) until the job has finished in order to respect the threadpoolsize

        int spaces=0; 
        if(dequeuedJob.job!=NULL)
        {
            for (int i=0; dequeuedJob.job[i]!='\0'; i++)
            {
    	        if(dequeuedJob.job[i]==' ') //finding how many spaces the job from commande actually has.
        	    spaces++;
	        }
        
            char **jobexec=malloc((spaces+2)*sizeof(char*)); //it will be passed in execvp inside fork
            if(jobexec==NULL){
                perror("something wrong in mallocing jobexec ");
                exit(-1);
            }
            char* token=strtok(dequeuedJob.job," ");

            for( int i=0;i<=spaces;i++)
            {
                jobexec[i]=token; 
                token = strtok(NULL," ");
            }

            jobexec[spaces+1]=NULL; //the execvp needs the string  array to end with a null terminator.
            pid_t child=fork();
            if(child==0)
            {
                int childpid=getpid();
                sprintf(filename,"%d.output",childpid); //formatting the name of the file where the redirection will happen.
                job_output=fopen(filename,"w");
                int job_output_fd = fileno(job_output);
                if(dup2(job_output_fd, STDOUT_FILENO)==-1){   
                    perror("something went wrong with dup2");
                    exit(-2);
                }
                fclose(job_output);

                if(execvp(jobexec[0], jobexec)==-1)
                {
                    perror("something is wrong in execvp:");
                    exit(-1);
                    
                }
                    
                
                exit(1);
                
            }   
            free(jobexec);
            
            int childpid= waitpid(0,NULL,0); //waitpid returns the pid of the child that died.
            
            sprintf(filename,"%d.output",childpid);

            //reading the job's output and sending to commnader through the socket
            job_output=fopen(filename,"r");
            if(job_output==0){
                perror("something went wrong opening the file in reading mode");
                exit(-2);
            } 
            struct stat stat; // in order to chech the size of the file.


            fstat(fileno(job_output),&stat); //https://stackoverflow.com/questions/2029103/correct-way-to-read-a-text-file-into-a-buffer-in-c
            int total_length= stat.st_size;
            char*temp_buffer = malloc((total_length+10)*sizeof(char));
            if(temp_buffer==NULL){
                perror("wrong in malloc of temp_buffer ");
                exit(-2);
            }

            //reading the file
            size_t newLen = fread(temp_buffer, sizeof(char), total_length, job_output);
            if ( ferror( job_output ) != 0 ) {
                fputs("Error reading file", stderr);
            } else {
                temp_buffer[newLen-1] = '\0'; /* Just to be safe. */
            }
            if((write(dequeuedJob.file_descriptor,&total_length,sizeof(total_length+1)))==-1)
            {
                perror("Error in Writing"); 
                exit(-2); 
            }
            if((write(dequeuedJob.file_descriptor,temp_buffer,total_length))==-1)
            {
                perror("Error in Writing \n"); 
                exit(2); 
            } 
            if(jobs_to_be_executed->size==0 )
                pthread_cond_signal(&exit_after_commands);
            fclose(job_output);
            remove(filename);
            free(temp_buffer); 
        
        }
        shutdown(dequeuedJob.file_descriptor,SHUT_RDWR);
        close(dequeuedJob.file_descriptor);	  // Close socket      
    }
   
    
    pthread_detach(pthread_self());
    pthread_exit(NULL);
}
