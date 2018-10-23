﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace OpenCVSharpTest {
    class IpDll {
        [DllImport("IP.dll")] extern public static void InverseImageC(IntPtr buf, int bw, int bh, int stride);
        [DllImport("IP.dll")] extern public static void InverseImageSse(IntPtr buf, int bw, int bh, int stride);
        [DllImport("IP.dll")] extern public static void InverseImageVec(IntPtr buf, int bw, int bh, int stride);
        [DllImport("IP.dll")] extern public static void InverseImageAvx(IntPtr buf, int bw, int bh, int stride);
    }
}
