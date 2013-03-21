//
//VMのメインプログラム
//

#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <jni.h>

#include "fvm.h"
#include "fvmClient.h"
#include "fvmExec.h"

extern char StartFileName[];
extern char VmFilename[];
extern char MountSdCardName[];

volatile int VmFinishFlg = 0;

jint
Java_com_luaridaworks_yamruby_MainActivity_VMStart( JNIEnv* env, jobject thiz, jstring jSdCardDrive, jstring jFileName)
{
const char *FileName = (*env)->GetStringUTFChars(env, jFileName, NULL);
const char *SdCardDrive = (*env)->GetStringUTFChars(env, jSdCardDrive, NULL);

	//乱数初期化
	srand(time(NULL));	//乱数の初期化

	//マウントされているsdcardのフォルダ名をセットします。
	strcpy( MountSdCardName, SdCardDrive );

	//開始ファイル名の設定(これは固定です)
	strcpy( StartFileName, MountSdCardName );
	strcat( StartFileName, VM_START_FILENAME );

	//実行するファイルパスをセットします。/sdcad/sarida/test/test.rb'というように、フルパスになります。
	strcpy( VmFilename, FileName );

	//LOGI("SdCardDrive");
	//LOGI(SdCardDrive);
	//LOGI("FileName");
	//LOGI(FileName);
	//LOGI("MountSdCardName");
	//LOGI(MountSdCardName);
	//LOGI("VmFilename");
	//LOGI(VmFilename);
	//LOGI("StartFileName");
	//LOGI(StartFileName);

	if( SocketOpen() ){
		//ソケットがオープンできなかった
		(*env)->ReleaseStringUTFChars(env, jFileName,  FileName);
		(*env)->ReleaseStringUTFChars(env, jSdCardDrive,  SdCardDrive);
	   return ( 2 );
	}

	VmFinishFlg = 0;

	while( VmFinishFlg==0 ){
		VmRun();
		if( VmFinishFlg!=0 ){
			//LOGE("VmFinishFlg: !0 ");
		}
		//VMFinishFlg = 1;
	}

	SocketClose();

	// native文字列開放をVMに通知
	(*env)->ReleaseStringUTFChars(env, jFileName,  FileName);
	(*env)->ReleaseStringUTFChars(env, jSdCardDrive,  SdCardDrive);

    return ( 1 );
}
