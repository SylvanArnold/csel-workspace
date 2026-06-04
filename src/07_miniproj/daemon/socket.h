#ifndef FAN_SOCKET_H
#define FAN_SOCKET_H

#include <stdint.h>

#define FAN_SOCKET_PATH "/var/run/fan-controller.sock"

typedef enum {
    CMD_GET_MODE,        
    CMD_SET_MODE,        
    CMD_GET_FREQUENCY,  
    CMD_SET_FREQUENCY,   
    CMD_GET_TEMP,      
} fan_cmd_t;

typedef struct {
    fan_cmd_t cmd;
    int32_t   value;     // used for SET commands
} fan_request_t;

typedef struct {
    int32_t status;      // 0 = ok, negative = error
    int32_t value;       // returned value for GET commands
} fan_response_t;

int socket_init(void);
int socket_accept_client(void);
void socket_handle_client(int client_fd);

#endif // FAN_SOCKET_H