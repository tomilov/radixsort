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

alignas(32) size_type bin[2][sizeof(size_type)][0x100] = {0};

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
    constexpr size_type l = sizeof(__m128i) / sizeof(size_type);
    const size_type m = std::size(v);
    const size_type n = (m + (l - 1)) / l;
    asm
    (
    "bloop:"
    "prefetchnta 256(%[v]);"
    "vmovdqa (%[v]), %%xmm0;"
    "lea 16(%[v]), %[v];"

    "vpextrb $0, %%xmm0, %%ebx;"
    "incl %[bin0](,%%ebx,4);"
    "vpextrb $1, %%xmm0, %%ebx;"
    "incl %[bin1](,%%ebx,4);"
    "vpextrb $2, %%xmm0, %%ebx;"
    "incl %[bin2](,%%ebx,4);"
    "vpextrb $3, %%xmm0, %%ebx;"
    "incl %[bin3](,%%ebx,4);"
    "vpextrb $4, %%xmm0, %%ebx;"
    "incl %[bin4](,%%ebx,4);"
    "vpextrb $5, %%xmm0, %%ebx;"
    "incl %[bin5](,%%ebx,4);"
    "vpextrb $6, %%xmm0, %%ebx;"
    "incl %[bin6](,%%ebx,4);"
    "vpextrb $7, %%xmm0, %%ebx;"
    "incl %[bin7](,%%ebx,4);"

    "vpextrb $8, %%xmm0, %%ebx;"
    "incl %[bin0](,%%ebx,4);"
    "vpextrb $9, %%xmm0, %%ebx;"
    "incl %[bin1](,%%ebx,4);"
    "vpextrb $10, %%xmm0, %%ebx;"
    "incl %[bin2](,%%ebx,4);"
    "vpextrb $11, %%xmm0, %%ebx;"
    "incl %[bin3](,%%ebx,4);"
    "vpextrb $12, %%xmm0, %%ebx;"
    "incl %[bin4](,%%ebx,4);"
    "vpextrb $13, %%xmm0, %%ebx;"
    "incl %[bin5](,%%ebx,4);"
    "vpextrb $14, %%xmm0, %%ebx;"
    "incl %[bin6](,%%ebx,4);"
    "vpextrb $15, %%xmm0, %%ebx;"
    "incl %[bin7](,%%ebx,4);"

    "dec %[n];"
    "jnz bloop;"
    :
    : [n]"r"(n), [v]"r"(v + 0),
      [bin0]"m"(bin[0][0]), [bin1]"m"(bin[0][1]), [bin2]"m"(bin[0][2]), [bin3]"m"(bin[0][3]),
      [bin4]"m"(bin[1][0]), [bin5]"m"(bin[1][1]), [bin6]"m"(bin[1][2]), [bin7]"m"(bin[1][3])
    : "memory", "cc",
      "%ebx",
      "%xmm0"
    );
    size_type sum[sizeof(size_type)] = {0, 0, 0, 0};
    for (size_type b = 0; b < sizeof(size_type); ++b) {
        for (size_type i = 0; i < 0x100; ++i) {
            bin[0][b][i] += bin[1][b][i];
            sum[b] += std::exchange(bin[0][b][i], sum[b]);
        }
    }
    for (auto & e : v) {
        _mm_prefetch(&e + 65, _MM_HINT_T2);
        s[bin[0][0][e & 0xFF]++] = e;
    }
    for (auto & e : s) {
        _mm_prefetch(&e + 65, _MM_HINT_T2);
        v[bin[0][1][(e >> (1 * 8)) & 0xFF]++] = e;
    }
    for (auto & e : v) {
        _mm_prefetch(&e + 65, _MM_HINT_T2);
        s[bin[0][2][(e >> (2 * 8)) & 0xFF]++] = e;
    }
    for (auto & e : s) {
        _mm_prefetch(&e + 65, _MM_HINT_T2);
        v[bin[0][3][e >> (3 * 8)]++] = e;
    }
    std::cout << std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
    assert(std::is_sorted(std::cbegin(v), std::cend(v)));
}
