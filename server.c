#include "common.h"

#define MAX_CLIENTS 10

static int srv_sock = NO_SOCKET;
static peer_t clients[MAX_CLIENTS];

/** Frees captured resources and closes the program **/
__attribute__((noreturn)) void shutdown_properly(int code)
{
    printf("Shuttign down the server...\n");
    if (srv_sock != NO_SOCKET && close(srv_sock)) {
        perror("close");
        code = EXIT_FAILURE;
    }

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (delete_peer(&clients[i])) {
            perror("delete_peer");
            code = EXIT_FAILURE;
        }
    }

    printf("Done\n");
    exit(code);
}

void handle_signal_action(int sig_number)
{
    if (sig_number == SIGINT) {
        printf("SIGINT was catched!\n");
        shutdown_properly(EXIT_SUCCESS);
    } else if (sig_number == SIGPIPE) {
        printf("SIGPIPE was catched!\n");
        shutdown_properly(EXIT_SUCCESS);
    }
}

/** Makes a socket address reusable **/
int make_address_reusable(int sock) {
    /* Make address reusable */
    int reused = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reused, sizeof(reused))) {
        perror("setsockopt(SO_REUSEADDR)");
        return -1;
    }
#ifdef SO_REUSEPORT
    /* Make port reusable */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reused, sizeof(reused))) {
        perror("setsockopt(SO_REUSEPORT)");
        return -1;
    }
#endif
    return 0;
}

/** Creates a new TCP-socket to listen incomming connections **/
int make_socket(int *sock)
{
    /* Initialize TCP-socket */
    *sock = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock < 0) {
        perror("socket");
        return -1;
    }
    /* Make socket address reusable */
    if (make_address_reusable(*sock)) {
        perror("make_address_reusable");
        return -1;
    }
    /* Setup address */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    addr.sin_port = htons(SERVER_PORT);
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        perror("inet_addr");
        return -1;
    }
    /* Bind to this address */
    if (bind(*sock, (struct sockaddr *)&addr, sizeof(struct sockaddr)) != 0) {
        perror("bind");
        return -1;
    }
    return 0;
}

int build_fd_sets(fd_sets_t *fds, peer_t *clients) {
    reset_fds(fds);

    FD_SET(srv_sock, &fds->r);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].sock != NO_SOCKET) {
            FD_SET(clients[i].sock, &fds->r);
        }
    }

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].sock != NO_SOCKET && clients[i].send_buf.cur > 0) {
            FD_SET(clients[i].sock, &fds->w);
        }
    }

    FD_SET(srv_sock, &fds->e);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].sock != NO_SOCKET) {
            FD_SET(clients[i].sock, &fds->e);
        }
    }

    return 0;
}

int init_clients(peer_t *clients) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        clients[i].sock = NO_SOCKET;
        if (create_peer(&clients[i])) {
            perror("create_peer");
            return -1;
        }
    }

    return 0;
}

int find_high_socket(peer_t *clients) {
    int high_socket = srv_sock;
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].sock > high_socket) {
            high_socket = clients[i].sock;
        }
    }
    return high_socket;
}

int main()
{
    if (setup_signals(handle_signal_action)) {
        printf("setup_signals\n");
        exit(EXIT_FAILURE);
    }
    if (make_socket(&srv_sock)) {
        printf("make_socket\n");
        shutdown_properly(EXIT_FAILURE);
    }

    fd_sets_t fds;

    if (init_clients(clients)) {
        printf("init_clients\n");
        shutdown_properly(EXIT_FAILURE);
    }

    for (;;) {
        build_fd_sets(&fds, clients);
        int high_socket = find_high_socket(clients);

        int activity = select(high_socket + 1, &fds.r, &fds.w, &fds.e, NULL);

        switch (activity) {
        case -1:
        case 0:
            perror("select");
            shutdown_properly(EXIT_FAILURE);
        default:
            if (FD_ISSET(srv_sock, &fds.e)) {
                printf("Exception for server socket\n");
                shutdown_properly(EXIT_FAILURE);
            }
            printf("OK!\n");
        }
    }
}
