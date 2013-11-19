// Color.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
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

	hResult = pSensor->NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiInitialize" << std::endl;
		return -1;
	}

	// Colorストリームの開始
	HANDLE hColorEvent = INVALID_HANDLE_VALUE;
	HANDLE hColorHandle = INVALID_HANDLE_VALUE;
	hColorEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = pSensor->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, 0, 2, hColorEvent, &hColorHandle );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiImageStreamOpen( COLOR )" << std::endl;
		return -1;
	}

	// Colorカメラの設定
	INuiColorCameraSettings* pCameraSettings;
	hResult = pSensor->NuiGetColorCameraSettings( &pCameraSettings );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiGetColorCameraSettings" << std::endl;
		return -1;
	}
	double contrast = 1.0f;
	pCameraSettings->GetContrast( &contrast );
	std::cout << "Contrast : " << contrast << std::endl;

	// 解像度の取得
	unsigned long refWidth = 0;
	unsigned long refHeight = 0;
	NuiImageResolutionToSize( NUI_IMAGE_RESOLUTION_640x480, refWidth, refHeight );
	int width = static_cast<int>( refWidth );
	int height = static_cast<int>( refHeight );

	HANDLE hEvents[1] = { hColorEvent };

	cv::namedWindow( "Color" );

	while( 1 ){
		// フレームの更新待ち
		ResetEvent( hColorEvent );
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

		// 表示
		cv::Mat colorMat( height, width, CV_8UC4, reinterpret_cast<unsigned char*>( colorLockedRect.pBits ) );
		cv::imshow( "Color", colorMat );
		
		// フレームの解放
		pColorFrameTexture->UnlockRect( 0 );
		pSensor->NuiImageStreamReleaseFrame( hColorHandle, &colorImageFrame );

		// ループの終了判定(Escキー)
		int key = cv::waitKey( 30 );
		if( key == VK_ESCAPE ){
			break;
		}
		// Colorカメラの設定(↑キー / ↓キー)
		else if( ( key >> 16 ) == VK_UP ){
			pCameraSettings->SetContrast( ( contrast < 2.0f ) ? contrast += 0.1f : contrast );
			std::cout << "Contrast : " << contrast << std::endl;
		}
		else if( ( key >> 16 ) == VK_DOWN ){
			pCameraSettings->SetContrast( ( contrast > 0.5f ) ? contrast -= 0.1f : contrast );
			std::cout << "Contrast : " << contrast << std::endl;
		}

	}
	
	// Kinectの終了処理
	pSensor->NuiShutdown();
	pCameraSettings->Release();
	CloseHandle( hColorEvent );

	cv::destroyAllWindows();

	return 0;
}
