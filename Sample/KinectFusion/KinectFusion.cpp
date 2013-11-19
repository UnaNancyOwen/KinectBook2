// KinectFusion.cpp : �R���\�[�� �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
// This source code is licensed under the MIT license. Please see the License in License.txt.
//

#include "stdafx.h"
#include <Windows.h>
#include <NuiApi.h>
#include <NuiKinectFusionApi.h>
// Kinect for Windows Developer Toolkit v1.8.0 - Samples/C++/KinectFusionExplorer-D2D�����p�A�ꕔ����
// KinectFusionHelper is: Copyright (c) Microsoft Corporation. All rights reserved.
#include "KinectFusionHelper.h"
#include <opencv2/opencv.hpp>

#include <ppl.h> // for Concurrency::parallel_for

/*
// If don't using KinectFusionHelper, define this function.
void SetIdentityMatrix(Matrix4 &mat)
{
	mat.M11 = 1; mat.M12 = 0; mat.M13 = 0; mat.M14 = 0;
	mat.M21 = 0; mat.M22 = 1; mat.M23 = 0; mat.M24 = 0;
	mat.M31 = 0; mat.M32 = 0; mat.M33 = 1; mat.M34 = 0;
	mat.M41 = 0; mat.M42 = 0; mat.M43 = 0; mat.M44 = 1;
}
*/

// �č\�������̃��Z�b�g
void ResetReconstruction( INuiFusionColorReconstruction* pReconstruction, Matrix4* pMat )
{
	std::cout << "Reset Reconstruction" << std::endl;
	SetIdentityMatrix( *pMat );
	pReconstruction->ResetReconstruction( pMat, nullptr );
}

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

	hResult = pSensor->NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX );
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

	// Near Mode�̐ݒ�
	hResult = pSensor->NuiImageStreamSetImageFrameFlags( hDepthPlayerHandle, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiImageStreamSetImageFrameFlags" << std::endl;
		return -1;
	}

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
	std::vector<NUI_COLOR_IMAGE_POINT> pColorPoint( width * height );

	// 3�����č\���̐ݒ�
	NUI_FUSION_RECONSTRUCTION_PARAMETERS reconstructionParameter;
	reconstructionParameter.voxelsPerMeter = 256;	// 1[m]������̃{�N�Z����[voxel/meter]
	reconstructionParameter.voxelCountX = 512;		// X���̍ő�{�N�Z����[voxel]
	reconstructionParameter.voxelCountY = 384;		// Y���̍ő�{�N�Z����[voxel]
	reconstructionParameter.voxelCountZ = 512;		// Z���̍ő�{�N�Z����[voxel]
													// �e���̎O�����č\���ł��鋗��[m] = voxelCount[voxel] / voxelPerMeter[voxel/meter]
													// �K�v�ȃ�������[byte] = voxelCountX[voxel] * voxelCountY[voxel] * voxelCountZ[voxel] * 4[byte/voxel]

	// �g�p�\�ȃ�������
	UINT memorySize;
	hResult = NuiFusionGetDeviceInfo( NUI_FUSION_RECONSTRUCTION_PROCESSOR_TYPE_AMP/*NUI_FUSION_RECONSTRUCTION_PROCESSOR_TYPE_CPU*/, -1, nullptr, 0, nullptr, 0, &memorySize );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiFusionGetDeviceInfo" << std::endl;
		return -1;
	}
	std::cout << "Available Memory Size[KB] : " << memorySize << std::endl;

	// Kinect Fusion�̃C���X�^���X�̐���
	Matrix4 worldToCameraTransform;
	SetIdentityMatrix( worldToCameraTransform );
	INuiFusionColorReconstruction* pReconstruction;
	hResult = NuiFusionCreateColorReconstruction( &reconstructionParameter, NUI_FUSION_RECONSTRUCTION_PROCESSOR_TYPE_AMP/*NUI_FUSION_RECONSTRUCTION_PROCESSOR_TYPE_CPU*/, -1, &worldToCameraTransform, &pReconstruction );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiFusionCreateReconstruction" << std::endl;
		return -1;
	}

	// Kinect Fusion�̂��߂̉摜
	// Depth
	NUI_FUSION_IMAGE_FRAME* pDepthFloatImageFrame;
	hResult = NuiFusionCreateImageFrame( NUI_FUSION_IMAGE_TYPE_FLOAT, width, height, nullptr, &pDepthFloatImageFrame );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiFusionCreateImageFrame( FLOAT )" << std::endl;
		return -1;
	}

	// SmoothDepth
	NUI_FUSION_IMAGE_FRAME* pSmoothDepthFloatImageFrame;
	hResult = NuiFusionCreateImageFrame( NUI_FUSION_IMAGE_TYPE_FLOAT, width, height, nullptr, &pSmoothDepthFloatImageFrame );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiFusionCreateImageFrame( FLOAT )" << std::endl;
		return -1;
	}

	// Color
	NUI_FUSION_IMAGE_FRAME* pColorImageFrame;
	hResult = NuiFusionCreateImageFrame( NUI_FUSION_IMAGE_TYPE_COLOR, width, height, nullptr, &pColorImageFrame );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiFusionCreateImageFrame( COLOR )" << std::endl;
		return -1;
	}

	// Point Cloud
	NUI_FUSION_IMAGE_FRAME* pPointCloudImageFrame;
	hResult = NuiFusionCreateImageFrame( NUI_FUSION_IMAGE_TYPE_POINT_CLOUD, width, height, nullptr, &pPointCloudImageFrame );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiFusionCreateImageFrame( POINT_CLOUD )" << std::endl;
		return -1;
	}

	// Surface
	NUI_FUSION_IMAGE_FRAME* pSurfaceImageFrame;
	hResult = NuiFusionCreateImageFrame( NUI_FUSION_IMAGE_TYPE_COLOR, width, height, nullptr, &pSurfaceImageFrame );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiFusionCreateImageFrame( COLOR )" << std::endl;
		return -1;
	}

	/*
	// Normal
	NUI_FUSION_IMAGE_FRAME* pNormalImageFrame;
	hResult = NuiFusionCreateImageFrame( NUI_FUSION_IMAGE_TYPE_COLOR, width, height, nullptr, &pNormalImageFrame );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiFusionCreateImageFrame( COLOR )" << std::endl;
		return -1;
	}
	*/

	HANDLE hEvents[2] = { hColorEvent, hDepthPlayerEvent };

	cv::namedWindow( "Color" );
	cv::namedWindow( "Depth" );
	cv::namedWindow( "Surface" );
	/*cv::namedWindow( "Normal" );*/

	while( 1 ){
		// �t���[���̍X�V�҂�
		ResetEvent( hColorEvent );
		ResetEvent( hDepthPlayerEvent );
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

		// Depth�Z���T�[����t���[�����擾
		NUI_IMAGE_FRAME depthPlayerImageFrame = { 0 };
		hResult = pSensor->NuiImageStreamGetNextFrame( hDepthPlayerHandle, 0, &depthPlayerImageFrame );
		if( FAILED( hResult ) ){
			std::cerr << "Error : NuiImageStreamGetNextFrame( DEPTH&PLAYER )" << std::endl;
			return -1;
		}

		// Depth&Player�f�[�^�̎擾
		BOOL nearMode = true;
		INuiFrameTexture* pDepthPlayerFrameTexture = nullptr;
		pSensor->NuiImageFrameGetDepthImagePixelFrameTexture( hDepthPlayerHandle, &depthPlayerImageFrame, &nearMode, &pDepthPlayerFrameTexture );
		NUI_LOCKED_RECT depthPlayerLockedRect;
		pDepthPlayerFrameTexture->LockRect( 0, &depthPlayerLockedRect, nullptr, 0 );

		// Depth�f�[�^����NUI_FUSION_IMAGE_FRAME�ɕϊ�
		NUI_DEPTH_IMAGE_PIXEL* pDepthPlayerPixel = reinterpret_cast<NUI_DEPTH_IMAGE_PIXEL*>( depthPlayerLockedRect.pBits );
		hResult = pReconstruction->DepthToDepthFloatFrame( pDepthPlayerPixel, width * height * sizeof(NUI_DEPTH_IMAGE_PIXEL), pDepthFloatImageFrame, NUI_FUSION_DEFAULT_MINIMUM_DEPTH/* 0.35[m] */, NUI_FUSION_DEFAULT_MAXIMUM_DEPTH/* 8.0[m] */, true );
		if( FAILED( hResult ) ){
			std::cerr << "Error :INuiFusionColorReconstruction::DepthToDepthFloatFrame" << std::endl;
			return -1;
		}

		// Color�f�[�^����NUI_FUSION_IMAGE_FRAME�ɕϊ�
		INuiFrameTexture* pColorImageFrameTexture = pColorImageFrame->pFrameTexture;
		NUI_LOCKED_RECT colorImageFrameLockedRect;
		pColorImageFrameTexture->LockRect( 0, &colorImageFrameLockedRect, nullptr, 0 );

		int* pSrc = reinterpret_cast<int*>( colorLockedRect.pBits );
		int* pDst = reinterpret_cast<int*>( colorImageFrameLockedRect.pBits );

		pCordinateMapper->MapDepthFrameToColorFrame( NUI_IMAGE_RESOLUTION_640x480, width * height, pDepthPlayerPixel, NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, width * height, &pColorPoint[0] );

		Concurrency::parallel_for( 0, height, [&]( int y ){
			for( int x = 0; x < width; x++ ){
				unsigned int index = y * width + x;
				if( pDepthPlayerPixel[index].depth != 0 ){
					pDst[index] = pSrc[pColorPoint[index].y * width + pColorPoint[index].x];
				}
				else{
					pDst[index] = 0;
				}
			}
		} );

		// Depth�f�[�^�̕�����
		// kernelWidth : �������t�B���^�[�̃T�C�Y 0(0x0 Not Smoothing) 1(3x3) 2(5x5) 3(7x7)
		// distanceThreshold : ������臒l[m] [0.01f-0.10f]
		hResult = pReconstruction->SmoothDepthFloatFrame( pDepthFloatImageFrame, pSmoothDepthFloatImageFrame, 1, 0.04f );
		if( FAILED( hResult ) ){
			std::cerr << "Error :INuiFusionColorReconstruction::SmoothDepthFloatFrame" << std::endl;
			return -1;
		}

		// 3�����`��̍č\������
		pReconstruction->GetCurrentWorldToCameraTransform( &worldToCameraTransform );
		hResult = pReconstruction->ProcessFrame( pSmoothDepthFloatImageFrame, pColorImageFrame, NUI_FUSION_DEFAULT_ALIGN_ITERATION_COUNT, NUI_FUSION_DEFAULT_INTEGRATION_WEIGHT, NUI_FUSION_DEFAULT_COLOR_INTEGRATION_OF_ALL_ANGLES, &worldToCameraTransform );
		if( FAILED( hResult ) ){
			// �G���[�������ꍇ�̓��Z�b�g����
			static int errorCount = 0;
			errorCount++;
			if ( errorCount >= 100 ) {
				errorCount = 0;
				ResetReconstruction( pReconstruction, &worldToCameraTransform );
			}
		}

		// Point Cloud�̎擾
		hResult = pReconstruction->CalculatePointCloud( pPointCloudImageFrame, pSurfaceImageFrame, &worldToCameraTransform );
		if( FAILED( hResult ) ){
			std::cerr << "Error : CalculatePointCloud" << std::endl;
			return -1;
		}

		/*
		// Surface�̎擾
		Matrix4 worldToBGRTransform = { 0.0f };
		worldToBGRTransform.M11 = reconstructionParameter.voxelsPerMeter / reconstructionParameter.voxelCountX;
		worldToBGRTransform.M22 = reconstructionParameter.voxelsPerMeter / reconstructionParameter.voxelCountY;
		worldToBGRTransform.M33 = reconstructionParameter.voxelsPerMeter / reconstructionParameter.voxelCountZ;
		worldToBGRTransform.M41 = 0.5f; // [0.0f-1.0f] * X���W
		worldToBGRTransform.M42 = 0.5f; // [0.0f-1.0f] * Y���W
		worldToBGRTransform.M43 = 0.0f; // [0.0f-1.0f] * Z���W
		worldToBGRTransform.M44 = 1.0f;

		hResult = NuiFusionShadePointCloud( pPointCloudImageFrame, &worldToCameraTransform, &worldToBGRTransform, pSurfaceImageFrame, pNormalImageFrame );
		if( FAILED( hResult ) ){
			std::cerr << "Error : NuiFusionShadePointCloud" << std::endl;
			return -1;
		}
		*/

		// Surface�摜�f�[�^�̎擾
		INuiFrameTexture* pSurfaceFrameTexture = pSurfaceImageFrame->pFrameTexture;
		NUI_LOCKED_RECT surfaceLockedRect;
		pSurfaceFrameTexture->LockRect( 0, &surfaceLockedRect, nullptr, 0 );

		/*
		// Normal�摜�f�[�^�̎擾
		INuiFrameTexture* pNormalFrameTexture = pNormalImageFrame->pFrameTexture;
		NUI_LOCKED_RECT normalLockedRect;
		pNormalFrameTexture->LockRect( 0, &normalLockedRect, nullptr, 0 );
		*/

		// �\��
		cv::Mat colorMat( height, width, CV_8UC4, reinterpret_cast<unsigned char*>( colorLockedRect.pBits ) );

		cv::Mat bufferMat = cv::Mat::zeros( height, width, CV_16UC1 );
		Concurrency::parallel_for( 0, height, [&]( int y ){
			for( int x = 0; x < width; x++ ){
				unsigned int index = y * width + x;
				bufferMat.at<unsigned short>( pColorPoint[index].y, pColorPoint[index].x ) = pDepthPlayerPixel[index].depth;
			}
		} );
		cv::Mat depthMat( height, width, CV_8UC1 );
		bufferMat.convertTo( depthMat, CV_8U, -255.0f / 10000.0f, 255.0f );

		cv::Mat surfaceMat( height, width, CV_8UC4, surfaceLockedRect.pBits );
		/*cv::Mat normalMat( height, width, CV_8UC4, normalLockedRect.pBits );*/

		cv::imshow( "Color", colorMat );
		cv::imshow( "Depth", depthMat );
		cv::imshow( "Surface", surfaceMat );
		/*cv::imshow( "Normal", normalMat );*/

		// �t���[���̉��
		pColorFrameTexture->UnlockRect( 0 );
		pDepthPlayerFrameTexture->UnlockRect( 0 );
		pColorImageFrameTexture->UnlockRect( 0 );
		pSurfaceFrameTexture->UnlockRect( 0 );
		/*pNormalFrameTexture->UnlockRect( 0 );*/
		pSensor->NuiImageStreamReleaseFrame( hColorHandle, &colorImageFrame );
		pSensor->NuiImageStreamReleaseFrame( hDepthPlayerHandle, &depthPlayerImageFrame );

		// ���[�v�̏I������(Esc�L�[)
		int key = cv::waitKey( 30 );
		if( key == VK_ESCAPE ){
			break;
		}
		// �蓮���Z�b�g(R�L�[)
		else if( key == 'r' ){
			ResetReconstruction( pReconstruction, &worldToCameraTransform );
		}
		// ���b�V���t�@�C���̕ۑ�(S�L�[)
		else if( key == 's' ){
			INuiFusionColorMesh *pMesh = nullptr;
			hResult = pReconstruction->CalculateMesh( 1, &pMesh );
			if( SUCCEEDED( hResult ) ){
				std::cout << "Save Mesh File" << std::endl;
				char* fileName = "mesh.ply";
				WriteAsciiPlyMeshFile( pMesh, fileName, true, true );
				//WriteBinarySTLMeshFile( pMesh, fileName, true );
				//WriteAsciiObjMeshFile( pMesh, fileName, true );
				pMesh->Release();
			}
		}
	}

	// Kinect�̏I������
	pSensor->NuiShutdown();
	pCordinateMapper->Release();
	pReconstruction->Release();
	NuiFusionReleaseImageFrame( pDepthFloatImageFrame );
	NuiFusionReleaseImageFrame( pSmoothDepthFloatImageFrame );
	NuiFusionReleaseImageFrame( pColorImageFrame );
	NuiFusionReleaseImageFrame( pPointCloudImageFrame );
	NuiFusionReleaseImageFrame( pSurfaceImageFrame );
	/*NuiFusionReleaseImageFrame( pNormalImageFrame );*/
	CloseHandle( hColorEvent );
	CloseHandle( hDepthPlayerEvent );

	cv::destroyAllWindows();

	return 0;
}
