#ifndef COMMON_H
#define COMMON_H

#include "message.h"    /** printf, memset, exit **/

#include <arpa/inet.h>  /** inet_addr **/
#include <errno.h>      /** errno **/
#include <fcntl.h>      /** fcntl **/
#include <netinet/in.h> /** sockaddr_in **/
#include <signal.h>     /** sigaction **/
#include <sys/socket.h> /** socket **/
#include <unistd.h>     /** close **/

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 15555

#define NO_SOCKET -1

typedef struct {
    int sock;
    struct sockaddr_in addr;
    msg_queue_t send_buf;
    msg_t sending_buf;
    int cur_sending_byte;
    msg_t receiving_buf;
    int cur_receiving_byte;
} peer_t;

typedef struct {
    fd_set w;
    fd_set r;
    fd_set e;
} fd_sets_t;

int create_peer(peer_t *peer);
int delete_peer(peer_t *peer);
char *get_peer_addrstr(peer_t *peer);
int receive_from_peer(peer_t *peer, int (*msg_handler)(peer_t *, msg_t *));
int send_to_peer(peer_t *peer);

void reset_fds(fd_sets_t *fds);

int setup_signals(void (*handler)(int));

#endif //COMMON_H
