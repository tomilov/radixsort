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

alignas(__m128i) size_type v[1200000];
alignas(__m128i) size_type s[std::size(v)];

constexpr size_type bits = std::numeric_limits< unsigned char >::digits;
constexpr size_type bytes = sizeof(size_type) / sizeof(unsigned char);
constexpr size_type mask = (1 << bits) - 1;

alignas(32) size_type bit[bytes][1 << bits] = {0};

alignas(__m128i) size_type w[std::size(v)];

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
                std::cout << std::distance(last, std::end(v)) << " of " << std::size(v) << " remains" << std::endl;
                std::generate(last, std::end(v), std::ref(g));
                std::sort(last, std::end(v));
                std::inplace_merge(std::begin(v), last, std::end(v));
            }
            std::shuffle(std::begin(v), std::end(v), g);
            break;
        }
        case 1 : {
            for (size_type i = 1; i < std::size(v); ++i) {
                v[i] = std::exchange(v[g() % i], i);
            }
            break;
        }
        case 2 : {
            std::iota(std::begin(v), std::end(v), 0);
            for (size_type i = 0; i < std::size(v) - 1; ++i) {
                std::swap(v[i + (g() % (std::size(v) - i))], v[i]);
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
        std::copy(std::cbegin(v), std::cend(v), std::begin(w));
        auto start = std::chrono::high_resolution_clock::now();
        std::sort(std::begin(w), std::end(w));
        std::cout << "timsort " << std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
    }

    {
        std::copy(std::cbegin(v), std::cend(v), std::begin(w));
        auto start = std::chrono::high_resolution_clock::now();
        std::stable_sort(std::begin(w), std::end(w));
        std::cout << "merge sort " << std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
    }

    {
        auto start = std::chrono::high_resolution_clock::now();
    #if 1
        const size_type m = std::size(v);
        constexpr size_type l = sizeof(__m128i) / bytes;
        const size_type n = (m + (l - 1)) / l;
        asm
        (
        "bloop:"
        "prefetchnta 512(%[v]);"
        "vmovdqa (%[v]), %%xmm0;"
        "lea 16(%[v]), %[v];"

        "vpextrb $0, %%xmm0, %[offset];"
        "incl %[bit0](,%[offset],4);"
        "vpextrb $1, %%xmm0, %[offset];"
        "incl %[bit1](,%[offset],4);"
        "vpextrb $2, %%xmm0, %[offset];"
        "incl %[bit2](,%[offset],4);"
        "vpextrb $3, %%xmm0, %[offset];"
        "incl %[bit3](,%[offset],4);"
        "vpextrb $4, %%xmm0, %[offset];"
        "incl %[bit0](,%[offset],4);"
        "vpextrb $5, %%xmm0, %[offset];"
        "incl %[bit1](,%[offset],4);"
        "vpextrb $6, %%xmm0, %[offset];"
        "incl %[bit2](,%[offset],4);"
        "vpextrb $7, %%xmm0, %[offset];"
        "incl %[bit3](,%[offset],4);"
        "vpextrb $8, %%xmm0, %[offset];"
        "incl %[bit0](,%[offset],4);"
        "vpextrb $9, %%xmm0, %[offset];"
        "incl %[bit1](,%[offset],4);"
        "vpextrb $10, %%xmm0, %[offset];"
        "incl %[bit2](,%[offset],4);"
        "vpextrb $11, %%xmm0, %[offset];"
        "incl %[bit3](,%[offset],4);"
        "vpextrb $12, %%xmm0, %[offset];"
        "incl %[bit0](,%[offset],4);"
        "vpextrb $13, %%xmm0, %[offset];"
        "incl %[bit1](,%[offset],4);"
        "vpextrb $14, %%xmm0, %[offset];"
        "incl %[bit2](,%[offset],4);"
        "vpextrb $15, %%xmm0, %[offset];"
        "incl %[bit3](,%[offset],4);"

        "dec %[n];"
        "jnz bloop;"
        :
        : [n]"r"(n), [v]"r"(v + 0), [offset]"r"(0),
          [bit0]"m"(bit[0]), [bit1]"m"(bit[1]), [bit2]"m"(bit[2]), [bit3]"m"(bit[3])
        : "memory", "cc",
          "%ebx",
          "%xmm0"
        );
    #else
        for (const size_type & e : v) {
            _mm_prefetch(&e + 65, _MM_HINT_T2);
            ++bit[0][e & mask];
            ++bit[1][(e >> (1 * bits)) & mask];
            ++bit[2][(e >> (2 * bits)) & mask];
            ++bit[3][e >> (3 * bits)];
        }
    #endif
        size_type sum[bytes] = {0, 0, 0, 0};
        for (size_type b = 0; b < bytes; ++b) {
            for (size_type i = 0; i < (1 << bits); ++i) {
                sum[b] += std::exchange(bit[b][i], sum[b]);
            }
        }
        std::cout << std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
        for (const size_type & e : v) {
            _mm_prefetch(&e + 256, _MM_HINT_T2);
            s[bit[0][e & mask]++] = e;
        }
        for (const size_type & e : s) {
            _mm_prefetch(&e + 256, _MM_HINT_T2);
            v[bit[1][(e >> (1 * bits)) & mask]++] = e;
        }
        for (const size_type & e : v) {
            _mm_prefetch(&e + 256, _MM_HINT_T2);
            s[bit[2][(e >> (2 * bits)) & mask]++] = e;
        }
        for (const size_type & e : s) {
            _mm_prefetch(&e + 256, _MM_HINT_T2);
            v[bit[3][e >> (3 * bits)]++] = e;
        }
        std::cout << "radix sort " << std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
        assert(std::is_sorted(std::cbegin(v), std::cend(v)));
    }
}
