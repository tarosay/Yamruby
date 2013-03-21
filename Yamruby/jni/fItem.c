//************************************************************************
// アイテム表示関連
//************************************************************************
#include <string.h>

#include "mruby.h"
#include "mruby/string.h"
#include "mruby/array.h"

#include "fvm.h"
#include "fvmExec.h"
#include "fvmClient.h"

extern int	SocketNumber;			//接続ソケット番号
extern char	SendData[];				//送信データ(512byte)
//struct RClass *itemModule;

//**************************************************
//2-10.item.add(0x0008)
//	item.add(文字[,フラグ])
//	0x03 0xSS(全体サイズ) 0x0008 0xFG 0xTS(文字長) 0xTM～タイトルデータ 0x06
//	oxFGの値
//		0x00: false
//		0x01: true;
//	アイテムを追加します
//**************************************************
mrb_value mrb_item_add(mrb_state *mrb, mrb_value self)
{
int tlen;
int i;
int flg;
mrb_value text;
char	*str;

	int n = mrb_get_args(mrb, "S|i", &text, &flg);
	//LOGE("RSTRING_PTR 1");
	str = RSTRING_PTR(text);
	//LOGE("RSTRING_PTR 2");

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x04;		//全体サイズ
	SendData[2] = 0x03;		//命令コード H
	SendData[3] = 0x08;		//命令コード L
	SendData[4] = 0x00;		//フラグ設定

	tlen = strlen( str );
	SendData[5] = (char)(tlen & 0xff);
	for( i=0; i<tlen; i++ ){
		SendData[6+i] = str[i];
	}

	SendData[7+tlen] = 0x06;
	SendData[1] = 8+tlen;		//全体サイズ

	if( n>=2 ){
		SendData[4] = (char)(flg & 0xff);
	}

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-11.item.clear(0x0009)
//	item.clear()
//	0x03 0x05(全体サイズ) 0x0009 0x06
//	アイテムをクリアします
//**************************************************
mrb_value mrb_item_clear(mrb_state *mrb, mrb_value self)
{
	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x05;		//全体サイズ
	SendData[2] = 0x03;		//命令コード H
	SendData[3] = 0x09;		//命令コード L
	SendData[4] = 0x06;		//終了コード

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-12.item.list(0x000A)
//	item.list(文字)
//	0x03 0x05(全体サイズ) 0x000A 0xTS(文字長) 0xTM～文字データ 0x06
//	戻りデータ
//	0x03 0x05(全体サイズ) 0xAC 0xNm(番号0～255) 0x06
//	0xACの値
//		0x00: キャンセル
//		0x01: 選択された
//		0x46: エラー(終了)
//	アイテムを選択します
// キャンセルは 0: 後は番号順
//**************************************************
mrb_value mrb_item_list(mrb_state *mrb, mrb_value self)
{
int		ret;
char	recv[512];
int		tlen;
int		i;
mrb_value text;
char	*str;

	mrb_get_args(mrb, "S", &text);
	str = RSTRING_PTR(text);

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x04;		//全体サイズ
	SendData[2] = 0x03;		//命令コード H
	SendData[3] = 0x0A;		//命令コード L

	tlen = strlen( str );
	SendData[4] = (char)(tlen & 0xff);
	for( i=0; i<tlen; i++ ){
		SendData[5+i] = str[i];
	}

	SendData[5+tlen] = 0x06;
	SendData[1] = 6+tlen;		//全体サイズ

	CommandSend( 0 );
	
	ret = 0;
	while(ret<1){
		ret = getRecvData( SocketNumber, recv, 511, 0, 2000000L );
		if( recv[2]==0x46 ){	//終了
			VmFinishMes(mrb);
			return mrb_nil_value();			//戻り値は無しですよ。
		}
	}

	if( recv[2]==1 ){
		ret = (int)recv[3];
	}
	else{
		ret = 0;
	}

	return mrb_fixnum_value( ret );
}

//**************************************************
//2-13.item.radio(0x000B)
//	item.radio(文字,初期選択位置)
//	0x03 0xSS(全体サイズ) 0x000B 0xPJ(初期選択位置1～) 0xTS(文字長) 0xTM～文字データ 0x06
//	戻りデータ
//	0x03 0x05(全体サイズ) 0xAC 0xNm(番号0～255) 0x06
//	0xACの値
//		0x00: キャンセル
//		0x01: 選択された
//		0x46: エラー(終了)
//	アイテムをラジオボタンで選択します
// キャンセルは 0: 後は番号順
//**************************************************
mrb_value mrb_item_radio(mrb_state *mrb, mrb_value self)
{
int		ret;
char	recv[512];
int		tlen;
int		i;
int		sp;
mrb_value text;
char	*str;

	mrb_get_args(mrb, "Si", &text, &sp);
	str = RSTRING_PTR(text);

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x04;		//全体サイズ
	SendData[2] = 0x03;		//命令コード H
	SendData[3] = 0x0B;		//命令コード L

	SendData[4] = (char)(sp & 0xff);	//初期選択位置
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
	
	if( recv[2]==1 ){
		ret = (int)recv[3];
	}
	else{
		ret = 0;
	}

	return mrb_fixnum_value( ret );
}

//**************************************************
//2-14.item.check(0x000C)
//	item.check(文字)
//	0x03 0xSS(全体サイズ) 0x000C 0xTS(文字長) 0xTM～文字データ 0x06
//	戻りデータ
//	0x03 0x14(全体サイズ) 0xAC 0xXX*8バイト 0x06
//	0xACの値
//		0x00: キャンセル
//		0x01: 選択された
//		0x46: エラー(終了)
//	0xXXの先頭順にビット単位でチェックが入っていれば 1、無ければ 0となっている
//	アイテムをチェックボタンで複数選択します
// キャンセルは 0: 後はチェックが入っているところのビットが1になっている選択肢は64個まで、戻り値は4つ0～65535までの値が4つ戻る
//**************************************************
mrb_value mrb_item_check(mrb_state *mrb, mrb_value self)
{
int		ret;
char	recv[512];
int		tlen;
int		i;
mrb_value text;
char	*str;

	mrb_get_args(mrb, "S", &text);
	str = RSTRING_PTR(text);

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x04;		//全体サイズ
	SendData[2] = 0x03;		//命令コード H
	SendData[3] = 0x0C;		//命令コード L

	tlen = strlen( str );
	SendData[4] = (char)(tlen & 0xff);
	for( i=0; i<tlen; i++ ){
		SendData[5+i] = str[i];
	}

	SendData[5+tlen] = 0x06;
	SendData[1] = 6+tlen;		//全体サイズ

	CommandSend( 0 );

	ret = 0;
	while(ret<1){
		ret = getRecvData( SocketNumber, recv, 511, 0, 2000000L );
		if( recv[2]==0x46 ){	//終了
			VmFinishMes(mrb);
			return mrb_nil_value();			//戻り値は無しですよ。
		}
	}

	mrb_value arv[5];

	ret = (int)recv[2];
	if( ret==1 ){
		arv[0] = mrb_fixnum_value(((((long)recv[3]) << 8) & 0xff00) + ((long)recv[4] & 0xff));
		arv[1] = mrb_fixnum_value(((((long)recv[5]) << 8) & 0xff00) + ((long)recv[6] & 0xff));
		arv[2] = mrb_fixnum_value(((((long)recv[7]) << 8) & 0xff00) + ((long)recv[8] & 0xff));
		arv[3] = mrb_fixnum_value(((((long)recv[9]) << 8) & 0xff00) + ((long)recv[10] & 0xff));
	}
	else{
		arv[0] = mrb_fixnum_value( 0 );
		arv[1] = mrb_fixnum_value( 0 );
		arv[2] = mrb_fixnum_value( 0 );
		arv[3] = mrb_fixnum_value( 0 );
	}

	arv[4] = mrb_fixnum_value( ret );

	return mrb_ary_new_from_values(mrb, 5, arv);
}

//**************************************************
// ライブラリを定義します
//**************************************************
void item_Init(mrb_state *mrb)
{
	struct RClass *itemModule = mrb_define_module(mrb, "Item");

	mrb_define_module_function(mrb, itemModule, "add", mrb_item_add, ARGS_REQ(1) | ARGS_OPT(1));
	mrb_define_module_function(mrb, itemModule, "clear", mrb_item_clear, ARGS_NONE());
	mrb_define_module_function(mrb, itemModule, "list", mrb_item_list, ARGS_REQ(1));
	mrb_define_module_function(mrb, itemModule, "radio", mrb_item_radio, ARGS_REQ(2));
	mrb_define_module_function(mrb, itemModule, "check", mrb_item_check, ARGS_REQ(1));
}
