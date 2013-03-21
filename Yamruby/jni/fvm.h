// mrubyVM共通のパラメータのヘッダ

#include <android/log.h>

#define  LOG_TAG    "mrubyVM"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


//Error Code define
#define VM_SOCKET_OPEN_ERROR	1
#define VM_CONNECT_ERROR		2


//Client Data
//#define VM_HOSTIP				"localhost"
#define VM_HOSTIP				"127.0.0.1"
#define VM_PORTNUMBER			60081

//#define VM_FILEPATH			"/sdcard/SaridaSAKURA/"
	
#define VM_START_FILENAME		"/Yamruby/yamruby.rb"
