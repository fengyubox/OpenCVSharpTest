// IP.cpp: DLL 응용 프로그램을 위해 내보낸 함수를 정의합니다.
//

#include "stdafx.h"
#include "IP.h"
#include <emmIntrin.h>  // SSE
#include <immintrin.h>  // AVX
#include <dvec.h>       // Vector class

IP_API void InverseImageC(BYTE *buf, int bw, int bh, int stride) {
    for (int y = 0; y < bh; y++) {
        BYTE* ppbuf = buf + stride * y;
        for (int x = 0; x < bw; x++, ppbuf++) {
            *ppbuf = ~(*ppbuf);
        }
    }
}

IP_API void InverseImageSse(BYTE *buf, int bw, int bh, int stride) {
    __m128i mfull = _mm_set1_epi8((char)255);
    int nloop = bw / 16; // 한 라인에서  SIMD처리 가능한 횟수
    for (int y = 0; y < bh; y++) {
        // SIMD처리
        __m128i *mpbuf = (__m128i *)(buf + stride * y);
        for (int n = 0; n < nloop; n++, mpbuf++) {
            __m128i mbuf = _mm_loadu_si128(mpbuf);
            mbuf = _mm_xor_si128(mbuf, mfull);
            _mm_storeu_si128(mpbuf, mbuf);
        }

        // C처리
        BYTE *ppbuf = (buf + stride * y + nloop * 16);
        for (int x = nloop * 16; x < bw; x++, ppbuf++) {
            *ppbuf = ~(*ppbuf);
        }
    }
}

IP_API void InverseImageVec(BYTE *buf, int bw, int bh, int stride) {
    I8vec16 vfull(255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255);
    int nloop = bw / 16;
    for (int y = 0; y < bh; y++) {
       // SIMD처리
        __m128i *mpbuf = (__m128i *)(buf + stride * y);
        for (int n = 0; n < nloop; n++, mpbuf++) {
            I8vec16 vbuf = _mm_loadu_si128(mpbuf);
            vbuf ^= vfull;
            _mm_storeu_si128(mpbuf, vbuf);
        }

        // C처리
        BYTE *pp = (buf + stride * y + nloop * 16);
        for (int x = nloop * 16; x < bw; x++, pp++) {
            *pp = ~(*pp);
        }
    }
}

IP_API void InverseImageAvx(BYTE *buf, int bw, int bh, int stride) {
    __m256i mfull = _mm256_set1_epi8((char)255);
    int nloop = bw / 32;
    for (int y = 0; y < bh; y++) {
        // SIMD처리
        __m256i *mpbuf = (__m256i *)(buf + stride * y);
        for (int n = 0; n < nloop; n++, mpbuf++) {
            __m256i mbuf = _mm256_loadu_si256(mpbuf);
            mbuf = _mm256_xor_si256(mbuf, mfull);
            _mm256_storeu_si256(mpbuf, mbuf);
        }

        // C처리
        BYTE *ppbuf = (buf + stride * y + nloop * 32);
        for (int x = nloop * 32; x < bw; x++, ppbuf++) {
            *ppbuf = ~(*ppbuf);
        }
    }
}

void GrassFire(BYTE *thr, BYTE *dst, int bw, int bh, int stride, int x, int y, BYTE label) {
    if (x < 0 || x >= bw || y < 0 || y >= bh)
        return;
    BYTE *thrval = (thr + stride * y + x);
    BYTE *dstval = (dst + stride * y + x);
    if (*thrval == 0)
        return;
    if (*dstval != 0)
        return;
    *dstval = label;
    GrassFire(thr, dst, bw, bh, stride, x-1, y-1, label);
    GrassFire(thr, dst, bw, bh, stride, x  , y-1, label);
    GrassFire(thr, dst, bw, bh, stride, x+1, y-1, label);
    GrassFire(thr, dst, bw, bh, stride, x-1, y  , label);
    GrassFire(thr, dst, bw, bh, stride, x+1, y  , label);
    GrassFire(thr, dst, bw, bh, stride, x-1, y+1, label);
    GrassFire(thr, dst, bw, bh, stride, x  , y+1, label);
    GrassFire(thr, dst, bw, bh, stride, x+1, y+1, label);
}

IP_API void MyBlobC(BYTE *thr, BYTE *dst, int bw, int bh, int stride) {
    int label = 0;
    for (int y = 0; y < bh; y++) {
        BYTE* pthr = thr + stride * y;
        BYTE* pdst = dst + stride * y;
        for (int x = 0; x < bw; x++, pthr++, pdst++) {
            if (*pthr == 0)
                continue;
            if (*pdst != 0)
                continue;
            label++;
            GrassFire(thr, dst, bw, bh, stride, x, y, label);
        }
    }

    for (int y = 0; y < bh; y++) {
        BYTE* pthr = thr + stride * y;
        BYTE* pdst = dst + stride * y;
        for (int x = 0; x < bw; x++, pthr++, pdst++) {
            *pdst = *pdst*10%256;
        }
    }
}