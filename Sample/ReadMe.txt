���ЁuKinect for Windows SDK ���H�v���O���~���O�v�T���v���v���O���� ReadMe.txt

���\��
�{�T���v���v���O�����̍\���͈ȉ��̂Ƃ���ł��B

    Sample.zip
    ��
    ��  // �T���v���v���O����
    ����Sample
    ��  ��
    ��  ����Sample.sln
    ��  ��
    ��  ����Color
    ��  ��  ����Color.vcxproj
    ��  ��  ����Color.cpp
    ��  ��
    ��  ����DepthAndPlayer
    ��  ��  ����DepthAndPlayer.vcxproj
    ��  ��  ����DepthAndPlayer.cpp
    ��  ��
    ��  ����Skeleton
    ��  ��  ����Skeleton.vcxproj
    ��  ��  ����Skeleton.cpp
    ��  ��
    ��  ����SpeechRecongnition
    ��  ��  ����SpeechRecongnition.vcxproj
    ��  ��  ����SpeechRecongnition.cpp
    ��  ��  ����KinectAudioStream.h
    ��  ��  ����KinectAudioStream.cpp
    ��  ��
    ��  ����FaceTracking
    ��  ��  ����FaceTracking.vcxproj
    ��  ��  ����FaceTracking.cpp
    ��  ��
    ��  ����KinectInteraction
    ��  ��  ����KinectInteraction.vcxproj
    ��  ��  ����KinectInteraction.cpp
    ��  ��
    ��  ����BackgroundRemoval
    ��  ��  ����BackgroundRemoval.vcxproj
    ��  ��  ����BackgroundRemoval.cpp
    ��  ��
    ��  ����KinectFusion
    ��      ����KinectFusion.vcxproj
    ��      ����KinectFusion.cpp
    ��      ����KinectFusionHelper.h
    ��  �@  ����KinectFusionHelper.cpp
    ��
    ��  // �v���p�e�B�V�[�g
    ����KinectBook.props
    ��
    ��  // ���C�Z���X
    ����License.txt
    ��
    ��  // ����
    ����ReadMe.txt


���T���v���v���O�����ɂ���
�{�T���v���v���O�����͈ȉ��̊��Ńr���h�ł���悤�ɐݒ肵�Ă��܂��B
���ɂ���Ă͓K�X�ݒ�����������Ă��������B

    Visual Studio Express 2012
    Kinect for Windows SDK v1.8
    Kinect for Windows Developer Toolkit v1.8.0
    Speech Platform SDK 11
    OpenCV 2.4.6


��Kinect�ɂ���
Kinect for Xbox360�ł�Near Mode�͓��삵�Ȃ����߁A�\�[�X�R�[�h�̊Y���ӏ����R�����g�A�E�g���Ă��������B

    /*
    // Near Mode�̐ݒ�
    hResult = pSensor->NuiImageStreamSetImageFrameFlags( hDepthPlayerHandle, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE );
    if( FAILED( hResult ) ){
        std::cerr << "Error : NuiImageStreamSetImageFrameFlags" << std::endl;
        return -1;
    }
    */

    NUI_IMAGE_DEPTH_MAXIMUM/*_NEAR_MODE*/

�܂��AKinect����肭���삵�Ȃ��ꍇ��Kinect for Windows Developer Toolkit��Kinect Explorer�ŊȒP�ɏ󋵂��m�F���邱�Ƃ��ł��܂��B
(�����̓v���O��������HRESULT�^�̃G���[�R�[�h����ł��m�F���邱�Ƃ��ł��܂��B)

    ��FNotPowered �� �d�����ʂ��Ă��Ȃ��\��������܂��B
                      USB/�d���P�[�u���̐ڑ����m�F���Ă��������B

USB�R���g���[���[�Ƃ̑����������ł���ꍇ������܂��B
�ʂ�USB�R���g���[���[�Ő��䂳��Ă���USB�|�[�g�ɐڑ��������Ə�肭���삷�邱�Ƃ�����܂��B


���v���p�e�B�[�V�[�g�ɂ���
�t���̃v���p�e�B�V�[�gKinectBook.props�ɂ͈ȉ��̐ݒ肪�L�q����Ă��܂��B
�쐬�����v���W�F�N�g�̐ݒ�ɖ��ɗ��Ǝv���܂��B
�Ȃ��A�{�T���v���v���O�����͕t���̃v���p�e�B�[�V�[�g�Ɉˑ����Ă��܂���B

    Kinect for Windows SDK v1.8
    Kinect for Windows Developer Toolkit v1.8.0
    Speech Platform SDK 11
    OpenCV 2.4.6


��OpenCV�ɂ���
OpenCV�̓o�[�W�����ɂ���ă��C�u���������قȂ�܂��B
����͈ȉ��̖����K���Ń��C�u�������������Ă��܂��B

Debug�p�̃��C�u�����t�@�C���ɂ͍Ō�Ɂud�v�����܂��B

    Debug   �Fopencv_������������d.lib
    Release �Fopencv_������������.lib

�������̓o�[�W�����ԍ��ł��B

    ��FOpenCV 2.4.6 �� opencv_������246.lib

�{���ł͊��ϐ��uOPENCV_VER�v�Ńo�[�W�����ԍ���������ݒ肵�Ă��܂��B

�������̓��W���[�����ł��B
���p����@�\�ɂ��킹�ă��C�u�����������N���܂��B
�悭�g���郂�W���[���͈ȉ��̂Ƃ���ł��B

    core    �F��b
    highgui �F���[�U�[�C���^�[�t�F�[�X
    imgproc �F�摜����

    ��Fcore �� opencv_core������.lib

�C���N���[�h����w�b�_�t�@�C����<opencv/opencv.hpp>�ł��B
�ꕔ�������قڂ��ׂĂ�OpenCV�֘A�̃w�b�_�t�@�C�����C���N���[�h����܂��B

    #include <opencv/opencv.hpp>

OpenCV 2.4.6�ɂ�Visual Studio 2013(vc12)�����̃o�C�i������������Ă��܂���D
OpenCV 2.4.6���g�p����ꍇ��CMake���g����Visual Studio 2013(Visual Studio 12�܂���Visual Studio 12 Win64)�̃\�����[�V�������쐬���C�r���h���Ă��������D

    OpenCV v2.4.6 documentation > OpenCV Tutorials > Introduction to OpenCV > Installation in Windows > Building the library
    http://docs.opencv.org/trunk/doc/tutorials/introduction/windows_install/windows_install.html#building-the-library


�����ϐ��ɂ���
Kinect for Windows SDK�����Kinect for Windows Developer Toolkit���C���X�g�[������ƈȉ��̊��ϐ����ݒ肳��܂��B
�{�T���v���v���O�����̊e��ݒ�͊��ϐ��Ɉˑ����Ă��܂��B
����̃o�[�W�����A�b�v�ɔ������ϐ��̕ϐ�����l���ω�����ꍇ������̂œK�X�ݒ�����������Ă��������B

    �C���X�g�[���Ŏ����Őݒ肳�����ϐ�
        �ϐ���                  �l
        KINECTSDK10_DIR         C:\Program Files\Microsoft SDKs\Kinect\v1.8\
        KINECT_TOOLKIT_DIR      C:\Program Files\Microsoft SDKs\Kinect\Developer Toolkit v1.8.0\
        FTSDK_DIR               C:\Program Files\Microsoft SDKs\Kinect\Developer Toolkit v1.8.0\

    �蓮�ŐV�K�ɍ쐬������ϐ�
        �ϐ���          �l
        OPENCV_DIR      C:\Program Files\opencv\
        OPENCV_VER      246

    �蓮�Œl��ǉ�������ϐ�(�l�̊Ԃ́u;�v�ŋ�؂�܂�)
        �ϐ���          �l(Win32)
        Path            ;%KINECT_TOOLKIT_DIR%\Redist\x86;%OPENCV_DIR%\build\x86\vc11\bin
                        �l(x64)
                        ;%KINECT_TOOLKIT_DIR%\Redist\amd64;%OPENCV_DIR%\build\x64\vc11\bin

�܂��A���ϐ��ő��̕ϐ����Q�Ƃ���ꍇ��%������%�ƋL�q���܂��B
Visual Studio�Ŋ��ϐ����Q�Ƃ���ꍇ��$(������)�ƋL�q���܂��B
���ꂼ��ϐ��̒l���W�J����ĔF������܂��B

    ��FKinect for Windows Developer Toolkit��Win32���s�t�@�C��(*.dll)�̃f�B���N�g�����w�肷��ꍇ
            ���ϐ�      �F %KINECT_TOOLKIT_DIR%\Redist\x86 �� C:\Program Files\Microsoft SDKs\Kinect\Developer Toolkit v1.8.0\Redist\x86

        Kinect for Windows SDK�̃C���N���[�h �f�B���N�g�����w�肷��ꍇ
            Visual Studio �F $(KINECTSDK10_DIR)\inc �� C:\Program Files\Microsoft SDKs\Kinect\v1.8\inc    

���ϐ��̐ݒ���ԈႦ���OS���N�����Ȃ��Ȃ�\��������̂Œ��ӂ��Ă��������B
�܂��A���ϐ���ύX�����ꍇ��Visual Studio���ċN�����ĕύX�𔽉f���Ă��������B


���X�^�[�g�A�b�v �v���W�F�N�g�ɂ���
�X�^�[�g�A�b�v �v���W�F�N�g�͕W���ł́u�V���O�� �X�^�[�g�A�b�v �v���W�F�N�g�v�ɂȂ��Ă��܂��B
���s����v���W�F�N�g��ύX����ɂ́A�\�����[�V�����̃R���e�L�X�g���j���[����[�v���p�e�B]��I���A
[���L�v���p�e�B]>[�X�^�[�g�A�b�v�v���W�F�N�g]�́u�V���O�� �X�^�[�g�A�b�v �v���W�F�N�g�v�̃h���b�v�_�E�����j���[������s�������v���W�F�N�g��I�����Đݒ肵�Ă��������B
�܂��A[���݂̑I��]�Ƀ`�F�b�N�{�b�N�X�����Đݒ肷��ƁA���ݑI������Ă���v���W�F�N�g���X�^�[�g�A�b�v �v���W�F�N�g�ɂȂ�̂ŕ֗��ł��B


���������x�ɂ���
�T���v���v���O�����𓮂����ꍇ�́uRelease�v�\���Ńr���h���Ă��������B
�R���p�C���̍œK�����L���ɂȂ�܂��B


������m�F
�{�T���v���v���O�����͈ȉ��̊��œ�����m�F���܂����B
�{�T���v���v���O�����͂��ׂĂ̊��ɂ��ē����ۏ؂�����̂ł͂���܂���B

    OS      �FWindows 8
    Sensor  �FKinect for Windows
    IDE     �FVisual Studio Express 2012 for Windows Desktop
    Library �FKinect for Windows SDK v1.8
              Kinect for Developer Toolkit v1.8.0
              Speech Platform SDK 11
              OpenCV 2.4.6


�����C�Z���X�ɂ���
�{�T���v���v���O�����̃��C�Z���X��MIT License�Ƃ��܂��B
���C�Z���X����License.txt���Q�Ƃ��Ă��������B


���Ɛӎ���
�{�T���v���v���O�����͎g�p�҂̐ӔC�ɂ����ė��p���Ă��������B
���̃T���v���v���O�����ɂ���Ĕ������������Ȃ��Q�E���Q�Ȃǂɑ΂��āA���쌠�҂͈�ؐӔC�𕉂�Ȃ����̂Ƃ��܂��B
�܂��A�{�T���v���v���O�����͗\���Ȃ��ύX��폜����ꍇ������܂��B


���ύX����
2013/11/03 �T���v���v���O�������쐬���܂����B
