#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>

#define N      10000000
#define N_sqrt 3162

// #define ORIG

#ifdef ORIG
static bool is_prime[N];
static int primes[N];

int *sieve(int n) {
    assert(n + 1 < N);
    for (int i = 0; i <= n; i++)
        is_prime[i] = true;

    for (int i = 2; i <= n; i++) {
        for (int j = i + i; j <= n; j += i) {
            is_prime[j] = false;
        }
    }

    int *p = primes;
    for (int i = 2; i <= n; i++)
        if (is_prime[i]) {
            *p++ = i;
        }
    *p = 0;
    return primes;
}
#else

static bool is_prime[N >> 1];
static int primes[N >> 1];

int *sieve(int n) {
    assert(n + 1 < N);
    memset(is_prime, true, n >> 1);

    int *p = primes;
    *p++ = 2;
    for (uint64_t i = 3; i <= n; i += 2) {
        if (is_prime[i >> 1]) {
            *p++ = i;
            if (i <= N_sqrt) {
                int add = i << 1;
                for (uint64_t j = i * i; j <= n; j += add) {
                    is_prime[j >> 1] = false;
                }
            }
        }
    }
    *p = 0;

    return primes;
}

#endif