// fast_memcpy.cpp

#include <xmmintrin.h>

#include "fast_memcpy.h"

void X_aligned_memcpy_sse2(char* dest, const char* src, const unsigned long size)
{
    for(int i=size/128; i>0; i--) {
        __m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
        _mm_prefetch(src + 128, _MM_HINT_NTA);
        _mm_prefetch(src + 160, _MM_HINT_NTA);
        _mm_prefetch(src + 194, _MM_HINT_NTA);
        _mm_prefetch(src + 224, _MM_HINT_NTA);

        xmm0 = _mm_load_si128((__m128i*)&src[   0]);
        xmm1 = _mm_load_si128((__m128i*)&src[  16]);
        xmm2 = _mm_load_si128((__m128i*)&src[  32]);
        xmm3 = _mm_load_si128((__m128i*)&src[  48]);
        xmm4 = _mm_load_si128((__m128i*)&src[  64]);
        xmm5 = _mm_load_si128((__m128i*)&src[  80]);
        xmm6 = _mm_load_si128((__m128i*)&src[  96]);
        xmm7 = _mm_load_si128((__m128i*)&src[ 112]);

        _mm_stream_si128((__m128i*)&dest[   0], xmm0);
        _mm_stream_si128((__m128i*)&dest[  16], xmm1);
        _mm_stream_si128((__m128i*)&dest[  32], xmm2);
        _mm_stream_si128((__m128i*)&dest[  48], xmm3);
        _mm_stream_si128((__m128i*)&dest[  64], xmm4);
        _mm_stream_si128((__m128i*)&dest[  80], xmm5);
        _mm_stream_si128((__m128i*)&dest[  96], xmm6);
        _mm_stream_si128((__m128i*)&dest[ 112], xmm7);
        src  += 128;
        dest += 128;
    }
}
