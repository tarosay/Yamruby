//************************************************************************
// カーネル関連
//************************************************************************
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "mruby.h"
#include "mruby/string.h"
#include "mruby/variable.h"
#include "mruby/array.h"

#include "fvm.h"
#include "fvmExec.h"
#include "fvmClient.h"

extern int	SocketNumber;			//接続ソケット番号
extern char	SendData[];				//送信データ(512byte)
extern char VmFilename[];
extern volatile char ProgVer[];
extern char MountSdCardName[];
extern char ExeFilename[];

//struct RClass *kernelModule;

//**************************************************
// 2-8.toast(0x0006)
//	toast(本文,表示期間)
//	0x03 0xSS(全体サイズ) 0x0206 0xHN 0xHS(本文文字長) 0xHM～本文文字データ 0x06
//	0xHNの値
//		0x00: 短い
//		0x01: 長い
// トーストを表示します
//**************************************************
mrb_value mrb_kernel_toast(mrb_state *mrb, mrb_value self)
{
mrb_value strValue;
int tlen;
int i;
int t;

	int n = mrb_get_args(mrb, "S|i", &strValue, &t);
	char *str = RSTRING_PTR(strValue);

	LOGI( str );

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x04;		//全体サイズ
	SendData[2] = 0x02;		//命令コード H
	SendData[3] = 0x06;		//命令コード L

	if( n>1 ){
		SendData[4] = (char)(t & 0xff);
	}
	else{
		SendData[4] = 0x00;
	}

	tlen = strlen(str);
	SendData[5] = (char)(tlen & 0xff);
	for( i=0; i<tlen; i++ ){
		SendData[6+i] = str[i];
	}
	SendData[6+tlen] = 0x06;
	SendData[1] = 7+tlen;		//全体サイズ

	if( CommandSend( 1 )==0 ){
		
		VmFinishMes(mrb);
		//return mrb_nil_value();	//戻り値は無しですよ。
	}

	return mrb_nil_value();	//戻り値は無しですよ。
}

//**************************************************
// 2-5.touch(0x0003)
//	touch( [タッチ取得(1)] )
//  画面タッチ状態を取得します
//	0xFnの値
//		0x00: 座標とタッチの状態を返す
//		0x01: タッチダウンもしくはタッチ状態であれば返る
//		0x02: タッチアップもしくは離れている上体であれば返る
//**************************************************
mrb_value mrb_kernel_touch(mrb_state *mrb, mrb_value self)
{
int t;
int	ret;
char	recv[32];

	int n = mrb_get_args(mrb, "|i", &t);
	
	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x06;		//全体サイズ
	SendData[2] = 0x00;		//命令コード H
	SendData[3] = 0x03;		//命令コード L
	SendData[4] = 0x00;
	SendData[5] = 0x06;

	if( n==0 ){
		SendData[4] = 0x00;
	}
	else{
		SendData[4] = (char)t;
	}

	CommandSend( 0 );

	ret = 0;
	while(ret<1){
		ret = getRecvData( SocketNumber, recv, 10, 0, 2000000L );
		if( (SendData[4]==0 && ret<1) || (recv[6]==0x46) ){		//終了

			VmFinishMes(mrb);
			return mrb_nil_value();			//戻り値は無しですよ。
		}
	}

	mrb_value arv[3];

	arv[0] = mrb_fixnum_value( ((recv[2]<<8) & 0xff00) + (recv[3] & 0xff) );
	arv[1] = mrb_fixnum_value( ((recv[4]<<8) & 0xff00) + (recv[5] & 0xff) );
	arv[2] = mrb_fixnum_value( recv[6] & 0xff );

	return mrb_ary_new_from_values(mrb, 3, arv);
}

//**************************************************
//  描画色を指定します
//**************************************************
mrb_value mrb_kernel_color(mrb_state *mrb, mrb_value self)
{
int r,g,b,alpha;

	int n = mrb_get_args(mrb, "iii|i", &r, &g, &b, &alpha);

	if( n<4 ){
		alpha = 0xff;
	}
	
	mrb_int col = ((alpha<<24)&0xFF000000) + ((r<<16)&0xFF0000) + ((g<<8)&0xFF00) + (b&0xFF);

	return mrb_fixnum_value(col);
}

//**************************************************
// 2-0-B.キー入力を取得します: inkey(0x000B)
//	inkey( [キー取得(1)] )
//	0x03 0x06 0x000B 0xFn 0x06	→	受信 0x03 0x08 0xXXXX 0xAA 0xMMMM 0x06
//	0xFnの値
//		0x00: キーコードと押されている状態を返す
//		0x01: キーが押されていれば返る
//		0x02: キーが押されていなければ返る
//		0x03: キーが押されていない状態から押されて、さらに離されたら返る
//
//	戻り値	XXXX 押されたキーコード
//			AA   押されている状態
//				0x00: ACTION_DOWN キーが押されている
//				0x01: ACTION_UP   キーが押されていない
//				0x46: エラー発生させて処理を停止させる
//			MMMM メタキーの状態
//**************************************************
mrb_value mrb_kernel_inkey(mrb_state *mrb, mrb_value self)
{
int t;
int ret;
char recv[32];

	int n = mrb_get_args(mrb, "|i", &t);
	
	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x06;		//全体サイズ
	SendData[2] = 0x00;		//命令コード H
	SendData[3] = 0x0B;		//命令コード L
	SendData[5] = 0x06;

	if( n==0 ){
		SendData[4] = 0x00;
	}
	else{
		SendData[4] = (char)t;
	}

	CommandSend( 0 );

	ret = 0;
	while(ret<1){
		ret = getRecvData( SocketNumber, recv, 10, 0, 2000000L );
		if( (SendData[4]==0 && ret<1) || (recv[6]==0x46) ){		//終了

			VmFinishMes(mrb);
			return mrb_nil_value();			//戻り値は無しですよ。
		}
	}

	mrb_value arv[3];

	arv[0] = mrb_fixnum_value( ((recv[2]<<8) & 0xff00) + (recv[3] & 0xff));
	arv[1] = mrb_fixnum_value( recv[4] & 0xff );
	arv[2] = mrb_fixnum_value( ((recv[5]<<8) & 0xff00) + (recv[6] & 0xff));

	return mrb_ary_new_from_values(mrb, 3, arv);
}


//**************************************************
// 2-7.dialog(0x0205)
//	dialog(タイトル,本文[,ボタンの種類])
//	0x03 0xSS(全体サイズ) 0x0005 0xSB 0xTS(タイトル文字長) 0xTM～タイトル文字データ 0xHS(本文文字長) 0xHM～本文文字データ 0x06
//	0xSB: ボタンの種類
//		0x00: ボタンなし
//		0x01: OK
//		0x02: Yes/No
//		0x03: Yes/No/Cancel
//	戻りデータ
//	0x03 0x04(全体サイズ) 0xRX(OK/Cancel) 0x06
//	0xRXの値
//		0x00: Cancel
//		0x01: OK/Yes
//		0x02: No
//		0x46: エラー(終了)
//  ダイアログを出します
//**************************************************
mrb_value mrb_kernel_dialog(mrb_state *mrb, mrb_value self)
{
int		tlen, mlen;
int		btn;
int		ret;
char	recv[512];
int i;
mrb_value title, mes;

	int n = mrb_get_args(mrb, "SS|i", &title, &mes, &btn);
	//LOGE("RSTRING_PTR 1");
	char *str1 = RSTRING_PTR(title);
	//LOGE("RSTRING_PTR 2");
	char *str2 = RSTRING_PTR(mes);

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x04;		//全体サイズ
	SendData[2] = 0x02;		//命令コード H
	SendData[3] = 0x05;		//命令コード L

	SendData[4] = 0x00;		//ボタン設定

	tlen = strlen( str1 );
	SendData[5] = (char)(tlen & 0xff);
	for( i=0; i<tlen; i++ ){
		SendData[6+i] = str1[i];
	}

	mlen = strlen( str2 );
	SendData[6+tlen] = (char)(mlen & 0xff);
	for( i=0; i<mlen; i++ ){
		SendData[7+tlen+i] = str2[i];
	}
	SendData[7+tlen+mlen] = 0x06;
	SendData[1] = 8+tlen+mlen;		//全体サイズ

	if( n>=3 ){
		SendData[4] = (char)(btn & 0xff);
	}

	CommandSend( 0 );

	ret = 0;
	while(ret<1){
		ret = getRecvData( SocketNumber, recv, 511, 0, 2000000L );
		if( recv[2]==0x46 ){	//終了
			VmFinishMes(mrb);
			return mrb_nil_value();			//戻り値は無しですよ。
		}
	}

	return mrb_fixnum_value( (int)recv[2] );
}

//**************************************************
// 2-2-7.文字入力させます: editText(0x0207)
//	editText(タイトル[,文字初期値フラグ])
//	0x03 0xSS(全体サイズ) 0x0207 0xSF 0xTS(タイトル長) 0xTM～タイトルデータ 0x06
//	0xSFの値
//		0x00: 初期化する(デフォルト)
//		0x01: 初期化しない
//	戻りデータ
//	0x03 0xSS(全体サイズ) 0xRX(OK/Cancel) 0xHS(入力文字長) 0xHM(入力データ) 0x06
//	0xRXの値
//		0x00: Cancel
//		0x01: OK
//		0x46: エラー(終了)・・・javaはフラグで管理する
//**************************************************
mrb_value mrb_kernel_editText(mrb_state *mrb, mrb_value self)
{
char	recv[512];
int		tlen;
int		i;
int		t;
int		ret;
mrb_value title;

	int n = mrb_get_args(mrb, "S|i", &title, &t);
	char *str = RSTRING_PTR(title);

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x04;		//全体サイズ
	SendData[2] = 0x02;		//命令コード H
	SendData[3] = 0x07;		//命令コード L
	SendData[4] = 0;

	if( n>1 ){
		if( t==1 ){	SendData[4] = 0x01;	}
	}

	tlen = strlen( str );
	SendData[5] = (char)(tlen & 0xff);
	for( i=0; i<tlen; i++ ){
		SendData[6+i] = str[i];
	}
	SendData[6+tlen] = 0x06;
	SendData[1] = 7+tlen;		//全体サイズ

	CommandSend( 0 );

	ret = 0;
	while(ret<1){
		ret = getRecvData( SocketNumber, recv, 511, 0, 2000000L );
		if( recv[2]==0x46 ){	//終了
			VmFinishMes(mrb);
			return mrb_nil_value();			//戻り値は無しですよ。
		}
	}

	str = recv;
	str += 4;
	recv[ recv[1]-1 ] = 0;
	t = (int)recv[2];

	mrb_value arv[2];

	arv[0] = mrb_str_new_cstr(mrb, str);
	arv[1] = mrb_fixnum_value((int)recv[2]);

	return mrb_ary_new_from_values(mrb, 2, arv);
}

//**************************************************
// 2-2-E.文字を初期化します: editsetText(0x020E)
//	editsetText(初期化文字)
//	0x03 0xSS(全体サイズ) 0x020E 0xTS(文字長) 0xTM～タイトルデータ 0x06
//**************************************************
mrb_value mrb_kernel_editsetText(mrb_state *mrb, mrb_value self)
{
int		tlen;
int		i;
mrb_value text;

	mrb_get_args(mrb, "S", &text);
	char *str = RSTRING_PTR(text);

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x04;		//全体サイズ
	SendData[2] = 0x02;		//命令コード H
	SendData[3] = 0x0E;		//命令コード L

	tlen = strlen( str );
	SendData[4] = (char)(tlen & 0xff);
	for( i=0; i<tlen; i++ ){
		SendData[5+i] = str[i];
	}
	SendData[5+tlen] = 0x06;
	SendData[1] = 6+tlen;		//全体サイズ

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();	//戻り値は無しですよ。
}

//**************************************************
// ライブラリを定義します
//**************************************************
void kernel_Init(mrb_state *mrb)
{
	mrb_define_method(mrb, mrb->kernel_module, "toast", mrb_kernel_toast, ARGS_REQ(1) | ARGS_OPT(1));
	mrb_define_method(mrb, mrb->kernel_module, "touch", mrb_kernel_touch, ARGS_OPT(1));
	mrb_define_method(mrb, mrb->kernel_module, "color", mrb_kernel_color, ARGS_REQ(3) | ARGS_OPT(1));
	mrb_define_method(mrb, mrb->kernel_module, "inkey", mrb_kernel_inkey, ARGS_OPT(1));
	mrb_define_method(mrb, mrb->kernel_module, "dialog", mrb_kernel_dialog, ARGS_REQ(2) | ARGS_OPT(1));
	mrb_define_method(mrb, mrb->kernel_module, "editText", mrb_kernel_editText, ARGS_REQ(1) | ARGS_OPT(1));
	mrb_define_method(mrb, mrb->kernel_module, "editsetText", mrb_kernel_editsetText, ARGS_REQ(1));
}
