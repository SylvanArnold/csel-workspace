#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "../common/fan_socket.h"

static int connect_to_daemon(void)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr = { .sun_family = AF_UNIX };
    strncpy(addr.sun_path, FAN_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    return fd;
}

static fan_response_t send_command(fan_cmd_t cmd, int32_t value)
{
    int fd = connect_to_daemon();

    fan_request_t req = { .cmd = cmd, .value = value };
    send(fd, &req, sizeof(req), 0);

    fan_response_t resp = {0};
    recv(fd, &resp, sizeof(resp), 0);
    close(fd);
    return resp;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr,
            "Usage:\n"
            "  fan-ctl get-mode\n"
            "  fan-ctl set-mode <0|1>\n"
            "  fan-ctl get-freq\n"
            "  fan-ctl set-freq <hz>\n"
            "  fan-ctl get-temp\n");
        return EXIT_FAILURE;
    }

    fan_response_t resp;

    if      (!strcmp(argv[1], "get-mode"))  resp = send_command(CMD_GET_MODE, 0);
    else if (!strcmp(argv[1], "set-mode"))  resp = send_command(CMD_SET_MODE,      atoi(argv[2]));
    else if (!strcmp(argv[1], "get-freq"))  resp = send_command(CMD_GET_FREQUENCY, 0);
    else if (!strcmp(argv[1], "set-freq"))  resp = send_command(CMD_SET_FREQUENCY, atoi(argv[2]));
    else if (!strcmp(argv[1], "get-temp"))  resp = send_command(CMD_GET_TEMP, 0);
    else { fprintf(stderr, "unknown command\n"); return EXIT_FAILURE; }

    if (resp.status < 0) {
        fprintf(stderr, "Error: %s\n", strerror(-resp.status));
        return EXIT_FAILURE;
    }

    printf("%d\n", resp.value);
    return EXIT_SUCCESS;
}