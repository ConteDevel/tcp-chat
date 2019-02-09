#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /** memcpy **/

#define SENDER_MAXSIZE 128
#define MSG_MAXSIZE 512
#define MSGBUF_MAXSIZE 16

typedef struct {
    char sender[SENDER_MAXSIZE];
    char data[MSG_MAXSIZE];
} msg_t;

typedef struct {
    size_t size;
    msg_t *data;
    int cur;
} msg_queue_t;

int prepare_msg(char *sender, char *data, msg_t *msg);
int print_msg(msg_t *msg);

int create_msg_queue(size_t queue_size, msg_queue_t *queue);
void delete_msg_queue(msg_queue_t *queue);
int enqueue(msg_queue_t *queue, msg_t *msg);
int dequeue(msg_queue_t *queue, msg_t *msg);
int dequeue_all(msg_queue_t *queue);

#endif //MESSAGE_H
