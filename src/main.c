/******************************************************************************
# file: /home/TU/tug35668/lab/cis_3207/lab_3/main.c
#
# Tarek Elseify
# March 11, 2019
# Lab 3 - Networked Spell Checker
# CIS 3207 SEC 001
# tug35668@temple.edu
#
# This program creates a networked spell checker using a thread
# safe system
******************************************************************************/

/* standard include statements */
/*                             */
#include "simple_server.h"

/* define default variables */
/*                          */
#define DEFAULT_DICTIONARY "dict.txt"
#define DEFAULT_PORT 10000
#define MAX_WORDS 99999
#define NUM_THREADS 2

/* set the command line options */
/*                              */
char *CMDL_OPTS[] = {"-p", "-d", NULL};
char *DICT_NAME;
int CONNECTION_PORT;

/* the queues will be available to all threads */
/*                                             */
queue *SOCKETS;
queue *LOGS;

/* hold the dictionary values */
/*                            */
char *DICT[MAX_WORDS];

/* lock for the SOCKETS and log queue */
/*                                    */
pthread_mutex_t SOCKETS_LOCK = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t LOG_LOCK = PTHREAD_MUTEX_INITIALIZER;

/* condition variables for Socket and log */
/*                                        */
pthread_cond_t Q_FULL = PTHREAD_COND_INITIALIZER;
pthread_cond_t Q_EMPTY = PTHREAD_COND_INITIALIZER;
pthread_cond_t LOG_FULL = PTHREAD_COND_INITIALIZER;
pthread_cond_t LOG_EMPTY = PTHREAD_COND_INITIALIZER;

/* log file */
/*          */
FILE *LOG_FILE;

/* function prototypes are listed here */
/*                                     */
void arg_parser(int argc, char *argv[]);
void set_dict(char *dict_file);
void *worker_thread(void *arg);
void add_entry(Socket *log);
void *log_thread(void *arg);
int spell_check(char *word);
void print_usage();
int client_exit(char *entry);


/******************************************************************************
#   function: main
#
#   arguments: none
#
#   returns: none
#
#   this is the main function of the program
******************************************************************************/
int main(int argc, char *argv[])
{

  /* open the log file to write */
  /*                            */
  if((LOG_FILE = fopen("log.txt", "w")) == NULL){
    fprintf(stderr, "%s\n", "failed to open log file to write");
    exit(0);
  }
  
  /* if no arguments were passed, set default values */
  /*                                                 */
  if(argc == 1){
    char *dict_file = DEFAULT_DICTIONARY;
    DICT_NAME = dict_file;
    int port_num = DEFAULT_PORT;
    CONNECTION_PORT = port_num;
  }

  /* only 1, 3, or 5 parameters can be passed */
  /*                                          */
  else if((argc % 2 == 0) || argc > 5){
    print_usage();
    exit(0);
  }

  /* set the user provided invalid arguments */
  /*                                         */
  else {
    arg_parser(argc, argv);
  }
  
  /* set the dicitionay values */
  /*                           */
  set_dict(DICT_NAME);

  /* write the dictionary name and port number */
  /*                                           */
  fprintf(LOG_FILE, "dictionary: %s\nport_no: %d\n\n", DICT_NAME, CONNECTION_PORT);

  /* flush the output stream */
  /*                         */
  fflush(LOG_FILE);

  /* create the socket and log queues */
  /*                                  */
  SOCKETS = create_queue();
  LOGS = create_queue();

  /* define the sockaddr struct as client */
  /*                                      */
  struct sockaddr_in client;

  /* gets the size of the struct */
  /*                             */
  int clientLen = sizeof(client);

  /* vars to be used to hold information */
  /*                                     */
  int connectionSocket, clientSocket;
  
  /* opens a Socket connection */
  /*                           */
  connectionSocket = open_listenfd(CONNECTION_PORT);

  /* if we failed to open a connection */
  /*                                   */
  if(connectionSocket == -1){
    printf("Could not connect to %d, maybe try another port number?\n", CONNECTION_PORT);
    return -1;
  }

  /* the connection is successful */
  /*                              */
  fprintf(stdout, "%s\n", "connection successful!");
  
  /* create an array of worker threads */
  /*                                   */
  pthread_t thread_pool[NUM_THREADS];

  /* create a single log thread */
  /*                            */
  pthread_t log_th;
  
  /* for the total number of threads */
  /*                                 */
  for(int i=0; i < NUM_THREADS; i++){

    /* create and launch the threads */
    /*                               */
    pthread_create(&thread_pool[i], NULL, &worker_thread, NULL);
  }

  /* launch the log thread */
  /*                       */
  pthread_create(&log_th, NULL, &log_thread, NULL);

  /* current Socket to process */
  /*                           */
  Socket *curr;
  Socket *log;
  
  /* transmit messages indefinitely */
  /*                                */
  while(True){    

    /* if we failed to accept the connection */
    /*                                       */
    if((clientSocket = accept(connectionSocket, (struct sockaddr*)&client, &clientLen)) == -1){
      fprintf(stderr, "%s\n", "could not connect to client");
      continue;
    }

    /* notify that we connected to the client */
    /*                                        */
    fprintf(stdout, "connected to client fd: %d\n", clientSocket);
    
    /* create the struct for SOCKETS */
    /*                               */
    curr = create_socket(clientSocket);
    log = create_log(clientSocket, NULL, ADDED);

    /* add the log to the log queue */
    /*                              */
    add_entry(log);
    
    /* obtain the lock to prevent others from accessing the queue */
    /*                                                            */
    pthread_mutex_lock(&SOCKETS_LOCK);

    /* spin lock if the queue is full */
    /*                                */
    while(fifo_full(SOCKETS)){
      pthread_cond_wait(&Q_FULL, &SOCKETS_LOCK);
    }
    
    /* add to the queue */
    /*                  */
    enqueue(SOCKETS, curr);
    
    /* unlock and allow other threads to work */
    /*                                        */
    pthread_mutex_unlock(&SOCKETS_LOCK);

    /* signal the worker threads */
    /*                           */
    pthread_cond_signal(&Q_EMPTY);
  }

  /* never reached */
  /*               */
  return 0;
}
/*                 */
/* end of function */


/******************************************************************************
#   function: add_entry
#
#   arguments: Socket *log - log to be enqueued
#
#   returns: none
#
#   this function safely adds a log to the log queue
******************************************************************************/
void add_entry(Socket *log)
{

  /* obtain the lock to prevent others from accessing the queue */
  /*                                                            */
  pthread_mutex_lock(&LOG_LOCK);

  /* wait if the log is full */
  /*                         */
  while(fifo_full(LOGS)){
    pthread_cond_wait(&LOG_FULL, &LOG_LOCK);
  }

  /* add to the queue */
  /*                  */
  enqueue(LOGS, log);

  /* unlock and allow other threads to work on queue */
  /*                                                 */
  pthread_mutex_unlock(&LOG_LOCK);

  /* signal the other threads */
  /*                          */
  pthread_cond_signal(&LOG_EMPTY);
}
/*                 */
/* end of function */


/******************************************************************************
#   function: arg_parser
#
#   arguments: void *arg - no arguments are passed
#
#   returns: none
#
#   this function safely writes to a file using a thread
******************************************************************************/
void *log_thread(void *arg)
{

  /* the log to write */
  /*                  */
  Socket *log;

  /* this thread will work forever */
  /*                               */
  while(True){

    /* obtain the lock */
    /*                 */
    pthread_mutex_lock(&LOG_LOCK);

    /* while the queue is empty, wait for condition */
    /*                                              */
    while(fifo_empty(LOGS)){
      pthread_cond_wait(&LOG_EMPTY, &LOG_LOCK);
    }

    /* remove a log from the queue */
    /*                             */
    log = dequeue(LOGS);

    /* unlock */
    /*        */
    pthread_mutex_unlock(&LOG_LOCK);

    /* signal the other threads */
    /*                          */
    pthread_cond_signal(&LOG_FULL);

    /* print the client fd */
    /*                     */
    fprintf(LOG_FILE, "client (fd: %d) ", log->socket_fd);

    /* flush the output stream */
    /*                         */
    fflush(LOG_FILE);
    
    /* if this is a user entry */
    /*                         */
    if(log->user_input != NULL){
      fprintf(LOG_FILE, "has entered \"%s\" ", log->user_input);
      fflush(LOG_FILE);
      free(log->user_input);
    }

    /* print the entry */
    /*                 */
    fprintf(LOG_FILE, "%s\n", log->log_input);
    free(log->log_input);
    
    /* flush the output stream */
    /*                         */
    fflush(LOG_FILE);
    free(log);
  }
}
/*                 */
/* end of function */


/******************************************************************************
#   function: arg_parser
#
#   arguments: int argc - number of arguments
#              char *argv[] - commandline arguments
#
#   returns: none
#
#   this function sets the command line arguments
******************************************************************************/
void arg_parser(int argc, char *argv[])
{

  /* name of the dict file */
  /*                       */
  char *dict_file;

  /* port number */
  /*             */
  int port_no;

  /* boolean values to see if we set either dict or port no */
  /*                                                        */
  int d_set = 0;
  int p_set = 0;

  /* for all the arguments provided (instead of main) */
  /*                                                  */
  for(int arg = 1; arg < argc; arg += 2){

    /* if the current arg is -p */
    /*                          */
    if(strcmp(argv[arg], CMDL_OPTS[0]) == 0){

      /* can not use invalid port numbers */
      /*                                  */
      if((port_no = atoi(argv[arg + 1])) < 1024 || port_no > 65535){
	fprintf(stderr, "%s\n", "main: invalid port number");
	exit(0);
      }

      /* set the CONNECTION_PORT */
      /*                        */
      CONNECTION_PORT = port_no;

      /* we have set the port number */
      /*                             */
      p_set = 1;
    }

    /* if the current argument is -d */
    /*                               */
    else if(strcmp(argv[arg], CMDL_OPTS[1]) == 0){

      /* allocate on the heap */
      /*                      */
      dict_file = (char *) malloc(strlen(argv[arg + 1]));

      /* copy to the dict file */
      /*                       */
      strcpy(dict_file, argv[arg + 1]);

      /* set the dict name */
      /*                   */
      DICT_NAME = dict_file;

      /* we have set this parameter */
      /*                            */
      d_set = 1;
    }

    /* the user input an invalid argument */
    /*                                    */
    else {

      /* prit the usage and exit */
      /*                         */
      print_usage();
      exit(0);
    }    
  }

  /* if the user did not set the dictionary */
  /*                                        */
  if(!d_set){

    /* allocate the dict file var */
    /*                            */
    dict_file = (char *) malloc(strlen(DEFAULT_DICTIONARY) * sizeof(char));

    /* copy the default to dict file */
    /*                               */
    strcpy(dict_file, DEFAULT_DICTIONARY);

    /* set the dict name */
    /*                   */
    DICT_NAME = dict_file;
  }

  /* if the user did not set the port number */
  /*                                         */
  if(!p_set){

    /* set port no to the default port */
    /*                                 */
    port_no = DEFAULT_PORT;

    /* set the connection port */
    /*                         */
    CONNECTION_PORT = port_no;
  }
}
/*                 */
/* end of function */


/******************************************************************************
#   function: client_exit
#
#   arguments: char *entry - user input
#
#   returns: int (1 or 0) - boolean logical value
#
#   this function returns true if it the user entered "exit"
******************************************************************************/
int client_exit(char *entry)
{

  /* copy to a buffer */
  /*                  */
  char lower_case[BUF_LEN + 1];
  strcpy(lower_case, entry);
  lower_case[strlen(entry)] = NUL_TERM;
  
  /* lowercase the word */
  /*                    */
  for(int ch = 0; ch < strlen(entry); ch++){
    lower_case[ch] = tolower(lower_case[ch]);
  }

  /* if it matches the word */
  /*                        */
  if(strcmp(&lower_case[0], "exit.") == 0){
    return True;
  }

  /* exit gracefully */
  /*                 */
  return False;
}
/*                 */
/* end of function */


/******************************************************************************
#   function: spell_check
#
#   arguments: char *word - word to be checked
#
#   returns: int (1 or 0) - boolean logical value
#
#   this function returns true if it finds a match of the given word
#   to the word in the dictionary
******************************************************************************/
int spell_check(char *word)
{

  /* copy to a buffer */
  /*                  */
  char lower_case[BUF_LEN + 1];
  strcpy(lower_case, word);
  lower_case[strlen(word)] = NUL_TERM;
  
  /* lowercase the word */
  /*                    */
  for(int ch = 0; ch < strlen(word); ch++){
    lower_case[ch] = tolower(lower_case[ch]);
  }

  /* index for the dictionary */
  /*                          */
  int d_ind = -1;

  /* compare to all the words */
  /*                          */
  while(DICT[++d_ind] != NULL){

    /* if we have a match */
    /*                    */
    if(strcmp(lower_case, DICT[d_ind]) == 0){
      return True;
    }
  }

  /* we did not find a match */
  /*                         */
  return False;
}
/*                 */
/* end of function */


/******************************************************************************
#   function: worker_thread
#
#   arguments: void *arg - no arguments are passed
#
#   returns: none
#
#   this is the function the worker threads will use to service clients
******************************************************************************/
void *worker_thread(void *arg)
{  
  
  /* Socket to be used */
  /*                   */
  Socket *to_service;
  Socket *log;
  int bytes_returned;
  int clientSocket;

  /* gets the user input without newline */
  /*                                     */
  char *usr_input = (char *) malloc(BUF_LEN * sizeof(char) + 1);
  
  /* set an allocated buffer */
  /*                         */
  char *recvBuffer;
  recvBuffer = (char *) malloc((BUF_LEN + 1) * sizeof(char));

  /* this thread will work forever */
  /*                               */
  while(True){

    /* obtain the lock */
    /*                 */
    pthread_mutex_lock(&SOCKETS_LOCK);

    /* while the queue is empty, wait for condition */
    /*                                              */
    while(fifo_empty(SOCKETS)){
      pthread_cond_wait(&Q_EMPTY, &SOCKETS_LOCK);
    }

    /* remove a Socket from the queue */
    /*                                */
    to_service = dequeue(SOCKETS);

    /* unlock */
    /*        */
    pthread_mutex_unlock(&SOCKETS_LOCK);

    /* signal main thread */
    /*                    */
    pthread_cond_signal(&Q_FULL);
    
    /* set the client Socket */
    /*                       */
    clientSocket = to_service->socket_fd;

    /* add the entry to the log */
    /*                          */
    log = create_log(clientSocket, NULL, ENTRY);
    add_entry(log);

    /* we can free this item */
    /*                       */
    free(to_service);
    
    /* send the client welcome and instruction prompt */
    /*                                                */
    send(clientSocket, WELCOME, strlen(WELCOME), 0);
    send(clientSocket, INSTRUCTION, strlen(INSTRUCTION), 0);
    
    /* service the client until connection closed */
    /*                                            */
    while(True){
      
      /* send the client a msg prompt */
      /*                              */
      send(clientSocket, MSGPROMPT, strlen(MSGPROMPT), 0);
      
      /* get the user input */
      /*                    */
      bytes_returned = recv(clientSocket, &recvBuffer[0], BUF_LEN, 0);
      
      /* if we failed to receive a message, send error message */
      /*                                                       */
      if(bytes_returned == -1){
	log = create_log(clientSocket, NULL, FAILED); 
	send(clientSocket, ERROR, strlen(ERROR), 0);
	add_entry(log);
	continue;
      }

      /* if all went well, null terminate the buffer */
      /*                                             */
      recvBuffer[bytes_returned - 2] = NUL_TERM;
      strcpy(usr_input, &recvBuffer[0]);

      /* if the escape key was returned, close connection and break */
      /*                                                            */
      if((recvBuffer[0] == 27) || (client_exit(recvBuffer) == True)){
	log = create_log(clientSocket, NULL, EXITED);
	send(clientSocket, GOODBYE, strlen(GOODBYE), 0);
	add_entry(log);
	close(clientSocket);
	break;
      }

      /* if the word was spelled CORRECTly */
      /*                                   */
      if(spell_check(recvBuffer)){

	/* add an entry, send client message */
	/*                                   */
	send(clientSocket, OK, strlen(OK), 0);
	log = create_log(clientSocket, &usr_input[0], CORRECT);
	add_entry(log);
      }

      /* if the word was misspelled */
      /*                            */
      else {

	/* add an entry, send client message */
	/*                                   */
	send(clientSocket, MISSPELLED, strlen(MISSPELLED), 0);
	log = create_log(clientSocket, &usr_input[0], INCORRECT);
	add_entry(log);
      }
    }
  }
}
/*                 */
/* end of function */


/******************************************************************************
#   function: set_dict
#
#   arguments: char *dict_file - name of the dictionary file
#
#   returns: none
#
#   this sets the dictionary to an array
******************************************************************************/
void set_dict(char *dict_file)
{

  /* dictionary file struct */
  /*                        */
  FILE *dictionary;

  /* if we failed to open the given file */
  /*                                     */
  if((dictionary = fopen(dict_file, "r")) == NULL){
    fprintf(stderr, "failed to open file %s\n", dict_file);
    exit(0);
  }

  /* buffer to hold each word */
  /*                          */
  char *buffer = NULL;

  /* will be allocated and point to the word */
  /*                                         */
  char *current_word;

  /* set the buffer size in excess */
  /*                               */
  size_t buff_size = 400;

  /* getline returns the number of characters it retrieved */
  /*                                                       */
  size_t num_chars;

  /* index of the dictionary */
  /*                         */
  int index = -1;

  /* while we haven't reached the end of the file */
  /*                                              */
  while(!feof(dictionary)){

    /* get the line, and return on failure */
    /*                                     */
    if((num_chars = getline(&buffer, &buff_size, dictionary)) == -1){
      return;
    }

    /* allocate the word -1 for the newline */
    /*                                      */
    current_word = (char *) malloc((num_chars - 1) * (sizeof(char)));

    /* copy everything but the newline at the end */
    /*                                            */
    strncpy(current_word, buffer, num_chars - 1);

    /* set all to lower case */
    /*                       */
    for(int ch = 0; ch < strlen(current_word); ch++){
      current_word[ch] = tolower(current_word[ch]);
    }
    
    /* point to the current word */
    /*                           */
    DICT[++index] = current_word;

    /* sets the current word to NULL */
    /*                               */
    current_word = NULL;

    /* sets the buffer to NULL */
    /*                         */
    buffer = NULL;
  }

  /* null terminate the dictionary */
  /*                               */
  DICT[++index] = NULL;
}
/*                 */
/* end of function */


/******************************************************************************
#   function: print_usage
#
#   arguments: none
#
#   returns: none
#
#   this function prints out the program usage
******************************************************************************/
void print_usage()
{
  
  /* print out to stderr */
  /*                     */
  fprintf(stderr, "%s\n", "usage: server -p [PORT] -d [DICT]");
}
/*                 */
/* end of function */


/*****************************************************************************/
/*---------------------------------end of file-------------------------------*/
