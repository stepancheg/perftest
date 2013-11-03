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

static inline long add(long* a, long b, bool use_atomic) {
    if (use_atomic) {
        __asm__ __volatile__(
            "lock\n\t"
            "xadd %0, %1"
            : "+r" (b), "+m" (*a)
            :
            : "memory");
    } else {
        __asm__ __volatile__(
            "xadd %0, %1"
            : "+r" (b), "+m" (*a)
            :
            : "memory");
    }
    return *a + b;
}

static void run_test(bool use_atomic) {
    long long start = microseconds();
    long long count = 0;
    while (microseconds() - start < 1000000) {
        long l = 0;
        long long batch = 10000000;
        for (int i = 0; i < batch; ++i) {
            add(&l, 1, use_atomic);
        }
        if (__builtin_expect(l != batch, 0)) {
            abort();
        }
        count += batch;
    }
    long long dps = 1000000L * (microseconds() - start);
    long long ps_per_call = dps / count;
    printf("using atomic: %s\n", use_atomic ? "true" : "false");
    printf("%lld ps per add\n", ps_per_call);
}

int main(int argc, char** argv) {
    for (;;) {
        run_test(false);
        run_test(true);
    }
    return 0;
}
