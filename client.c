#include "common.h"

#define CLIENT_NAME_MAXSIZE 32

static peer_t server;

__attribute__((noreturn)) void shutdown_properly(int code)
{
    printf("Shuttign down the client...\n");
    if (delete_peer(&server)) {
        printf("delete_peer()\n");
        code = EXIT_FAILURE;
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

void get_client_name(int argc, char **argv, char *client_name)
{
    if (argc > 1) {
        strcpy(client_name, argv[1]);
    } else {
        strcpy(client_name, "anonymous");
    }
}

int connect_server(peer_t *server)
{
    if (create_peer(server)) {
        printf("create_peer()\n");
        return -1;
    }
    // create socket
    server->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server->sock < 0) {
        perror("socket()");
        return -1;
    }

    // set up addres
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    addr.sin_port = htons(SERVER_PORT);

    server->addr = addr;


    if (connect(server->sock, (struct sockaddr *)&addr,
                sizeof(struct sockaddr)) != 0) {
        perror("connect()");
        return -1;
    }

    printf("Connected to %s:%d.\n", SERVER_ADDR, SERVER_PORT);

    return 0;
}

void build_fd_sets(peer_t *server, fd_sets_t *fds)
{
    reset_fds(fds);

    FD_SET(STDIN_FILENO, &fds->r);
    FD_SET(server->sock, &fds->r);

    if (server->send_buf.cur > 0) {
        FD_SET(server->sock, &fds->w);
    }

    FD_SET(STDIN_FILENO, &fds->e);
    FD_SET(server->sock, &fds->e);
}

int read_input(peer_t *server, char *client_name)
{
    char read_buffer[MSG_MAXSIZE]; // buffer for stdin
    if (read_from_stdin(read_buffer, MSG_MAXSIZE) != 0) {
        printf("read_from_stdin()\n");
        return -1;
    }
    // Create new message and enqueue it.
    msg_t new_msg;
    prepare_msg(client_name, read_buffer, &new_msg);

    if (peer_add_to_send(server, &new_msg) != 0) {
        printf("Send buffer is overflowed, try later!\n");
        return 0;
    }

    return 0;
}

void handle_msg(__attribute__((unused)) peer_t *sender, msg_t *msg) {
    print_msg(msg);
}

int main(int argc, char **argv)
{
    if (setup_signals(handle_signal_action)) {
        printf("setup_signals()\n");
        exit(EXIT_FAILURE);
    }

    char name[CLIENT_NAME_MAXSIZE];
    get_client_name(argc, argv, name);

    if (connect_server(&server)) {
        printf("connect_server()\n");
        shutdown_properly(EXIT_FAILURE);
    }

    /* Set nonblock for stdin. */
    int flag = fcntl(STDIN_FILENO, F_GETFL, 0);
    flag |= O_NONBLOCK;
    fcntl(STDIN_FILENO, F_SETFL, flag);

    fd_sets_t fds;

    printf("Type message:\n");

    int maxfd = server.sock;
    for (;;) {
        build_fd_sets(&server, &fds);

        int activity = select(maxfd + 1, &fds.r, &fds.w, &fds.e, NULL);
        switch (activity) {
        case -1:
        case 0:
            perror("select()");
            shutdown_properly(EXIT_FAILURE);
        default:
            if (FD_ISSET(STDIN_FILENO, &fds.r) && read_input(&server, name)) {
                printf("read_input()");
                shutdown_properly(EXIT_FAILURE);
            }
            if (FD_ISSET(STDIN_FILENO, &fds.e)) {
                printf("Exception for STDIN.\n");
                shutdown_properly(EXIT_FAILURE);
            }
            if (FD_ISSET(server.sock, &fds.r) &&
                    receive_from_peer(&server, &handle_msg)) {
                printf("receive_from_peer()\n");
                shutdown_properly(EXIT_FAILURE);
            }
            if (FD_ISSET(server.sock, &fds.w) && send_to_peer(&server)) {
                printf("send_to_peer()\n");
                shutdown_properly(EXIT_FAILURE);
            }
            if (FD_ISSET(server.sock, &fds.e)) {
                printf("Exception for server socket.\n");
                shutdown_properly(EXIT_FAILURE);
            }
        }

        printf("Type message:\n");
    }
}
