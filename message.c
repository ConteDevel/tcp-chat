#include "message.h"

int prepare_msg(char *sender, char *data, msg_t *msg)
{
    sprintf(msg->sender, "%s", sender);
    sprintf(msg->data, "%s", data);
    return 0;
}

int print_msg(msg_t *msg)
{
    printf("Message: \"%s: %s\"\n", msg->sender, msg->data);
    return 0;
}

int create_msg_queue(size_t queue_size, msg_queue_t *queue)
{
    queue->data = (msg_t *)calloc(queue_size, sizeof(msg_t));

    if (!queue->data) {
        perror("calloc");
        return -1;
    }

    queue->size = queue_size;
    queue->cur = 0;

    return 0;
}

void delete_msg_queue(msg_queue_t *queue)
{
    free(queue->data);
    queue->data = NULL;
}

int enqueue(msg_queue_t *queue, msg_t *msg)
{
    if (queue->cur == (int)queue->size) {
        return -1;
    }
    memcpy(&queue->data[queue->cur], msg, sizeof(msg_t));
    ++queue->cur;

    return 0;
}

int dequeue(msg_queue_t *queue, msg_t *msg)
{
    if (queue->cur == 0) {
        return -1;
    }

    memcpy(msg, &queue->data[queue->cur - 1], sizeof(msg_t));
    --queue->cur;

    return 0;
}

int dequeue_all(msg_queue_t *queue)
{
    queue->cur = 0;

    return 0;
}
