#ifndef _SIMPLE_SERVER_H
#define _SIMPLE_SERVER_H
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define NUL_TERM '\0'
#define BUF_LEN 512
#define MAX_QUEUE_SIZE 10
#define True 1
#define False 0
#define ADDED "has been added to the queue"
#define ENTRY "has joined the network"
#define EXITED "has exited the network"
#define CORRECT "correctly"
#define INCORRECT "incorrectly"
#define WELCOME "Welcome to the Networked Spell Checker!\n"
#define INSTRUCTION "Keep entering words for me to check.\nUse the escape key, or enter \"exit.\", to close the connection.\n"
#define ERROR "Sorry, I didn't quite get that! Please try again\n"
#define GOODBYE "Goodbye!\n"
#define FAILED "failed to send message"
#define MSGPROMPT ">>> "
#define OK "OK\n"
#define MISSPELLED "MISSPELLED\n"


typedef struct SocketStruct {
  int socket_fd;
  char *user_input;
  char *log_input;
  struct SocketStruct *next;
}Socket;

typedef struct Queue {
  Socket *head;
  Socket *tail;
  int size;
}queue;

typedef struct ThreadArgs {
  queue *sockets_p;
  queue *logs_p;
}th_args;


/* function prototypes for the queue struct */
/*                                          */
Socket *create_socket(int client_fd);
Socket *create_log(int client_fd, char *user_input, char *log_entry);
queue *create_queue();
void enqueue(queue *q, Socket *new_socket);
Socket *dequeue(queue *q);
void print_queue(queue *q);
void destroy(queue *q);
int fifo_empty(queue *q);
int fifo_full(queue *q);

int open_listenfd(int);
#endif
