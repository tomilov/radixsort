#include <iostream>
#include <random>
#include <algorithm>
#include <chrono>
#include <iterator>

#include <cstdint>

using size_type = std::uint32_t;

alignas(64) size_type v[1200000];

#ifdef __linux__
#include <sched.h>
#endif

int main()
{
#ifdef __linux__
    {
        cpu_set_t m;
        int status;

        CPU_ZERO(&m);
        CPU_SET(0, &m);
        status = sched_setaffinity(0, sizeof(m), &m);
        if (status != 0) {
            perror("sched_setaffinity");
        }
    }
#endif
    std::mt19937 g(0);

    for (size_type i = 1; i < std::size(v); ++i) {
        v[i] = std::exchange(v[g() % i], i);
    }
    auto start = std::chrono::high_resolution_clock::now();
    std::sort(std::begin(v), std::end(v));
    std::cout << std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
}
