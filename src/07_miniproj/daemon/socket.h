/**
 * @file    fan_socket.h
 * @brief   Socket handler for fan controller daemon
 * @author  Sylvan Arnold
 */

#ifndef FAN_SOCKET_H
#define FAN_SOCKET_H

#include <stdint.h>
#include "../common/fan_socket.h"

#define FAN_SOCKET_PATH "/var/run/fan-controller.sock"

int socket_init(void);
int socket_accept_client(void);
void socket_handle_client(int client_fd);

#endif // FAN_SOCKET_H