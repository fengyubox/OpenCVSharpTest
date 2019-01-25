// IP.cpp: DLL 응용 프로그램을 위해 내보낸 함수를 정의합니다.
//

#include "stdafx.h"
#include "IP.h"
#include <emmIntrin.h>  // SSE
#include <immintrin.h>  // AVX
#include <dvec.h>       // Vector class

IP_API void InverseC(BYTE *buf, int bw, int bh, int stride) {
    for (int y = 0; y < bh; y++) {
        BYTE* ppbuf = buf + stride * y;
        for (int x = 0; x < bw; x++, ppbuf++) {
            *ppbuf = ~(*ppbuf);
        }
    }
}

IP_API void InverseSse(BYTE *buf, int bw, int bh, int stride) {
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

IP_API void InverseVec(BYTE *buf, int bw, int bh, int stride) {
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

IP_API void InverseAvx(BYTE *buf, int bw, int bh, int stride) {
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

inline BYTE GetPixelEdgeReplicate(BYTE* ptr, int bw, int bh, int step, int x, int y) {
    if (x < 0) x = 0;
    else if (x > bw - 1) x = bw - 1;

    if (y < 0) y = 0;
    else if (y > bh - 1) y = bh - 1;

    return *(ptr + step * y + x);
}

inline void ErodeEdgePixel(BYTE* srcPtr, BYTE* dstPtr, int bw, int bh, int step, int x, int y) {
    BYTE s1 = GetPixelEdgeReplicate(srcPtr, bw, bh, step, x - 1, y - 1);
    BYTE s2 = GetPixelEdgeReplicate(srcPtr, bw, bh, step, x, y - 1);
    BYTE s3 = GetPixelEdgeReplicate(srcPtr, bw, bh, step, x + 1, y - 1);
    BYTE s4 = GetPixelEdgeReplicate(srcPtr, bw, bh, step, x - 1, y);
    BYTE s5 = GetPixelEdgeReplicate(srcPtr, bw, bh, step, x, y);
    BYTE s6 = GetPixelEdgeReplicate(srcPtr, bw, bh, step, x + 1, y);
    BYTE s7 = GetPixelEdgeReplicate(srcPtr, bw, bh, step, x - 1, y + 1);
    BYTE s8 = GetPixelEdgeReplicate(srcPtr, bw, bh, step, x, y + 1);
    BYTE s9 = GetPixelEdgeReplicate(srcPtr, bw, bh, step, x + 1, y + 1);

    BYTE min = s1;
    if (s2 < min) min = s2;
    if (s3 < min) min = s3;
    if (s4 < min) min = s4;
    if (s5 < min) min = s5;
    if (s6 < min) min = s6;
    if (s7 < min) min = s7;
    if (s8 < min) min = s8;
    if (s9 < min) min = s9;

    *(dstPtr + step * y + x) = min;
}

IP_API void ErodeC(BYTE* srcPtr, BYTE* dstPtr, int bw, int bh, int step) {
    int x1 = 1, x2 = bw - 2;
    int y1 = 1, y2 = bh - 2;
    for (int y = y1; y <= y2; y++) {
        BYTE* psrc = &srcPtr[y * step + x1];
        BYTE* pdst = &dstPtr[y * step + x1];
        BYTE* s1 = (psrc - step - 1);
        BYTE* s2 = (psrc - step);
        BYTE* s3 = (psrc - step + 1);
        BYTE* s4 = (psrc - 1);
        BYTE* s6 = (psrc + 1);
        BYTE* s7 = (psrc + step - 1);
        BYTE* s8 = (psrc + step);
        BYTE* s9 = (psrc + step + 1);
        for (int x = x1; x <= x2; x++, psrc++, pdst++, s1++, s2++, s3++, s4++, s6++, s7++, s8++, s9++) {
            BYTE min = *s1;
            if (*s2 < min) min = *s2;
            if (*s3 < min) min = *s3;
            if (*s4 < min) min = *s4;
            if (*psrc < min) min = *psrc;
            if (*s6 < min) min = *s6;
            if (*s7 < min) min = *s7;
            if (*s8 < min) min = *s8;
            if (*s9 < min) min = *s9;

            *(pdst) = min;
        }
    }

    // edge pixel process
    for (int x = 0; x < bw; x++) {
        int yTop = 0, yBottom = bh - 1;
        ErodeEdgePixel(srcPtr, dstPtr, bw, bh, step, x, yTop);
        ErodeEdgePixel(srcPtr, dstPtr, bw, bh, step, x, yBottom);
    }
    for (int y = 1; y < bh - 1; y++) {
        int xLeft = 0, xRight = bw - 1;
        ErodeEdgePixel(srcPtr, dstPtr, bw, bh, step, xLeft, y);
        ErodeEdgePixel(srcPtr, dstPtr, bw, bh, step, xRight, y);
    }
}

IP_API void ErodeC2(BYTE* srcPtr, BYTE* dstPtr, int bw, int bh, int step) {
    int x1 = 1, x2 = bw - 2;
    int y1 = 1, y2 = bh - 2;
    for (int y = y1; y <= y2; y++) {
        BYTE* psrc = &srcPtr[y * step + x1];
        BYTE* pdst = &dstPtr[y * step + x1];
        
        for (int x = x1; x <= x2; x++, psrc++, pdst++) {
            BYTE s1 = *(psrc - step - 1);
            BYTE s2 = *(psrc - step);
            BYTE s3 = *(psrc - step + 1);
            BYTE s4 = *(psrc - 1);
            BYTE s5 = *(psrc);
            BYTE s6 = *(psrc + 1);
            BYTE s7 = *(psrc + step - 1);
            BYTE s8 = *(psrc + step);
            BYTE s9 = *(psrc + step + 1);

            BYTE min = s1;
            if (s2 < min) min = s2;
            if (s3 < min) min = s3;
            if (s4 < min) min = s4;
            if (s5 < min) min = s5;
            if (s6 < min) min = s6;
            if (s7 < min) min = s7;
            if (s8 < min) min = s8;
            if (s9 < min) min = s9;

            *(pdst) = min;
        }
    }

    // edge pixel process
    for (int x = 0; x < bw; x++) {
        int yTop = 0, yBottom = bh - 1;
        ErodeEdgePixel(srcPtr, dstPtr, bw, bh, step, x, yTop);
        ErodeEdgePixel(srcPtr, dstPtr, bw, bh, step, x, yBottom);
    }
    for (int y = 1; y < bh - 1; y++) {
        int xLeft = 0, xRight = bw - 1;
        ErodeEdgePixel(srcPtr, dstPtr, bw, bh, step, xLeft, y);
        ErodeEdgePixel(srcPtr, dstPtr, bw, bh, step, xRight, y);
    }
}

IP_API void ErodeSse(BYTE* srcPtr, BYTE* dstPtr, int bw, int bh, int step) {
    int x1 = 1, x2 = bw - 2;
    int y1 = 1, y2 = bh - 2;
    
    int len = x2 - x1 + 1;
    int nloop = len / 16;
    for (int y = y1; y <= y2; y++) {
        // 16byte씩 나눈 나머지 처리
        int   xx2 = x1 + nloop * 16;    // 나머지 시작 x좌표
        BYTE* psrc = &srcPtr[y * step + xx2];
        BYTE* pdst = &dstPtr[y * step + xx2];
        BYTE* s1 = (psrc - step - 1);
        BYTE* s2 = (psrc - step);
        BYTE* s3 = (psrc - step + 1);
        BYTE* s4 = (psrc - 1);
        BYTE* s6 = (psrc + 1);
        BYTE* s7 = (psrc + step - 1);
        BYTE* s8 = (psrc + step);
        BYTE* s9 = (psrc + step + 1);
        for (int x = xx2; x <= x2; x++, psrc++, pdst++, s1++, s2++, s3++, s4++, s6++, s7++, s8++, s9++) {
            BYTE min = *s1;
            if (*s2 < min) min = *s2;
            if (*s3 < min) min = *s3;
            if (*s4 < min) min = *s4;
            if (*psrc < min) min = *psrc;
            if (*s6 < min) min = *s6;
            if (*s7 < min) min = *s7;
            if (*s8 < min) min = *s8;
            if (*s9 < min) min = *s9;

            *(pdst) = min;
        }

        // MMX 시작
        psrc = &srcPtr[y*step + x1];
        pdst = &dstPtr[y*step + x1];

        // 버퍼 메모리 주소를 mmx타입의 주소로 캐스팅 : for loop iteration시 포인터 주소 점프 위해
        __m128i *mpsrc = (__m128i*)psrc;
        __m128i *mpdst = (__m128i*)pdst;
        __m128i *mp1 = (__m128i*)(psrc - step - 1), *mp2 = (__m128i*)(psrc - step), *mp3 = (__m128i*)(psrc - step + 1);
        __m128i *mp4 = (__m128i*)(psrc - 1),                                        *mp6 = (__m128i*)(psrc + 1);
        __m128i *mp7 = (__m128i*)(psrc + step - 1), *mp8 = (__m128i*)(psrc + step), *mp9 = (__m128i*)(psrc + step + 1);

        // 
        for (int x = 0; x < nloop; x++, mpdst++, mpsrc++, mp1++, mp2++, mp3++, mp4++, mp6++, mp7++, mp8++, mp9++) {
            // 16byte씩 src 9픽셀 로드
            __m128i m1 = _mm_loadu_si128(mp1);
            __m128i m2 = _mm_loadu_si128(mp2);
            __m128i m3 = _mm_loadu_si128(mp3);
            __m128i m4 = _mm_loadu_si128(mp4);
            __m128i m5 = _mm_loadu_si128(mpsrc);
            __m128i m6 = _mm_loadu_si128(mp6);
            __m128i m7 = _mm_loadu_si128(mp7);
            __m128i m8 = _mm_loadu_si128(mp8);
            __m128i m9 = _mm_loadu_si128(mp9);
            
            // 16개의 바이트에 대해서 최소값 선택
            __m128i mind = m1;
            mind = _mm_min_epu8(mind, m2);
            mind = _mm_min_epu8(mind, m3);
            mind = _mm_min_epu8(mind, m4);
            mind = _mm_min_epu8(mind, m5);
            mind = _mm_min_epu8(mind, m6);
            mind = _mm_min_epu8(mind, m7);
            mind = _mm_min_epu8(mind, m8);
            mind = _mm_min_epu8(mind, m9);

            // 아웃풋에 저장
            _mm_storeu_si128(mpdst, mind);
        }
    }

    // edge pixel process
    for (int x = 0; x < bw; x++) {
        int yTop = 0, yBottom = bh - 1;
        ErodeEdgePixel(srcPtr, dstPtr, bw, bh, step, x, yTop);
        ErodeEdgePixel(srcPtr, dstPtr, bw, bh, step, x, yBottom);
    }
    for (int y = 1; y < bh - 1; y++) {
        int xLeft = 0, xRight = bw - 1;
        ErodeEdgePixel(srcPtr, dstPtr, bw, bh, step, xLeft, y);
        ErodeEdgePixel(srcPtr, dstPtr, bw, bh, step, xRight, y);
    }
}