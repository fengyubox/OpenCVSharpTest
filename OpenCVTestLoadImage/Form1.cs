﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using OpenCvSharp;
using OpenCvSharp.Extensions;
using System.Runtime.InteropServices;
using System.Windows.Forms.DataVisualization.Charting;
using System.Drawing;

namespace OpenCVTestLoadImage {
   public partial class Form1 : Form {
      public Form1() {
         InitializeComponent();
      }

      VideoCapture cap;

      private void btnLoad_Click(object sender, EventArgs e) {
         if (this.dlgOpen.ShowDialog() != DialogResult.OK)
            return;

         var matSrc = new Mat(this.dlgOpen.FileName);
         this.ProcessImage(matSrc);
         matSrc.Dispose();
      }

      private void btnClipboard_Click(object sender, EventArgs e) {
         Image img = Clipboard.GetImage();
         if (img == null)
            return;

         Bitmap bmp = new Bitmap(img);
         var matSrc = bmp.ToMat();
         this.ProcessImage(matSrc);
         matSrc.Dispose();
         bmp.Dispose();
      }

      private void btnLive_Click(object sender, EventArgs e) {
         if (this.cap == null) {
            this.cap = new VideoCapture(0);
            this.timer1.Enabled = true;
            this.btnLive.Text = "Live Stop";
         } else {
            this.cap.Dispose();
            this.cap = null;
            this.timer1.Enabled = false;
            this.btnLive.Text = "Live";
         }
      }

      private void timer1_Tick(object sender, EventArgs e) {
         var matSrc = new Mat();
         this.cap.Read(matSrc);
         this.ProcessImage(matSrc);
         matSrc.Dispose();
      }

      // 이미지 처리
      private void ProcessImage(Mat matSrc) {
         DrawMat(matSrc, this.pbxSrc);
         var histR = GetHistogram(matSrc, 2);
         var histG = GetHistogram(matSrc, 1);
         var histB = GetHistogram(matSrc, 0);
         DrawHistogram(histR, this.chtSrc.Series[0], "R", Color.Red  );
         DrawHistogram(histG, this.chtSrc.Series[1], "G", Color.Green);
         DrawHistogram(histB, this.chtSrc.Series[2], "B", Color.Blue );
         using (var matGray = matSrc.CvtColor(ColorConversionCodes.BGR2GRAY)) {
            DrawMat(matGray, this.pbxDst);
            var histo = GetHistogram(matGray, 0);
            float acc = 0;
            var histoAccum = histo.Select(val => acc += val).ToArray();
            DrawHistogram(histo, this.chtDst.Series[0], "GRAY", Color.Black);
            DrawHistogram(histoAccum, this.chtDst.Series[1], "Accum", Color.Red, AxisType.Secondary);
         }
      }

      public static void DrawHistogram(float[] histo, Series series, string name, Color color, AxisType yAxisTYpe = AxisType.Primary) {
         series.Name = name;
         series.Color = color;
         series.Points.Clear();
         series.YAxisType = yAxisTYpe;
         for (int i = 0; i < histo.Length; i++) {
            series.Points.AddXY(i, histo[i]);
         }
         series.Enabled = true;
      }

      public static float[] GetHistogram(Mat matSrc, int channel) {
         var images = new Mat[] { matSrc };
         var channels = new int[] { channel };
         InputArray mask = null;
         Mat hist = new Mat();
         int dims = 1;
         int width = matSrc.Cols, height = matSrc.Rows;
         const int histogramSize = 256;
         int[] histSize = { histogramSize };
         Rangef[] ranges = { new Rangef(0, histogramSize) };

         Cv2.CalcHist(images, channels, mask, hist, dims, histSize, ranges);
         var histo = new float[256];
         Marshal.Copy(hist.Data, histo, 0, 256);
         return histo;
      }

      private static void DrawMat(Mat mat, PictureBox pbx) {
         var bmpOld = pbx.Image;
         var bmpSrc = mat.ToBitmap();
         pbx.Image = bmpSrc;
         if (bmpOld != null)
            bmpOld.Dispose();
      }
   }
}
