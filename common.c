#include "common.h"

int create_peer(peer_t *peer)
{
    if (create_msg_queue(MSGBUF_MAXSIZE, &peer->send_buf)) {
        perror("create_msg_queue");
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
