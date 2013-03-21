//************************************************************************
// Ya mrubyの呼び出し実行モジュールプログラム 2013.3.21
//************************************************************************
#include <string.h>

volatile char	ProgVer[] = {"0.07"};

//mruby用ヘッダ
#include "fvm.h"
#include "fvmExec.h"
#include "fvmClient.h"

#include "mruby.h"
#include "mruby/compile.h"

#include "fKernel.h"
//#include "ffirmata.h"
#include "fSystem.h"
#include "fCanvas.h"
#include "fItem.h"
#include "fSound.h"
#include "fSensor.h"

/*
#include "fHttp.h"
#include "fSock.h"
#include "fSprite.h"
#include "frtc.h"
#include "fserial.h"
*/
extern char	SendData[];				//送信データ(256byte)
extern int	SocketNumber;			//接続ソケット番号


//実行するスクリプトファイル名が格納される。
char StartFileName[128];
char VmFilename[128];
char ExeFilename[128];				//現在実行されているファイルのパス名

char MountSdCardName[64];			//マウントされているSDCardのフォルダ名の入っている '/sdcard/'のように入っている

volatile extern int	VmFinishFlg;

/*
static const luaL_Reg HttpTbl[] = {
	{"status", HttpStatus },
	{"get", HttpGet },
	{"post", HttpPost },
	{"addHeader", HttpAddHeader },
	{"addParam", HttpAddParam },
	{"setPostFile", HttpSetPostFile },
	{"setContentType", HttpSetContentType },
	{"clrHeader", HttpClrHeader },
	{"clrParam", HttpClrParam },
	{NULL,NULL}
};

static const luaL_Reg SockTbl[] = {
	{"nsend", socknSend },
	{"nrecv", socknRecv },
	{"send", sockSend },
	{"recv", sockRecv },
	{"getAddress", sockGetAddress },
	{"connectOpen", sockConnectOpen },
	{"listenOpen", sockListenOpen },
	{"close", sockClose },
	{"ngetAddress", socknGetAddress },
	{"nconnectOpen", socknConnectOpen },
	{"nlistenOpen", socknListenOpen },
	{"nclose", socknClose },
	{NULL,NULL}
};

static const luaL_Reg SpriteTbl[] = {
 	{"put", spritePut },
	{"move", spriteMove },
	{"inArea", spriteinArea },
	{"touch", spriteTouch },
	{"define", spriteDefine },
	{"clear", spriteClear },
	{"init", spriteInit },
	{NULL,NULL}
};

static const luaL_Reg rtcTbl[] = {
 	{"begin", RTCbegin },
 	{"setDateTime", RTCsetDateTime },
 	{"getDateTime", RTCgetDateTime },
	{NULL,NULL}
};

static const luaL_Reg serialTbl[] = {
	{"send", serialSend },
	{"read", serialRead },
	{NULL,NULL}
};
*/

//**************************************************
//  VM終了メッセージ
//**************************************************
void	VmFinishMes( mrb_state *mrb )
{
	mrb_raise(mrb, mrb_class_obj_get(mrb, "DalvikError"), "Not Responce from Dalvik.");
}

//**************************************************
//  エラーメッセージにファイル名があるかチェックして
// 無ければ-1を返します。(ファイル名というよりは':'をチェックしています
//**************************************************
int	ErrMessCheck( char *str )
{
int		mlen;
int		i;
int		ret = -1;

	mlen = strlen( str );

	for( i=0; i<mlen; i++ ){
		if( str[i]==':' ){
			ret = 0;
			break;	
		}
	}
	return( ret );
}

//**************************************************
//  エラーメッセージを送信します
//**************************************************
void	ErrorSend( int err, char *mes )
{
int		mlen, nlen;
int		i;

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x04;		//全体サイズ
	SendData[2] = 0x00;		//命令コード H
	SendData[3] = 0xFF;		//命令コード L

	SendData[4] = (char)err;

	if( ErrMessCheck( mes )==-1 ){
		mlen = strlen( ExeFilename );
		nlen = strlen( mes );
		SendData[5] = (char)((3+mlen+nlen) & 0xff);
		for( i=0; i<mlen; i++ ){	SendData[6+i] = ExeFilename[i];	}
		SendData[6+mlen] = ':';
		SendData[7+mlen] = '0';
		SendData[8+mlen] = ':';
		for( i=0; i<nlen; i++ ){	SendData[9+mlen+i] = mes[i];	}
		SendData[9+mlen+nlen] = 0x06;
		SendData[1] = (char)((10+mlen+nlen) & 0xff);
	}
	else{
		mlen = strlen( mes );
		SendData[5] = (char)(mlen & 0xff);
		for( i=0; i<mlen; i++ ){	SendData[6+i] = mes[i];	}
		SendData[6+mlen] = 0x06;
		SendData[1] = (char)((7+mlen) & 0xff);
	}

	//LOGI("ErrorSend");
	CommandSend( 1 );

	//LOGI( mes );
}

//**************************************************
//  スクリプト言語を実行します
//**************************************************
void VmRun( void )
{
	mrb_state *mrb = mrb_open();
	
	if(mrb == NULL){
		LOGE( "Can not Open mrb!!" );
		VmFinishFlg = 1;	//終了する
		return;
	}
	kernel_Init(mrb);	//カーネル関連メソッドの設定
	system_Init(mrb);	//システム関連メソッドの設定
	canvas_Init(mrb);	//Canvas関連メソッドの設定
	item_Init(mrb);		//Item関連メソッドの設定
	sound_Init(mrb);	//Sound関連メソッドの設定
	sensor_Init(mrb);	//Sensor関連メソッドの設定
	//firmata_Init(mrb);	//Firmata関連メソッドの設定

	//sprintf(SendData, "mrb->arena_idx=%d",mrb->arena_idx);
	//LOGE(SendData);

	strcpy( ExeFilename, VmFilename );		//実行するファイルをExeFilename[]に入れる。
	strcpy( VmFilename, StartFileName );	//とりあえず、VmFilename[]をStartFileName[]に初期化する。

	mrbc_context *context = mrbc_context_new(mrb);
	
	mrbc_filename(mrb, context, ExeFilename);	//行った先で、ファイル名分のメモリを確保して、c->filenameに入れている

	LOGI( ExeFilename );
	FILE *fp = NULL;
	if( (fp = fopen(ExeFilename, "r")) == NULL){
		VmFinishFlg = 1;	//終了する
		LOGE( "Can not Open!!" );

		//エラー表示
		ErrorSend( 7, ExeFilename );
		return;
	}

	//mribyを実行します
	mrb_value value = mrb_load_file_cxt(mrb, fp, context);

	mrbc_context_free(mrb, context);

	if (mrb->exc) {	//エラーが無いときはNULLです

		ErrorSend( 100, "Any Error" );

		if (!mrb_undef_p(value)) {
			mrb_value obj = mrb_obj_value(mrb->exc);
			obj = mrb_funcall(mrb, obj, "inspect", 0);

			//printf("\nlen= %d\n",RSTRING_LEN(obj));
			//printf("---\n");
			//fwrite(RSTRING_PTR(obj), RSTRING_LEN(obj), 1, stdout);
			//putc('\n', stdout);
		}
		VmFinishFlg = 1;	//VM終了する
		LOGE("Any Error!!");
	}

	fclose( fp );
	mrb_close(mrb);

	//サウンドを止める
	if( SoundClearAll()==-1 ){
		VmFinishFlg = 1;	//VM終了する
	}

	//Main画面とワーク画面を初期化する
	if( ResetBmp()==-1 ){
		VmFinishFlg = 1;	//VM終了する
		LOGE("Can not Reset Main or Work Bitmap!!");
	}

	/*
	luaL_openlib(LuaLinkP, "sprite", SpriteTbl, 0);
	luaL_openlib(LuaLinkP, "http", HttpTbl, 0);
	luaL_openlib(LuaLinkP, "sock", SockTbl, 0);
	luaL_openlib(LuaLinkP, "rtc", rtcTbl, 0);
	luaL_openlib(LuaLinkP, "serial", serialTbl, 0);


	if( err==LUA_ERRSYNTAX ){
		str = (char*)lua_tostring( LuaLinkP, -1);
		ErrorSend( 2, str );
		//LOGE("Syntax Error");
		//LOGE( str );
		LuaVMFinishFlg = 1;	//LuaVM終了する
	}
	else if( err==LUA_ERRMEM ){
		str = (char*)lua_tostring( LuaLinkP, -1);
		ErrorSend( 3, str );
		//LOGE("Memory Error Under Compile");
		//LOGE( str );
		LuaVMFinishFlg = 1;	//LuaVM終了する
	}
	else{
		err = lua_pcall( LuaLinkP, 0, LUA_MULTRET, 0 );
		if( err==LUA_ERRRUN ){
			str = (char*)lua_tostring( LuaLinkP, -1);
			ErrorSend( 4, str );
			//LOGE("Run Error");
			//LOGE( str );
			LuaVMFinishFlg = 1;	//LuaVM終了する
		}
		else if( err==LUA_ERRMEM ){
			str = (char*)lua_tostring( LuaLinkP, -1);
			ErrorSend( 5, str );
			//LOGE("Memory Secured Error");
			//LOGE( str );
			LuaVMFinishFlg = 1;	//LuaVM終了する
		}
		else if( err==LUA_ERRERR ){
			str = (char*)lua_tostring( LuaLinkP, -1);
			ErrorSend( 6, str );
			//LOGE("Handler Function Error");
			//LOGE( str );
			LuaVMFinishFlg = 1;	//LuaVM終了する
		}
	}

	*/
}

//**************************************************
//  エラーメッセージ
//**************************************************
/*
void	LuaErrorMes( lua_State *LuaLinkP, char* mes, int type )
{
char mess[128];

	strcpy( mess, ExeFilename );
	strcat( mess, ":" );
	strcat( mess, mes );
	if( type==0 ){
		strcat( mess, "() : Not Enough Arguments" );
	}
	lua_settop(LuaLinkP, 0);
	lua_pushstring( LuaLinkP, mess );
	lua_error( LuaLinkP );

	//ErrorSend( 0x01, mes );
	//LOGE( mess );
}
*/


//**************************************************
//
//マウントされているsdcardのフォルダ名を取得します。
// 戻り値 -1:エラー 0:成功
//
// 2-2-A.マウントされているSDカードのディレクトリ名を取得します: getSdcard(0x020A)
//	0x03 0x05  0x020A 0x06
//	戻りデータ
//	0x03 0xSS(全体サイズ) 0xRX(OK) 0xHS(SDCARDマウント名長さ) 0xHM(SDCARDマウント名) 0x06
//	0xRXの値
//		0x00: OK
//		0x46: エラー(終了)・・・javaはフラグで管理する
//	
//	マウント名戻り値の仕様 '/sdcard' このように、最後に/をつけない。
//**************************************************
int	getMountSDCardName( void )
{
char	recv[66];
int		tlen;
int		i;
int		ret;

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x05;		//全体サイズ
	SendData[2] = 0x02;		//命令コード H
	SendData[3] = 0x0A;		//命令コード L
	SendData[4] = 0x06;		//終了コード

	CommandSend( 0 );

	ret = 0;
	while(ret<1){
		ret = getRecvData( SocketNumber, recv, 65, 0, 2000000L );
		if( recv[2]==0x46 ){	//終了
			return( -1 );		//戻り値は1つですよ。
		}
	}

	tlen = (int)recv[3];
	if( tlen>61 ){	return( -1 );	}

	for( i=0; i<tlen; i++ ){
		MountSdCardName[i] = recv[4+i];
	}
	//MountSdCardName[tlen] = '/';
	//MountSdCardName[tlen+1] = 0;
	MountSdCardName[tlen] = 0;

	return( 0 );
}


//**************************************************
//
// ScriptFilenameに初めに実行するファイル名(フォルダ名込み)を取得します。
// 戻り値 -1:エラー 0:成功
//
// 2-2-B.実行するファイル名を取得する: getRunfile(0x020B)
//	0x03 0x05  0x020B 0x06
//	戻りデータ
//	0x03 0xSS(全体サイズ) 0xRX(OK) 0xHS(Scriptファイル名長さ) 0xHM(Scriptファイル名) 0x06
//	0xRXの値
//		0x00: OK
//		0x46: エラー(終了)・・・javaはフラグで管理する
//
//	スクリプトファイル名の仕様 '/sdcad/sarida/test/test.lua'というように、フルパスで指定する。
//**************************************************
int	getStartScriptFilename( void )
{
char	recv[130];
int		tlen;
int		i;
int		ret;

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x05;		//全体サイズ
	SendData[2] = 0x02;		//命令コード H
	SendData[3] = 0x0B;		//命令コード L
	SendData[4] = 0x06;		//終了コード

	CommandSend( 0 );

	ret = 0;
	while(ret<1){
		ret = getRecvData( SocketNumber, recv, 130, 0, 2000000L );
		if( recv[2]==0x46 ){	//終了
			return( -1 );		//戻り値は1つですよ。
		}
	}

	tlen = (int)recv[3];
	if( tlen>127 ){	return( -1 );	}

	for( i=0; i<tlen; i++ ){
		VmFilename[i] = recv[4+i];
	}
	VmFilename[tlen] = 0;

	return( 0 );
}

//************************************************************************
// b[0],b[1],b[2],b[3]をfloatに変換します
//************************************************************************
float btof( unsigned char *b, int revFlg )
{
float	f0;
float	f1 = 1.0f;
int		ni;
int		i;

	if( b[0]==0 && b[1]==0 && b[2]==0 && b[3]==0 ){	return( 0.0f );	}

	if(revFlg==0){
		ni = (((int)b[0]<<1) & 0xff) + (((int)b[1]>>7) & 0xff) - 127;
		f0 = 1.0f + (float)(b[1]&0x7F)/0x80 + (float)b[2]/0x8000 + (float)b[3]/0x800000;

		for( i=0; i<ni; i++ ){	f1 *= 2.0f;	}

		f0 *= ((b[0]&0x80)==0?f1:-f1);
	}
	else{
		ni = (((int)b[3]<<1) & 0xff) + (((int)b[2]>>7) & 0xff) - 127;
		f0 = 1.0f + (float)(b[2]&0x7F)/0x80 + (float)b[1]/0x8000 + (float)b[0]/0x800000;
		for( i=0; i<ni; i++ ){	f1 *= 2.0f;	}
		f0 *= ((b[3]&0x80)==0?f1:-f1);
	}
	return( f0 );
}
