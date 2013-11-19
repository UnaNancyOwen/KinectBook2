// BackgroundRemoval.cpp : �R���\�[�� �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
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

	// Kinect�̃C���X�^���X�����A������
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

	// Color�X�g���[��
	HANDLE hColorEvent = INVALID_HANDLE_VALUE;
	HANDLE hColorHandle = INVALID_HANDLE_VALUE;
	hColorEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = pSensor->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, 0, 2, hColorEvent, &hColorHandle );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiImageStreamOpen( COLOR )" << std::endl;
		return -1;
	}

	// Depth&Player�X�g���[��
	HANDLE hDepthPlayerEvent = INVALID_HANDLE_VALUE;
	HANDLE hDepthPlayerHandle = INVALID_HANDLE_VALUE;
	hDepthPlayerEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = pSensor->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, NUI_IMAGE_RESOLUTION_640x480, 0, 2, hDepthPlayerEvent, &hDepthPlayerHandle );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiImageStreamOpen( DEPTH&PLAYER )" << std::endl;
		return -1;
	}

	// Skeleton�X�g���[��
	HANDLE hSkeletonEvent = INVALID_HANDLE_VALUE;
	hSkeletonEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = pSensor->NuiSkeletonTrackingEnable( hSkeletonEvent, 0 );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiSkeletonTrackingEnable" << std::endl;
		return -1;
	}

	// �w�i�����X�g���[��
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

	// �w�i�摜�̓ǂݍ���
	cv::Mat backgroundMat = cv::imread( "Background.jpg" );
	if( backgroundMat.empty() ){
		std::cerr << " Error : cv::imread" << std::endl;
		return -1;
	}
	cv::cvtColor( backgroundMat, backgroundMat, CV_BGR2BGRA );

	// �𑜓x�̎擾
	unsigned long refWidth = 0;
	unsigned long refHeight = 0;
	NuiImageResolutionToSize( NUI_IMAGE_RESOLUTION_640x480, refWidth, refHeight );
	int width = static_cast<int>( refWidth );
	int height = static_cast<int>( refHeight );

	// �ʒu���킹�̐ݒ�
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
			// Color�J��������t���[�����擾
			NUI_IMAGE_FRAME colorImageFrame = { 0 };
			hResult = pSensor->NuiImageStreamGetNextFrame( hColorHandle, 0, &colorImageFrame );
			if( FAILED( hResult ) ){
				std::cerr << "Error : NuiImageStreamGetNextFrame( COLOR )" << std::endl;
				return -1;
			}

			// Color�摜�f�[�^�̎擾
			INuiFrameTexture* pColorFrameTexture = colorImageFrame.pFrameTexture;
			NUI_LOCKED_RECT colorLockedRect;
			pColorFrameTexture->LockRect( 0, &colorLockedRect, nullptr, 0 );

			std::copy( &colorLockedRect.pBits[0], &colorLockedRect.pBits[colorLockedRect.size], colorBuffer.begin() );

			// �w�i�����̃J���[����
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
			// Depth�Z���T�[����t���[�����擾
			NUI_IMAGE_FRAME depthPlayerImageFrame = { 0 };
			hResult = pSensor->NuiImageStreamGetNextFrame( hDepthPlayerHandle, 0, &depthPlayerImageFrame );
			if( FAILED( hResult ) ){
				std::cerr << "Error : NuiImageStreamGetNextFrame( DEPTH&PLAYER )" << std::endl;
				return -1;
			}

			// Depth&Player�f�[�^�̎擾
			// �]���̃v���C���[ID�𗘗p�����w�i����
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

			// �w�i������Depth����
			pBackgroundRemovedColorStream->ProcessDepth( depthPlayerLockedRect.size, depthPlayerLockedRect.pBits, depthPlayerImageFrame.liTimeStamp );

			pDepthPlayerFrameTexture->UnlockRect( 0 );
			pSensor->NuiImageStreamReleaseFrame( hDepthPlayerHandle, &depthPlayerImageFrame );
		}

		ret = WaitForSingleObject( hSkeletonEvent, 0 );
		if( ret == WAIT_OBJECT_0 ){
			// Skeleton�t���[�����擾
			NUI_SKELETON_FRAME skeletonFrame = { 0 };
			hResult = pSensor->NuiSkeletonGetNextFrame( 0, &skeletonFrame );
			if( FAILED( hResult ) ){
				std::cout << "Error : NuiSkeletonGetNextFrame" << std::endl;
				return -1;
			}

			// �w�i�����̃X�P���g������
			pBackgroundRemovedColorStream->ProcessSkeleton( NUI_SKELETON_COUNT, skeletonFrame.SkeletonData, skeletonFrame.liTimeStamp );

			// �ŏ��Ɍ��o�����ǐՂ��Ă���l�̂ݕ\������
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


		// �w�i�����̏���
		ret = WaitForSingleObject( hBackgroundRemovedEvent, 0 );
		if( ret == WAIT_OBJECT_0 ){
			NUI_BACKGROUND_REMOVED_COLOR_FRAME frame = { 0 };
			hResult = pBackgroundRemovedColorStream->GetNextFrame( 0, &frame );
			if( SUCCEEDED( hResult ) ){
				cv::Mat backgroundRemovedColorMat( height, width, CV_8UC4 );

				int alpha = 255;
				for( UINT i = 0; i < frame.backgroundRemovedColorDataLength; ++i ){
					// �s�N�Z�����Ƃ̏����̍ŏ��ɃA���t�@�l���擾����
					if( ( i % 4 ) == 0 ){
						alpha = frame.pBackgroundRemovedColorData[i + 3];
						if( alpha != 0 ){
							int a = 0;
						}
					}

					// �A���t�@�l�ȊO�́A�����������s��
					if( ( i % 4 ) != 3 ){
						// �w�i�F�ƑO�i�F(�v���C���[)����������
						auto back = ( 255.0f - alpha ) * backgroundMat.data[i];
						auto front = alpha * frame.pBackgroundRemovedColorData[i];
						backgroundRemovedColorMat.data[i] = static_cast<unsigned char>( ( back + front ) / 255.0f );
					}
				}

				cv::imshow( "BackgroundRemoved", backgroundRemovedColorMat );
				pBackgroundRemovedColorStream->ReleaseFrame( &frame );
			}
		}

		// ���[�v�̏I������(Esc�L�[)
		if( cv::waitKey( 30 ) == VK_ESCAPE ){
			break;
		}
	}

	// Kinect�̏I������
	pSensor->NuiShutdown();
	pSensor->NuiSkeletonTrackingDisable();
	pCordinateMapper->Release();
	CloseHandle( hColorEvent );
	CloseHandle( hDepthPlayerEvent );
	CloseHandle( hSkeletonEvent );

	cv::destroyAllWindows();

	return 0;
}
