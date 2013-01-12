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

static int current = 0;

static void* thread_proc(void* param) {
    bool left = (bool) param;
    int leftn = left ? 1 : 0;
    for (;;) {
        long long start = microseconds();
        long long count = 0;
        while (microseconds() - start < 1000000) {
            long long iterations = 1000;
            for (int i = 0; i < iterations; ++i) {
                while (__atomic_load_n(&current, __ATOMIC_SEQ_CST) != leftn * 2) {
                    _mm_pause();
                }
                __atomic_store_n(&current, leftn * 2 + 1, __ATOMIC_SEQ_CST);
                //printf("%d\n", left);
                getpid();
                __atomic_store_n(&current, (1 - leftn) * 2, __ATOMIC_SEQ_CST);
            }
            count += iterations;
        }
        if (left) {
            long long dns = 1000ll * (microseconds() - start);
            long long ns_per_call = dns / count;
            printf("%lld ns per step\n", ns_per_call);
        }
    }
}

int main() {
    pthread_t a;
    pthread_t b;
    E(pthread_create(&a, NULL, &thread_proc, (void*) true));
    E(pthread_create(&b, NULL, &thread_proc, (void*) false));
    E(pthread_join(a, NULL));
    E(pthread_join(b, NULL));
    return 0;
}

// vim: set ts=4 sw=4 et:
