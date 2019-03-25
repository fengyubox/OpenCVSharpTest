// IP.cpp: DLL 응용 프로그램을 위해 내보낸 함수를 정의합니다.
//

#include "stdafx.h"
#include "IP.h"
#include <emmIntrin.h>  // SSE
#include <immintrin.h>  // AVX
#include <dvec.h>       // Vector class
#include <ppl.h>
#include <algorithm>
#include <ppl.h>
//#include <ipp.h>
//#include <ippcv_l.h>

using namespace concurrency;

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
    Iu8vec16 vfull(255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255);
    int nloop = bw / 16;
    for (int y = 0; y < bh; y++) {
       // SIMD처리
        __m128i *mpbuf = (__m128i *)(buf + stride * y);
        for (int n = 0; n < nloop; n++, mpbuf++) {
            Iu8vec16 vbuf = _mm_loadu_si128(mpbuf);
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

inline BYTE GetBorderPixelReplicate(BYTE* ptr, int bw, int bh, int step, int x, int y) {
    if (x < 0) x = 0;
    else if (x > bw - 1) x = bw - 1;

    if (y < 0) y = 0;
    else if (y > bh - 1) y = bh - 1;

    return *(ptr + step * y + x);
}

inline void ErodeBorderPixel(BYTE* srcPtr, BYTE* dstPtr, int bw, int bh, int step, int x, int y) {
    BYTE s0 = GetBorderPixelReplicate(srcPtr, bw, bh, step, x - 1, y - 1);
    BYTE s1 = GetBorderPixelReplicate(srcPtr, bw, bh, step, x, y - 1);
    BYTE s2 = GetBorderPixelReplicate(srcPtr, bw, bh, step, x + 1, y - 1);
    BYTE s3 = GetBorderPixelReplicate(srcPtr, bw, bh, step, x - 1, y);
    BYTE s4 = GetBorderPixelReplicate(srcPtr, bw, bh, step, x, y);
    BYTE s5 = GetBorderPixelReplicate(srcPtr, bw, bh, step, x + 1, y);
    BYTE s6 = GetBorderPixelReplicate(srcPtr, bw, bh, step, x - 1, y + 1);
    BYTE s7 = GetBorderPixelReplicate(srcPtr, bw, bh, step, x, y + 1);
    BYTE s8 = GetBorderPixelReplicate(srcPtr, bw, bh, step, x + 1, y + 1);

    BYTE min = s0;
    if (s1 < min) min = s1;
    if (s2 < min) min = s2;
    if (s3 < min) min = s3;
    if (s4 < min) min = s4;
    if (s5 < min) min = s5;
    if (s6 < min) min = s6;
    if (s7 < min) min = s7;
    if (s8 < min) min = s8;

    *(dstPtr + step * y + x) = min;
}

inline void ErodeBorder(BYTE* srcPtr, BYTE* dstPtr, int bw, int bh, int step) {
    for (int x = 0; x < bw; x++) {
        int yTop = 0;
        int yBottom = bh - 1;
        ErodeBorderPixel(srcPtr, dstPtr, bw, bh, step, x, yTop);
        ErodeBorderPixel(srcPtr, dstPtr, bw, bh, step, x, yBottom);
    }
    for (int y = 1; y < bh - 1; y++) {
        int xLeft = 0;
        int xRight = bw - 1;
        ErodeBorderPixel(srcPtr, dstPtr, bw, bh, step, xLeft, y);
        ErodeBorderPixel(srcPtr, dstPtr, bw, bh, step, xRight, y);
    }
}

inline void Erode1Line(BYTE* sp, BYTE* dp, int step, int x1, int x2) {
    int ofs[] = { -step - 1, -step, -step + 1, -1, 0, 1, step - 1, step, step + 1, };
    
    BYTE* s0 = sp + ofs[0];
    BYTE* s1 = sp + ofs[1];
    BYTE* s2 = sp + ofs[2];
    BYTE* s3 = sp + ofs[3];
    BYTE* s4 = sp + ofs[4];
    BYTE* s5 = sp + ofs[5];
    BYTE* s6 = sp + ofs[6];
    BYTE* s7 = sp + ofs[7];
    BYTE* s8 = sp + ofs[8];
    for (int x = x1; x < x2; x++, dp++, s0++, s1++, s2++, s3++, s4++, s5++, s6++, s7++, s8++) {
        BYTE min = *s0;
        if (*s1 < min) min = *s1;
        if (*s2 < min) min = *s2;
        if (*s3 < min) min = *s3;
        if (*s4 < min) min = *s4;
        if (*s5 < min) min = *s5;
        if (*s6 < min) min = *s6;
        if (*s7 < min) min = *s7;
        if (*s8 < min) min = *s8;
        *(dp) = min;
    }
}

inline void Erode1LineSse(BYTE* sp, BYTE* dp, int step, int x1, int x2) {
    int ofs[] = { -step - 1, -step, -step + 1, -1, 0, 1, step - 1, step, step + 1, };

    int len = x2 - x1;
    int nloop = len / 16;

    // 자투리 처리
    Erode1Line(sp + nloop * 16, dp + nloop * 16, step, x1 + nloop * 16, x2);

    // MMX 시작
    // 버퍼 메모리 주소를 mmx타입의 주소로 캐스팅 : for loop iteration시 포인터 주소 점프 위해
    __m128i *mpdst = (__m128i*)dp;
    __m128i* mp0 = (__m128i*)(sp + ofs[0]);
    __m128i* mp1 = (__m128i*)(sp + ofs[1]);
    __m128i* mp2 = (__m128i*)(sp + ofs[2]);
    __m128i* mp3 = (__m128i*)(sp + ofs[3]);
    __m128i* mp4 = (__m128i*)(sp + ofs[4]);
    __m128i* mp5 = (__m128i*)(sp + ofs[5]);
    __m128i* mp6 = (__m128i*)(sp + ofs[6]);
    __m128i* mp7 = (__m128i*)(sp + ofs[7]);
    __m128i* mp8 = (__m128i*)(sp + ofs[8]);
    // 루프만큼 실행
    for (int x = 0; x < nloop; x++, mpdst++, mp0++, mp1++, mp2++, mp3++, mp4++, mp5++, mp6++, mp7++, mp8++) {
        // 16BYTE씩 src 9픽셀 로드
        __m128i m0 = _mm_loadu_si128(mp0);
        __m128i m1 = _mm_loadu_si128(mp1);
        __m128i m2 = _mm_loadu_si128(mp2);
        __m128i m3 = _mm_loadu_si128(mp3);
        __m128i m4 = _mm_loadu_si128(mp4);
        __m128i m5 = _mm_loadu_si128(mp5);
        __m128i m6 = _mm_loadu_si128(mp6);
        __m128i m7 = _mm_loadu_si128(mp7);
        __m128i m8 = _mm_loadu_si128(mp8);

        // 16개의 바이트에 대해서 최소값 선택
        __m128i min = m0;
        min = _mm_min_epu8(min, m1);
        min = _mm_min_epu8(min, m2);
        min = _mm_min_epu8(min, m3);
        min = _mm_min_epu8(min, m4);
        min = _mm_min_epu8(min, m5);
        min = _mm_min_epu8(min, m6);
        min = _mm_min_epu8(min, m7);
        min = _mm_min_epu8(min, m8);

        // 아웃풋에 저장
        _mm_storeu_si128(mpdst, min);
    }
}

IP_API void ErodeC(BYTE* srcPtr, BYTE* dstPtr, int bw, int bh, int step, ParallelMode parallelMode) {
    ErodeBorder(srcPtr, dstPtr, bw, bh, step);

    int x1 = 1, x2 = bw - 1;
    int y1 = 1, y2 = bh - 1;

    auto actionErode1Line = [=](int y) -> void {
        BYTE* sp = &srcPtr[y * step + x1];
        BYTE* dp = &dstPtr[y * step + x1];
        Erode1Line(sp, dp, step, x1, x2);
    };

    if (parallelMode == ParallelMode::Parallel) {
        parallel_for(y1, y2, actionErode1Line);
    } else {
        for (int y = y1; y < y2; y++) {
            actionErode1Line(y);
        }
    }
}

IP_API void ErodeSse(BYTE* srcPtr, BYTE* dstPtr, int bw, int bh, int step, ParallelMode parallelMode) {
    ErodeBorder(srcPtr, dstPtr, bw, bh, step);

    int x1 = 1, x2 = bw - 1;
    int y1 = 1, y2 = bh - 1;

    auto actionErode1Line = [=](int y) -> void {
        BYTE* sp = &srcPtr[y * step + x1];
        BYTE* dp = &dstPtr[y * step + x1];
        Erode1LineSse(sp, dp, step, x1, x2);
    };

    if (parallelMode == ParallelMode::Parallel) {
        parallel_for(y1, y2, actionErode1Line);
    } else {
        for (int y = y1; y < y2; y++) {
            actionErode1Line(y);
        }
    }
}

IP_API void ErodeIpp(BYTE* srcPtr, BYTE* dstPtr, int bw, int bh, int step) {
}

#define RANGE_S(lval,min,max,val)       \
      {if ((val)<(min)) lval = (min);     \
      else if ((val)>(max)) lval = (max);     \
      else lval = val;}

float MIN4CF(float v1, float v3, float v5, float v7, float v9) {
    float ret;

    ret = v1;
    if (ret > v3) ret = v3;
    if (ret > v5) ret = v5;
    if (ret > v7) ret = v7;
    if (ret > v9) ret = v9;

    return ret;
}

float MIN4PF(float v2, float v4, float v5, float v6, float v8) {
    float ret;

    ret = v2;
    if (ret > v4) ret = v4;
    if (ret > v5) ret = v5;
    if (ret > v6) ret = v6;
    if (ret > v8) ret = v8;

    return ret;
}

BOOL CreateEDMF(BYTE *img_src, int img_bw, int img_bh, int r_x1, int r_y1, int r_x2, int r_y2)
{
    float DIST_A = 1;
    float DIST_B = (float)sqrt(2);

    int x, y;
    int idx;
    int k1, k2, k3, k4, me, k6, k7, k8, k9;
    int edmIdx;
    int edmme;
    float mincross, minplus;
    int r_w, r_h;
    int er_x1, er_x2, er_y1, er_y2;

    float *pedm = NULL;

    r_w = r_x2 - r_x1 + 1;
    r_h = r_y2 - r_y1 + 1;

    pedm = new float[r_w*r_h];
    if (pedm == NULL) return FALSE;

    //Image Copy
    for (y = r_y1; y <= r_y2; y++) {
        idx = y * img_bw;
        edmIdx = (y - r_y1)*r_w;
        for (x = r_x1; x <= r_x2; x++) {
            me = idx + x;
            edmme = edmIdx + (x - r_x1);
            pedm[edmme] = img_src[me];
        }
    }

    er_x1 = 0;
    er_y1 = 0;
    er_x2 = r_x2 - r_x1;
    er_y2 = r_y2 - r_y1;

    //To Right and Bottom
    for (y = er_y1 + 1; y < er_y2; y++) {
        idx = y * r_w;
        for (x = er_x1 + 1; x < er_x2; x++) {
            me = idx + x;
            if (pedm[me] > 0) {
                k1 = me - r_w - 1;      k2 = me - r_w;        k3 = me - r_w + 1;
                k4 = me - 1;                              k6 = me + 1;
                k7 = me + r_w - 1;      k8 = me + r_w;        k9 = me + r_w + 1;

                mincross = MIN4CF(pedm[k1], pedm[k3],
                    pedm[me],
                    pedm[k7], pedm[k9]);

                minplus = MIN4PF(pedm[k2],
                    pedm[k4], pedm[me], pedm[k6],
                    pedm[k8]);

                if (mincross < minplus) {
                    pedm[me] = mincross + DIST_B;
                }
                else {
                    pedm[me] = minplus + DIST_A;
                }
            }
        }
    }

    //To Left and Top
    for (y = er_y2 - 1; y > er_y1; y--) {
        idx = y * r_w;
        for (x = er_x2 - 1; x > er_x1; x--) {
            me = idx + x;
            if (pedm[me] > 0) {
                k1 = me - r_w - 1;      k2 = me - r_w;        k3 = me - r_w + 1;
                k4 = me - 1;                              k6 = me + 1;
                k7 = me + r_w - 1;      k8 = me + r_w;        k9 = me + r_w + 1;

                mincross = MIN4CF(pedm[k1], pedm[k3],
                    pedm[me],
                    pedm[k7], pedm[k9]);

                minplus = MIN4PF(pedm[k2],
                    pedm[k4], pedm[me], pedm[k6],
                    pedm[k8]);

                if (mincross < minplus) {
                    pedm[me] = mincross + DIST_B;
                }
                else {
                    pedm[me] = minplus + DIST_A;
                }
            }
        }
    }


    //Image Copy
    for (y = r_y1; y <= r_y2; y++) {
        idx = y * img_bw;
        edmIdx = (y - r_y1)*r_w;
        for (x = r_x1; x <= r_x2; x++) {
            me = idx + x;
            edmme = edmIdx + (x - r_x1);
            if (pedm[edmme] > 0) {
                RANGE_S(img_src[me], 0, 255, (BYTE)round(pedm[edmme]));
            }
        }
    }


    delete[] pedm;

    return TRUE;
}