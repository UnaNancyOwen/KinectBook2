// KinectInteraction.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
// This source code is licensed under the MIT license. Please see the License in License.txt.
//

#include "stdafx.h"
#include <Windows.h>
#include <NuiApi.h>
#include <KinectInteraction.h>
#include <opencv2/opencv.hpp>


class KinectAdapter : public INuiInteractionClient
{
public:

	HRESULT STDMETHODCALLTYPE QueryInterface(
		REFIID riid,
		void **ppv )
	{
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef()
	{
		return 2;
	}

	ULONG STDMETHODCALLTYPE Release()
	{
		return 1;
	}

	HRESULT STDMETHODCALLTYPE GetInteractionInfoAtLocation(
		DWORD skeletonTrackingId, NUI_HAND_TYPE handType,
		FLOAT x, FLOAT y,
		_Out_ NUI_INTERACTION_INFO *pInteractionInfo )
	{
		pInteractionInfo->IsGripTarget          = false;
		pInteractionInfo->IsPressTarget         = false;
		pInteractionInfo->PressTargetControlId  = 0;
		pInteractionInfo->PressAttractionPointX = 0.0f;
		pInteractionInfo->PressAttractionPointY = 0.0f;
		return S_OK;
	}
};

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

	hResult = pSensor->NuiInitialize( NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_SKELETON );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiInitialize" << std::endl;
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

	// インタラクションストリーム
	INuiInteractionStream* pInteractionStream;
	KinectAdapter adapter;
	NuiCreateInteractionStream( pSensor, &adapter, &pInteractionStream );

	HANDLE hInteractionEvent = INVALID_HANDLE_VALUE;
	hInteractionEvent = CreateEvent( nullptr, true, false, nullptr );
	pInteractionStream->Enable( hInteractionEvent );

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

	// カラーテーブル
	cv::Vec3b color[7];
	color[0] = cv::Vec3b(   0,   0,   0 );
	color[1] = cv::Vec3b( 255,   0,   0 );
	color[2] = cv::Vec3b(   0, 255,   0 );
	color[3] = cv::Vec3b(   0,   0, 255 );
	color[4] = cv::Vec3b( 255, 255,   0 );
	color[5] = cv::Vec3b( 255,   0, 255 );
	color[6] = cv::Vec3b(   0, 255, 255 );

	cv::namedWindow( "Depth" );
	cv::namedWindow( "Player" );
	cv::namedWindow( "Skeleton" );
	cv::namedWindow( "Interaction" );

	while( 1 ){
		// フレームの更新待ち
		DWORD ret;

		// Depth&Playerストリームの処理
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
			BOOL nearMode = false;
			INuiFrameTexture* pDepthPlayerFrameTexture = nullptr;
			pSensor->NuiImageFrameGetDepthImagePixelFrameTexture( hDepthPlayerHandle, &depthPlayerImageFrame, &nearMode, &pDepthPlayerFrameTexture );
			NUI_LOCKED_RECT depthPlayerLockedRect;
			pDepthPlayerFrameTexture->LockRect( 0, &depthPlayerLockedRect, nullptr, 0 );

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
			bufferMat.convertTo( depthMat, CV_8U, -255.0f / NUI_IMAGE_DEPTH_MAXIMUM, 255.0f );

			cv::imshow( "Depth", depthMat );
			cv::imshow( "Player", playerMat );

			// インタラクションのDepth処理
			pInteractionStream->ProcessDepth( depthPlayerLockedRect.size, depthPlayerLockedRect.pBits, depthPlayerImageFrame.liTimeStamp );

			pDepthPlayerFrameTexture->UnlockRect( 0 );
			pSensor->NuiImageStreamReleaseFrame( hDepthPlayerHandle, &depthPlayerImageFrame );
		}

		// Skeletonストリームの処理
		ret = WaitForSingleObject( hSkeletonEvent, 0 );
		if( ret == WAIT_OBJECT_0 ){
			// Skeletonフレームを取得
			NUI_SKELETON_FRAME skeletonFrame = { 0 };
			hResult = pSensor->NuiSkeletonGetNextFrame( 0, &skeletonFrame );
			if( FAILED( hResult ) ){
				std::cout << "Error : NuiSkeletonGetNextFrame" << std::endl;
				return -1;
			}

			cv::Mat skeletonMat = cv::Mat::zeros( height, width, CV_8UC3 );
			NUI_COLOR_IMAGE_POINT colorPoint;
			for( int count = 0; count < NUI_SKELETON_COUNT; count++ ){
				NUI_SKELETON_DATA skeletonData = skeletonFrame.SkeletonData[count];
				if( skeletonData.eTrackingState == NUI_SKELETON_TRACKED ){
					for( int position = 0; position < NUI_SKELETON_POSITION_COUNT; position++ ){
						pCordinateMapper->MapSkeletonPointToColorPoint( &skeletonData.SkeletonPositions[position], NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, &colorPoint );
						cv::circle( skeletonMat, cv::Point( colorPoint.x, colorPoint.y ), 10, static_cast<cv::Scalar>( color[count + 1] ), -1, CV_AA );
					}
				}
			}

			cv::imshow( "Skeleton", skeletonMat );

			// インタラクションのスケルトン処理
			Vector4 reading = { 0 };
			pSensor->NuiAccelerometerGetCurrentReading( &reading );
			pInteractionStream->ProcessSkeleton( NUI_SKELETON_COUNT, skeletonFrame.SkeletonData, &reading, skeletonFrame.liTimeStamp );
		}

		// Interactionストリームの処理
		ret = WaitForSingleObject( hInteractionEvent, 0 );
		if( ret == WAIT_OBJECT_0 ){
			// インタラクションフレームを取得する
			NUI_INTERACTION_FRAME interactionFrame = { 0 } ;
			hResult = pInteractionStream->GetNextFrame( 0, &interactionFrame );
			if( FAILED( hResult ) ){
				std::cout << "Error : GetNextFrame" << std::endl;
				return -1;
			}

			cv::Mat interactionMat = cv::Mat::zeros( height, width, CV_8UC3 );

			for( int userCount = 0; userCount < NUI_SKELETON_COUNT; userCount++ ){
				NUI_USER_INFO userInfo = interactionFrame.UserInfos[userCount];
				if( userInfo.SkeletonTrackingId == 0 ){
					continue;
				}

				for( int handCount = 0; handCount < NUI_USER_HANDPOINTER_COUNT; handCount++ ){
					NUI_HANDPOINTER_INFO handInfo = userInfo.HandPointerInfos[handCount];
					if( ( 0.0f <= handInfo.X ) && ( handInfo.X < 1.0f ) && ( 0.0f <= handInfo.Y ) && ( handInfo.Y < 1.0f ) ){
						cv::circle( interactionMat, cv::Point( static_cast<int>( width * handInfo.X ), static_cast<int>( height * handInfo.Y ) ), 5, cv::Scalar( 255, 255, 255 ), -1 );
					}

					// 手の状態を表示する
					if( handInfo.HandType == NUI_HAND_TYPE_RIGHT ){
						std::cout << "Right ";
					}
					else if( handInfo.HandType == NUI_HAND_TYPE_RIGHT ){
						std::cout << "Left  ";
					}

					if( handInfo.State & NUI_HANDPOINTER_STATE_TRACKED ){
						//std::cout << "TRACKED" << std::endl;
					}

					if( handInfo.State & NUI_HANDPOINTER_STATE_ACTIVE ){
						std::cout << "ACTIVE ";
					}

					if( handInfo.State & NUI_HANDPOINTER_STATE_INTERACTIVE ){
						std::cout << "INTERACTIVE ";
					}

					if( handInfo.State & NUI_HANDPOINTER_STATE_PRESSED ){
						std::cout << "PRESSED ";
					}

					if( handInfo.State & NUI_HANDPOINTER_STATE_PRIMARY_FOR_USER ){
						std::cout << "PRIMARY_FOR_USER ";
					}

					std::cout << std::endl;

					// 手のイベントを表示する
					if( handInfo.HandEventType == NUI_HAND_EVENT_TYPE_GRIP ){
						std::cout << "Grip" << std::endl;
					}
					else if( handInfo.HandEventType == NUI_HAND_EVENT_TYPE_GRIPRELEASE ){
						std::cout << "GripRelease" << std::endl;
					}
				}
			}

			cv::imshow( "Interaction", interactionMat );
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
	CloseHandle( hDepthPlayerEvent );
	CloseHandle( hSkeletonEvent );

	cv::destroyAllWindows();

	return 0;
}
