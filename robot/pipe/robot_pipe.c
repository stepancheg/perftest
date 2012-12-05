#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#define E(call) do { if (call < 0) { perror(#call); exit(1); }; } while (false)

int ltr_pipes[2];
int rtl_pipes[2];

static long long microseconds() {
    struct timeval tv;
    E(gettimeofday(&tv, NULL));
    return 1000000LL * tv.tv_sec + tv.tv_usec;
}

static void* thread_proc(void* param) {
    bool left = (bool) param;
    int write_pipe = left ? ltr_pipes[1] : rtl_pipes[1];
    int read_pipe = left ? rtl_pipes[0] : ltr_pipes[0];
    if (left)
        E(write(write_pipe, "", 1));
    for (;;) {
        long long start = microseconds();
        long long count = 0;
        while (microseconds() - start < 1000000) {
            long long iterations = 1000;
            for (int i = 0; i < iterations; ++i) {
                char buf[1];
                E(read(read_pipe, buf, 1));
                E(write(write_pipe, "", 1));
            }
            count += iterations;
            //printf("%d\n", left);
        }
        if (left) {
            long long dns = 1000L * (microseconds() - start);
            long long ns_per_call = dns / count;
            printf("%lld ns per step\n", ns_per_call);
        }
    }
    return NULL;
}

int main(int argc, char** argv) {
    E(pipe(ltr_pipes));
    E(pipe(rtl_pipes));
    pthread_t a;
    pthread_t b;
    E(pthread_create(&a, NULL, &thread_proc, (void*) true));
    E(pthread_create(&b, NULL, &thread_proc, (void*) false));
    E(pthread_join(a, NULL));
    E(pthread_join(b, NULL));
    return 0;
}
