/******************************************************************************
# file: /home/TU/tug35668/lab/cis_3207/lab_3/queue.c
#
# Tarek Elseify
# March 26, 2019
# Lab 3 - Networked Spell Checker
# CIS 3207 SEC 001
# tug35668@temple.edu
#
# This program repurposes the queue used in lab 1 for sockets
# instead of events
******************************************************************************/

/* standard include statements */
/*                             */
#include "simple_server.h"


/******************************************************************************
#   function: create_socket
#
#   arguments: int client_fd - the client Socket file descriptor
#
#   returns: Socket *ret - pointer to initialized socket
#
#   this function returns a malloced initialized Socket struct
******************************************************************************/
Socket *create_socket(int client_fd)
{

  /* allocate a new Socket */
  /*                       */
  Socket *ret = (Socket *) malloc(sizeof(Socket));
  ret->next = NULL;
  
  /* set the Socket fd */
  /*                   */
  ret->socket_fd = client_fd;
  ret->user_input = NULL;
  ret->log_input = NULL;
  
  /* return the Socket */
  /*                   */
  return ret;
}
/*                 */
/* end of function */


/******************************************************************************
#   function: create_log
#
#   arguments: int client_fd - fd of the client socket
#              char *user_entry - what the user entered
#              char *log_entry - entry for the log queue
#
#   returns: Socket *ret - pointer to initialized Socket
#
#   this function uses the Socket struct as a log entry
******************************************************************************/
Socket *create_log(int client_fd, char *user_entry, char *log_entry)
{

  /* allocate a new Socket */
  /*                       */
  Socket *ret = (Socket *) malloc(sizeof(Socket));
  ret->next = NULL;
  
  /* check to see if user input is null */
  /*                                    */
  if(user_entry == NULL){
    ret->user_input = NULL;
  }

  /* allocate the user entry */
  /*                         */
  else{
    ret->user_input = (char *) malloc(strlen(user_entry) * sizeof(char) + 1);
    strcpy(ret->user_input, user_entry);
  }
  
  /* set the Socket fd and log */
  /*                           */
  ret->socket_fd = client_fd;
  ret->log_input = (char *) malloc(strlen(log_entry) * sizeof(char) + 1);
  strcpy(ret->log_input, log_entry);

  /* return the Socket */
  /*                   */
  return ret;
}
/*                 */
/* end of function */


/******************************************************************************
#   function: create_queue
#
#   arguments: none
#
#   returns: queue *new_queue - pointer to initialized queue
#
#   this function returns a malloced initialized queue
******************************************************************************/
queue *create_queue()
{

  /* create a malloc queue */
  /*                       */
  queue *new_queue = (queue *) malloc(sizeof(queue));

  /* initialize values with 0 and null */
  /*                                   */
  new_queue->head = NULL;
  new_queue->tail = NULL;
  new_queue->size = 0;

  /* return the queue pointer */
  /*                          */
  return new_queue;
}
/*                 */
/* end of function */


/******************************************************************************
#   function: enqueue
#
#   arguments: queue *q - pointer to a queue
#              Socket *new_Socket - pointer to the Socket to be enqueued
#
#   returns: none
#
#   this function enqueues a new Socket to a given queue
******************************************************************************/
void enqueue(queue *q, Socket *new_socket)
{

  /* check to see if the queue is full */
  /*                                   */
  if(q->size == MAX_QUEUE_SIZE){

    /* we can not add any more sockets */
    /*                                */
    fprintf(stdout, "%s\n", " .... WARNING: queue is full, unable to enqueue ....");
    return;
  }

  /* if the queue is empty */
  /*                       */
  if(fifo_empty(q)){

    /* set both the head and tail as the new Socket */
    /*                                             */
    q->head = new_socket;
    q->tail = new_socket;
    q->size++;
  }

  /* if the queue is not empty */
  /*                           */
  else {

    /* let the tail's socket's next point to this new Socket */
    /*                                                     */
    q->tail->next = new_socket;

    /* set the tail to the next Socket */
    /*                                */
    q->tail = new_socket;
    q->size++;
  }
}
/*                 */
/* end of function */


/******************************************************************************
#   function: dequeue
#
#   arguments: queue *q - pointer to a queue
#
#   returns: Socket *ret - the socket's address to be popped
#
#   this function returns the head of the queue
******************************************************************************/
Socket *dequeue(queue *q)
{

  /* if we try to dequeue an empty queue */
  /*                                     */
  if(fifo_empty(q)){

    /* return null */
    /*             */
    fprintf(stdout, "%s\n", " .... WARNING: queue is empty, unable to dequeue ....");
    return NULL;
  }

  /* if the queue is not empty */
  /*                           */
  else {

    /* save the Socket in the head */
    /*                            */
    Socket *ret = q->head;

    /* if the head has a next */
    /*                        */
    if(q->head->next != NULL){

      /* set the head as the next element */
      /*                                  */
      q->head = q->head->next;
    }

    /* return the Socket */
    /*                  */
    q->size--;
    return ret;
  }
}
/*                 */
/* end of function */


/******************************************************************************
#   function: print_queue
#
#   arguments: queue *q - pointer to a queue
#
#   returns: none
#
#   this function prints all the sockets in a queue
******************************************************************************/
void print_queue(queue *q)
{

  /* gets the starting position of the queue */
  /*                                         */
  Socket *current = q->head;

  /* while the Socket is not null */
  /*                             */
  while(current != NULL){

    /* print the information of the Socket */
    /*                                    */
    printf("Socket id: %d\n", current->socket_fd);

    /* go to the next Socket */
    /*                      */
    current = current->next;
  }
}
/*                 */
/* end of function */


/******************************************************************************
#   function: destroy
#
#   arguments: queue *q - pointer to a queue
#
#   returns: none
#
#   this function destroys all sockets in a queue
******************************************************************************/
void destroy(queue *q)
{

  /* gets the starting position of the queue */
  /*                                         */
  Socket *current = q->head;

  /* ensure that head is NULL */
  /*                          */
  q->head = NULL;
  
  /* while the Socket is not null */
  /*                             */
  while(current != NULL){

    /* free the Socket */
    /*                */
    free(current);
    
    /* go to the next Socket */
    /*                      */
    current = current->next;
  }

  /* ensure the tail is NULL */
  /*                         */
  q->tail = NULL;
}
/*                 */
/* end of function */


/******************************************************************************
#   function: fifo_empty
#
#   arguments: queue *q - pointer to a queue
#
#   returns: int (1 or 0)
#
#   this function returns a logical int value representing if 
#   a queue is empty or not
******************************************************************************/
int fifo_empty(queue *q)
{

  /* if the size of the queue is 0 */
  /*                               */
  if(q->size == 0){

    /* the queue is empty */
    /*                    */
    return 1;
  }

  /* else, it is not empty */
  /*                       */
  return 0;
}
/*                 */
/* end of function */


/******************************************************************************
#   function: fifo_full
#
#   arguments: queue *q - pointer to a queue
#
#   returns: int (1 or 0)
#
#   this function returns a logical int value representing if 
#   a queue is full or not
******************************************************************************/
int fifo_full(queue *q)
{

  /* if the size of the queue is 0 */
  /*                               */
  if(q->size == MAX_QUEUE_SIZE){

    /* the queue is empty */
    /*                    */
    return 1;
  }

  /* else, it is not empty */
  /*                       */
  return 0;
}
/*                 */
/* end of function */

/*****************************************************************************/
/*---------------------------------end of file-------------------------------*/
