#define _GNU_SOURCE        

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <sched.h>
#include <errno.h>


// signal handler                                                
static void signal_handler(int signum)
{
    const char *name;
    switch (signum) {
        case SIGHUP:  name = "SIGHUP";  break;
        case SIGINT:  name = "SIGINT";  break;
        case SIGQUIT: name = "SIGQUIT"; break;
        case SIGABRT: name = "SIGABRT"; break;
        case SIGTERM: name = "SIGTERM"; break;
        default:      name = "UNKNOWN"; break;
    }
    char buf[128];
    int len = snprintf(buf, sizeof(buf),
        "[PID %d] Signal %s received and ignored.\n", getpid(), name);
    write(STDOUT_FILENO, buf, len);
}

// Register signals to catch                                          
static void setup_signals(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  

    int signals[] = { SIGHUP, SIGINT, SIGQUIT, SIGABRT, SIGTERM }; // signals to catch

    for (size_t i = 0; i < 5; i++) { 
        if (sigaction(signals[i], &sa, NULL) == -1) { // register signal handler
            perror("sigaction");
            exit(EXIT_FAILURE);
        }
    }
}

// Pin the current process to a given CPU core                          
static void pin_to_core(int core)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);

    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) == -1) {
        // continue even if affinity cannot be set
        fprintf(stderr, "[PID %d] Warning: sched_setaffinity(core %d): %s\n",
                getpid(), core, strerror(errno));
    } else {
        printf("[PID %d] Affinity pinned to core %d.\n", getpid(), core);
    }
}


// child process                                               
static void child_process(int sock_fd)
{
    pin_to_core(1);
    setup_signals();

    printf("[CHILD PID %d] Started on core 1.\n", getpid());
    fflush(stdout);

    // Messages to send to the parent
    const char *messages[] = {
        "Hello from the child!",
        "How are you, parent?",
        "Bybye",
        "exit", // Keyword to kill the app
        NULL
    };

    for (int i = 0; messages[i] != NULL; i++) {
        sleep(1);  // short pause

        ssize_t sent = write(sock_fd, messages[i], strlen(messages[i]) + 1);
        if (sent == -1) {
            perror("[CHILD] write");
            break;
        }
        printf("[CHILD PID %d] Message sent: \"%s\"\n", getpid(), messages[i]);
        fflush(stdout);

        if (strcmp(messages[i], "exit") == 0)
            break;
    }

    close(sock_fd);
    printf("[CHILD PID %d] Terminated.\n", getpid());
    fflush(stdout);
    exit(EXIT_SUCCESS);
}


// parent process                                              */
static void parent_process(int sock_fd, pid_t child_pid)
{
    pin_to_core(0);
    setup_signals();

    printf("[PARENT PID %d] Waiting for messages from child (PID %d)...\n\n",
           getpid(), child_pid);
    fflush(stdout);

    char buf[256];

    while (1) {
        ssize_t n = read(sock_fd, buf, sizeof(buf) - 1);
        if (n <= 0) {
            if (n == -1 && errno == EINTR)
                continue;   // interrupted by a signal, retry 
            break;
        }
        buf[n] = '\0'; // null-terminate the received message

        printf("[PARENT PID %d] Message received: \"%s\"\n", getpid(), buf);
        fflush(stdout);

        if (strcmp(buf, "exit") == 0) {
            printf("\n[PARENT PID %d] \"exit\" message received — shutting down.\n",
                   getpid());
            fflush(stdout);
            break;
        }
    }

    close(sock_fd); // close the socket after done reading

    // Wait for child to finish
    int status;
    waitpid(child_pid, &status, 0);
    printf("[PARENT PID %d] Child exited (status %d). Goodbye!\n",
           getpid(), WEXITSTATUS(status));
}


int main(void)
{
    int sv[2];  // socket pair file descriptors: sv[0] for parent, sv[1] for child

    printf("Starting program \n");

    // create the socket pair for IPC communication
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
        perror("socketpair");
        return EXIT_FAILURE;
    }

    // fork the process to create a child
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) { // if pid is 0, we are in the child process
        close(sv[0]); // child does not use the parent side of the socket
        child_process(sv[1]); // start the child process
    } else {
        close(sv[1]);     
        parent_process(sv[0], pid); // start the parent process
    }

    return EXIT_SUCCESS;
}