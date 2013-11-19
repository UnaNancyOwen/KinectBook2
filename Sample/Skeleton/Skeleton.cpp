// Skeleton.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
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

	hResult = pSensor->NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_SKELETON );
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

	// Skeletonストリーム
	HANDLE hSkeletonEvent = INVALID_HANDLE_VALUE;
	hSkeletonEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = pSensor->NuiSkeletonTrackingEnable( hSkeletonEvent, 0 );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiSkeletonTrackingEnable" << std::endl;
		return -1;
	}

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

	HANDLE hEvents[3] = { hColorEvent, hDepthPlayerEvent, hSkeletonEvent };

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
	cv::namedWindow( "Skeleton" );

	while( 1 ){
		// フレームの更新待ち
		ResetEvent( hColorEvent );
		ResetEvent( hDepthPlayerEvent );
		ResetEvent( hSkeletonEvent );
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

		// Skeletonフレームを取得
		NUI_SKELETON_FRAME skeletonFrame = { 0 };
		hResult = pSensor->NuiSkeletonGetNextFrame( 0, &skeletonFrame );
		if( FAILED( hResult ) ){
			std::cout << "Error : NuiSkeletonGetNextFrame" << std::endl;
			return -1;
		}

		/*
		// スムージング
		NUI_TRANSFORM_SMOOTH_PARAMETERS smoothParameter;
		smoothParameter.fSmoothing = 0.5; // 平滑化[0.0f-1.0f]
		smoothParameter.fCorrection = 0.5; // 補正量[0.0f-1.0f]
		smoothParameter.fPrediction = 0.0f; // 予測フレーム数[0.0f-]
		smoothParameter.fJitterRadius = 0.05f; // ジッタ抑制半径[0.0f-]
		smoothParameter.fMaxDeviationRadius = 0.04f; // 最大抑制範囲[0.0f-]

		hResult = NuiTransformSmooth( &skeletonFrame, &smoothParameter );
		*/

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

		cv::Mat skeletonMat = cv::Mat::zeros( height, width, CV_8UC3 );
		NUI_COLOR_IMAGE_POINT colorPoint;
		for( int count = 0; count < NUI_SKELETON_COUNT; count++ ){
			NUI_SKELETON_DATA skeletonData = skeletonFrame.SkeletonData[count];
			if( skeletonData.eTrackingState == NUI_SKELETON_TRACKED ){
				for( int position = 0; position < NUI_SKELETON_POSITION_COUNT; position++ ){
					pCordinateMapper->MapSkeletonPointToColorPoint( &skeletonData.SkeletonPositions[position], NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, &colorPoint );
					if( ( colorPoint.x >= 0 ) && ( colorPoint.x < width ) && ( colorPoint.y >= 0 ) && ( colorPoint.y < height ) ){
						cv::circle( skeletonMat, cv::Point( colorPoint.x, colorPoint.y ), 10, static_cast<cv::Scalar>( color[count + 1] ), -1, CV_AA );
					}
				}

				std::stringstream ss;
				ss << skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HIP_CENTER].z;
				pCordinateMapper->MapSkeletonPointToColorPoint( &skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HEAD], NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, &colorPoint );
				if( ( colorPoint.x >= 0 ) && ( colorPoint.x < width ) && ( colorPoint.y >= 0 ) && ( colorPoint.y < height ) ){
					cv::putText( skeletonMat, ss.str(), cv::Point( colorPoint.x - 50, colorPoint.y - 20 ), cv::FONT_HERSHEY_SIMPLEX, 1.5f, static_cast<cv::Scalar>( color[count + 1] ) );
				}
			}
			else if( skeletonData.eTrackingState == NUI_SKELETON_POSITION_ONLY ){
				pCordinateMapper->MapSkeletonPointToColorPoint( &skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HIP_CENTER], NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, &colorPoint );
				if( ( colorPoint.x >= 0 ) && ( colorPoint.x < width ) && ( colorPoint.y >= 0 ) && ( colorPoint.y < height ) ){
						cv::circle( skeletonMat, cv::Point( colorPoint.x, colorPoint.y ), 10, static_cast<cv::Scalar>( color[count + 1] ), -1, CV_AA );
				}
			}
		}

		cv::imshow( "Color", colorMat );
		cv::imshow( "Depth", depthMat );
		cv::imshow( "Player", playerMat );
		cv::imshow( "Skeleton", skeletonMat );

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
	pSensor->NuiSkeletonTrackingDisable();
	pCordinateMapper->Release();
	CloseHandle( hColorEvent );
	CloseHandle( hDepthPlayerEvent );
	CloseHandle( hSkeletonEvent );

	cv::destroyAllWindows();

	return 0;
}
