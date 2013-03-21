//************************************************************************
// グラフィック関連
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

//struct RClass *canvasModule;

//**************************************************
// putgもgetgも引数が全く同じなので、同じルーチンを使う
//**************************************************
void putgetSub(mrb_state *mrb)
{
int x0, y0, x1, y1;
int mx0, my0, mx1, my1;

	mrb_get_args(mrb, "iiiiiiii", &x0, &y0, &x1, &y1, &mx0, &my0, &mx1, &my1);

	SendData[4] = (char)((x0>>8) & 0xff);
	SendData[5] = (char)(x0 & 0xff);
	SendData[6] = (char)((y0>>8) & 0xff);
	SendData[7] = (char)(y0 & 0xff);
	SendData[8] = (char)((x1>>8) & 0xff);
	SendData[9] = (char)(x1 & 0xff);
	SendData[10] = (char)((y1>>8) & 0xff);
	SendData[11] = (char)(y1 & 0xff);
	SendData[12] = (char)((mx0>>8) & 0xff);
	SendData[13] = (char)(mx0 & 0xff);
	SendData[14] = (char)((my0>>8) & 0xff);
	SendData[15] = (char)(my0 & 0xff);
	SendData[16] = (char)((mx1>>8) & 0xff);
	SendData[17] = (char)(mx1 & 0xff);
	SendData[18] = (char)((my1>>8) & 0xff);
	SendData[19] = (char)(my1 & 0xff);
	SendData[20] = 0x06;
}

//**************************************************
// 2-4-0.ワーク画面の画像をメイン画面に表示します: canvas.putg(0x0400)
//	canvas.putg( 画面表示エリアLeftTopX座標(2), 画面表示エリアLeftTopY座標(2)
//				, 画面表示エリアRightBottomX座標(2), 画面表示エリアRightBottomY座標(2)
//				,ワークLeftTopX座標(2), ワークLeftTopY座標(2)
//				, ワークRightBottomX座標(2), ワークRightBottomY座標(2) )
//	画面表示エリアとワークの指定範囲が異なる場合は、自動的に扁平します。
//	0x03 0x15 0x0400 0xXXX0 0xYYY0 0xXXX1 0xYYY1 0xMMX0 0xMMY0 0xMMX1 0xMMY1 0x06
//**************************************************
mrb_value mrb_canvas_putg(mrb_state *mrb, mrb_value self)
{
	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x15;			//全体サイズ
	SendData[2] = 0x04;			//命令コード H
	SendData[3] = 0x00;			//命令コード L

	putgetSub( mrb );	//SendData[]にセットする関数を呼び出す

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
//  文字を表示のサブルーチン
//**************************************************
void drawtextSub(mrb_state *mrb, int codeL )
{
int x,y,size,fcol,bcol;
int		mlen;
int		i;
mrb_value text;
char	*str;

	int n = mrb_get_args(mrb, "Siiii|i", &text, &x, &y, &size, &fcol, &bcol);
	str = RSTRING_PTR(text);

	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x04;			//全体サイズ
	SendData[2] = 0x01;			//命令コード H
	SendData[3] = (char)codeL;	//命令コード L

	SendData[4] = (char)((x>>8) & 0xff);
	SendData[5] = (char)(x & 0xff);
	SendData[6] = (char)((y>>8) & 0xff);
	SendData[7] = (char)(y & 0xff);
	SendData[8] = (char)((size>>8) & 0xff);
	SendData[9] = (char)(size & 0xff);

	unsigned long col = fcol;
	SendData[10] = (char)((col>>24) & 0xff);
	SendData[11] = (char)((col>>16) & 0xff);
	SendData[12] = (char)((col>>8) & 0xff);
	SendData[13] = (char)(col & 0xff);

	mlen = strlen( str );
	SendData[14] = (char)(mlen & 0xff);
	for( i=0; i<mlen; i++ ){
		SendData[15+i] = str[i];
	}
	SendData[15+mlen] = 0x06;
	SendData[1] = 16 + mlen;

	if( n>5 ){
		SendData[15+mlen] = 0x01;
		col = (unsigned long)bcol;
		SendData[16+mlen] = (char)((col>>24) & 0xff);
		SendData[17+mlen] = (char)((col>>16) & 0xff);
		SendData[18+mlen] = (char)((col>>8) & 0xff);
		SendData[19+mlen] = (char)(col & 0xff);
		SendData[20+mlen] = 0x06;
		SendData[1] = 21 + mlen;
	}
}

//**************************************************
// 2-1-3.文字を描きます(putflushが必要): canvas.putText(0x0103)
//	canvas.putText( 文字列(255), X座標(2), Y座標(2), 文字サイズ(2), 文字色(4)[,背景色(4)] )
//	0x03 0xSS(全体サイズ) 0x0103 0xXXXX 0xYYYY 0xFFFF 0xCCCCCCCC 0xMS(文字長) 0xMM～文字データ 0x06
//	0x03 0xSS(全体サイズ) 0x0103 0xXXXX 0xYYYY 0xFFFF 0xCCCCCCCC 0xMS(文字長) 0xMM～文字データ 0xBg 0xBBBBBBBB 0x06
//	背景処理があるときは、文字データの後に0xBg(==1で背景処理あり)と背景色データが付きます。
//**************************************************
mrb_value mrb_canvas_putText(mrb_state *mrb, mrb_value self)
{
	//文字データをSendData[]にセットします
	drawtextSub( mrb, 3 );

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// テキストボックスのサブルーチン
//**************************************************
mrb_value SubTextBox(mrb_state *mrb, int codeL)
{
int x, y, size, fcol, w, bcol;
int		mlen;
int		i;
mrb_value text;
char	*str;

	int n = mrb_get_args(mrb, "Siiiii|i", &text, &x, &y, &size, &fcol, &w, &bcol);
	str = RSTRING_PTR(text);

	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x04;			//全体サイズ
	SendData[2] = 0x01;			//命令コード H
	SendData[3] = (char)codeL;			//命令コード L

	SendData[4] = (char)((x>>8) & 0xff);
	SendData[5] = (char)(x & 0xff);
	SendData[6] = (char)((y>>8) & 0xff);
	SendData[7] = (char)(y & 0xff);
	SendData[8] = (char)((size>>8) & 0xff);
	SendData[9] = (char)(size & 0xff);

	unsigned long col = (unsigned long)fcol;
	SendData[10] = (char)((col>>24) & 0xff);
	SendData[11] = (char)((col>>16) & 0xff);
	SendData[12] = (char)((col>>8) & 0xff);
	SendData[13] = (char)(col & 0xff);

	mlen = strlen( str );
	SendData[14] = (char)(mlen & 0xff);
	for( i=0; i<mlen; i++ ){
		SendData[15+i] = str[i];
	}

	SendData[15+mlen] = (char)((w>>8) & 0xff);
	SendData[16+mlen] = (char)(w & 0xff);

	SendData[17+mlen] = 0x06;
	SendData[1] = 18 + mlen;

	if( n>6 ){
		SendData[17+mlen] = 0x01;
		col = (unsigned long)bcol;
		SendData[18+mlen] = (char)((col>>24) & 0xff);
		SendData[19+mlen] = (char)((col>>16) & 0xff);
		SendData[20+mlen] = (char)((col>>8) & 0xff);
		SendData[21+mlen] = (char)(col & 0xff);
		SendData[22+mlen] = 0x06;
		SendData[1] = 23 + mlen;
	}

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_fixnum_value((mrb_int)SendData[4]);
}

//**************************************************
// 2-1-8.指定範囲で改行して文字を描きます(putflushが必要): canvas.putTextBox(0x0108)
//	canvas.putTextBox( 文字列(255), X座標(2), Y座標(2), 文字サイズ(2), 文字色(4), 描画幅(2)[,背景色(4)] )
//	0x03 0xSS(全体サイズ) 0x0108 0xXXXX 0xYYYY 0xFFFF 0xCCCCCCCC 0xMS(文字長) 0xMM～文字データ 0xHHHH 0x06
//	0x03 0xSS(全体サイズ) 0x0108 0xXXXX 0xYYYY 0xFFFF 0xCCCCCCCC 0xMS(文字長) 0xMM～文字データ 0xHHHH 0xBg 0xBBBBBBBB 0x06
//	背景処理があるときは、文字データの後に0xBg(==1で背景処理あり)と背景色データが付きます。
//	戻りデータ
//	0x03 0x06 0x00 0x00 0xGG(表示行数) 0x06
//	0xGGの値: 表示した行数(1～)
//**************************************************
mrb_value mrb_canvas_putTextBox(mrb_state *mrb, mrb_value self)
{
	return( SubTextBox( mrb, 8 ) );
}

//**************************************************
// 文字を回転させて表示するサブルーチン
//**************************************************
int SubTextRotate(mrb_state *mrb, int codeL)
{
int x, y, angle, size, fcol, bcol;
int		mlen;
int		i;
mrb_value text;
char	*str;

	int n = mrb_get_args(mrb, "Siiiii|i", &text, &x, &y, &angle, &size, &fcol, &bcol);
	str = RSTRING_PTR(text);

	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x04;			//全体サイズ
	SendData[2] = 0x01;			//命令コード H
	SendData[3] = (char)codeL;			//命令コード L

	SendData[4] = (char)((x>>8) & 0xff);
	SendData[5] = (char)(x & 0xff);
	SendData[6] = (char)((y>>8) & 0xff);
	SendData[7] = (char)(y & 0xff);
	SendData[8] = (char)((angle>>8) & 0xff);
	SendData[9] = (char)(angle & 0xff);
	SendData[10] = (char)((size>>8) & 0xff);
	SendData[11] = (char)(size & 0xff);

	unsigned long col = (unsigned long)fcol;
	SendData[12] = (char)((col>>24) & 0xff);
	SendData[13] = (char)((col>>16) & 0xff);
	SendData[14] = (char)((col>>8) & 0xff);
	SendData[15] = (char)(col & 0xff);

	mlen = strlen( str );
	SendData[16] = (char)(mlen & 0xff);
	for( i=0; i<mlen; i++ ){
		SendData[17+i] = str[i];
	}

	SendData[17+mlen] = 0x06;
	SendData[1] = 18 + mlen;

	if( n>6 ){
		SendData[17+mlen] = 0x01;
		col = (unsigned long)bcol;
		SendData[18+mlen] = (char)((col>>24) & 0xff);
		SendData[19+mlen] = (char)((col>>16) & 0xff);
		SendData[20+mlen] = (char)((col>>8) & 0xff);
		SendData[21+mlen] = (char)(col & 0xff);
		SendData[22+mlen] = 0x06;
		SendData[1] = 23 + mlen;
	}

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return 1;
	}
	return 0;		//戻り値は無しですよ。
}

//**************************************************
// 2-1-9.文字を回転させて描きます(putflushが必要): canvas.putTextRotate(0x0109)
//	canvas.putTextRotate( 文字列, 中心X座標, 中心Y座標, 回転角度, 文字サイズ, 文字色 [,背景色] )
//	0x03 0xSS(全体サイズ) 0x0109 0xXXXX 0xYYYY oxRRRR 0xFFFF 0xCCCCCCCC 0xMS(文字長) 0xMM～文字データ 0x06
//	0x03 0xSS(全体サイズ) 0x0109 0xXXXX 0xYYYY 0xFFFF 0xCCCCCCCC 0xMS(文字長) 0xMM～文字データ 0xBg 0xBBBBBBBB 0x06
//	背景処理があるときは、文字データの後に0xBg(==1で背景処理あり)と背景色データが付きます。
//**************************************************
mrb_value mrb_canvas_putTextRotate(mrb_state *mrb, mrb_value self)
{
	SubTextRotate( mrb, 9 );
	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-1-B.センタリングして文字を描きます(putflushが必要): canvas.putTextCenter(0x010B)
//	canvas.putTextCenter( 文字列(255), X座標(2), Y座標(2), 文字サイズ(2), 文字色(4)[,背景色(4)] )
//	0x03 0xSS(全体サイズ) 0x010B 0xXXXX 0xYYYY 0xFFFF 0xCCCCCCCC 0xMS(文字長) 0xMM～文字データ 0x06
//	0x03 0xSS(全体サイズ) 0x010B 0xXXXX 0xYYYY 0xFFFF 0xCCCCCCCC 0xMS(文字長) 0xMM～文字データ 0xBg 0xBBBBBBBB 0x06
//	背景処理があるときは、文字データの後に0xBg(==1で背景処理あり)と背景色データが付きます。
//**************************************************
mrb_value mrb_canvas_putTextCenter(mrb_state *mrb, mrb_value self)
{
	//文字データをSendData[]にセットします
	drawtextSub( mrb, 0x0B );

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
//  画面を指定色で塗りつぶしのサブルーチン
//**************************************************
mrb_value SubCls( mrb_state *mrb, int codeH, int codeL )
{
int ccol;

	int n = mrb_get_args(mrb, "|i", &ccol);

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x0A;		//全体サイズ
	SendData[2] = (char)codeH;		//命令コード H
	SendData[3] = (char)codeL;		//命令コード L

	if( n<1 ){
		SendData[4] = 0;
		SendData[5] = 0;
		SendData[6] = 0;
		SendData[7] = 0;
		SendData[8] = 1;
    }
    else{
		unsigned long col = (unsigned long)ccol;
		SendData[4] = (char)((col>>24) & 0xff);
		SendData[5] = (char)((col>>16) & 0xff);
		SendData[6] = (char)((col>>8) & 0xff);
		SendData[7] = (char)(col & 0xff);
		SendData[8] = 0;
	}
	SendData[9] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
//2-0-7.画面を塗りつぶします(putflushが必要): canvas.putCls(0x0007)
//	canvas.putCls( 背景色(4) )
//	0x03 0x0A(全体サイズ) 0x0007 0xCCCCCCCC 0xFL 0x06
//	引数省略時は、メイン画面を透明色でクリアします(透明色を引きつめる)
//	0xFLの値
//		0:ノーマル
//		1:透明色で初期化する
//**************************************************
mrb_value mrb_canvas_putCls(mrb_state *mrb, mrb_value self)
{
	return ( SubCls( mrb, 0, 7 ) );
}

//**************************************************
// 2-4-3.メイン画面に書き込みます: canvas.flush(0x0403)
//	canvas.flush([x0,y0,x1,y1])
//	0x03 0x0E 0x0403 0xFL 0xXXX0 0xYYY0 0xXXX1 0xYYY1 0x06
//	0xFLの値
//		0x00: 画面全体をフラッシュする
//		0x01: 指定エリアをフラッシュする
//	0xXXX0～0xYYY1はフラッシュする範囲を示します。(x0,y0)-(x1,y1)のエリアをフラッシュします。
//**************************************************
mrb_value mrb_canvas_putflush(mrb_state *mrb, mrb_value self)
{
int x0,y0,x1,y1;

	int n = mrb_get_args(mrb, "|iiii", &x0, &y0, &x1, &y1);

	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x0E;			//全体サイズ
	SendData[2] = 0x04;			//命令コード H
	SendData[3] = 0x03;			//命令コード L

    if( n==0 ){
		SendData[4] = 0;
		SendData[5] = 0;
		SendData[6] = 0;
		SendData[7] = 0;
		SendData[8] = 0;
		SendData[9] = 0;
		SendData[10] = 0;
		SendData[11] = 0;
		SendData[12] = 0;
    }
	else{
		SendData[4] = 0x01;
		SendData[5] = (char)((x0>>8) & 0xff);
		SendData[6] = (char)(x0 & 0xff);
		SendData[7] = (char)((y0>>8) & 0xff);
		SendData[8] = (char)(y0 & 0xff);
		SendData[9] = (char)((x1>>8) & 0xff);
		SendData[10] = (char)(x1 & 0xff);
		SendData[11] = (char)((y1>>8) & 0xff);
		SendData[12] = (char)(y1 & 0xff);
	}
	
	SendData[13] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-4-9.ワーク画面をSurfaceViewにフラッシュします: canvas.workflush(0x0409)
//	canvas.workflush(x0,y0,x1,y1)
//	0x03 0x0E 0x0409 0xFL 0xXXX0 0xYYY0 0xXXX1 0xYYY1 0x06
//	0xFLの値
//		特に設定はない
//	0xXXX0～0xYYY1はフラッシュする範囲を示します。ワークエリアの(x0,y0)-(x1,y1)のエリアをSurfaceView全体にフラッシュします。
//**************************************************
mrb_value mrb_canvas_workflush(mrb_state *mrb, mrb_value self)
{
int x0,y0,x1,y1;

	mrb_get_args(mrb, "iiii", &x0, &y0, &x1, &y1);

	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x0E;			//全体サイズ
	SendData[2] = 0x04;			//命令コード H
	SendData[3] = 0x09;			//命令コード L
	SendData[4] = 0x00;

	SendData[5] = (char)((x0>>8) & 0xff);
	SendData[6] = (char)(x0 & 0xff);
	SendData[7] = (char)((y0>>8) & 0xff);
	SendData[8] = (char)(y0 & 0xff);
	SendData[9] = (char)((x1>>8) & 0xff);
	SendData[10] = (char)(x1 & 0xff);
	SendData[11] = (char)((y1>>8) & 0xff);
	SendData[12] = (char)(y1 & 0xff);

	SendData[13] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-4-B.ワーク画面のある場所からある場所をコピーします: canvas.putWork(0x040B)
//	canvas.putWork(xs0,ys0,xs1,ys1,xd0,yd0,xd1,yd1)
//	0x03 0x15 0x040B 0xXS00 0xYS00 0xXS11 0xYS11 0xXD00 0xYD00 0xXD11 0xYD11 0x06
//	戻りデータ
//	0x03 0x06 0x00 0x00 0xAC(結果) 0x06
//**************************************************
mrb_value mrb_canvas_putWork(mrb_state *mrb, mrb_value self)
{

	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x15;			//全体サイズ
	SendData[2] = 0x04;			//命令コード H
	SendData[3] = 0x0B;			//命令コード L

	putgetSub( mrb );	//SendData[]にセットする関数を呼び出す

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
//2-1-1.文字を描きます: canvas.drawText(0x0101)
//	canvas.drawText( 文字列(255), X座標(2), Y座標(2), 文字サイズ(2), 文字色(4)[,背景色(4)] )
//	0x03 0xSS(全体サイズ) 0x0101 0xXXXX 0xYYYY 0xFFFF 0xCCCCCCCC 0xMS(文字長) 0xMM～文字データ 0x06
//	0x03 0xSS(全体サイズ) 0x0101 0xXXXX 0xYYYY 0xFFFF 0xCCCCCCCC 0xMS(文字長) 0xMM～文字データ 0xBg 0xBBBBBBBB 0x06
//	背景処理があるときは、文字データの後に0xBg(==1で背景処理あり)と背景色データが付きます。
//**************************************************
mrb_value mrb_canvas_drawText(mrb_state *mrb, mrb_value self)
{
	//文字データをSendData[]にセットします
	drawtextSub( mrb, 1 );

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
//画面に線を引くサブルーチン
//**************************************************
mrb_value SubLine( mrb_state *mrb, int codeL )
{
int x0, y0, x1, y1, fcol;
unsigned long	col;

	mrb_get_args(mrb, "iiiii", &x0, &y0, &x1, &y1, &fcol);

	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x11;			//全体サイズ
	SendData[2] = 0x00;			//命令コード H
	SendData[3] = (char)codeL;			//命令コード L

	SendData[4] = (char)((x0>>8) & 0xff);
	SendData[5] = (char)(x0 & 0xff);
	SendData[6] = (char)((y0>>8) & 0xff);
	SendData[7] = (char)(y0 & 0xff);
	SendData[8] = (char)((x1>>8) & 0xff);
	SendData[9] = (char)(x1 & 0xff);
	SendData[10] = (char)((y1>>8) & 0xff);
	SendData[11] = (char)(y1 & 0xff);

	col = (unsigned long)fcol;
	SendData[12] = (char)((col>>24) & 0xff);
	SendData[13] = (char)((col>>16) & 0xff);
	SendData[14] = (char)((col>>8) & 0xff);
	SendData[15] = (char)(col & 0xff);

	SendData[16] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
//2-0-4.画面に線を引きます: canvas.line(0x0004)
//	canvas.drawLine( startX, startY, stopX, stopY, 文字色)
//	0x03 0x11 0x0004 0xXXXX 0xYYYY 0xAAAA 0xBBBB 0xCCCCCCCC 0x06
//**************************************************
mrb_value mrb_canvas_drawLine(mrb_state *mrb, mrb_value self)
{
	return( SubLine( mrb, 4 ));
}

//**************************************************
// 画面にボックスを描くサブルーチン
//**************************************************
mrb_value SubRect( mrb_state *mrb, int codeL )
{
int x0, y0, x1, y1, fcol, mode;
unsigned long	col;
int 	tmp;

	int n= mrb_get_args(mrb, "iiiii|i", &x0, &y0, &x1, &y1, &fcol, &mode);

	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x12;			//全体サイズ
	SendData[2] = 0x00;			//命令コード H
	SendData[3] = (char)codeL;			//命令コード L

	if( x0>x1 ){
		tmp = x0;
		x0 = x1;
		x1 = tmp;
	}
	if( y0>y1 ){
		tmp = y0;
		y0 = y1;
		y1 = tmp;
	}
	SendData[4] = (char)((x0>>8) & 0xff);
	SendData[5] = (char)(x0 & 0xff);
	SendData[6] = (char)((y0>>8) & 0xff);
	SendData[7] = (char)(y0 & 0xff);
	SendData[8] = (char)((x1>>8) & 0xff);
	SendData[9] = (char)(x1 & 0xff);
	SendData[10] = (char)((y1>>8) & 0xff);
	SendData[11] = (char)(y1 & 0xff);

	col = (unsigned long)fcol;
	SendData[12] = (char)((col>>24) & 0xff);
	SendData[13] = (char)((col>>16) & 0xff);
	SendData[14] = (char)((col>>8) & 0xff);
	SendData[15] = (char)(col & 0xff);

	if( n>5 ){
		SendData[16] = (char)mode;
	}
	else{
		SendData[16] = 0;
	}

	SendData[17] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-0-5.画面にボックスを描きます: canvas.drawRect(0x0005)
//	canvas.drawRect( startX, startY, stopX, stopY, 色 [,塗りつぶし])
//	0x03 0x12 0x0005 0xXXXX 0xYYYY 0xAAAA 0xBBBB 0xCCCCCCCC 0xNN 0x06
//	0xNNの値
//		0: 枠だけ
//		1: 塗りつぶし
//**************************************************
mrb_value mrb_canvas_drawRect(mrb_state *mrb, mrb_value self)
{
	return( SubRect( mrb, 5 ) );
}

//**************************************************
//2-1-2.画面を塗りつぶします: canvas.drawCls(0x0102)
//	canvas.drawCls( [背景色(4)] )
//	0x03 0x0A(全体サイズ) 0x0102 0xCCCCCCCC 0xFL 0x06
//	引数省略時は、メイン画面を透明色でクリアします(透明色を引きつめる)
//	0xFLの値
//		0:ノーマル
//		1:透明色で初期化する
//**************************************************
mrb_value mrb_canvas_drawCls(mrb_state *mrb, mrb_value self)
{
	return ( SubCls( mrb, 1, 2 ) );
}

//**************************************************
// 画面に円を描くサブルーチン
//**************************************************
mrb_value SubCircle( mrb_state *mrb, int codeL )
{
int x, y, r, fcol, mode;
unsigned long	col;

	int n= mrb_get_args(mrb, "iiii|i", &x, &y, &r, &fcol, &mode);

	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x10;			//全体サイズ
	SendData[2] = 0x00;			//命令コード H
	SendData[3] = (char)codeL;			//命令コード L

	SendData[4] = (char)((x>>8) & 0xff);
	SendData[5] = (char)(x & 0xff);
	SendData[6] = (char)((y>>8) & 0xff);
	SendData[7] = (char)(y & 0xff);
	SendData[8] = (char)((r>>8) & 0xff);
	SendData[9] = (char)(r & 0xff);

	col = (unsigned long)fcol;
	SendData[10] = (char)((col>>24) & 0xff);
	SendData[11] = (char)((col>>16) & 0xff);
	SendData[12] = (char)((col>>8) & 0xff);
	SendData[13] = (char)(col & 0xff);

	if( n>4 ){
		SendData[14] = (char)mode;
	}
	else{
		SendData[14] = 0;
	}

	SendData[15] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-0-6.画面に円を描きます: canvas.drawCircle(0x0006)
//	canvas.drawCircle( CenterX, CenterY, RadiusX, 色 [,塗りつぶし])
//	0x03 0x10 0x0006 0xXXXX 0xYYYY 0xRRRR 0xCCCCCCCC 0xNN 0x06
//	0xNNの値
//		0: 枠だけ
//		1: 塗りつぶし
//**************************************************
mrb_value mrb_canvas_drawCircle(mrb_state *mrb, mrb_value self)
{
	return( SubCircle( mrb, 6 ) );
}

//**************************************************
// 2-4-6.ワーク画面の画像を回転させてメイン画面に表示します: canvas.putrotg(0x0406)
//	canvas.putrotg( 画面表示中心X座標(2), 画面表示エリア中心Y座標(2), 回転角度(2)
//				,ワークLeftTopX座標(2), ワークLeftTopY座標(2), ワークRightBottomX座標(2), ワークRightBottomY座標(2) )
//	0x03 0x13 0x0406 0xXXXX 0xYYYY 0xKKKK 0xMMX0 0xMMY0 0xMMX1 0xMMY1 0x06
//**************************************************
mrb_value mrb_canvas_putrotg(mrb_state *mrb, mrb_value self)
{
int x, y, angle, mx0, my0, mx1, my1;

	mrb_get_args(mrb, "iiiiiii", &x, &y, &angle, &mx0, &my0, &mx1, &my1);

	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x13;			//全体サイズ
	SendData[2] = 0x04;			//命令コード H
	SendData[3] = 0x06;			//命令コード L

	SendData[4] = (char)((x>>8) & 0xff);
	SendData[5] = (char)(x & 0xff);
	SendData[6] = (char)((y>>8) & 0xff);
	SendData[7] = (char)(y & 0xff);
	SendData[8] = (char)((angle>>8) & 0xff);
	SendData[9] = (char)(angle & 0xff);
	SendData[10] = (char)((mx0>>8) & 0xff);
	SendData[11] = (char)(mx0 & 0xff);
	SendData[12] = (char)((my0>>8) & 0xff);
	SendData[13] = (char)(my0 & 0xff);
	SendData[14] = (char)((mx1>>8) & 0xff);
	SendData[15] = (char)(mx1 & 0xff);
	SendData[16] = (char)((my1>>8) & 0xff);
	SendData[17] = (char)(my1 & 0xff);

	SendData[18] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-0-8.画面に線を引きます(putflushが必要): canvas.putLine(0x0008)
//	canvas.putLine( startX, startY, stopX, stopY, 色)
//	0x03 0x11 0x0008 0xXXXX 0xYYYY 0xAAAA 0xBBBB 0xCCCCCCCC 0x06
//**************************************************
mrb_value mrb_canvas_putLine(mrb_state *mrb, mrb_value self)
{
	return( SubLine( mrb, 8 ));
}

//**************************************************
// 2-0-9.画面にボックスを描きます(putflushが必要): canvas.putRect(0x0009)
//	canvas.putRect( startX, startY, stopX, stopY, 色 [,塗りつぶし])
//	0x03 0x12 0x0009 0xXXXX 0xYYYY 0xAAAA 0xBBBB 0xCCCCCCCC 0xNN 0x06
//	0xNNの値
//		0: 枠だけ
//		1: 塗りつぶし
//**************************************************
mrb_value mrb_canvas_putRect(mrb_state *mrb, mrb_value self)
{
	return( SubRect( mrb, 9 ) );
}

//**************************************************
// 2-0-A.画面に円を描きます(putflushが必要): canvas.putCircle(0x000A)
//	canvas.putCircle( CenterX, CenterY, RadiusX, 色 [,塗りつぶし])
//	0x03 0x10 0x000A 0xXXXX 0xYYYY 0xRRRR 0xCCCCCCCC 0xNN 0x06
//	0xNNの値
//		0: 枠だけ
//		1: 塗りつぶし
//**************************************************
mrb_value mrb_canvas_putCircle(mrb_state *mrb, mrb_value self)
{
	return( SubCircle( mrb, 0x0A ) );
}

//**************************************************
//2-1-5.センタリングして文字を描きます: canvas.drawTextCenter(0x0105)
//	canvas.drawText( 文字列(255), X座標(2), Y座標(2), 文字サイズ(2), 文字色(4)[,背景色(4)] )
//	0x03 0xSS(全体サイズ) 0x0105 0xXXXX 0xYYYY 0xFFFF 0xCCCCCCCC 0xMS(文字長) 0xMM～文字データ 0x06
//	0x03 0xSS(全体サイズ) 0x0105 0xXXXX 0xYYYY 0xFFFF 0xCCCCCCCC 0xMS(文字長) 0xMM～文字データ 0xBg 0xBBBBBBBB 0x06
//	背景処理があるときは、文字データの後に0xBg(==1で背景処理あり)と背景色データが付きます。
//  設定x座標を中心にして文字を表示します
//**************************************************
mrb_value mrb_canvas_drawTextCenter(mrb_state *mrb, mrb_value self)
{
	//文字データをSendData[]にセットします
	drawtextSub( mrb, 5 );

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-1-7.文字を回転させて描きます: canvas.drawTextRotate(0x0107)
//	canvas.drawTextRotate( 文字列, 中心X座標, 中心Y座標, 回転角度, 文字サイズ, 文字色 [,背景色] )
//	0x03 0xSS(全体サイズ) 0x0107 0xXXXX 0xYYYY oxRRRR 0xFFFF 0xCCCCCCCC 0xMS(文字長) 0xMM～文字データ 0x06
//	0x03 0xSS(全体サイズ) 0x0101 0xXXXX 0xYYYY 0xFFFF 0xCCCCCCCC 0xMS(文字長) 0xMM～文字データ 0xBg 0xBBBBBBBB 0x06
//	背景処理があるときは、文字データの後に0xBg(==1で背景処理あり)と背景色データが付きます。
//**************************************************
mrb_value mrb_canvas_drawTextRotate(mrb_state *mrb, mrb_value self)
{
	SubTextRotate( mrb, 7 );
	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-4-1.メイン画面の画像をワーク画面に取得します: canvas.getg(0x0401)
//	canvas.getg( 画面表示エリアLeftTopX座標(2), 画面表示エリアLeftTopY座標(2), 画面表示エリアRightBottomX座標(2), 画面表示エリアRightBottomY座標(2)
//				,ワークLeftTopX座標(2), ワークLeftTopY座標(2), ワークRightBottomX座標(2), ワークRightBottomY座標(2) )
//	画面表示エリアとワークの指定範囲が異なる場合は、自動的に扁平します。
//	0x03 0x15 0x0401 0xXXX0 0xYYY0 0xXXX1 0xYYY1 0xMMX0 0xMMY0 0xMMX1 0xMMY1 0x06
//**************************************************
mrb_value mrb_canvas_getg(mrb_state *mrb, mrb_value self)
{
	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x15;			//全体サイズ
	SendData[2] = 0x04;			//命令コード H
	SendData[3] = 0x01;			//命令コード L

	putgetSub( mrb );	//SendData[]にセットする関数を呼び出す

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 画面サイズの取得
//**************************************************
mrb_value mrb_canvas_getviewSize(mrb_state *mrb, mrb_value self)
{
	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x06;		//全体サイズ
	SendData[2] = 0x01;		//命令コード H
	SendData[3] = 0x04;		//命令コード L
	SendData[4] = 0x00;
	SendData[5] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}
	
	mrb_value arv[2];

	arv[0] = mrb_fixnum_value( ((SendData[2]<<8) & 0xff00) + (SendData[3] & 0xff) );
	arv[1] = mrb_fixnum_value( ((SendData[4]<<8) & 0xff00) + (SendData[5] & 0xff) );

	return mrb_ary_new_from_values(mrb, 2, arv);
}

//**************************************************
//2-1-6.指定範囲で改行して文字を描きます: canvas.drawTextBox(0x0106)
//	canvas.drawText( 文字列(255), X座標(2), Y座標(2), 文字サイズ(2), 文字色(4), 描画幅(2)[,背景色(4)] )
//	0x03 0xSS(全体サイズ) 0x0106 0xXXXX 0xYYYY 0xFFFF 0xCCCCCCCC 0xMS(文字長) 0xMM～文字データ 0xHHHH 0x06
//	0x03 0xSS(全体サイズ) 0x0106 0xXXXX 0xYYYY 0xFFFF 0xCCCCCCCC 0xMS(文字長) 0xMM～文字データ 0xHHHH 0xBg 0xBBBBBBBB 0x06
//	背景処理があるときは、文字データの後に0xBg(==1で背景処理あり)と背景色データが付きます。
//	戻りデータ
//	0x03 0x06 0x00 0x00 0xGG(表示行数) 0x06
//	0xGGの値: 表示した行数(1～)
//**************************************************
mrb_value mrb_canvas_drawTextBox(mrb_state *mrb, mrb_value self)
{
	return( SubTextBox( mrb, 6 ) );
}

//**************************************************
// loadBmpとsaveBmpのサブルーチン
//**************************************************
void drawLoadsaveSub( int x0, int y0, int x1, int y1, char *str)
{
int		mlen;
int		i;

	SendData[4] = (char)((x0>>8) & 0xff);
	SendData[5] = (char)(x0 & 0xff);
	SendData[6] = (char)((y0>>8) & 0xff);
	SendData[7] = (char)(y0 & 0xff);
	SendData[8] = (char)((x1>>8) & 0xff);
	SendData[9] = (char)(x1 & 0xff);
	SendData[10] = (char)((y1>>8) & 0xff);
	SendData[11] = (char)(y1 & 0xff);
	mlen = strlen( str );
	SendData[12] = (char)(mlen & 0xff);
	for( i=0; i<mlen; i++ ){
		SendData[13+i] = str[i];
	}
	SendData[13+mlen] = 0x06;
	SendData[1] = 14 + mlen;
}

//**************************************************
// 2-4-2.画像ファイルをワーク画面に取得します: canvas.loadBmp(0x0402)
//	canvas.loadBmp( 画像ファイル名, ワークLeftTopX座標(2), ワークLeftTopY座標(2), ワークRightBottomX座標(2), ワークRightBottomY座標(2) [,縮小回数] )
//	画像ファイルサイズとワークの指定範囲が異なる場合は、自動的に扁平します。0xNNは縮小回数です。省略時は1になります。
//	0x03 0xSS(全体サイズ) 0x0402 0xMMX0 0xMMY0 0xMMX1 0xMMY1 0xTS(文字長) 0xTM～ファイル名文字データ 0xNN 0x06
//	戻りデータ
//	0x03 0x06 0x00 0x00 0xAC(結果) 0x06
//	0xACの値
//		0x00: 成功
//		0x01: 失敗(関数は-1を返します)
//**************************************************
mrb_value mrb_canvas_loadBmp(mrb_state *mrb, mrb_value self)
{
int x0, y0, x1, y1, kai;
mrb_value text;
char	*str;

	int n = mrb_get_args(mrb, "Siiii|i", &text, &x0, &y0, &x1, &y1, &kai);
	str = RSTRING_PTR(text);

	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x04;			//全体サイズ
	SendData[2] = 0x04;			//命令コード H
	SendData[3] = 0x02;			//命令コード L

	drawLoadsaveSub( x0, y0, x1, y1, str );

	SendData[SendData[1]-1] = 0x01;
	if( n>5 ){
		SendData[SendData[1]-1] = (char)kai;
	}
	SendData[(int)SendData[1]] = 0x06;
	SendData[1] += 1;

	CommandSend( 0 );

	//ack待ち
	int ret = 0;
	int lenok = getRecvData( SocketNumber, SendData, 10, 0, 10000000L );

	if( lenok<1 || SendData[4]==1 ){
		//10sec待って何も返ってこなかったら
		ret = -1;
	}
	
	return mrb_fixnum_value( ret );
}

//**************************************************
// 2-4-4.ワーク画面の指定エリアをファイルに保存します: canvas.saveBmp(0x0404)
//	canvas.saveBmp( 画像ファイル名, ワークLeftTopX座標(2), ワークLeftTopY座標(2), ワークRightBottomX座標(2), ワークRightBottomY座標(2) )
//	画像ファイルサイズとワークの指定範囲が異なる場合は、自動的に扁平します。
//	0x03 0xSS(全体サイズ) 0x0404 0xMMX0 0xMMY0 0xMMX1 0xMMY1 0xTS(文字長) 0xTM～ファイル名文字データ 0x06
//	戻りデータ
//	0x03 0x06 0x00 0x00 0xAC(結果) 0x06
//	0xACの値
//		0x00: 成功
//		0x01: 失敗(関数は-1を返します)
//**************************************************
mrb_value mrb_canvas_saveBmp(mrb_state *mrb, mrb_value self)
{
int x0, y0, x1, y1;
mrb_value text;
char	*str;

	mrb_get_args(mrb, "Siiii", &text, &x0, &y0, &x1, &y1);
	str = RSTRING_PTR(text);

	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x04;			//全体サイズ
	SendData[2] = 0x04;			//命令コード H
	SendData[3] = 0x04;			//命令コード L

	drawLoadsaveSub( x0, y0, x1, y1, str );

	CommandSend( 0 );

	//ack待ち
	int ret = 0;
	int lenok = getRecvData( SocketNumber, SendData, 10, 0, 10000000L );

	if( lenok<1 || SendData[4]==1 ){
		//10sec待って何も返ってこなかったら
		ret = -1;
	}
	
	return mrb_fixnum_value( ret );
}

//**************************************************
// 2-4-5.ワーク画面をクリアします: canvas.workCls(0x0405)
//	canvas.workCls([Color])
//	0x03 0x09 0x0405 0xCCCCCCCC 0x06
//	色の指定が無い場合は、0でクリアします。
//**************************************************
mrb_value mrb_canvas_workCls(mrb_state *mrb, mrb_value self)
{
unsigned long	col;
int ccol;

	int n = mrb_get_args(mrb, "|i", &ccol);

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x09;		//全体サイズ
	SendData[2] = 0x04;		//命令コード H
	SendData[3] = 0x05;		//命令コード L

	if( n<1 ){
		SendData[4] = 0;
		SendData[5] = 0;
		SendData[6] = 0;
		SendData[7] = 0;
    }
	else{
		col = (unsigned long)ccol;
		SendData[4] = (char)((col>>24) & 0xff);
		SendData[5] = (char)((col>>16) & 0xff);
		SendData[6] = (char)((col>>8) & 0xff);
		SendData[7] = (char)(col & 0xff);
	}

	SendData[8] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-1-A.ピクセルの色を取得します: canvas.getColor(0x010A)
//	canvas.getColor( X座標, Y座標 )
//	0x03 0x09 0x010A 0xXXXX 0xYYYY 0x06
//	戻りデータ
//	0x03 0x08 0xRX 0xCCCCCCCC 0x06
//	0xRXの値
//		0x01: OK
//		0x46: エラー(終了)
//
//	0xCCCCCCCCの値
//		ピクセルの色(α値,R値,G値,B値)
//**************************************************
mrb_value mrb_canvas_getColor(mrb_state *mrb, mrb_value self)
{
int x,y;
int ret;
char recv[512];

	mrb_get_args(mrb, "ii", &x, &y);

	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x09;			//全体サイズ
	SendData[2] = 0x01;			//命令コード H
	SendData[3] = 0x0A;			//命令コード L

	SendData[4] = (char)((x>>8) & 0xff);
	SendData[5] = (char)(x & 0xff);
	SendData[6] = (char)((y>>8) & 0xff);
	SendData[7] = (char)(y & 0xff);

	SendData[8] = 0x06;

	CommandSend( 0 );

	ret = 0;
	while(ret<1){
		ret = getRecvData( SocketNumber, recv, 511, 0, 2000000L );
		if( recv[2]==0x46 ){	//終了
			VmFinishMes(mrb);
			return mrb_nil_value();			//戻り値は無しですよ。
		}
	}

	mrb_int cola = recv[3] & 0xff;
	mrb_int col = ((((long)recv[4])<<16) & 0xff0000)
				 + ((((long)recv[5])<<8) & 0xff00)
				 + ((long)recv[6] & 0xff);
		
	mrb_value arv[2];

	arv[0] = mrb_fixnum_value( col );
	arv[1] = mrb_fixnum_value( cola );

	return mrb_ary_new_from_values(mrb, 2, arv);
}

//**************************************************
// BMPの画面サイズ設定のサブルーチン
//**************************************************
mrb_value SubsetBmp( mrb_state *mrb, int codeL )
{
int w,h;

	mrb_get_args(mrb, "ii", &w, &h);

	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x09;			//全体サイズ
	SendData[2] = 0x04;			//命令コード H
	SendData[3] = (char)codeL;			//命令コード L

	SendData[4] = (char)((w>>8) & 0xff);
	SendData[5] = (char)(w & 0xff);

	SendData[6] = (char)((h>>8) & 0xff);
	SendData[7] = (char)(h & 0xff);

	SendData[8] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-4-7.ワーク画面のサイズを設定します: canvas.setWorkBmp(0x0407)
//	canvas.setWorkBmp(width,height)
//	0x03 0x09 0x0407 0xWWWW 0xHHHH 0x06
//**************************************************
mrb_value mrb_canvas_setWorkBmp(mrb_state *mrb, mrb_value self)
{
	return( SubsetBmp( mrb, 7 ));
}

//**************************************************
// 2-4-8.メイン画面のサイズを設定します: canvas.setMainBmp(0x0408)
//	canvas.setMainBmp(width,height)
//	0x03 0x09 0x0408 0xWWWW 0xHHHH 0x06
//**************************************************
mrb_value mrb_canvas_setMainBmp(mrb_state *mrb, mrb_value self)
{
	return( SubsetBmp( mrb, 8 ));
}

//**************************************************
// 画面を横向きに初期化します。
// メイン画面とワーク画面のサイズを初期化します
// -1:エラー
//**************************************************
int ResetBmp( void )
{
	//メイン画面のリセット
	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x09;			//全体サイズ
	SendData[2] = 0x04;			//命令コード H
	SendData[3] = 0x07;			//命令コード L
	SendData[4] = 0x00;
	SendData[5] = 0x00;
	SendData[6] = 0x00;
	SendData[7] = 0x00;
	SendData[8] = 0x06;

	//LOGE("MainBMP");
	if( CommandSend( 1 )==0 ){	return( -1 );	}

	//ワーク画面のリセット
	SendData[0] = 0x03;			//先頭コード
	SendData[1] = 0x09;			//全体サイズ
	SendData[2] = 0x04;			//命令コード H
	SendData[3] = 0x08;			//命令コード L
	SendData[4] = 0x00;
	SendData[5] = 0x00;
	SendData[6] = 0x00;
	SendData[7] = 0x00;
	SendData[8] = 0x06;

	//LOGE("WorkBMP");
	if( CommandSend( 1 )==0 ){	return( -1 );	}

	return( 0 );
}

//**************************************************
// 2-4-A.画像ファイルのサイズを取得します: canvas.getBmpSize(0x040A)
//	canvas.getBmpSize( 画像ファイル名 )
//	画像ファイルサイズとワークの指定範囲が異なる場合は、自動的に扁平します。0xNNは縮小回数です。省略時は1になります。
//	0x03 0xSS(全体サイズ) 0x040A 0xTS(文字長) 0xTM～ファイル名文字データ 0x06
//	戻りデータ
//	0x03 0x0E 0x00 0x00 0xAC(結果) 0xWWWWWWWW 0xHHHHHHHH 0x06
//	0xACの値
//		0x00: 成功
//		0x01: 失敗
//
//	0xWWWWWWWWの値
//		幅サイズ
//	0xHHHHHHHHの値
//		高さサイズ
//**************************************************
mrb_value mrb_canvas_getBmpSize(mrb_state *mrb, mrb_value self)
{
int	ret;
char recv[32];
int mlen;
int i;
mrb_value text;
char	*str;

	mrb_get_args(mrb, "S", &text);
	str = RSTRING_PTR(text);

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x01;		//全体サイズ
	SendData[2] = 0x04;		//命令コード H
	SendData[3] = 0x0A;		//命令コード L

	mlen = strlen( str );
	SendData[4] = (char)(mlen & 0xff);
	for( i=0; i<mlen; i++ ){
		SendData[5+i] = str[i];
	}
	SendData[5+mlen] = 0x06;
	SendData[1] = 6 + mlen;

	CommandSend( 0 );

	ret = 0;
	while(ret<1){
		ret = getRecvData( SocketNumber, recv, 22, 0, 2000000L );
		if( (SendData[4]==0 && ret<1) || (recv[6]==0x46) ){	//終了
			VmFinishMes(mrb);
			return mrb_nil_value();			//戻り値は無しですよ。
		}
	}
			
	mrb_value arv[2];

	arv[0] = mrb_fixnum_value( ((recv[6]<<16) & 0xff0000) + ((recv[7]<<8) & 0xff00) + (recv[8] & 0xff));
	arv[1] = mrb_fixnum_value( ((recv[10]<<16) & 0xff0000) + ((recv[11]<<8) & 0xff00) + (recv[12] & 0xff));

	return mrb_ary_new_from_values(mrb, 2, arv);
}

//**************************************************
// ライブラリを定義します
//**************************************************
void canvas_Init(mrb_state *mrb)
{
	struct RClass *canvasModule = mrb_define_module(mrb, "Canvas");

	mrb_define_module_function(mrb, canvasModule, "putg", mrb_canvas_putg, ARGS_REQ(8));
	mrb_define_module_function(mrb, canvasModule, "putText", mrb_canvas_putText, ARGS_REQ(5) | ARGS_OPT(1));
	mrb_define_module_function(mrb, canvasModule, "putTextBox", mrb_canvas_putTextBox, ARGS_REQ(6) | ARGS_OPT(1));
	mrb_define_module_function(mrb, canvasModule, "putTextRotate", mrb_canvas_putTextRotate, ARGS_REQ(6) | ARGS_OPT(1));
	mrb_define_module_function(mrb, canvasModule, "putTextCenter", mrb_canvas_putTextCenter, ARGS_REQ(5) | ARGS_OPT(1));
	mrb_define_module_function(mrb, canvasModule, "putCls", mrb_canvas_putCls, ARGS_OPT(1));
	mrb_define_module_function(mrb, canvasModule, "putflush", mrb_canvas_putflush, ARGS_OPT(4));
	mrb_define_module_function(mrb, canvasModule, "workflush", mrb_canvas_workflush, ARGS_REQ(4));
	mrb_define_module_function(mrb, canvasModule, "putWork", mrb_canvas_putWork, ARGS_REQ(8));
	mrb_define_module_function(mrb, canvasModule, "drawText", mrb_canvas_drawText, ARGS_REQ(5) | ARGS_OPT(1));
	mrb_define_module_function(mrb, canvasModule, "drawLine", mrb_canvas_drawLine, ARGS_REQ(5));
	mrb_define_module_function(mrb, canvasModule, "drawRect", mrb_canvas_drawRect, ARGS_REQ(5) | ARGS_OPT(1));
	mrb_define_module_function(mrb, canvasModule, "drawCls", mrb_canvas_drawCls, ARGS_OPT(1));
	mrb_define_module_function(mrb, canvasModule, "drawCircle", mrb_canvas_drawCircle, ARGS_REQ(4) | ARGS_OPT(1));
	mrb_define_module_function(mrb, canvasModule, "putrotg", mrb_canvas_putrotg, ARGS_REQ(7));
	mrb_define_module_function(mrb, canvasModule, "putLine", mrb_canvas_putLine, ARGS_REQ(5));
	mrb_define_module_function(mrb, canvasModule, "putRect", mrb_canvas_putRect, ARGS_REQ(5) | ARGS_OPT(1));
	mrb_define_module_function(mrb, canvasModule, "putCircle", mrb_canvas_putCircle, ARGS_REQ(4) | ARGS_OPT(1));
	mrb_define_module_function(mrb, canvasModule, "drawTextCenter", mrb_canvas_drawTextCenter, ARGS_REQ(5) | ARGS_OPT(1));
	mrb_define_module_function(mrb, canvasModule, "drawTextRotate", mrb_canvas_drawTextRotate, ARGS_REQ(6) | ARGS_OPT(1));
	mrb_define_module_function(mrb, canvasModule, "getg", mrb_canvas_getg, ARGS_REQ(8));
	mrb_define_module_function(mrb, canvasModule, "getviewSize", mrb_canvas_getviewSize, ARGS_NONE());
	mrb_define_module_function(mrb, canvasModule, "drawTextBox", mrb_canvas_drawTextBox, ARGS_REQ(6) | ARGS_OPT(1));
	mrb_define_module_function(mrb, canvasModule, "loadBmp", mrb_canvas_loadBmp, ARGS_REQ(5) | ARGS_OPT(1));
	mrb_define_module_function(mrb, canvasModule, "saveBmp", mrb_canvas_saveBmp, ARGS_REQ(5));
	mrb_define_module_function(mrb, canvasModule, "workCls", mrb_canvas_workCls, ARGS_OPT(1));
	mrb_define_module_function(mrb, canvasModule, "getColor", mrb_canvas_getColor, ARGS_REQ(2));
	mrb_define_module_function(mrb, canvasModule, "setWorkBmp", mrb_canvas_setWorkBmp, ARGS_REQ(2));
	mrb_define_module_function(mrb, canvasModule, "setMainBmp", mrb_canvas_setMainBmp, ARGS_REQ(2));
	mrb_define_module_function(mrb, canvasModule, "getBmpSize", mrb_canvas_getBmpSize, ARGS_REQ(1));
}
