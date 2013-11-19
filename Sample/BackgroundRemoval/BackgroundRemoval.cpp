// BackgroundRemoval.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
// This source code is licensed under the MIT license. Please see the License in License.txt.
//

#include "stdafx.h"
#include <memory>
#include <Windows.h>
#include <NuiApi.h>
#include <KinectBackgroundRemoval.h>
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

	// 背景除去ストリーム
	INuiBackgroundRemovedColorStream* pBackgroundRemovedColorStream = 0;
	hResult = NuiCreateBackgroundRemovedColorStream( pSensor, &pBackgroundRemovedColorStream );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiCreateBackgroundRemovedColorStream" << std::endl;
		return -1;
	}

	HANDLE hBackgroundRemovedEvent = INVALID_HANDLE_VALUE;
	hBackgroundRemovedEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = pBackgroundRemovedColorStream->Enable( NUI_IMAGE_RESOLUTION_640x480, NUI_IMAGE_RESOLUTION_640x480, hBackgroundRemovedEvent );
	if( FAILED( hResult ) ){
		std::cerr << "Error : INuiBackgroundRemovedColorStream::Enable" << std::endl;
		return -1;
	}

	// 背景画像の読み込み
	cv::Mat backgroundMat = cv::imread( "Background.jpg" );
	if( backgroundMat.empty() ){
		std::cerr << " Error : cv::imread" << std::endl;
		return -1;
	}
	cv::cvtColor( backgroundMat, backgroundMat, CV_BGR2BGRA );

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
	std::vector<NUI_COLOR_IMAGE_POINT> colorPoint( width * height );
	std::vector<unsigned char> colorBuffer( width * height * 4 );

	cv::namedWindow( "Conventional" );
	cv::namedWindow( "BackgroundRemoved" );

	while( 1 ){
		DWORD ret;

		ret = WaitForSingleObject( hColorEvent, 0 );
		if( ret == WAIT_OBJECT_0 ){
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

			std::copy( &colorLockedRect.pBits[0], &colorLockedRect.pBits[colorLockedRect.size], colorBuffer.begin() );

			// 背景除去のカラー処理
			hResult = pBackgroundRemovedColorStream->ProcessColor( colorLockedRect.size, colorLockedRect.pBits, colorImageFrame.liTimeStamp );
			if( FAILED( hResult ) ){
				std::cerr << "Error : INuiBackgroundRemovedColorStream::ProcessColor" << std::endl;
				return -1;
			}

			pColorFrameTexture->UnlockRect( 0 );
			pSensor->NuiImageStreamReleaseFrame( hColorHandle, &colorImageFrame );
		}

		ret = WaitForSingleObject( hDepthPlayerEvent, 0 );
		if( ret == WAIT_OBJECT_0 ){
			// Depthセンサーからフレームを取得
			NUI_IMAGE_FRAME depthPlayerImageFrame = { 0 };
			hResult = pSensor->NuiImageStreamGetNextFrame( hDepthPlayerHandle, 0, &depthPlayerImageFrame );
			if( FAILED( hResult ) ){
				std::cerr << "Error : NuiImageStreamGetNextFrame( DEPTH&PLAYER )" << std::endl;
				return -1;
			}

			// Depth&Playerデータの取得
			// 従来のプレイヤーIDを利用した背景除去
			BOOL nearMode = false;
			INuiFrameTexture* pDepthPlayerFrameTexture = nullptr;
			pSensor->NuiImageFrameGetDepthImagePixelFrameTexture( hDepthPlayerHandle, &depthPlayerImageFrame, &nearMode, &pDepthPlayerFrameTexture );
			NUI_LOCKED_RECT depthPlayerLockedRect;
			pDepthPlayerFrameTexture->LockRect( 0, &depthPlayerLockedRect, nullptr, 0 );

			cv::Mat conventionalMat = cv::Mat::zeros( height, width, CV_8UC3 );
			NUI_DEPTH_IMAGE_PIXEL* pDepthPlayerPixel = reinterpret_cast<NUI_DEPTH_IMAGE_PIXEL*>( depthPlayerLockedRect.pBits );
			pCordinateMapper->MapDepthFrameToColorFrame( NUI_IMAGE_RESOLUTION_640x480, width * height, pDepthPlayerPixel, NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, width * height, &colorPoint[0] );
			for( int y = 0; y < height; y++ ){
				for( int x = 0; x < width; x++ ){
					unsigned int index = y * width + x;
					unsigned int colorIndex = index * 4;
					if( pDepthPlayerPixel[index].playerIndex != 0 ){
						if( ( 0 <= colorPoint[index].y && ( colorPoint[index].y < height ) ) && ( ( 0 <= colorPoint[index].x ) && ( colorPoint[index].x < width ) ) ){
							conventionalMat.at<cv::Vec3b>( colorPoint[index].y, colorPoint[index].x ) = cv::Vec3b( colorBuffer[colorIndex], colorBuffer[colorIndex + 1], colorBuffer[colorIndex + 2] );
						}
					}
				}
			}

			cv::imshow( "Conventional", conventionalMat );

			// 背景除去のDepth処理
			pBackgroundRemovedColorStream->ProcessDepth( depthPlayerLockedRect.size, depthPlayerLockedRect.pBits, depthPlayerImageFrame.liTimeStamp );

			pDepthPlayerFrameTexture->UnlockRect( 0 );
			pSensor->NuiImageStreamReleaseFrame( hDepthPlayerHandle, &depthPlayerImageFrame );
		}

		ret = WaitForSingleObject( hSkeletonEvent, 0 );
		if( ret == WAIT_OBJECT_0 ){
			// Skeletonフレームを取得
			NUI_SKELETON_FRAME skeletonFrame = { 0 };
			hResult = pSensor->NuiSkeletonGetNextFrame( 0, &skeletonFrame );
			if( FAILED( hResult ) ){
				std::cout << "Error : NuiSkeletonGetNextFrame" << std::endl;
				return -1;
			}

			// 背景除去のスケルトン処理
			pBackgroundRemovedColorStream->ProcessSkeleton( NUI_SKELETON_COUNT, skeletonFrame.SkeletonData, skeletonFrame.liTimeStamp );

			// 最初に検出した追跡している人のみ表示する
			DWORD trackSkeletonId = NUI_SKELETON_INVALID_TRACKING_ID;
			for( int i = 0; i < NUI_SKELETON_COUNT; ++i ){
				NUI_SKELETON_DATA skeleton = skeletonFrame.SkeletonData[i];
				if( NUI_SKELETON_TRACKED == skeleton.eTrackingState ){
					trackSkeletonId = skeleton.dwTrackingID;
					break;
				}
			}

			if( trackSkeletonId != NUI_SKELETON_INVALID_TRACKING_ID ){
				hResult = pBackgroundRemovedColorStream->SetTrackedPlayer( trackSkeletonId );
				if( FAILED( hResult ) ){
					std::cout << "Error : SetTrackedPlayer" << std::endl;
					return -1;
				}
			}
		}


		// 背景除去の処理
		ret = WaitForSingleObject( hBackgroundRemovedEvent, 0 );
		if( ret == WAIT_OBJECT_0 ){
			NUI_BACKGROUND_REMOVED_COLOR_FRAME frame = { 0 };
			hResult = pBackgroundRemovedColorStream->GetNextFrame( 0, &frame );
			if( SUCCEEDED( hResult ) ){
				cv::Mat backgroundRemovedColorMat( height, width, CV_8UC4 );

				int alpha = 255;
				for( UINT i = 0; i < frame.backgroundRemovedColorDataLength; ++i ){
					// ピクセルごとの処理の最初にアルファ値を取得する
					if( ( i % 4 ) == 0 ){
						alpha = frame.pBackgroundRemovedColorData[i + 3];
						if( alpha != 0 ){
							int a = 0;
						}
					}

					// アルファ値以外は、合成処理を行う
					if( ( i % 4 ) != 3 ){
						// 背景色と前景色(プレイヤー)を合成する
						auto back = ( 255.0f - alpha ) * backgroundMat.data[i];
						auto front = alpha * frame.pBackgroundRemovedColorData[i];
						backgroundRemovedColorMat.data[i] = static_cast<unsigned char>( ( back + front ) / 255.0f );
					}
				}

				cv::imshow( "BackgroundRemoved", backgroundRemovedColorMat );
				pBackgroundRemovedColorStream->ReleaseFrame( &frame );
			}
		}

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
