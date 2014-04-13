#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>
#include <stdlib.h>

#define E(call) do { if (call < 0) { perror(#call); exit(1); }; } while (false)

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static long long microseconds() {
    struct timeval tv;
    E(gettimeofday(&tv, NULL));
    return 1000000LL * tv.tv_sec + tv.tv_usec;
}

static bool current = true;

static void* thread_proc(void* param) {
    bool left = (bool) param;
    for (;;) {
        long long start = microseconds();
        long long count = 0;
        while (microseconds() - start < 1000000) {
            long long iterations = 1000;
            for (int i = 0; i < iterations; ++i) {
                E(pthread_mutex_lock(&mutex));
                while (current != left) {
                    E(pthread_cond_wait(&cond, &mutex));
                }
                current = !left;
                E(pthread_cond_signal(&cond));
                E(pthread_mutex_unlock(&mutex));
                //printf("%d\n", left);
            }
            count += iterations;
        }
        if (left) {
            long long dns = 1000L * (microseconds() - start);
            long long ns_per_call = dns / count;
            // result is about 5us per step on Linux and 10us on FreeBSD
            printf("%lld ns per step\n", ns_per_call);
        }
    }
    return NULL;
}

int main(int argc, char** argv) {
    printf("using 2 threads\n");
    pthread_t a;
    pthread_t b;
    E(pthread_create(&a, NULL, &thread_proc, (void*) true));
    E(pthread_create(&b, NULL, &thread_proc, (void*) false));
    E(pthread_join(a, NULL));
    E(pthread_join(b, NULL));
    return 0;
}
