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

int main(int argc, char** argv) {
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    for (;;) {
        long long start = microseconds();
        long long count = 0;
        while (microseconds() - start < 1000000) {
            for (int i = 0; i < 1000000; ++i) {
                E(pthread_mutex_lock(&mutex));
                E(pthread_mutex_unlock(&mutex));
            }
            count += 1000000;
        }
        long long dns = 1000L * (microseconds() - start);
        long long ns_per_call = dns / count;
        printf("%lld ns per lock/unlock\n", ns_per_call);
    }
    return 0;
}
