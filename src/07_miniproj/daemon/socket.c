#include <sys/socket.h>
#include <sys/un.h>
#include "socket.h"
#include <syslog.h>
#include "fan.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static int server_sock_fd = -1;

// returns a socket file descriptor on success
int socket_init(void)
{
    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
    };
    strncpy(addr.sun_path, FAN_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    server_sock_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (server_sock_fd < 0) {
        syslog(LOG_ERR, "socket() failed: %m");
        return -1;
    }

    unlink(FAN_SOCKET_PATH); // remove stale socket if daemon crashed
    if (bind(server_sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        syslog(LOG_ERR, "bind() failed: %m");
        return -1;
    }

    if (listen(server_sock_fd, 8) < 0) {
        syslog(LOG_ERR, "listen() failed: %m");
        return -1;
    }

    syslog(LOG_INFO, "IPC socket ready at %s", FAN_SOCKET_PATH);
    return server_sock_fd;
}

// returns client socket fd on success
int socket_accept_client(void)
{
    int client_fd = accept(server_sock_fd, NULL, NULL);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            syslog(LOG_ERR, "accept() failed: %m");
        }
        return -1;
    }
    return client_fd;
}

// handle client request and send response
void socket_handle_client(int client_fd)
{
    fan_request_t  req  = {0};
    fan_response_t resp = {0};

    ssize_t n = recv(client_fd, &req, sizeof(req), 0);
    if (n != sizeof(req)) {
        resp.status = -EINVAL;
        goto send_resp;
    }

    switch (req.cmd) {
        case CMD_GET_MODE:
            resp.value  = fan_is_manual_mode() ? 1 : 0;
            resp.status = 0;
            break;

        case CMD_SET_MODE:
            if (fan_is_manual_mode() != (bool)req.value)
                fan_toggle_mode();
            resp.status = 0;
            break;

        case CMD_GET_FREQUENCY:
            resp.value  = fan_read_frequency();
            resp.status = (resp.value < 0) ? resp.value : 0;
            break;

        case CMD_SET_FREQUENCY:
            if (!fan_is_manual_mode()) {
                resp.status = -EPERM;
            } else if (req.value < FAN_FREQUENCY_MIN ||
                       req.value > FAN_FREQUENCY_MAX) {
                resp.status = -ERANGE;
            } else {
                fan_set_frequency(req.value);
                resp.status = 0;
            }
            break;

        case CMD_GET_TEMP: {
            int temp_mc = fan_get_cpu_temp();
            resp.value  = temp_mc / 1000; // degrees C
            resp.status = 0;
            break;
        }

        default:
            resp.status = -ENOSYS;
            break;
    }

send_resp:
    send(client_fd, &resp, sizeof(resp), 0);
    close(client_fd);
}