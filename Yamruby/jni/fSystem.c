//************************************************************************
// システム・ハード関連
//************************************************************************
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "mruby.h"
#include "mruby/string.h"


#include "fvm.h"
#include "fvmExec.h"
#include "fvmClient.h"

extern int	SocketNumber;			//接続ソケット番号
extern char	SendData[];				//送信データ(512byte)
extern char VmFilename[];
extern volatile char ProgVer[];
extern char MountSdCardName[];
extern char ExeFilename[];

//struct RClass *systemModule;

//**************************************************
// 2-2-D.Saridaを完全に終了させます: exit(0x020D)
//	exit()
//	0x03 0x05 0x020D 0x06
//	戻りデータ
//	0x03 0x04 0x46 0x06
//	エラー値がもどり、即終了
//**************************************************
mrb_value mrb_system_exit(mrb_state *mrb, mrb_value self)
{
	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x05;			//全体サイズ
	SendData[2] = 0x02;			//命令コード H
	SendData[3] = 0x0D;			//命令コード L
	SendData[4] = 0x06;			//終了コード

	CommandSend( 0 );

	VmFinishMes(mrb);
	return mrb_nil_value();	//戻り値は無しですよ。
}

//**************************************************
// setRun( ファイル名 )
// 次に実行するスクリプトファイルをVmFilenameにセットします。
//**************************************************
mrb_value mrb_system_setrun(mrb_state *mrb, mrb_value self)
{
mrb_value text;

	mrb_get_args(mrb, "S", &text);
	char *str = RSTRING_PTR(text);

	strcpy( (char*)VmFilename, (char*)str );
	
	return mrb_nil_value();	//戻り値は無しですよ。
}
	
//**************************************************
//  システムのバージョンを取得します
//**************************************************
mrb_value mrb_system_version(mrb_state *mrb, mrb_value self)
{
	return mrb_str_new_cstr(mrb, (const char*)ProgVer);
}

//**************************************************
// 2-2-8.Androidアプリを明示的に呼び出します: expCall(0x0208)
//	expCall( クラス名 [,データ,タイプ] )
//	0x03 0xSS(全体サイズ) 0x0208 0xCC(class名長) 0xCM～class名データ 0xDD(Data長) 0xDM～データ 0xTC(Type長) 0xTM～cTypeデータ 0x06
//	このコマンドの後は、Saridaは終了します。ただしエラーが返ったときは、構文処理を続けます。
//	戻りデータ
//	0x03 0x04 0xRX 0x06
//	0xRXの値
//		0x00: 通常終了
//		0x46: エラー(終了)
//**************************************************
mrb_value mrb_system_expCall(mrb_state *mrb, mrb_value self)
{
int		mlen;
int		nlen;
int		olen;
int		i;
mrb_value clas, dat, typ;
char	*str1, *str2, *str3;
 
    int n = mrb_get_args(mrb, "S|SS", &clas, &dat, &typ);
	str1 = RSTRING_PTR(clas);
	str2 = RSTRING_PTR(dat);
	str3 = RSTRING_PTR(typ);
	
	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x04;			//全体サイズ
	SendData[2] = 0x02;			//命令コード H
	SendData[3] = 0x08;			//命令コード L

	mlen = strlen( str1 );
	SendData[4] = (char)(mlen & 0xff);
	for( i=0; i<mlen; i++ ){
		SendData[5+i] = str1[i];
	}
	SendData[5+mlen] = 0x00;
	SendData[6+mlen] = 0x00;
	SendData[7+mlen] = 0x06;
	SendData[1] = 8 + mlen;

	if( n>1 ){
		nlen = strlen( str2 );
		SendData[5+mlen] = (char)(nlen & 0xff);
		for( i=0; i<nlen; i++ ){
			SendData[6+mlen+i] = str2[i];
		}
		SendData[6+mlen+nlen] = 0x00;
		SendData[7+mlen+nlen] = 0x06;
		SendData[1] = 8 + mlen + nlen;
	}

	if( n>2 ){
		olen = strlen( str3 );
		SendData[6+mlen+nlen] = (char)(olen & 0xff);
		for( i=0; i<olen; i++ ){
			SendData[7+mlen+nlen+i] = str3[i];
		}
		SendData[7+mlen+nlen+olen] = 0x06;
		SendData[1] = 8 + mlen + nlen + olen;
	}

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();	//戻り値は無しですよ。
}

//**************************************************
// 2-2-9.AndroidアプリをACTION_VIEWで暗黙的に呼び出します: impCallActionView(0x0209)
//	impCallActionView( データ文字列, タイプ )
//	0x03 0xSS(全体サイズ) 0x0209 0xCC(URI長) 0xCM～URIデータ 0xTT(Type長) 0xYY～(Type文字列) 0x06
//	このコマンドの後は、Saridaは終了します。ただしエラーが返ったときは、構文処理を続けます。
//	impCallActionSendでは、
//		it.setAction(Intent.ACTION_VIEW);
//		it.setDataAndType( データ文字列, タイプ);
//	のみ対応しています。
//	戻りデータ
//	0x03 0x04 0xRX 0x06
//	0xRXの値
//		0x00: 通常終了
//		0x46: エラー(終了)
//**************************************************
mrb_value mrb_system_impCallActionView(mrb_state *mrb, mrb_value self)
{
int		mlen, nlen;
int		i;
mrb_value dat, typ;
char	*str1, *str2;

	mrb_get_args(mrb, "SS", &dat, &typ);
	str1 = RSTRING_PTR(dat);
	str2 = RSTRING_PTR(typ);

	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x04;			//全体サイズ
	SendData[2] = 0x02;			//命令コード H
	SendData[3] = 0x09;			//命令コード L

	mlen = strlen( str1 );
	SendData[4] = (char)(mlen & 0xff);
	for( i=0; i<mlen; i++ ){
		SendData[5+i] = str1[i];
	}

	nlen = strlen( str2 );
	SendData[5+mlen] = (char)(nlen & 0xff);
	for( i=0; i<nlen; i++ ){
		SendData[6+mlen+i] = str2[i];
	}
	SendData[6+mlen+nlen] = 0x06;

	SendData[1] = 7 + mlen + nlen;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-2-C.AndroidアプリをACTION_SENDで暗黙的に呼び出します: impCallActionSend(0x020C)
//	impCallActionSend( 送信文字列 [, "text/plain"等] )
//	0x03 0xSS(全体サイズ) 0x020C 0xCC(URI長) 0xCM～URIデータ 0xTT(Type長) 0xYY～(Type文字列) 0x06
//	このコマンドの後は、Saridaは終了します。ただしエラーが返ったときは、構文処理を続けます。
//	impCallActionSendでは、
//		it.setAction(Intent.ACTION_SEND);
//		it.putExtra(Intent.EXTRA_TEXT, strText);
//		it.setType("text/plain");
//	のみ対応しています。
//	戻りデータ
//	0x03 0x04 0xRX 0x06
//	0xRXの値
//		0x00: 通常終了
//		0x46: エラー(終了)
//**************************************************
mrb_value mrb_system_impCallActionSend(mrb_state *mrb, mrb_value self)
{
int		mlen, nlen;
int		i;
mrb_value dat, typ;
char	*str1, *str2;

	int n = mrb_get_args(mrb, "S|S", &dat, &typ);
	str1 = RSTRING_PTR(dat);
	str2 = RSTRING_PTR(typ);

	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x04;			//全体サイズ
	SendData[2] = 0x02;			//命令コード H
	SendData[3] = 0x0C;			//命令コード L

	mlen = strlen( str1 );
	SendData[4] = (char)(mlen & 0xff);
	for( i=0; i<mlen; i++ ){
		SendData[5+i] = str1[i];
	}

	if( n>1 ){
		nlen = strlen( str2 );
		SendData[5+mlen] = (char)(nlen & 0xff);
		for( i=0; i<nlen; i++ ){
			SendData[6+mlen+i] = str2[i];
		}
		SendData[6+mlen+nlen] = 0x06;
		SendData[1] = 7 + mlen + nlen;
	}
	else{
		SendData[5+mlen] = 0;
		SendData[6+mlen] = 0x06;
		SendData[1] = 7 + mlen;	
	}

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// system.getCardMnt()
// linuxのsdcardのフォルダを取得します
//**************************************************
mrb_value mrb_system_getCardMnt(mrb_state *mrb, mrb_value self)
{
	return mrb_str_new_cstr(mrb, (const char*)MountSdCardName);
}

//**************************************************
// 指定された秒数待ちます
// 1sec以下も待てます
//**************************************************
void secWait( double sec )
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
	double pt = tv.tv_sec + (double)tv.tv_usec*1e-6;
	while( 1 ){
	    gettimeofday(&tv, NULL);
		if( (tv.tv_sec + (double)tv.tv_usec*1e-6) - pt>sec ){	break;	}
	}
}

//**************************************************
//2-2-F.画面の方向を設定します: setScreen(0x020F)
//	setScreen(向き)
//	0x03 0x06 0x020F 0xMM 0x06
//	0xMMの値
//		0x00: 横向き(デフォルト)
//		0x01: 縦向き
//	戻りデータ
//	0x03 0x04 0x46 0x06
//	エラー値がもどり、即終了
//**************************************************
mrb_value mrb_system_setScreen(mrb_state *mrb, mrb_value self)
{
int di;

	mrb_get_args(mrb, "i", &di);
    
	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x06;		//全体サイズ
	SendData[2] = 0x02;		//命令コード H
	SendData[3] = 0x0F;		//命令コード L
	
	SendData[4] = (char)(di & 0xff);
	SendData[5] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	//500ms待つ
	secWait( 0.5 );

	//グラフィックを透明色に塗ります
	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x0A;		//全体サイズ
	SendData[2] = 0x01;		//命令コード H
	SendData[3] = 0x02;		//命令コード L
	SendData[4] = 0;
	SendData[5] = 0;
	SendData[6] = 0;
	SendData[7] = 0;
	SendData[8] = 1;
	SendData[9] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-2-10.画面のスリープを設定します:setSleep(0x0210)
//	setSleep(off(keep)/on)
//	0x03 0x06 0x0210 0xMM 0x06
//	0xMMの値
//		0x00: スリープしない。
//		0x01: スリープする。
//	戻りデータ
//	0x03 0x04 0xRX 0x06
//	0xRXの値
//		0x00: 通常終了
//		0x46: エラー(終了)
//**************************************************
mrb_value mrb_system_setSleep(mrb_state *mrb, mrb_value self)
{
int mode;

	mrb_get_args(mrb, "i", &mode);

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x06;		//全体サイズ
	SendData[2] = 0x02;		//命令コード H
	SendData[3] = 0x10;		//命令コード L
	
	SendData[4] = (char)(mode & 0xff);
	SendData[5] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}
	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// system.getAppPath()
// アプリケーションのパスを取得します
//**************************************************
mrb_value mrb_system_getAppPath(mrb_state *mrb, mrb_value self)
{
char	apppath[256];
int	i;

	strcpy( apppath, ExeFilename );
	int len = strlen( apppath );
	for( i=len-1; i>=0; i-- ){
		if( apppath[i]=='/' ){
			apppath[i] = 0;
			break;
		}
	}

	return mrb_str_new_cstr(mrb, apppath);
}

////**************************************************
//// system.getSec()
//// linuxの時間をus精度で取得します
////**************************************************
//int systemGetSec( lua_State *LuaLinkP )
//{
//	lua_settop(LuaLinkP, 0);			//スタックのクリア
//
//    struct timeval tv;
//    gettimeofday(&tv, NULL);
//	lua_Number lnum = tv.tv_sec + (double)tv.tv_usec*1e-6;
//
//	lua_pushnumber( LuaLinkP, lnum );
//
//	return( 1 );
//}

//**************************************************
// ライブラリを定義します
//**************************************************
void system_Init(mrb_state *mrb)
{
	struct RClass *systemModule = mrb_define_module(mrb, "System");

	mrb_define_module_function(mrb, systemModule, "exit", mrb_system_exit, ARGS_NONE());
	mrb_define_module_function(mrb, systemModule, "setrun", mrb_system_setrun, ARGS_REQ(1));
	mrb_define_module_function(mrb, systemModule, "version", mrb_system_version, ARGS_NONE());
	mrb_define_module_function(mrb, systemModule, "expCall", mrb_system_expCall, ARGS_REQ(1) | ARGS_OPT(2));
	mrb_define_module_function(mrb, systemModule, "impCallActionView", mrb_system_impCallActionView, ARGS_REQ(2));
	mrb_define_module_function(mrb, systemModule, "impCallActionSend", mrb_system_impCallActionSend, ARGS_REQ(1) | ARGS_OPT(1));
	mrb_define_module_function(mrb, systemModule, "getCardMnt", mrb_system_getCardMnt, ARGS_NONE());
	mrb_define_module_function(mrb, systemModule, "setScreen", mrb_system_setScreen, ARGS_REQ(1));
	mrb_define_module_function(mrb, systemModule, "setSleep", mrb_system_setSleep, ARGS_REQ(1));
	mrb_define_module_function(mrb, systemModule, "getAppPath", mrb_system_getAppPath, ARGS_NONE());

	//{"getSec", systemGetSec },
}
