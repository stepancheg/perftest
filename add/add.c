#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdbool.h>

#define E(call) do { if (__builtin_expect(call < 0, 0)) { perror(#call); exit(1); }; } while (false)

long long microseconds() {
    struct timeval tv;
    E(gettimeofday(&tv, NULL));
    return 1000000LL * tv.tv_sec + tv.tv_usec;
}

#ifndef USE_ATOMIC
#error define USE_ATOMIC 1 or 0
#endif

static inline long add(long* a, long b) {
    __asm__ __volatile__(
#if USE_ATOMIC
        "lock\n\t"
#endif
        "xadd %0, %1"
        : "+r" (b), "+m" (a)
        :
        : "memory");
    return *a + b;
}

int main(int argc, char** argv) {
    printf("using atomic: %s\n", USE_ATOMIC ? "true" : "false");
    for (;;) {
        long long start = microseconds();
        long long count = 0;
        while (microseconds() - start < 1000000) {
            long l = 0;
            long long batch = 10000000;
            for (int i = 0; i < batch; ++i) {
                add(&l, 1);
            }
            count += batch;
        }
        long long dps = 1000000L * (microseconds() - start);
        long long ps_per_call = dps / count;
        printf("%lld ps per add\n", ps_per_call);
    }
    return 0;
}
