#include "common.h"

int create_peer(peer_t *peer)
{
    if (create_msg_queue(MSGBUF_MAXSIZE, &peer->send_buf)) {
        printf("create_msg_queue");
        return -1;
    }
    peer->cur_sending_byte = -1;
    peer->cur_receiving_byte = 0;

    return 0;
}

int delete_peer(peer_t *peer)
{
    if (peer->sock != NO_SOCKET && close(peer->sock)) {
        perror("close");
        return -1;
    }
    delete_msg_queue(&peer->send_buf);

    return 0;
}

char *get_peer_addrstr(peer_t *peer)
{
    static char ret[INET_ADDRSTRLEN + 10];
    char peer_ipv4_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer->addr.sin_addr, peer_ipv4_str, INET_ADDRSTRLEN);
    sprintf(ret, "%s:%d", peer_ipv4_str, peer->addr.sin_port);

    return ret;
}

int peer_add_to_send(peer_t *peer, msg_t *msg)
{
    return enqueue(&peer->send_buf, msg);
}

/* Receive message from peer and handle it with message_handler(). */
int receive_from_peer(peer_t *peer, void (*msg_handler)(peer_t *, msg_t *))
{
    ssize_t received_count;
    ssize_t received_total = 0;
    do {
        // Is completely received?
        if ((size_t)peer->cur_receiving_byte >= sizeof(peer->receiving_buf)) {
            msg_handler(peer, &peer->receiving_buf);
            peer->cur_receiving_byte = 0;
        }

        // Count bytes to send.
        size_t len;
        len = sizeof(peer->receiving_buf) - (size_t)peer->cur_receiving_byte;
        char *buf = (char *)&peer->receiving_buf + peer->cur_receiving_byte;
        received_count = recv(peer->sock, buf, len, MSG_DONTWAIT);
        if (received_count < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                perror("recv()");
                return -1;
            }
        } else if (received_count == 0) {
            return -1;
        } else if (received_count > 0) {
            peer->cur_receiving_byte += received_count;
            received_total += received_count;
        }
    } while (received_count > 0);

    return 0;
}

int send_to_peer(peer_t *peer)
{
    ssize_t send_count;
    ssize_t send_total = 0;
    do {
        // If sending message has sent and send the next message from queue
        if (peer->cur_sending_byte < 0 ||
                (size_t)peer->cur_sending_byte >= sizeof(peer->sending_buf)) {
            if (dequeue(&peer->send_buf, &peer->sending_buf) != 0) {
                peer->cur_sending_byte = -1;
                break;
            }
            peer->cur_sending_byte = 0;
        }
        // Count bytes to send.
        size_t len = sizeof(peer->sending_buf) - (size_t)peer->cur_sending_byte;
        msg_t *buf = &peer->sending_buf + peer->cur_sending_byte;
        send_count = send(peer->sock, buf, len, 0);
        if (send_count < 0) {
            // we have read as many as possible
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                perror("send");
                return -1;
            }
        } else if (send_count > 0) {
            peer->cur_sending_byte += send_count;
            send_total += send_count;
        }
    } while (send_count > 0);

    return 0;
}

void reset_fds(fd_sets_t *fds) {
    FD_ZERO(&fds->r);
    FD_ZERO(&fds->w);
    FD_ZERO(&fds->e);
}

/** Subscribes to events when thread was terminated or no reader of the pipe **/
int setup_signals(void (*handler)(int))
{
    struct sigaction sa;
    sa.sa_handler = handler;
    if (sigaction(SIGINT, &sa, 0) != 0) {
        perror("sigaction()");
        return -1;
    }
    if (sigaction(SIGPIPE, &sa, 0) != 0) {
        perror("sigaction()");
        return -1;
    }
    return 0;
}

int read_from_stdin(char *buffer, size_t max_len)
{
    memset(buffer, 0, max_len);

    ssize_t cur_count = 0;
    ssize_t total = 0;

    do {
        cur_count = read(STDIN_FILENO, buffer, max_len);
        if (cur_count < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                perror("read()");
                return -1;
            }
        } else if (cur_count > 0) {
            total += cur_count;
            if (total > (ssize_t)max_len) {
                printf("Message too large and will be chopped.\n");
                fflush(STDIN_FILENO);
                break;
            }
        }
    } while (cur_count > 0);

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }

    return 0;
}
