// Color.cpp : �R���\�[�� �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
// This source code is licensed under the MIT license. Please see the License in License.txt.
//

#include "stdafx.h"
#include <Windows.h>
#include <NuiApi.h>
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

	hResult = pSensor->NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiInitialize" << std::endl;
		return -1;
	}

	// Color�X�g���[���̊J�n
	HANDLE hColorEvent = INVALID_HANDLE_VALUE;
	HANDLE hColorHandle = INVALID_HANDLE_VALUE;
	hColorEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = pSensor->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, 0, 2, hColorEvent, &hColorHandle );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiImageStreamOpen( COLOR )" << std::endl;
		return -1;
	}

	// Color�J�����̐ݒ�
	INuiColorCameraSettings* pCameraSettings;
	hResult = pSensor->NuiGetColorCameraSettings( &pCameraSettings );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiGetColorCameraSettings" << std::endl;
		return -1;
	}
	double contrast = 1.0f;
	pCameraSettings->GetContrast( &contrast );
	std::cout << "Contrast : " << contrast << std::endl;

	// �𑜓x�̎擾
	unsigned long refWidth = 0;
	unsigned long refHeight = 0;
	NuiImageResolutionToSize( NUI_IMAGE_RESOLUTION_640x480, refWidth, refHeight );
	int width = static_cast<int>( refWidth );
	int height = static_cast<int>( refHeight );

	HANDLE hEvents[1] = { hColorEvent };

	cv::namedWindow( "Color" );

	while( 1 ){
		// �t���[���̍X�V�҂�
		ResetEvent( hColorEvent );
		WaitForMultipleObjects( ARRAYSIZE( hEvents ), hEvents, true, INFINITE );

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

		// �\��
		cv::Mat colorMat( height, width, CV_8UC4, reinterpret_cast<unsigned char*>( colorLockedRect.pBits ) );
		cv::imshow( "Color", colorMat );
		
		// �t���[���̉��
		pColorFrameTexture->UnlockRect( 0 );
		pSensor->NuiImageStreamReleaseFrame( hColorHandle, &colorImageFrame );

		// ���[�v�̏I������(Esc�L�[)
		int key = cv::waitKey( 30 );
		if( key == VK_ESCAPE ){
			break;
		}
		// Color�J�����̐ݒ�(���L�[ / ���L�[)
		else if( ( key >> 16 ) == VK_UP ){
			pCameraSettings->SetContrast( ( contrast < 2.0f ) ? contrast += 0.1f : contrast );
			std::cout << "Contrast : " << contrast << std::endl;
		}
		else if( ( key >> 16 ) == VK_DOWN ){
			pCameraSettings->SetContrast( ( contrast > 0.5f ) ? contrast -= 0.1f : contrast );
			std::cout << "Contrast : " << contrast << std::endl;
		}

	}
	
	// Kinect�̏I������
	pSensor->NuiShutdown();
	pCameraSettings->Release();
	CloseHandle( hColorEvent );

	cv::destroyAllWindows();

	return 0;
}
