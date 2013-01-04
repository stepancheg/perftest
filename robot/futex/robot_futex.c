#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>
#include <linux/futex.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

#define E(call) do { if (call < 0) { perror(#call); exit(1); }; } while (false)

extern int futex(int *uaddr, int op, int val, const struct timespec *timeout,
                 int *uaddr2, int val3);


static long long microseconds() {
    struct timeval tv;
    E(gettimeofday(&tv, NULL));
    return 1000000LL * tv.tv_sec + tv.tv_usec;
}

int current = true;

static void* thread_proc(void* param) {
    bool left = (bool) param;
    for (;;) {
        long long start = microseconds();
        long long count = 0;
        while (microseconds() - start < 1000000) {
            long long iterations = 1000;
            for (int i = 0; i < iterations; ++i) {
                if (syscall(SYS_futex, &current, FUTEX_WAIT, !left, NULL, NULL, 0) < 0) {
                    if (errno != EWOULDBLOCK) {
                        perror("SYS_futex");
                        abort();
                    }
                }
                current = !left;
                E(syscall(SYS_futex, &current, FUTEX_WAKE, 1, NULL, NULL, 0) < 0);
                //printf("%d\n", left);
            }
            count += iterations;
        }
        if (left) {
            long long dns = 1000LL * (microseconds() - start);
            long long ns_per_call = dns / count;
            printf("%lld ns per step\n", ns_per_call);
        }
    }
    return NULL;
}

int main() {
    pthread_t a;
    pthread_t b;
    E(pthread_create(&a, NULL, &thread_proc, (void*) true));
    E(pthread_create(&b, NULL, &thread_proc, (void*) false));
    E(pthread_join(a, NULL));
    E(pthread_join(b, NULL));
}

// vim: set ts=4 sw=4 et:
