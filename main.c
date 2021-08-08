"パッケージのインストール"
#include "myJpeg.h"
#include <time.h>
#include <pthread.h>

"使用するスレッド数"
#define NUM_THREAD 4

"スレッドで参照するデータ"
struct thread_data{
  BITMAPDATA_t *output;
  BITMAPDATA_t *input;
  double scaleW;
  double scaleH;
  int startm;
  int endm;
  int startn;
  int endn;
};

void *resize(void *arg);

"スレッドの処理"
void *resize(void *arg){
  int m, n, c;
  int m0, m1, n0, n1;
  double originalm, originaln;
  double dm, dn;
  struct thread_data *thdata = (struct thread_data*)arg;

  /* このスレッドの処理する範囲に対して線形補間で画像拡大 */
  for(n = thdata->startn; n < thdata->endn; n++){
    for(m = thdata->startm; m < thdata->endm; m++){
      for(c = 0; c < thdata->input->ch; c++){

        /* 入力画像における横方向座標を線形補間で算出 */
        originalm = (double)m / (double)thdata->scaleW;
        m0 = (int)originalm;
        dm = originalm - m0;
        m1 = m0 + 1;
        if(m1 == thdata->input->width) m1 = thdata->input->width - 1;

        /* 入力画像における縦方向座標を線形補間で算出 */
        originaln = (double)n / (double)thdata->scaleH;
        n0 = (int)originaln;
        dn = originaln - n0;
        n1 = n0 + 1;
        if(n1 == thdata->input->height) n1 = thdata->input->height - 1;

        /* 線形補間で算出した座標のRGB値を拡大後画像用のメモリに格納 */
        thdata->output->data[thdata->output->ch * (m + n * thdata->output->width) + c]
          = thdata->input->data[thdata->input->ch * (m1 + n1 * thdata->input->width) + c] * dm * dn
          + thdata->input->data[thdata->input->ch * (m1 + n0 * thdata->input->width) + c] * dm * (1 - dn)
          + thdata->input->data[thdata->input->ch * (m0 + n1 * thdata->input->width) + c] * (1- dm) * dn
          + thdata->input->data[thdata->input->ch * (m0 + n0 * thdata->input->width) + c] * (1 -dm) * (1 - dn);
      }
    }
  }
  return NULL;
}

int main(int argc, char *argv[]){
  BITMAPDATA_t bitmap, scaledBitmap;
  double scaleW, scaleH;
  char outname[256];
  int n;
  pthread_t thread[NUM_THREAD];
  struct thread_data thdata[NUM_THREAD];
  time_t start, end;

  if(argc != 4){
    printf("ファイル名、幅方向拡大率、高さ方向拡大率の３つを引数に指定してください\n");
    return -1;
  }

  scaleW = atof(argv[2]);
  scaleH = atof(argv[3]);

  if(jpegFileReadDecode(&bitmap, argv[1]) == -1){
    printf("jpegFileReadDecode error\n");
    return -1;
  }

  scaledBitmap.width = scaleW * bitmap.width;
  scaledBitmap.height = scaleH * bitmap.height;
  scaledBitmap.ch = bitmap.ch;
  if(scaledBitmap.width == 0 || scaledBitmap.height == 0){
    printf("拡大縮小後の幅もしくは高さが０です\n");
    freeBitmapData(&bitmap);
    return -1;
  }

  "拡大後画像のBITMAPデータ格納用のメモリ確保"
  scaledBitmap.data = (unsigned char*)malloc(sizeof(unsigned char) * scaledBitmap.width * scaledBitmap.height * scaledBitmap.ch);
  if(scaledBitmap.data == NULL){
    printf("malloc scaledBitmap error\n");
    freeBitmapData(&bitmap);
    return -1;
  }

  "スレッドで参照するためのデータを格納"
  for(n = 0; n < NUM_THREAD; n++){
    thdata[n].output = &scaledBitmap;
    thdata[n].input = &bitmap;
    thdata[n].scaleW = scaleW;
    thdata[n].scaleH = scaleH;
    thdata[n].startm = 0;
    thdata[n].endm = scaledBitmap.width;
    thdata[n].startn = n * scaledBitmap.height / NUM_THREAD;
    thdata[n].endn = (n+1) * scaledBitmap.height / NUM_THREAD;
  }
  
  thdata[NUM_THREAD-1].endn = scaledBitmap.height;

  start = time(NULL);

  for(n = 0; n < NUM_THREAD; n++){
    pthread_create(&thread[n], NULL, resize, &thdata[n]);
  }

  "同期"
  for(n = 0; n < NUM_THREAD; n++){
    pthread_join(thread[n], NULL);
  }

  "処理時間の計測"
  end = time(NULL);
  printf("processing time:%ld[s]\n", end - start);

  "画像の出力"
  sprintf(outname, "%s", "result.jpeg");
  if(jpegFileEncodeWrite(&scaledBitmap, outname) == -1){
    printf("jpegFileEncodeWrite error\n");
    freeBitmapData(&scaledBitmap);
    freeBitmapData(&bitmap);
    return -1;
  }

  freeBitmapData(&scaledBitmap);
  freeBitmapData(&bitmap);

  return 0;
}