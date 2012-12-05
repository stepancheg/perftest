#include <sys/epoll.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>

#define E(call) do { if (call < 0) { perror(#call); exit(1); }; } while (false)

static int pipe_fds[2];

static void* thread_proc(void* param) {
    int pollfd = (int) param;
    printf("sleeping for 1s\n");
    sleep(1);
    E(pipe(pipe_fds));
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.u32 = 0;
    printf("about to call epoll_ctl and then write\n");
    E(epoll_ctl(pollfd, EPOLL_CTL_ADD, pipe_fds[0], &event));
    E(write(pipe_fds[1], "", 1));
    return NULL;
}

int main(int argc, char** argv) {
    pthread_t thread;
    int pollfd = epoll_create(1);
    if (pollfd < 0) {
        perror("epoll_create");
        exit(1);
    }
    E(pthread_create(&thread, NULL, &thread_proc, (void*) pollfd));

    struct epoll_event events[0];
    int count = epoll_wait(pollfd, events, 1, -1);
    if (count < 0) {
        perror("epoll_wait");
        exit(1);
    }

    printf("%d\n", count);

    return 0;
}

