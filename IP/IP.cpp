// IP.cpp: DLL 응용 프로그램을 위해 내보낸 함수를 정의합니다.
//

#include "stdafx.h"
#include "IP.h"
#include <emmIntrin.h>

IP_API void InverseImage(BYTE *buf, int bw, int bh, int stride) {
   for (int y = 0; y < bh; y++) {
      BYTE* pp = buf + stride * y;
      for (int x = 0; x < bw; x++, pp++) {
         *pp = ~(*pp);
      }
   }
}

IP_API void MmxInverseImage(BYTE *buf, int bw, int bh, int stride) {
   int nloop = bw / 16;
   for (int y = 0; y < bh; y++) {
      __m128i *mpbuf = (__m128i *)(buf + stride * y);
      __m128i mfull = _mm_set1_epi8(0xFF);
      for (int n = 0; n < nloop; n++, mpbuf++) {
         __m128i mbuf = _mm_loadu_si128(mpbuf);
         mbuf = _mm_andnot_si128(mbuf, mfull);
         _mm_storeu_si128(mpbuf, mbuf);
      }

      BYTE *pp = (buf + stride * y + nloop * 16);
      for (int x = nloop * 16; x < bw; x++, pp++) {
         *pp = ~(*pp);
      }
   }
}
