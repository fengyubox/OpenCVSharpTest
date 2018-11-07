﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Drawing;
using System.Runtime.InteropServices;

namespace OpenCVSharpTest {
    unsafe class MyBlobs {
        private static int GetNeighborLabels(int[] labels, int bw, int bh, int x, int y, int[] nbrs) {
            int nbrCount = 0;
            int label;
            if (x != 0) {
                // check left
                label = labels[bw*y+x-1];
                if (label != 0) {
                    nbrs[nbrCount++] = label;
                }
            }
            if (y != 0) {
                // check top
                label = labels[bw*(y-1)+x];
                if (label != 0) {
                    nbrs[nbrCount++] = label;
                }
                if (x != 0) {
                    // check lt
                    label = labels[bw*(y-1)+x-1];
                    if (label != 0) {
                        nbrs[nbrCount++] = label;
                    }
                }
                if (x != bw - 1) {
                    // check rt
                    label = labels[bw*(y-1)+x+1];
                    if (label != 0) {
                        nbrs[nbrCount++] = label;
                    }
                }
            }

            return nbrCount;
        }

        private static int GetRootLabel(List<int> links, int label) {
            int root = label;
            while (links[root] != 0) {
                root = links[root];
            }

            return root;
        }

        private static int GetMinRootLabel(List<int> links, int[] nbrs, int nbrCount) {
            int minLabel = GetRootLabel(links, nbrs[0]);
            for (int i=0; i<nbrCount; i++) {
                var rootLabel = GetRootLabel(links, nbrs[i]);
                if (rootLabel < minLabel) {
                    minLabel = rootLabel;
                }
            }
            return minLabel;
        }

        private static void DrawLabels(int[] labels, IntPtr draw, int bw, int bh, int stride) {
            for (int y = 0; y < bh; y++) {
                for (int x = 0; x < bw; x++) {
                    int label = labels[bw*y+x];
                    int color = (label);
                    Marshal.WriteByte(draw+stride*y+x, (byte)color); 
                }
            }
        }

        public static MyBlob[] Label(IntPtr src, int bw, int bh, int stride) {
            byte *psrc = (byte *)src.ToPointer();
            
            // label 버퍼
            int[] labels = Enumerable.Repeat(0, bw*bh).ToArray();

            // link 테이블
            var links = new List<int>();
            links.Add(0);
            
            // 1st stage
            // labeling with scan
            int[] nbrs = new int[4];
            for (int y = 0; y < bh; y++) {
                for (int x = 0; x < bw; x++) {
                    // 배경이면 skip
                    if (psrc[stride*y+x] == 0)
                        continue;
                    
                    // 주변 4개의 label버퍼 조사 (l, tl, t, tr)
                    int nbrCount = GetNeighborLabels(labels, bw, bh, x, y, nbrs);
                    if (nbrCount == 0) {
                        // 주변에 없다면 새번호 생성하고 라벨링
                        int newLabel = links.Count;
                        labels[bw*y+x] = newLabel;
                        // link 테이블에 새 루트 라벨 추가
                        links.Add(0);
                    } else {
                        // 주변에 있다면 주변 라벨들의 루트중 최소라벨
                        int minLabel = GetMinRootLabel(links, nbrs, nbrCount);
                        labels[bw*y+x] = minLabel;
                        // link 테이블에서 주변 라벨의 parent 수정
                        for (int i=0; i<nbrCount; i++) {
                            int label = nbrs[i];
                            // 라벨이 min라벨이 아니라면 라벨의 link를 minlabel로 바꿈
                            // 이전 link가 0이 아니라면 이전 link의 link도 minlabel로 바꿈
                            while (label != minLabel) {
                                var oldLink = links[label];
                                links[label] = minLabel;
                                if (oldLink == 0)
                                    break;
                                label = oldLink;
                            }
                        }
                    }
                }
            }

            // 2nd stage
            // links 수정
            for (int i=0; i<links.Count; i++) {
                if (links[i] == 0)
                    continue;
                links[i] = GetRootLabel(links, links[i]);
            }

            // labels 수정
            for (int y = 0; y < bh; y++) {
                for (int x = 0; x < bw; x++) {
                    int label = labels[bw*y+x];
                    if (label == 0)
                        continue;
                    var link = links[label];
                    if (link == 0)
                        continue;
                    labels[bw*y+x] = link;
                }
            }

            // 3. 후처리
            // link index 수정
            Dictionary<int, int> relabelTable = new Dictionary<int, int>();
            int newIndex = 1;
            for (int i=1; i<links.Count; i++) {
                var link = links[i];
                if (link == 0) {
                    relabelTable.Add(i, newIndex++);
                }
            }
            // labels 수정
            for (int y = 0; y < bh; y++) {
                for (int x = 0; x < bw; x++) {
                    var label = labels[bw*y+x];
                    if (label == 0)
                        continue;
                    labels[bw*y+x] = relabelTable[label];
                }
            }

            // 4. 데이터 추출 (-1 : 0라벨을 제외 이므로)
            MyBlob[] blobs = new MyBlob[relabelTable.Count];
            List<Point>[] pixels = new List<Point>[relabelTable.Count];
            for (int i=0; i<blobs.Length; i++) {
                blobs[i] = new MyBlob();
                pixels[i] = new List<Point>();
            }

            for (int y = 0; y < bh; y++) {
                for (int x = 0; x < bw; x++) {
                    var label = labels[bw*y+x];
                    if (label == 0)
                        continue;
                    int idx = label-1;
                    pixels[idx].Add(new Point(x, y));
                    var blob = blobs[idx];
                    blob.centroid.X += x;
                    blob.centroid.Y += y;
                    if (x < blob.minX) blob.minX = x;
                    if (y < blob.minY) blob.minY = y;
                    if (x > blob.maxX) blob.maxX = x;
                    if (y > blob.maxY) blob.maxY = y;
                }
            }

            for (int i=0; i<blobs.Length; i++) {
                var blob = blobs[i];
                blob.pixels = pixels[i].ToArray();
                blob.centroid.X /= blob.pixels.Length;
                blob.centroid.Y /= blob.pixels.Length;
            }

            return blobs;
        }
    }

    class MyBlob {
        public Point[] pixels;
        public Point centroid = new Point(0,0);
        public int minX = int.MaxValue-1;
        public int minY = int.MaxValue-1;
        public int maxX = -1;
        public int maxY = -1;
    }
}