#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static long long microseconds() {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0)
        perror("gettimeofday");
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
                if (pthread_mutex_lock(&mutex) < 0)
                    perror("pthread_mutex_lock");
                while (current != left) {
                    if (pthread_cond_wait(&cond, &mutex) < 0)
                        perror("pthread_cond_wait");
                }
                current = !left;
                if (pthread_cond_signal(&cond) < 0)
                    perror("pthread_cond_signal");
                if (pthread_mutex_unlock(&mutex) < 0)
                    perror("pthread_mutex_unlock");
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
    pthread_t a;
    pthread_t b;
    if (pthread_create(&a, NULL, &thread_proc, (void*) true) < 0)
        perror("pthread_create");
    if (pthread_create(&b, NULL, &thread_proc, (void*) false) < 0)
        perror("pthread_create");
    if (pthread_join(a, NULL) < 0)
        perror("pthread_join");
    if (pthread_join(b, NULL) < 0)
        perror("pthread_join");
    return 0;
}
