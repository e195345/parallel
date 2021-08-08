#include "myJpeg.h"
#include <time.h>
#include <pthread.h>

/* スレッドの数 */
#define NUM_THREAD 4

/* スレッドで参照するデータ */
struct thread_data{
  /* 拡大後画像の情報および拡大後BITMAPデータ格納用 */
  BITMAPDATA_t *output;
  /* 入力画像の情報 */
  BITMAPDATA_t *input;
  /* 拡大率 */
  double scaleW;
  double scaleH;
  /* このスレッドで処理する範囲 */
  int startm;
  int endm;
  int startn;
  int endn;
};

void *resize(void *arg);

/* スレッドで実行する関数 */
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
  /* 画像のBITMAPデータ情報 */
  BITMAPDATA_t bitmap, scaledBitmap;
  /* 拡大率 */
  double scaleW, scaleH;
  /* 出力ファイル名 */
  char outname[256];
  /* スレッドのインデックス */
  int n;
  /* スレッド */
  pthread_t thread[NUM_THREAD];
  /* スレッドで参照するデータ */
  struct thread_data thdata[NUM_THREAD];
  /* 処理時間計測用 */
  time_t start, end;

  /* 引数チェック */
  if(argc != 4){
    printf("ファイル名、幅方向拡大率、高さ方向拡大率の３つを引数に指定してください\n");
    return -1;
  }

  /* 引数から拡大率を取得 */
  scaleW = atof(argv[2]);
  scaleH = atof(argv[3]);

  /* 入力jpegファイル読み込んで情報bitmapへ格納 */
  if(jpegFileReadDecode(&bitmap, argv[1]) == -1){
    printf("jpegFileReadDecode error\n");
    return -1;
  }

  /* 入力画像の情報と拡大率から拡大後の画像の情報を格納 */
  scaledBitmap.width = scaleW * bitmap.width;
  scaledBitmap.height = scaleH * bitmap.height;
  scaledBitmap.ch = bitmap.ch;

  /* パラメータチェック */
  if(scaledBitmap.width == 0 || scaledBitmap.height == 0){
    printf("拡大縮小後の幅もしくは高さが０です\n");
    freeBitmapData(&bitmap);
    return -1;
  }

  /* 拡大後画像のBITMAPデータ格納用のメモリ確保 */
  scaledBitmap.data = (unsigned char*)malloc(sizeof(unsigned char) * scaledBitmap.width * scaledBitmap.height * scaledBitmap.ch);
  if(scaledBitmap.data == NULL){
    printf("malloc scaledBitmap error\n");
    freeBitmapData(&bitmap);
    return -1;
  }

  /* スレッドで参照するためのデータを格納 */
  for(n = 0; n < NUM_THREAD; n++){
    /* 出力画像の情報および拡大後画像のBITMAPデータ格納用 */
    thdata[n].output = &scaledBitmap;
    /* 入力画像の情報 */
    thdata[n].input = &bitmap;
    /* 横方向拡大率 */
    thdata[n].scaleW = scaleW;
    /* 縦方向拡大率 */
    thdata[n].scaleH = scaleH;
    /* このスレッドで処理する横方向の開始点 */
    thdata[n].startm = 0;
    /* このスレッドで処理する横方向の終了点 */
    thdata[n].endm = scaledBitmap.width;
    /* このスレッドで処理する横縦方向の開始点 */
    thdata[n].startn = n * scaledBitmap.height / NUM_THREAD;
    /* このスレッドで処理する縦方向の終了点 */
    thdata[n].endn = (n+1) * scaledBitmap.height / NUM_THREAD;
  }
  /* 画像の高さがNUM_THRADの倍数でない場合の微調整 */
  thdata[NUM_THREAD-1].endn = scaledBitmap.height;

  start = time(NULL);

  /* NUM_THREAD個ののスレッドを生成 */
  /* スレッドで実行するのresize関数 */
  for(n = 0; n < NUM_THREAD; n++){
    pthread_create(&thread[n], NULL, resize, &thdata[n]);
  }

  /* NUM_THREAD個のスレッドの終了まで待つ */
  for(n = 0; n < NUM_THREAD; n++){
    pthread_join(thread[n], NULL);
  }

  end = time(NULL);
  printf("processing time:%ld[s]\n", end - start);

  /* 拡大後画像の出力ファイル名を設定 */
  sprintf(outname, "%s", "linear.jpeg");

  /* 拡大後画像をJPEG形式でファイル出力 */
  if(jpegFileEncodeWrite(&scaledBitmap, outname) == -1){
    printf("jpegFileEncodeWrite error\n");
    freeBitmapData(&scaledBitmap);
    freeBitmapData(&bitmap);
    return -1;
  }

  /* 確保したメモリを解放 */
  freeBitmapData(&scaledBitmap);
  freeBitmapData(&bitmap);

  return 0;
}