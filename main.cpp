#include <limits>
#include <iostream>
#include <random>
#include <algorithm>
#include <iterator>
#include <chrono>

#include <cassert>

#include <x86intrin.h>

using size_type = std::uint32_t;

alignas(__m128i) size_type v[10000000];
alignas(__m128i) size_type s[std::size(v)];

constexpr size_type bits = std::numeric_limits< unsigned char >::digits;
constexpr size_type mask = (1 << bits) - 1;

alignas(32) size_type bin[sizeof(size_type)][1 << bits] = {0};

alignas(__m128i) size_type w[std::size(v)];

int main()
{
    std::random_device rd;
    std::mt19937 g(rd());

    std::generate(std::begin(v), std::end(v), g);
    for (;;) {
        auto last = std::unique(std::begin(v), std::end(v));
        if (last == std::end(v)) {
            break;
        }
        std::generate(last, std::end(v), g);
    }
    std::sort(std::begin(v), std::end(v));

    std::shuffle(std::begin(v), std::end(v), g);
    std::copy(std::cbegin(v), std::cend(v), std::begin(w));
    auto start = std::chrono::high_resolution_clock::now();
    std::sort(std::begin(w), std::end(w));
    std::cout << std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::high_resolution_clock::now() - start).count() << std::endl;

    start = std::chrono::high_resolution_clock::now();
    const size_type m = std::size(v);
#if 1
    constexpr size_type l = sizeof(__m128i) / sizeof(size_type);
    const size_type n = (m + (l - 1)) / l;
    asm
    (
    "bloop:"
    "prefetchnta 256(%[v]);"
    "vmovdqa (%[v]), %%xmm0;"
    "lea 16(%[v]), %[v];"

    "vpextrb $0, %%xmm0, %[offset];"
    "incl %[bin0](,%[offset],4);"
    "vpextrb $1, %%xmm0, %[offset];"
    "incl %[bin1](,%[offset],4);"
    "vpextrb $2, %%xmm0, %[offset];"
    "incl %[bin2](,%[offset],4);"
    "vpextrb $3, %%xmm0, %[offset];"
    "incl %[bin3](,%[offset],4);"
    "vpextrb $4, %%xmm0, %[offset];"
    "incl %[bin0](,%[offset],4);"
    "vpextrb $5, %%xmm0, %[offset];"
    "incl %[bin1](,%[offset],4);"
    "vpextrb $6, %%xmm0, %[offset];"
    "incl %[bin2](,%[offset],4);"
    "vpextrb $7, %%xmm0, %[offset];"
    "incl %[bin3](,%[offset],4);"
    "vpextrb $8, %%xmm0, %[offset];"
    "incl %[bin0](,%[offset],4);"
    "vpextrb $9, %%xmm0, %[offset];"
    "incl %[bin1](,%[offset],4);"
    "vpextrb $10, %%xmm0, %[offset];"
    "incl %[bin2](,%[offset],4);"
    "vpextrb $11, %%xmm0, %[offset];"
    "incl %[bin3](,%[offset],4);"
    "vpextrb $12, %%xmm0, %[offset];"
    "incl %[bin0](,%[offset],4);"
    "vpextrb $13, %%xmm0, %[offset];"
    "incl %[bin1](,%[offset],4);"
    "vpextrb $14, %%xmm0, %[offset];"
    "incl %[bin2](,%[offset],4);"
    "vpextrb $15, %%xmm0, %[offset];"
    "incl %[bin3](,%[offset],4);"

    "dec %[n];"
    "jnz bloop;"
    :
    : [n]"r"(n), [v]"r"(v + 0), [offset]"r"(0),
      [bin0]"m"(bin[0]), [bin1]"m"(bin[1]), [bin2]"m"(bin[2]), [bin3]"m"(bin[3])
    : "memory", "cc",
      "%ebx",
      "%xmm0"
    );
#else
    for (const size_type & e : v) {
        ++bin[0][0][e & mask];
        ++bin[0][1][(e >> (1 * bits)) & mask];
        ++bin[0][2][(e >> (2 * bits)) & mask];
        ++bin[0][3][e >> (3 * bits)];
    }
#endif
    std::cout << std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
    start = std::chrono::high_resolution_clock::now();
    size_type sum[sizeof(size_type)] = {0, 0, 0, 0};
    for (size_type b = 0; b < sizeof(size_type); ++b) {
        for (size_type i = 0; i < (1 << bits); ++i) {
            sum[b] += std::exchange(bin[b][i], sum[b]);
        }
    }
    for (const size_type & e : v) {
        _mm_prefetch(&e + 65, _MM_HINT_T2);
        s[bin[0][e & mask]++] = e;
    }
    for (const size_type & e : s) {
        _mm_prefetch(&e + 65, _MM_HINT_T2);
        v[bin[1][(e >> (1 * bits)) & mask]++] = e;
    }
    for (const size_type & e : v) {
        _mm_prefetch(&e + 65, _MM_HINT_T2);
        s[bin[2][(e >> (2 * bits)) & mask]++] = e;
    }
    for (const size_type & e : s) {
        _mm_prefetch(&e + 65, _MM_HINT_T2);
        v[bin[3][e >> (3 * bits)]++] = e;
    }
    std::cout << std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
    assert(std::is_sorted(std::cbegin(v), std::cend(v)));
}
