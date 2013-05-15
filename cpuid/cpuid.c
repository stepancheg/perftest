#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>


#define E(call) do { if (__builtin_expect(call < 0, 0)) { perror(#call); exit(1); }; } while (false)


long long microseconds() {
    struct timeval tv;
    E(gettimeofday(&tv, NULL));
    return 1000000LL * tv.tv_sec + tv.tv_usec;
}


int main() {
    for (;;) {
        long long start = microseconds();
        long long count = 0;
        while (microseconds() - start < 1000000) {
            int iter = 1000000;
            for (int i = 0; i < iter; ++i) {
                int function_number = 1;
                int a, b, c, d;
                __asm__ __volatile__("cpuid"
                        : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
                        : "a"(function_number)
                        );
            }
            count += iter;
        }
        long long dns = 1000L * (microseconds() - start);
        long long ns_per_call = dns / count;
        printf("%lld ns per cpuid\n", ns_per_call);
    }
    return 0;
}
