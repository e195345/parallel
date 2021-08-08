環境構築
     以下のサイトよりjpegsrc.v9c.tar.gzをダウンロードし, libjpegをインストールする
     http://www.ijg.org/files/


実行方法
    コンパイル
        $ gcc myJpeg.c -c
        $ gcc main.c -c
        $ gcc myJpeg.o main.o -ljpeg -lpthread -o main.exe  
    実行
        $ ./main.exe sample.jpg 8 8
            第１引数：入力する JPEG ファイルへのパス
            第２引数：横方向の拡大率
            第３引数：縦方向の拡大率


リポジトリの内容
    sample.jpg
        今回使用した画像

    main.c
        マルチスレッドプログラミングを用いて作成したコード

    myJpeg.h
        libjpeg が提供する関数の宣言や定数の定義等を行なっているコード

    myJpeg.c
        libjpeg を使用するための関数

    myJpeg.h, myJpeg.cは以下サイトよりそのまま引用した
    https://daeudaeu.com/libjpeg/