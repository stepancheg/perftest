#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>


static inline int futex(int *uaddr, int op, int val, const struct timespec *timeout,
    int *uaddr2, int val3)
{
    return syscall(SYS_futex, uaddr, op, val, timeout, uaddr2, val3);
}


#define E(call) do { if (__builtin_expect(call < 0, 0)) { perror(#call); exit(1); }; } while (false)

long long microseconds() {
    struct timeval tv;
    E(gettimeofday(&tv, NULL));
    return 1000000LL * tv.tv_sec + tv.tv_usec;
}

static int var = 1;

int main() {
	for (;;) {
	    long long start = microseconds();
	    long long count = 0;
	    while (microseconds() - start < 1000000) {
	        int iter = 1000000;
	        for (int i = 0; i < iter; ++i) {
	            E(futex(&var, FUTEX_WAKE, 1, NULL, NULL, 0));
	        }
	        count += iter;
	    }
	    long long dns = 1000L * (microseconds() - start);
	    long long ns_per_call = dns / count;
	    printf("%lld ns per FUTEX_WAKE\n", ns_per_call);
    }
}

// vim: set ts=4 sw=4 et:
