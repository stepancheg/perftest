#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>

long long microseconds() {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0)
        perror("gettimeofday");
    return 1000000LL * tv.tv_sec + tv.tv_usec;
}

int main(int argc, char** argv) {
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    for (;;) {
        long long start = microseconds();
        long long count = 0;
        while (microseconds() - start < 1000000) {
            for (int i = 0; i < 1000000; ++i) {
                if (pthread_mutex_lock(&mutex) < 0)
                    perror("pthread_mutex_lock");
                if (pthread_mutex_unlock(&mutex) < 0)
                    perror("pthread_mutex_lock");
            }
            count += 1000000;
        }
        long long dns = 1000L * (microseconds() - start);
        long long ns_per_call = dns / count;
        printf("%lld ns per lock/unlock\n", ns_per_call);
    }
    return 0;
}
