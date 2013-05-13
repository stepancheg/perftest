#define _GNU_SOURCE

#include <utmpx.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

#define E(call) do { if (__builtin_expect(call < 0, 0)) { perror(#call); exit(1); }; } while (false)

long long microseconds() {
    struct timeval tv;
    E(gettimeofday(&tv, NULL));
    return 1000000LL * tv.tv_sec + tv.tv_usec;
}

pthread_mutex_t exchange_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t exchange_cond = PTHREAD_COND_INITIALIZER;
int state = 0;

void transition(int check_state) {
    E(pthread_mutex_lock(&exchange_mutex));
    while (state != check_state) {
        E(pthread_cond_wait(&exchange_cond, &exchange_mutex));
    }
    state = (state + 1) % 4;
    E(pthread_cond_signal(&exchange_cond));
    E(pthread_mutex_unlock(&exchange_mutex));
}

void exchange(bool left) {
    if (left) {
        transition(0);
        transition(2);
    } else {
        transition(1);
        transition(3);
    }
}

int right_affinity = 0;

#define ARRAY_SIZE 10000
typedef char array_t[ARRAY_SIZE];
array_t data[2];

void* thread_proc(void* param) {
    bool left = *((bool*) param);

    pthread_t self = pthread_self();
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    if (left) {
        CPU_SET(0, &cpu_set);
    } else {
        CPU_SET(right_affinity, &cpu_set);
    }
    E(pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpu_set));

    for (;;) {
        long long start = microseconds();
        long long count = 0;
        int sum = 0;
        while (microseconds() - start < 1000000) {
            int iter = 1;
            count += iter;
            for (int j = 0; j < iter; ++j) {
                bool even = j % 2 == 0;
                array_t* array = &data[even ? 0 : 1];
                if (left) {
                    for (int i = 0; i < ARRAY_SIZE; ++i) {
                        (*array)[i] = i % 100 + (even ? 0 : 1);
                    }
                } else {
                    sum = 0;
                    for (int i = 0; i < ARRAY_SIZE; ++i) {
                        sum += (*array)[i];
                    }
                }
                exchange(left);
            }
        }
        if (!left) {
            long long dus = 1L * (microseconds() - start);
            long long us_per_call = dus / count;
            printf("%lld us per op\n", us_per_call);
        }
        //printf("%s cpu %d\n", left ? "left" : "right", sched_getcpu());
    }
    return NULL;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <right-thread-affinity>\n", argv[0]);
        exit(1);
    }
    right_affinity = atoi(argv[1]);

    bool true_data = true;
    bool false_data = false;
    pthread_t t1;
    pthread_t t2;
    E(pthread_create(&t1, NULL, &thread_proc, &true_data));
    E(pthread_create(&t2, NULL, &thread_proc, &false_data));
    E(pthread_join(t1, NULL));
    E(pthread_join(t2, NULL));
    return 0;
}
