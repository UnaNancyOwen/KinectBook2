// FaceTracking.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
// This source code is licensed under the MIT license. Please see the License in License.txt.
//

#include "stdafx.h"
#include <Windows.h>
#include <NuiApi.h>
#include <FaceTrackLib.h>
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
		std::cerr << "Error : NuiImageStreamOpen(camera)" << std::endl;
		return -1;
	}

	// Depth&Playerストリーム(Near Mode)
	HANDLE hDepthPlayerEvent = INVALID_HANDLE_VALUE;
	HANDLE hDepthPlayerHandle = INVALID_HANDLE_VALUE;
	hDepthPlayerEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = pSensor->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, NUI_IMAGE_RESOLUTION_640x480, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE, 2, hDepthPlayerEvent, &hDepthPlayerHandle );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiImageStreamOpen( DEPTH&PLAYER )" << std::endl;
		return -1;
	}

	// Skeletonストリーム
	HANDLE hSkeletonEvent = INVALID_HANDLE_VALUE;
	hSkeletonEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = pSensor->NuiSkeletonTrackingEnable( hSkeletonEvent, NUI_SKELETON_TRACKING_FLAG_SUPPRESS_NO_FRAME_DATA | NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT | NUI_SKELETON_TRACKING_FLAG_ENABLE_IN_NEAR_RANGE );
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

	// Face Tracking初期化
	IFTFaceTracker* pFT = FTCreateFaceTracker();
	if( !pFT ){
		std::cerr << "Error : FTCreateFaceTracker" << std::endl;
		return -1;
	}

	FT_CAMERA_CONFIG colorConfig  = { width, height, NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS };
	FT_CAMERA_CONFIG depthConfig  = { width, height, NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS * 2.0f };
	hResult = pFT->Initialize( &colorConfig, &depthConfig, nullptr, nullptr );
	if( FAILED( hResult ) ){
		std::cerr << "Error : Initialize" << std::endl;
		return -1;
	}

	// Face Trackingの結果インターフェース
	IFTResult* pFTResult = nullptr;
	hResult = pFT->CreateFTResult( &pFTResult );
	if( FAILED( hResult )  || !pFTResult ){
		std::cerr << "Error : CreateFTResult" << std::endl;
		return -1;
	}

	// Face Trackingのための画像
	IFTImage* pColorImage = FTCreateImage();
	IFTImage* pDepthImage = FTCreateImage();
	if( !pColorImage || !pDepthImage )
	{
		std::cerr << "Error : FTCreateImage" << std::endl;
		return -1;
	}
	pColorImage->Allocate( width, height, FTIMAGEFORMAT_UINT8_B8G8R8X8 );
	pDepthImage->Allocate( width, height, FTIMAGEFORMAT_UINT16_D13P3 );

	FT_VECTOR3D* pHintPoint = nullptr;
	bool lastTrack = false;

	HANDLE hEvents[3] = { hColorEvent, hDepthPlayerEvent, hSkeletonEvent };

	cv::namedWindow( "Face Tracking" );
	cv::namedWindow( "Depth" );

	while ( 1 ){
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
		INuiFrameTexture* pColorTexture = colorImageFrame.pFrameTexture;
		NUI_LOCKED_RECT colorLockedRect;
		pColorTexture->LockRect( 0, &colorLockedRect, nullptr, 0 );

		cv::Mat colorMat( height, width, CV_8UC4, reinterpret_cast<unsigned char*>( colorLockedRect.pBits ) );

		// Face Trackingのための画像へコピー 
		memcpy( pColorImage->GetBuffer(), &colorLockedRect.pBits[0], colorLockedRect.size );

		// Depthセンサーからフレームを取得
		NUI_IMAGE_FRAME depthPlayerImageFrame = { 0 };
		hResult = pSensor->NuiImageStreamGetNextFrame( hDepthPlayerHandle, 0, &depthPlayerImageFrame );
		if( FAILED( hResult ) ){
			std::cerr << "Error : NuiImageStreamGetNextFrame( DEPTH&PLAYER )" << std::endl;
			return -1;
		}

		// Depthデータの取得
		INuiFrameTexture* pDepthPlayerTexture = depthPlayerImageFrame.pFrameTexture;
		NUI_LOCKED_RECT depthPlayerLockedRect;
		pDepthPlayerTexture->LockRect( 0, &depthPlayerLockedRect, nullptr, 0 );

		// Face Trackingのための画像へコピー
		memcpy( pDepthImage->GetBuffer(), &depthPlayerLockedRect.pBits[0], depthPlayerLockedRect.size );

		// Depth&Playerを画像化する
		cv::Mat bufferMat16U = cv::Mat::zeros( height, width, CV_16UC1 );
		unsigned short* pBuffer = reinterpret_cast<unsigned short*>( depthPlayerLockedRect.pBits );
		NUI_DEPTH_IMAGE_POINT depthPoint;
		NUI_COLOR_IMAGE_POINT colorPoint;
		for( int y = 0; y < height; y++ ){
			for( int x = 0; x < width; x++ ){
				unsigned int index = y * width +x;
				depthPoint.x = x;
				depthPoint.y = y;
				depthPoint.depth = pBuffer[index];
				pCordinateMapper->MapDepthPointToColorPoint( NUI_IMAGE_RESOLUTION_640x480, &depthPoint, NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, &colorPoint );
				if( ( colorPoint.x >= 0 ) && ( colorPoint.x < width ) && ( colorPoint.y >= 0 ) && ( colorPoint.y < height ) ){
					bufferMat16U.at<unsigned short>(  colorPoint.y, colorPoint.x ) = NuiDepthPixelToDepth( pBuffer[index] );
				}
			}
		}

		cv::Mat bufferMat8U( height, width, CV_8UC1 );
		bufferMat16U.convertTo( bufferMat8U, CV_8U, -255.0f / NUI_IMAGE_DEPTH_MAXIMUM_NEAR_MODE, 255.0f );
		cv::Mat depthMat( height, width, CV_8UC3 );
		cv::cvtColor( bufferMat8U, depthMat, CV_GRAY2BGR );

		// Skeletonフレームを取得
		NUI_SKELETON_FRAME skeletonFrame = { 0 };
		hResult = pSensor->NuiSkeletonGetNextFrame( 0, &skeletonFrame );
		if( FAILED( hResult ) ){
			std::cerr << "Error : NuiSkeletonGetNextFrame" << std::endl;
			return -1;
		}

		// Skeletonデータ(頭、首)の取得
		bool skeletonTracked[NUI_SKELETON_COUNT];
		FT_VECTOR3D neckPoint[NUI_SKELETON_COUNT];
		FT_VECTOR3D headPoint[NUI_SKELETON_COUNT];
		for( int count = 0; count < NUI_SKELETON_COUNT; count++ ){
			headPoint[count] = neckPoint[count] = FT_VECTOR3D(0, 0, 0);
			skeletonTracked[count] = false;
			NUI_SKELETON_DATA skeletonData = skeletonFrame.SkeletonData[count];
			if( skeletonData.eTrackingState == NUI_SKELETON_TRACKED ){
				skeletonTracked[count] = true;
				if( skeletonData.eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_HEAD] != NUI_SKELETON_POSITION_NOT_TRACKED ){
					headPoint[count].x = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HEAD].x;
					headPoint[count].y = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HEAD].y;
					headPoint[count].z = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HEAD].z;
				}
				if( skeletonData.eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_SHOULDER_CENTER] != NUI_SKELETON_POSITION_NOT_TRACKED ){
					neckPoint[count].x = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].x;
					neckPoint[count].y = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].y;
					neckPoint[count].z = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].z;
				}
			}
		}

		// 一番近いPlayerのSkleton(頭、首)をヒントとして与える
		int selectedSkeleton = -1;
		float smallestDistance = 0.0f;
		if( pHintPoint == nullptr ){
			for( int count = 0 ; count < NUI_SKELETON_COUNT ; count++ ){
				if( skeletonTracked[count] && ( smallestDistance == 0 || headPoint[count].z < smallestDistance ) ){
					smallestDistance = headPoint[count].z;
					selectedSkeleton = count;
				}
			}
		}
		else{
			for( int count = 0 ; count < NUI_SKELETON_COUNT ; count++ ){
				if( skeletonTracked[count] ){
					float distance = std::abs( headPoint[count].x - pHintPoint[1].x ) + std::abs( headPoint[count].y - pHintPoint[1].y ) + std::abs( headPoint[count].z - pHintPoint[1].z );
					if( smallestDistance == 0.0f || distance < smallestDistance ){
						smallestDistance = distance;
						selectedSkeleton = count;
					}
				}
			}
		}

		if( selectedSkeleton != -1 ){
			FT_VECTOR3D hint[2];
			hint[0] = neckPoint[selectedSkeleton];
			hint[1] = headPoint[selectedSkeleton];
			pHintPoint = hint;

			NUI_SKELETON_DATA hintSkeleton = skeletonFrame.SkeletonData[selectedSkeleton];
			pCordinateMapper->MapSkeletonPointToDepthPoint( &hintSkeleton.SkeletonPositions[NUI_SKELETON_POSITION_HEAD], NUI_IMAGE_RESOLUTION_640x480, &depthPoint ); 
			cv::circle( depthMat, cv::Point( static_cast<int>( depthPoint.x ), static_cast<int>( depthPoint.y ) ), 10, cv::Scalar( 0, 0, 255 ), -1, CV_AA );
			pCordinateMapper->MapSkeletonPointToDepthPoint( &hintSkeleton.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER], NUI_IMAGE_RESOLUTION_640x480, &depthPoint ); 
			cv::circle( depthMat, cv::Point( static_cast<int>( depthPoint.x ), static_cast<int>( depthPoint.y ) ), 10, cv::Scalar( 0, 0, 255 ), -1, CV_AA );
		}
		else{
			pHintPoint = nullptr;
		}

		// センサーデータを設定する
		FT_SENSOR_DATA sensorData;
		sensorData.pVideoFrame = pColorImage;
		sensorData.pDepthFrame = pDepthImage;
		sensorData.ZoomFactor = 1.0f;
		POINT viewOffset = { 0, 0 };
		sensorData.ViewOffset = viewOffset;

		// FaceTrackingの検出・追跡 
		if( lastTrack ){
			// This method is faster than StartTracking() and is used only for tracking. But, If the face being tracked moves too far from the previous location, this method fails.
			hResult = pFT->ContinueTracking( &sensorData, pHintPoint, pFTResult );
			if( FAILED( hResult ) || FAILED( pFTResult->GetStatus() ) ){
				lastTrack = false;
			}
		}
		else{
			// This process is more expensive than simply tracking (done by calling ContinueTracking()), but more robust.
			hResult = pFT->StartTracking( &sensorData, nullptr, pHintPoint, pFTResult );
			if( SUCCEEDED( hResult ) && SUCCEEDED( pFTResult->GetStatus() ) ){
				lastTrack = true;
			}
			else{
				lastTrack = false;
			}
		}

		// Face Tracking結果描画
		if( lastTrack && SUCCEEDED( pFTResult->GetStatus() ) ){
			// 2Dの座標を取得する
			FT_VECTOR2D* pPoints = 0;
			UINT pointCount = 0;
			hResult = pFTResult->Get2DShapePoints( &pPoints, &pointCount );
			if( SUCCEEDED( hResult ) ){
				for( int i = 0; i < pointCount; ++i ){
					cv::circle( colorMat, cv::Point( static_cast<int>( pPoints[i].x ), static_cast<int>( pPoints[i].y ) ), 1, cv::Scalar( 0, 0, 255 ) );
				}
			}

			// 顔の領域を取得する
			RECT faceRect;
			hResult = pFTResult->GetFaceRect( &faceRect );
			if( SUCCEEDED( hResult ) ){
				cv::rectangle( colorMat, cv::Rect( faceRect.left, faceRect.top, faceRect.right - faceRect.left, faceRect.bottom - faceRect.top ), cv::Scalar( 0, 255, 0 ) );
			}

			// 顔の向きを取得する
			FLOAT scale;
			FLOAT rotationXYZ[3];
			FLOAT translationXYZ[3];
			hResult = pFTResult->Get3DPose( &scale, rotationXYZ, translationXYZ );
			if( SUCCEEDED( hResult ) ){
				// Pitch(X:0),Yaw(Y:1),Roll(Z:2)
				std::stringstream ss;
				ss << rotationXYZ[0] << ", " << rotationXYZ[1] << ", " << rotationXYZ[2];
				cv::putText( colorMat, ss.str(), cv::Point( 0, 30 ), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar( 255, 255, 255 ) );
			}
		}

		cv::imshow( "Face Tracking", colorMat );
		cv::imshow( "Depth", depthMat );

		// フレームの解放
		pColorTexture->UnlockRect( 0 );
		pDepthPlayerTexture->UnlockRect( 0 );
		pSensor->NuiImageStreamReleaseFrame( hColorHandle, &colorImageFrame );
		pSensor->NuiImageStreamReleaseFrame( hDepthPlayerHandle, &depthPlayerImageFrame );

		// ループの終了判定(Escキー)
		if( cv::waitKey( 30 ) == VK_ESCAPE ){
			break;
		}
	}

	// Kinectの終了処理
	pFT->Release();
	pFTResult->Release();
	pColorImage->Release();
	pDepthImage->Release();
	pSensor->NuiShutdown();
	pSensor->NuiSkeletonTrackingDisable();
	CloseHandle( hColorEvent );
	CloseHandle( hDepthPlayerEvent );
	CloseHandle( hSkeletonEvent );

	cv::destroyAllWindows();

	return 0;
}
