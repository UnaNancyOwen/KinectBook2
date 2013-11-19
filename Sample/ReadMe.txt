書籍「Kinect for Windows SDK 実践プログラミング」サンプルプログラム ReadMe.txt

■構成
本サンプルプログラムの構成は以下のとおりです。

    Sample.zip
    │
    │  // サンプルプログラム
    ├─Sample
    │  │
    │  ├─Sample.sln
    │  │
    │  ├─Color
    │  │  ├─Color.vcxproj
    │  │  └─Color.cpp
    │  │
    │  ├─DepthAndPlayer
    │  │  ├─DepthAndPlayer.vcxproj
    │  │  └─DepthAndPlayer.cpp
    │  │
    │  ├─Skeleton
    │  │  ├─Skeleton.vcxproj
    │  │  └─Skeleton.cpp
    │  │
    │  ├─SpeechRecongnition
    │  │  ├─SpeechRecongnition.vcxproj
    │  │  ├─SpeechRecongnition.cpp
    │  │  ├─KinectAudioStream.h
    │  │  └─KinectAudioStream.cpp
    │  │
    │  ├─FaceTracking
    │  │  ├─FaceTracking.vcxproj
    │  │  └─FaceTracking.cpp
    │  │
    │  ├─KinectInteraction
    │  │  ├─KinectInteraction.vcxproj
    │  │  └─KinectInteraction.cpp
    │  │
    │  ├─BackgroundRemoval
    │  │  ├─BackgroundRemoval.vcxproj
    │  │  └─BackgroundRemoval.cpp
    │  │
    │  └─KinectFusion
    │      ├─KinectFusion.vcxproj
    │      ├─KinectFusion.cpp
    │      ├─KinectFusionHelper.h
    │  　  └─KinectFusionHelper.cpp
    │
    │  // プロパティシート
    ├─KinectBook.props
    │
    │  // ライセンス
    ├─License.txt
    │
    │  // 説明
    └─ReadMe.txt


■サンプルプログラムについて
本サンプルプログラムは以下の環境でビルドできるように設定しています。
環境によっては適宜設定を書き換えてください。

    Visual Studio Express 2012
    Kinect for Windows SDK v1.8
    Kinect for Windows Developer Toolkit v1.8.0
    Speech Platform SDK 11
    OpenCV 2.4.6


■Kinectについて
Kinect for Xbox360ではNear Modeは動作しないため、ソースコードの該当箇所をコメントアウトしてください。

    /*
    // Near Modeの設定
    hResult = pSensor->NuiImageStreamSetImageFrameFlags( hDepthPlayerHandle, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE );
    if( FAILED( hResult ) ){
        std::cerr << "Error : NuiImageStreamSetImageFrameFlags" << std::endl;
        return -1;
    }
    */

    NUI_IMAGE_DEPTH_MAXIMUM/*_NEAR_MODE*/

また、Kinectが上手く動作しない場合はKinect for Windows Developer ToolkitのKinect Explorerで簡単に状況を確認することができます。
(これらはプログラム中でHRESULT型のエラーコードからでも確認することができます。)

    例：NotPowered → 電源が通っていない可能性があります。
                      USB/電源ケーブルの接続を確認してください。

USBコントローラーとの相性が原因である場合もあります。
別のUSBコントローラーで制御されているUSBポートに接続し直すと上手く動作することがあります。


■プロパティーシートについて
付属のプロパティシートKinectBook.propsには以下の設定が記述されています。
作成したプロジェクトの設定に役に立つと思います。
なお、本サンプルプログラムは付属のプロパティーシートに依存していません。

    Kinect for Windows SDK v1.8
    Kinect for Windows Developer Toolkit v1.8.0
    Speech Platform SDK 11
    OpenCV 2.4.6


■OpenCVについて
OpenCVはバージョンによってライブラリ名が異なります。
現状は以下の命名規則でライブラリ名がつけられています。

Debug用のライブラリファイルには最後に「d」がつきます。

    Debug   ：opencv_◯◯◯△△△d.lib
    Release ：opencv_◯◯◯△△△.lib

△△△はバージョン番号です。

    例：OpenCV 2.4.6 → opencv_◯◯◯246.lib

本書では環境変数「OPENCV_VER」でバージョン番号△△△を設定しています。

◯◯◯はモジュール名です。
利用する機能にあわせてライブラリをリンクします。
よく使われるモジュールは以下のとおりです。

    core    ：基礎
    highgui ：ユーザーインターフェース
    imgproc ：画像処理

    例：core → opencv_core△△△.lib

インクルードするヘッダファイルは<opencv/opencv.hpp>です。
一部を除きほぼすべてのOpenCV関連のヘッダファイルがインクルードされます。

    #include <opencv/opencv.hpp>

OpenCV 2.4.6にはVisual Studio 2013(vc12)向けのバイナリが同梱されていません。
OpenCV 2.4.6を使用する場合はCMakeを使ってVisual Studio 2013(Visual Studio 12またはVisual Studio 12 Win64)のソリューションを作成し、ビルドしてください。

    OpenCV v2.4.6 documentation > OpenCV Tutorials > Introduction to OpenCV > Installation in Windows > Building the library
    http://docs.opencv.org/trunk/doc/tutorials/introduction/windows_install/windows_install.html#building-the-library


■環境変数について
Kinect for Windows SDKおよびKinect for Windows Developer Toolkitをインストールすると以下の環境変数が設定されます。
本サンプルプログラムの各種設定は環境変数に依存しています。
今後のバージョンアップに伴い環境変数の変数名や値が変化する場合があるので適宜設定を書き換えてください。

    インストールで自動で設定される環境変数
        変数名                  値
        KINECTSDK10_DIR         C:\Program Files\Microsoft SDKs\Kinect\v1.8\
        KINECT_TOOLKIT_DIR      C:\Program Files\Microsoft SDKs\Kinect\Developer Toolkit v1.8.0\
        FTSDK_DIR               C:\Program Files\Microsoft SDKs\Kinect\Developer Toolkit v1.8.0\

    手動で新規に作成する環境変数
        変数名          値
        OPENCV_DIR      C:\Program Files\opencv\
        OPENCV_VER      246

    手動で値を追加する環境変数(値の間は「;」で区切ります)
        変数名          値(Win32)
        Path            ;%KINECT_TOOLKIT_DIR%\Redist\x86;%OPENCV_DIR%\build\x86\vc11\bin
                        値(x64)
                        ;%KINECT_TOOLKIT_DIR%\Redist\amd64;%OPENCV_DIR%\build\x64\vc11\bin

また、環境変数で他の変数を参照する場合は%◯◯◯%と記述します。
Visual Studioで環境変数を参照する場合は$(◯◯◯)と記述します。
それぞれ変数の値が展開されて認識されます。

    例：Kinect for Windows Developer ToolkitのWin32実行ファイル(*.dll)のディレクトリを指定する場合
            環境変数      ： %KINECT_TOOLKIT_DIR%\Redist\x86 → C:\Program Files\Microsoft SDKs\Kinect\Developer Toolkit v1.8.0\Redist\x86

        Kinect for Windows SDKのインクルード ディレクトリを指定する場合
            Visual Studio ： $(KINECTSDK10_DIR)\inc → C:\Program Files\Microsoft SDKs\Kinect\v1.8\inc    

環境変数の設定を間違えるとOSが起動しなくなる可能性もあるので注意してください。
また、環境変数を変更した場合はVisual Studioを再起動して変更を反映してください。


■スタートアップ プロジェクトについて
スタートアップ プロジェクトは標準では「シングル スタートアップ プロジェクト」になっています。
実行するプロジェクトを変更するには、ソリューションのコンテキストメニューから[プロパティ]を選択、
[共有プロパティ]>[スタートアッププロジェクト]の「シングル スタートアップ プロジェクト」のドロップダウンメニューから実行したいプロジェクトを選択して設定してください。
また、[現在の選択]にチェックボックスを入れて設定すると、現在選択されているプロジェクトがスタートアップ プロジェクトになるので便利です。


■処理速度について
サンプルプログラムを動かす場合は「Release」構成でビルドしてください。
コンパイラの最適化が有効になります。


■動作確認
本サンプルプログラムは以下の環境で動作を確認しました。
本サンプルプログラムはすべての環境について動作を保証するものではありません。

    OS      ：Windows 8
    Sensor  ：Kinect for Windows
    IDE     ：Visual Studio Express 2012 for Windows Desktop
    Library ：Kinect for Windows SDK v1.8
              Kinect for Developer Toolkit v1.8.0
              Speech Platform SDK 11
              OpenCV 2.4.6


■ライセンスについて
本サンプルプログラムのライセンスはMIT Licenseとします。
ライセンス文はLicense.txtを参照してください。


■免責事項
本サンプルプログラムは使用者の責任において利用してください。
このサンプルプログラムによって発生したいかなる障害・損害などに対して、著作権者は一切責任を負わないものとします。
また、本サンプルプログラムは予告なく変更や削除する場合があります。


■変更履歴
2013/11/03 サンプルプログラムを作成しました。
