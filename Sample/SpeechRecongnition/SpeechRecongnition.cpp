// SpeechRecongnition.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
// This source code is licensed under the MIT license. Please see the License in License.txt.
//

#include "stdafx.h"
#include <Windows.h>
#include <NuiApi.h>
#include <opencv2/opencv.hpp>

// Kinect for Windows Developer Toolkit v1.8.0 - Samples/C++/SpeechBasics-D2Dより引用
// KinectAudioStream.h and .cpp : Copyright (c) Microsoft Corporation.  All rights reserved.
#include "KinectAudioStream.h"

// Microsoft Speech Platform SDK 11
#include <sapi.h>
//#include <sphelper.h> // SpFindBestToken()
#include <strsafe.h>
#include <intsafe.h>

#include <mfapi.h>
#include <uuids.h>
#include <wmcodecdsp.h>

#pragma comment( lib, "sapi.lib" )
#pragma comment( lib, "dmoguids.lib" )
#pragma comment( lib, "Msdmo.lib" )
#pragma comment( lib, "amstrmid.lib" )
#pragma comment( lib, "avrt.lib" )


int _tmain( int argc, _TCHAR* argv[] )
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

	hResult = pSensor->NuiInitialize( NUI_INITIALIZE_FLAG_USES_AUDIO );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiInitialize" << std::endl;
		return -1;
	}

	// Audioストリームの初期化(InitializeAudioStream)
	std::cout << "InitializeAudioStream" << std::endl;
	INuiAudioBeam* pNuiAudioSource;
	hResult = pSensor->NuiGetAudioSource( &pNuiAudioSource );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiGetAudioSource" << std::endl;
		return -1;
	}

	IMediaObject* pMediaObject = nullptr;
	IPropertyStore* pPropertyStore = nullptr;
	pNuiAudioSource->QueryInterface( IID_IMediaObject, reinterpret_cast<void**>( &pMediaObject ) );
	pNuiAudioSource->QueryInterface( IID_IPropertyStore, reinterpret_cast<void**>( &pPropertyStore ) );

	PROPVARIANT propvariant;
	PropVariantInit( &propvariant );
	propvariant.vt = VT_I4;
	propvariant.lVal = static_cast<LONG>( 4 );
	pPropertyStore->SetValue( MFPKEY_WMAAECMA_SYSTEM_MODE, propvariant );
	PropVariantClear( &propvariant );

	WAVEFORMATEX waveFormat = { AudioFormat, AudioChannels, AudioSamplesPerSecond, AudioAverageBytesPerSecond, AudioBlockAlign, AudioBitsPerSample, 0 };
	DMO_MEDIA_TYPE mediaType = { 0 };
	MoInitMediaType( &mediaType, sizeof( WAVEFORMATEX ) );

	mediaType.majortype = MEDIATYPE_Audio;
	mediaType.subtype = MEDIASUBTYPE_PCM;
	mediaType.lSampleSize = 0;
	mediaType.bFixedSizeSamples = true;
	mediaType.bTemporalCompression = false;
	mediaType.formattype = FORMAT_WaveFormatEx;
	memcpy( mediaType.pbFormat, &waveFormat, sizeof( WAVEFORMATEX ) );

	pMediaObject->SetOutputType( 0, &mediaType, 0 ); 

	KinectAudioStream* audioStream = new KinectAudioStream( pMediaObject );

	IStream* pStream = nullptr;
	audioStream->QueryInterface( IID_IStream, reinterpret_cast<void**>( &pStream ) );

	CoInitialize( nullptr );
	ISpStream* pSpeechStream = nullptr;
	CoCreateInstance( CLSID_SpStream, NULL, CLSCTX_INPROC_SERVER, __uuidof(ISpStream), reinterpret_cast<void**>( &pSpeechStream ) );

	pSpeechStream->SetBaseStream( pStream, SPDFID_WaveFormatEx, &waveFormat );

	MoFreeMediaType( &mediaType );
	pStream->Release();
	pPropertyStore->Release();
	pMediaObject->Release();
	pNuiAudioSource->Release();

	// 音声認識器を作成(CreateSpeechRecognizer)
	std::cout << "CreateSpeechRecognizer" << std::endl;
	ISpRecognizer* pSpeechRecognizer;
	CoCreateInstance( CLSID_SpInprocRecognizer, nullptr, CLSCTX_INPROC_SERVER, __uuidof(ISpRecognizer), reinterpret_cast<void**>( &pSpeechRecognizer ) );

	pSpeechRecognizer->SetInput( pSpeechStream, false );

	/*
	// If can use ATL, easier to using SpFindBestToken(sphelper.h). When using Professional or more.
	ISpObjectToken* pEngineToken = nullptr;
	SpFindBestToken( SPCAT_RECOGNIZERS, L"Language=411;Kinect=True", NULL, &pEngineToken ); // Japanese "Language=411;Kinect=True" English "Language=409;Kinect=True"
	*/

	///*
	// If can't use ATL, alternative to using SpFIndBestToken(sphelper.h). When using Express.
	const wchar_t* pVendorPreferred = L"VendorPreferred";
	const unsigned long lengthVendorPreferred = static_cast<unsigned long>( wcslen( pVendorPreferred ) );
	unsigned long length;
	ULongAdd( lengthVendorPreferred, 1, &length );
	wchar_t* pAttribsVendorPreferred = new wchar_t[ length ];
	StringCchCopyW( pAttribsVendorPreferred, length, pVendorPreferred );

	ISpObjectTokenCategory* pTokenCategory = nullptr;
	CoCreateInstance( CLSID_SpObjectTokenCategory, nullptr, CLSCTX_ALL, __uuidof(ISpObjectTokenCategory), reinterpret_cast<void**>( &pTokenCategory ) );

	pTokenCategory->SetId( SPCAT_RECOGNIZERS, false );

	IEnumSpObjectTokens* pEnumTokens = nullptr;
	CoCreateInstance( CLSID_SpMMAudioEnum, nullptr, CLSCTX_ALL, __uuidof(IEnumSpObjectTokens), reinterpret_cast<void**>( &pEnumTokens ) );

	pTokenCategory->EnumTokens( L"Language=411;Kinect=True", pAttribsVendorPreferred, &pEnumTokens ); // Japanese "Language=411;Kinect=True" English "Language=409;Kinect=True"

	delete[] pAttribsVendorPreferred;
	
	ISpObjectToken* pEngineToken = nullptr;
	pEnumTokens->Next( 1, &pEngineToken, nullptr );
	//*/

	pSpeechRecognizer->SetRecognizer( pEngineToken );
	
	ISpRecoContext* pSpeechContext;
	pSpeechRecognizer->CreateRecoContext( &pSpeechContext );

	pEngineToken->Release();
	///*
	pTokenCategory->Release();
	pEnumTokens->Release();
	//*/

	// 音声認識辞書の作成(LoadSpeechGrammar)
	std::cout << "LoadSpeechGrammar" << std::endl;
	ISpRecoGrammar* pSpeechGrammar;
	pSpeechContext->CreateGrammar( 1, &pSpeechGrammar );

	pSpeechGrammar->LoadCmdFromFile( L"SpeechRecognition_Ja.grxml", /*SPLO_STATIC*/SPLO_DYNAMIC ); // http://www.w3.org/TR/speech-grammar/ (UTF-8/CRLF)
	
	audioStream->StartCapture();
	pSpeechGrammar->SetRuleState( nullptr, nullptr, SPRS_ACTIVE );
	pSpeechRecognizer->SetRecoState( SPRST_ACTIVE_ALWAYS );
	pSpeechContext->SetInterest( SPFEI( SPEI_RECOGNITION ), SPFEI( SPEI_RECOGNITION ) );
	pSpeechContext->Resume( 0 );

	HANDLE hSpeechEvent = INVALID_HANDLE_VALUE;
	hSpeechEvent = pSpeechContext->GetNotifyEventHandle();
	HANDLE hEvents[1] = { hSpeechEvent };

	int width = 640;
	int height = 480;

	cv::Mat audioMat = cv::Mat::zeros( height, width, CV_8UC3 );
	cv::namedWindow( "Audio" );

	bool exit = false;

	std::cout << std::endl << "Speech Recognition Start..." << std::endl << std::endl;

	while( 1 ){
		// イベントの更新待ち
		ResetEvent( hSpeechEvent );
		unsigned long waitObject = MsgWaitForMultipleObjectsEx( ARRAYSIZE( hEvents ), hEvents, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE );

		if( waitObject == WAIT_OBJECT_0 ){
			// イベントの取得
			const float confidenceThreshold = 0.3f;
			SPEVENT eventStatus;
			unsigned long eventFetch = 0;
			pSpeechContext->GetEvents( 1, &eventStatus, &eventFetch );
			while( eventFetch > 0 ){
				switch( eventStatus.eEventId ){
					// 音声認識イベント(SPEI_HYPOTHESIS:推定またはSPEI_RECOGNITION:認識)
					case SPEI_HYPOTHESIS:
					case SPEI_RECOGNITION:
						if( eventStatus.elParamType == SPET_LPARAM_IS_OBJECT ){
							// フレーズの取得
							ISpRecoResult* pRecoResult = reinterpret_cast<ISpRecoResult*>( eventStatus.lParam );
							SPPHRASE* pPhrase = nullptr;
							hResult = pRecoResult->GetPhrase( &pPhrase );
							if( SUCCEEDED( hResult ) ){
								if( ( pPhrase->pProperties != nullptr ) && ( pPhrase->pProperties->pFirstChild != nullptr ) ){
									// 辞書のフレーズタグと比較
									const SPPHRASEPROPERTY* pSemantic = pPhrase->pProperties->pFirstChild;
									if( pSemantic->SREngineConfidence > confidenceThreshold ){
										if( wcscmp( L"あか", pSemantic->pszValue ) == 0 ){
											std::cout << "あか" << std::endl;
											audioMat = cv::Scalar( 0, 0, 255 );
										}
										else if( wcscmp( L"みどり", pSemantic->pszValue ) == 0 ){
											std::cout << "みどり" << std::endl;
											audioMat = cv::Scalar( 0, 255, 0 );
										}
										else if( wcscmp( L"あお", pSemantic->pszValue ) == 0 ){
											std::cout << "あお" << std::endl;
											audioMat = cv::Scalar( 255, 0, 0 );
										}
										else if( wcscmp( L"おわり", pSemantic->pszValue ) == 0 ){
											exit = true;
										}
									}
								}
								CoTaskMemFree( pPhrase );
							}
						}
						break;

					default:
						break;
				}
				pSpeechContext->GetEvents( 1, &eventStatus, &eventFetch );
			}
		}

		// 表示
		cv::imshow( "Audio", audioMat );

		// ループの終了判定(Escキー)
		if( cv::waitKey( 30 ) == VK_ESCAPE || exit ){
			break;
		}
	}

	// 終了処理
	audioStream->StopCapture();
	pSpeechRecognizer->SetRecoState( SPRST_INACTIVE );
	CoUninitialize();
	pSensor->NuiShutdown();
	CloseHandle( hSpeechEvent );

	cv::destroyAllWindows();

	return 0;
}
