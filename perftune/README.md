测试性能时可以只保留计算的部分，尽量减少系统调用（main函数中只做调用`sieve`）

```
perf record ./perftune-64
perf report

perf stat -r 10 ./perftune-64
perf stat -r 10 -e cycles,instructions,L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses ./perftune-64
```

## 未经修改

```
     411.12 msec task-clock        #    1.003 CPUs utilized            ( +-  0.96% )
          1      context-switches  #    2.442 /sec                     ( +- 24.49% )
          0      cpu-migrations    #    0.000 /sec                   
        488      page-faults       #    1.192 K/sec                    ( +- 20.88% )
984,368,398      cycles            #    2.404 GHz                      ( +-  0.70% )
731,436,146      instructions      #    0.75  insn per cycle           ( +-  0.05% )
194,132,565      branches          #  474.150 M/sec                    ( +-  0.04% )
  1,116,875      branch-misses     #    0.58% of all branches          ( +-  0.03% )
```

perf说大部分的耗时都在这里，确实这里有很多情况被重复计算了
```c
for (int i = 2; i <= n; i++) {
    for (int j = i + i; j <= n; j += i) {
        is_prime[j] = false;
    }
}
```

## 算法优化

最一般的算法优化，j从$i^2$开始，每次`j+=i`。又考虑到偶数不可能为素数，所以还能简化。
```c
#define N      10000000
#define N_sqrt 3162
static bool is_prime[N];
static int primes[N];
int *sieve(int n) {
    assert(n + 1 < N);
    for (int i = 0; i <= n; i++)
        is_prime[i] = true;

    int *p = primes;
    *p++ = 2;

    // i肯定是奇数，从3开始，每次加2
    for (uint64_t i = 3; i <= n; i += 2) {
        if (is_prime[i]) {
            *p++ = i;
            
            // i > N_sqrt时，j的初始值就超过N了，就不用做了
            if (i <= N_sqrt) {
                int add = i + i;
                // j的初始值为i*i（奇数），每次加的应该是偶数（2倍的i）
                for (uint64_t j = i * i; j <= n; j += add) {
                    is_prime[j] = false;
                }
            }
        }
    }
    *p = 0;

    return primes;
}
```

在耗时上已经有了接近10倍的加速，但Cache Miss太严重了
```
      46.55 msec task-clock        #    0.966 CPUs utilized            ( +-  1.89% )
          0      context-switches  #    0.000 /sec                   
          0      cpu-migrations    #    0.000 /sec                   
        488      page-faults       #   10.231 K/sec                    ( +-  0.09% )
111,459,805      cycles            #    2.337 GHz                      ( +-  1.88% )
106,934,367      instructions      #    0.94  insn per cycle           ( +-  0.03% )
 30,217,579      branches          #  633.535 M/sec                    ( +-  0.02% )
    990,184      branch-misses     #    3.28% of all branches          ( +-  0.23% )


112,574,766      cycles                                                     ( +-  3.19% )  (66.18%)
101,925,364      instructions           #    0.89  insn per cycle           ( +-  0.62% )  (83.06%)
  5,218,640      L1-dcache-loads                                            ( +-  4.65% )  (83.13%)
  5,802,087      L1-dcache-load-misses  #  104.59% of all L1-dcache accesses  ( +-  0.98% )  (83.05%)
     20,630      LLC-loads                                                  ( +-  5.91% )  (83.14%)
      5,702      LLC-load-misses        #   30.81% of all LL-cache accesses  ( +-  9.18% )  (84.50%)
```

### Cache优化

因为偶数全都不用考虑，所以可以把数组减小为$\frac{1}{2}$，可以让Cache Hit多一些
```c
#define N      10000000
#define N_sqrt 3162

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
```

稍微高了些，但还是没到实验文档里的“3.02  insn per cycle”、“L1-dcache-load-misses： 11.17%”
```
     17.15 msec task-clock        #    0.647 CPUs utilized            ( +-  3.91% )
         1      context-switches  #   38.440 /sec                     ( +- 23.46% )
         0      cpu-migrations    #    0.000 /sec                   
       526      page-faults       #   20.219 K/sec                    ( +-  2.72% )
68,872,617      cycles            #    2.647 GHz                      ( +-  1.36% )
94,921,982      instructions      #    1.33  insn per cycle           ( +-  0.07% )
20,243,738      branches          #  778.169 M/sec                    ( +-  0.07% )
   743,369      branch-misses     #    3.67% of all branches          ( +-  0.19% )

 68,419,120      cycles                                                     ( +-  0.86% )  (67.37%)
100,803,493      instructions           #    1.49  insn per cycle           ( +-  0.38% )  (86.04%)
  6,712,116      L1-dcache-loads                                            ( +-  0.58% )  (86.03%)
  5,121,724      L1-dcache-load-misses  #   77.31% of all L1-dcache accesses  ( +-  1.08% )  (86.08%)
     13,400      LLC-loads                                                  ( +-  1.86% )  (86.04%)
      2,826      LLC-load-misses        #   19.81% of all LL-cache accesses  ( +-  5.10% )  (74.49%)
```
