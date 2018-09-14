#include <limits>
#include <iostream>
#include <random>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <functional>

#include <cassert>

#include <x86intrin.h>

using size_type = std::uint32_t;

namespace
{

constexpr size_type m = 80000;
alignas(__m128i) size_type v[m];
alignas(__m128i) size_type s[m];

constexpr size_type nbits = std::numeric_limits< unsigned char >::digits;
constexpr size_type nbytes = sizeof(size_type) / sizeof(unsigned char);
constexpr size_type bmask = (1u << nbits) - 1;

alignas(64) size_type byte[nbytes][1u << nbits] = {};

}

int main()
{
    std::random_device rd;
    std::mt19937 g(rd());

    {
        auto start = std::chrono::high_resolution_clock::now();
        switch (0) {
        case 0 : {
            std::generate(std::begin(v), std::end(v), std::ref(g));
            std::sort(std::begin(v), std::end(v));
            for (;;) {
                auto last = std::unique(std::begin(v), std::end(v));
                if (last == std::end(v)) {
                    break;
                }
                std::cout << std::distance(last, std::end(v)) << " of " << m << " remains" << std::endl;
                std::generate(last, std::end(v), std::ref(g));
                std::sort(last, std::end(v));
                std::inplace_merge(std::begin(v), last, std::end(v));
            }
            std::shuffle(std::begin(v), std::end(v), g);
            break;
        }
        case 1 : {
            for (size_type i = 1; i < m; ++i) {
                v[i] = std::exchange(v[g() % i], i);
            }
            break;
        }
        case 2 : {
            std::iota(std::begin(v), std::end(v), 0);
            for (size_type i = 0; i < m - 1; ++i) {
                std::swap(v[i + (g() % (m - i))], v[i]);
            }
            break;
        }
        default : {
            assert(false);
        }
        }
        std::cout << "generated " << std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
    }

    {
        std::copy(std::cbegin(v), std::cend(v), std::begin(s));
        auto start = std::chrono::high_resolution_clock::now();
        std::sort(std::begin(s), std::end(s));
        std::cout << "timsort " << std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
    }

    {
        std::copy(std::cbegin(v), std::cend(v), std::begin(s));
        auto start = std::chrono::high_resolution_clock::now();
        std::stable_sort(std::begin(s), std::end(s));
        std::cout << "merge sort " << std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
    }

    {
        auto start = std::chrono::high_resolution_clock::now();
    #if 1
        constexpr size_type l = sizeof(__m128i) / nbytes;
        const size_type n = (m + (l - 1)) / l;
        asm
        (
        "bloop:"
        "prefetcht1 384(%[v]);"
        "vmovdqa (%[v]), %%xmm0;"
        "add $16, %[v];"

        "vpextrb $0, %%xmm0, %[offset];"
        "incl %[byte0](,%[offset],4);"
        "vpextrb $1, %%xmm0, %[offset];"
        "incl %[byte1](,%[offset],4);"
        "vpextrb $2, %%xmm0, %[offset];"
        "incl %[byte2](,%[offset],4);"
        "vpextrb $3, %%xmm0, %[offset];"
        "incl %[byte3](,%[offset],4);"
        "vpextrb $4, %%xmm0, %[offset];"
        "incl %[byte0](,%[offset],4);"
        "vpextrb $5, %%xmm0, %[offset];"
        "incl %[byte1](,%[offset],4);"
        "vpextrb $6, %%xmm0, %[offset];"
        "incl %[byte2](,%[offset],4);"
        "vpextrb $7, %%xmm0, %[offset];"
        "incl %[byte3](,%[offset],4);"
        "vpextrb $8, %%xmm0, %[offset];"
        "incl %[byte0](,%[offset],4);"
        "vpextrb $9, %%xmm0, %[offset];"
        "incl %[byte1](,%[offset],4);"
        "vpextrb $10, %%xmm0, %[offset];"
        "incl %[byte2](,%[offset],4);"
        "vpextrb $11, %%xmm0, %[offset];"
        "incl %[byte3](,%[offset],4);"
        "vpextrb $12, %%xmm0, %[offset];"
        "incl %[byte0](,%[offset],4);"
        "vpextrb $13, %%xmm0, %[offset];"
        "incl %[byte1](,%[offset],4);"
        "vpextrb $14, %%xmm0, %[offset];"
        "incl %[byte2](,%[offset],4);"
        "vpextrb $15, %%xmm0, %[offset];"
        "incl %[byte3](,%[offset],4);"

        "dec %[n];"
        "jnz bloop;"
        :
        : [n]"r"(n), [v]"r"(v + 0), [offset]"r"(0),
          [byte0]"m"(byte[0]), [byte1]"m"(byte[1]), [byte2]"m"(byte[2]), [byte3]"m"(byte[3])
        : "memory", "cc",
          "%xmm0"
        );
    #else
        for (const size_type & e : v) {
            _mm_prefetch(&e + 96, _MM_HINT_T2);
            ++byte[0][e & bmask];
            ++byte[1][(e >> (1 * bits)) & bmask];
            ++byte[2][(e >> (2 * bits)) & bmask];
            ++byte[3][e >> (3 * bits)];
        }
    #endif
        size_type sum[nbytes] = {0, 0, 0, 0};
        for (size_type b = 0; b < nbytes; ++b) {
            for (size_type i = 0; i < (1u << nbits); ++i) {
                sum[b] += std::exchange(byte[b][i], sum[b]);
            }
        }
        std::cout << std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
        for (const size_type & e : v) {
            _mm_prefetch(&e + 96, _MM_HINT_T2);
            s[byte[0][e & bmask]++] = e;
        }
        for (const size_type & e : s) {
            _mm_prefetch(&e + 96, _MM_HINT_T2);
            v[byte[1][(e >> (1 * nbits)) & bmask]++] = e;
        }
        for (const size_type & e : v) {
            _mm_prefetch(&e + 96, _MM_HINT_T2);
            s[byte[2][(e >> (2 * nbits)) & bmask]++] = e;
        }
        for (const size_type & e : s) {
            _mm_prefetch(&e + 96, _MM_HINT_T2);
            v[byte[3][e >> (3 * nbits)]++] = e;
        }
        std::cout << "radix sort " << std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
        assert(std::is_sorted(std::cbegin(v), std::cend(v)));
    }
}
