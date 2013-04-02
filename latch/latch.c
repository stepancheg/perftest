#define _GNU_SOURCE

#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <errno.h>


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

void nanosleep_nanos(long long ns) {
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = ns;
    struct timespec rem;
    E(nanosleep(&req, &rem));
}

#if !defined(IMPL) || !(IMPL == 1 || IMPL == 2 || IMPL == 3)
#error define IMPL to either 1 for mutex, 2 for futex, 3 for muted advanced
#elif IMPL == 1
#define IMPL_MUTEX
#define IMPL_NAME "mutex"
#elif IMPL == 2
#define IMPL_FUTEX
#define IMPL_NAME "futex"
#elif IMPL == 3
#define IMPL_FUTEX_ADVANCED
#define IMPL_NAME "futex advanced"
#else
#error impossible
#endif

#ifndef WAITERS
#define WAITERS 3
#endif





struct latch {
    // 0 unlocked
    // 1 locked
    // 2 locked and got waiters in futex impl
    int locked;
#ifdef IMPL_MUTEX
    pthread_mutex_t mutex;
    pthread_cond_t condvar;
#endif
};

void latch_init(struct latch* latch) {
    memset(latch, 0, sizeof(struct latch));
#ifdef IMPL_MUTEX
    E(pthread_mutex_init(&latch->mutex, NULL));
    E(pthread_cond_init(&latch->condvar, NULL));
#endif
}

void latch_wait(struct latch* latch) {
    // optimistic
    int l = __atomic_load_n(&latch->locked, __ATOMIC_SEQ_CST);
    if (l == 0) {
        return;
    }

#ifdef IMPL_MUTEX
    E(pthread_mutex_lock(&latch->mutex));
    while (latch->locked) {
        E(pthread_cond_wait(&latch->condvar, &latch->mutex));
    }
    E(pthread_mutex_unlock(&latch->mutex));
#elif defined(IMPL_FUTEX)
    int r = futex(&latch->locked, FUTEX_WAIT, 1, NULL, NULL, 0);
    if (r < 0 && errno != EWOULDBLOCK) {
        perror("futex");
        exit(2);
    }
#elif defined(IMPL_FUTEX_ADVANCED)
    if (l == 2) {
        int r = futex(&latch->locked, FUTEX_WAIT, 2, NULL, NULL, 0);
        if (r < 0 && errno != EWOULDBLOCK) {
            perror("futex");
            exit(2);
        }
    }
    int x = 1;
    __atomic_compare_exchange_n(&latch->locked, &x, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    if (x == 0) {
        // unlocked meanwhile
        return;
    } else {
        int r = futex(&latch->locked, FUTEX_WAIT, 2, NULL, NULL, 0);
        if (r < 0 && errno != EWOULDBLOCK) {
            perror("futex");
            exit(2);
        }
    }
#else
#error
#endif
}

void latch_lock(struct latch* latch) {
#ifndef IMPL_FUTEX_ADVANCED
    __atomic_store_n(&latch->locked, 1, __ATOMIC_SEQ_CST);
#else
    // optimistic
    int old = __atomic_load_n(&latch->locked, __ATOMIC_SEQ_CST);
    if (old != 0)
        return;

    int x = 0;
    __atomic_compare_exchange_n(&latch->locked, &x, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#endif
}

void latch_unlock(struct latch* latch) {
    // optimistic
    if (__atomic_load_n(&latch->locked, __ATOMIC_SEQ_CST) == 0) {
        return;
    }

#ifdef IMPL_MUTEX
    E(pthread_mutex_lock(&latch->mutex));
    latch->locked = 0;
    E(pthread_cond_broadcast(&latch->condvar));
    E(pthread_mutex_unlock(&latch->mutex));
#elif defined(IMPL_FUTEX)
    __atomic_store_n(&latch->locked, 0, __ATOMIC_SEQ_CST);
    E(futex(&latch->locked, FUTEX_WAKE, INT_MAX, NULL, NULL, 0));
#else
    if (__atomic_exchange_n(&latch->locked, 0, __ATOMIC_SEQ_CST) == 2) {
        // wake only if there are waiters
        E(futex(&latch->locked, FUTEX_WAKE, INT_MAX, NULL, NULL, 0));
    }
#endif
}

static void* waiter(void* param) {
    struct latch* latch = (struct latch*) param;
    for (;;) {
        latch_wait(latch);
    }
    return NULL;
}

int main() {
    printf("using " IMPL_NAME " implementation, %d waiters\n", WAITERS);

    struct latch latch;
    latch_init(&latch);

    pthread_t waiters[WAITERS];
    for (int i = 0; i < sizeof(waiters) / sizeof(pthread_t); ++i) {
        E(pthread_create(&waiters[i], NULL, waiter, &latch));
    }
    for (;;) {
        long long start = microseconds();
        long long count = 0;
        while (microseconds() - start < 1000000) {
            int iter = 100000;
            for (int i = 0; i < iter; ++i) {
                latch_lock(&latch);
                getppid(); // duration of getppid is 55 ns, git waiters a chance to wait
                latch_unlock(&latch);
            }
            count += iter;
        }
        long long dns = 1000L * (microseconds() - start);
        long long ns_per_call = dns / count;
        printf("%lld ns per latch lock/unlock pair\n", ns_per_call);
    }
}

// vim: set ts=4 sw=4 et:
