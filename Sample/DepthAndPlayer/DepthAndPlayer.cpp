// DepthAndPlayer.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
// This source code is licensed under the MIT license. Please see the License in License.txt.
//

#include "stdafx.h"
#include <Windows.h>
#include <NuiApi.h>
#include <opencv2/opencv.hpp>


int _tmain(int argc, _TCHAR* argv[])
{
	cv::setUseOptimized( true );

	// Kinectのインスタンス生成、初期化
	INuiSensor* pSensor;
	HRESULT hResult = S_OK;
	hResult = NuiCreateSensorByIndex( 0, &pSensor );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiCreateSensorByIndex" << std::endl;
		return -1;
	}

	hResult = pSensor->NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiInitialize" << std::endl;
		return -1;
	}

	// Colorストリーム
	HANDLE hColorEvent = INVALID_HANDLE_VALUE;
	HANDLE hColorHandle = INVALID_HANDLE_VALUE;
	hColorEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = pSensor->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, 0, 2, hColorEvent, &hColorHandle );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiImageStreamOpen( COLOR )" << std::endl;
		return -1;
	}

	// Depth&Playerストリーム
	HANDLE hDepthPlayerEvent = INVALID_HANDLE_VALUE;
	HANDLE hDepthPlayerHandle = INVALID_HANDLE_VALUE;
	hDepthPlayerEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = pSensor->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, NUI_IMAGE_RESOLUTION_640x480, 0, 2, hDepthPlayerEvent, &hDepthPlayerHandle );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiImageStreamOpen( DEPTH&PLAYER )" << std::endl;
		return -1;
	}

	/*
	// Near Modeの設定
	hResult = pSensor->NuiImageStreamSetImageFrameFlags( hDepthPlayerHandle, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiImageStreamSetImageFrameFlags" << std::endl;
		return -1;
	}
	*/

	// 解像度の取得
	unsigned long refWidth = 0;
	unsigned long refHeight = 0;
	NuiImageResolutionToSize( NUI_IMAGE_RESOLUTION_640x480, refWidth, refHeight );
	int width = static_cast<int>( refWidth );
	int height = static_cast<int>( refHeight );

	// 位置合わせの設定
	INuiCoordinateMapper* pCordinateMapper;
	hResult = pSensor->NuiGetCoordinateMapper( &pCordinateMapper );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiGetCoordinateMapper" << std::endl;
		return -1;
	}
	std::vector<NUI_COLOR_IMAGE_POINT> pColorPoint( width * height );

	HANDLE hEvents[2] = { hColorEvent, hDepthPlayerEvent };

	// カラーテーブル
	cv::Vec3b color[7];
	color[0] = cv::Vec3b(   0,   0,   0 );
	color[1] = cv::Vec3b( 255,   0,   0 );
	color[2] = cv::Vec3b(   0, 255,   0 );
	color[3] = cv::Vec3b(   0,   0, 255 );
	color[4] = cv::Vec3b( 255, 255,   0 );
	color[5] = cv::Vec3b( 255,   0, 255 );
	color[6] = cv::Vec3b(   0, 255, 255 );

	cv::namedWindow( "Color" );
	cv::namedWindow( "Depth" );
	cv::namedWindow( "Player" );

	while( 1 ){
		// フレームの更新待ち
		ResetEvent( hColorEvent );
		ResetEvent( hDepthPlayerEvent );
		WaitForMultipleObjects( ARRAYSIZE( hEvents ), hEvents, true, INFINITE );

		// Colorカメラからフレームを取得
		NUI_IMAGE_FRAME colorImageFrame = { 0 };
		hResult = pSensor->NuiImageStreamGetNextFrame( hColorHandle, 0, &colorImageFrame );
		if( FAILED( hResult ) ){
			std::cerr << "Error : NuiImageStreamGetNextFrame( COLOR )" << std::endl;
			return -1;
		}

		// Color画像データの取得
		INuiFrameTexture* pColorFrameTexture = colorImageFrame.pFrameTexture;
		NUI_LOCKED_RECT colorLockedRect;
		pColorFrameTexture->LockRect( 0, &colorLockedRect, nullptr, 0 );

		// Depthセンサーからフレームを取得
		NUI_IMAGE_FRAME depthPlayerImageFrame = { 0 };
		hResult = pSensor->NuiImageStreamGetNextFrame( hDepthPlayerHandle, 0, &depthPlayerImageFrame );
		if( FAILED( hResult ) ){
			std::cerr << "Error : NuiImageStreamGetNextFrame( DEPTH&PLAYER )" << std::endl;
			return -1;
		}

		// Depth&Playerデータの取得
		BOOL nearMode = false;
		INuiFrameTexture* pDepthPlayerFrameTexture = nullptr;
		pSensor->NuiImageFrameGetDepthImagePixelFrameTexture( hDepthPlayerHandle, &depthPlayerImageFrame, &nearMode, &pDepthPlayerFrameTexture );
		NUI_LOCKED_RECT depthPlayerLockedRect;
		pDepthPlayerFrameTexture->LockRect( 0, &depthPlayerLockedRect, nullptr, 0 );

		// 表示
		cv::Mat colorMat( height, width, CV_8UC4, reinterpret_cast<unsigned char*>( colorLockedRect.pBits ) );

		cv::Mat bufferMat = cv::Mat::zeros( height, width, CV_16UC1 );
		cv::Mat playerMat = cv::Mat::zeros( height, width, CV_8UC3 );
		NUI_DEPTH_IMAGE_PIXEL* pDepthPlayerPixel = reinterpret_cast<NUI_DEPTH_IMAGE_PIXEL*>( depthPlayerLockedRect.pBits );
		pCordinateMapper->MapDepthFrameToColorFrame( NUI_IMAGE_RESOLUTION_640x480, width * height, pDepthPlayerPixel, NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, width * height, &pColorPoint[0] );
		for( int y = 0; y < height; y++ ){
			for( int x = 0; x < width; x++ ){
				unsigned int index = y * width + x;
				bufferMat.at<unsigned short>( pColorPoint[index].y, pColorPoint[index].x ) = pDepthPlayerPixel[index].depth;
				playerMat.at<cv::Vec3b>( pColorPoint[index].y, pColorPoint[index].x ) = color[pDepthPlayerPixel[index].playerIndex];
			}
		}
		cv::Mat depthMat( height, width, CV_8UC1 );
		bufferMat.convertTo( depthMat, CV_8U, -255.0f / 10000.0f, 255.0f );

		cv::imshow( "Color", colorMat );
		cv::imshow( "Depth", depthMat );
		cv::imshow( "Player", playerMat );

		// フレームの解放
		pColorFrameTexture->UnlockRect( 0 );
		pDepthPlayerFrameTexture->UnlockRect( 0 );
		pSensor->NuiImageStreamReleaseFrame( hColorHandle, &colorImageFrame );
		pSensor->NuiImageStreamReleaseFrame( hDepthPlayerHandle, &depthPlayerImageFrame );

		// ループの終了判定(Escキー)
		if( cv::waitKey( 30 ) == VK_ESCAPE ){
			break;
		}
	}

	// Kinectの終了処理
	pSensor->NuiShutdown();
	pCordinateMapper->Release();
	CloseHandle( hColorEvent );
	CloseHandle( hDepthPlayerEvent );
	
	cv::destroyAllWindows();

	return 0;
}
