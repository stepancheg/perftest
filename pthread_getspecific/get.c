#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>


#define E(call) do { if (__builtin_expect(call < 0, 0)) { perror(#call); exit(1); }; } while (false)

long long microseconds() {
    struct timeval tv;
    E(gettimeofday(&tv, NULL));
    return 1000000LL * tv.tv_sec + tv.tv_usec;
}

int main() {
    pthread_key_t k;
	E(pthread_key_create(&k, NULL));
	E(pthread_setspecific(k, &main));
	for (;;) {
	    long long start = microseconds();
	    long long count = 0;
	    while (microseconds() - start < 1000000) {
	        int iter = 1000000;
	        for (int i = 0; i < iter; ++i) {
	            void* m = pthread_getspecific(k);
	            (void) m;
	        }
	        count += iter;
	    }
	    long long dns = 1000L * (microseconds() - start);
	    long long ns_per_call = dns / count;
	    printf("%lld ns per op\n", ns_per_call);
    }
}

// vim: set ts=4 sw=4 et:
