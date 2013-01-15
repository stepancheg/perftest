#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <xmmintrin.h>

#define E(call) do { if (call < 0) { perror(#call); exit(1); }; } while (false)

static long long microseconds() {
    struct timeval tv;
    E(gettimeofday(&tv, NULL));
    return 1000000LL * tv.tv_sec + tv.tv_usec;
}

static const int thread_count = 8;

static int current = 0;

static void* thread_proc(void* param) {
    int robot_n = (int) param;
    for (;;) {
        long long start = microseconds();
        long long count = 0;
        while (microseconds() - start < 1000000) {
            long long iterations = 1000;
            for (int i = 0; i < iterations; ++i) {
                while (__atomic_load_n(&current, __ATOMIC_SEQ_CST) != robot_n * 2) {
#ifdef USE_PAUSE
                    _mm_pause();
#endif
                }
                __atomic_store_n(&current, robot_n * 2 + 1, __ATOMIC_SEQ_CST);
                //printf("%d\n", robot_n);
                getpid();
                __atomic_store_n(&current, (robot_n + 1) % thread_count * 2, __ATOMIC_SEQ_CST);
            }
            count += iterations;
        }
        if (robot_n == 0) {
            long long dns = 1000ll * (microseconds() - start);
            long long ns_per_call = dns / count;
            printf("%lld ns per step\n", ns_per_call);
        }
    }
}

int main() {
    printf("using %d threads\n", thread_count);
#ifdef USE_PAUSE
    printf("using pause\n");
#else
    printf("not using pause\n");
#endif
    pthread_t robots[thread_count];
    for (int i = 0; i < thread_count; ++i) {
        E(pthread_create(&robots[i], NULL, &thread_proc, (void*) i));
    }
    for (int i = 0; i < thread_count; ++i) {
        E(pthread_join(robots[i], NULL));
    }
    return 0;
}

// vim: set ts=4 sw=4 et:
