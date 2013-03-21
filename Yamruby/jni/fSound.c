//************************************************************************
// サウンド関連
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

//**************************************************
// 2-5-2.サウンド再生します: sound.start(0x0502)
//	sound.start(サウンド番号(0～7)[,loop])
//	0x03 0x07 0x0502 0xBN(サウンド番号) 0xLP 0x06
//	引数 0xLP
//		0: ループ無し
//		1: ループ有り
//**************************************************
mrb_value mrb_sound_start(mrb_state *mrb, mrb_value self)
{
int	snum;
int lp;
int ret = 0;

	int n = mrb_get_args(mrb, "i|i", &snum, &lp);

   	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x07;		//全体サイズ
	SendData[2] = 0x05;		//命令コード H
	SendData[3] = 0x02;		//命令コード L

	SendData[4] = (char)(snum & 0xff);

	if( n>1 ){
		SendData[5] = (char)(lp & 0xff);
	}
	else{
		SendData[5] = 0x00;
	}
	SendData[6] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	if( SendData[4]==1 ){
		ret = -1;
	}

	return mrb_fixnum_value( ret );
}

//**************************************************
// 2-5-1.サウンドファイルをセットします: sound.setSoundFile(0x0501)
//	sound.setSoundFile(サウンドファイル名, サウンド番号(0～7), BGM・効果音フラグ )
//	0x03 0xLC(全体サイズ) 0x0501 0xBN(サウンド番号) 0xBG(BGM効果音フラグ) 0xFL(ファイル名長) 0xFN(ファイル名～) 0x06
//	引数 0xBG
//		0: BGMタイプで多重再生不可
//		1: 効果音タイプで多重再生有効(4重まで)
//	戻り値
//		0: 設定成功
//		-1: 設定失敗
//**************************************************
mrb_value mrb_sound_setSoundFile(mrb_state *mrb, mrb_value self)
{
int tlen;
int i;
int snum;
int bgmflg;
mrb_value text;
char	*str;

	mrb_get_args(mrb, "Sii", &text, &snum, &bgmflg );
	str = RSTRING_PTR(text);

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x04;		//全体サイズ
	SendData[2] = 0x05;		//命令コード H
	SendData[3] = 0x01;		//命令コード L

	SendData[4] = (char)(snum & 0xff);
	SendData[5] = (char)(bgmflg & 0xff);

	tlen = strlen( str );
	SendData[6] = (char)(tlen & 0xff);
	for( i=0; i<tlen; i++ ){
		SendData[7+i] = str[i];
	}
	SendData[7+tlen] = 0x06;
	SendData[1] = 8+tlen;		//全体サイズ

	CommandSend( 0 );

	//ack待ち
	int lenok = getRecvData( SocketNumber, SendData, 10, 0, 10000000L );
	
	int ret = 0;
	if( lenok<1 || SendData[4]==1 ){
		//10sec待って何も返ってこなかったら
		ret = -1;
	}

	return mrb_fixnum_value( ret );
}

//**************************************************
// サウンドコマンドのサブルーチン
//**************************************************
int SubSound( int snum, int comnum )
{
	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x06;		//全体サイズ
	SendData[2] = 0x05;		//命令コード H
	SendData[3] = (char)(comnum & 0xff);		//命令コード L
	
	SendData[4] = (char)(snum & 0xff);
	SendData[5] = 0x06;
	if( CommandSend( 1 )==0 ){
		return( -1 );
	}
	return( 0 );
}

//**************************************************
// 2-5-3.サウンド最初から再生します: sound.restart(0x0503)
//	sound.restart(サウンド番号(0～7) )
//	0x03 0x06 0x0503 0xBN(サウンド番号) 0x06
//**************************************************
mrb_value mrb_sound_restart(mrb_state *mrb, mrb_value self)
{
int snum;

	mrb_get_args(mrb, "i", &snum );

	int ret = SubSound( snum, 3 );
	if( ret==-1 ){
		VmFinishMes(mrb);
		//return mrb_nil_value();			//戻り値は無しですよ。
	}
	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-5-4.サウンドの再生を止めます: sound.stop(0x0504)
//	sound.stop(サウンド番号(0～7) )
//	0x03 0x06 0x0504 0xBN(サウンド番号) 0x06
//**************************************************
mrb_value mrb_sound_stop(mrb_state *mrb, mrb_value self)
{
int snum;

	mrb_get_args(mrb, "i", &snum );

	int ret = SubSound( snum, 4 );
	if( ret==-1 ){
		VmFinishMes(mrb);
		//return mrb_nil_value();			//戻り値は無しですよ。
	}
	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-5-5.サウンドの再生を一時停止します: sound.pause(0x0505)
//	sound.pause(サウンド番号(0～7) )
//	0x03 0x06 0x0505 0xBN(サウンド番号) 0x06
//**************************************************
mrb_value mrb_sound_pause(mrb_state *mrb, mrb_value self)
{
int snum;

	mrb_get_args(mrb, "i", &snum );

	int ret = SubSound( snum, 5 );
	if( ret==-1 ){
		VmFinishMes(mrb);
		//return mrb_nil_value();			//戻り値は無しですよ。
	}
	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-5-6サウンドが鳴っているかどうか調べます: sound.isPlay(ox0506)
//	sound.isPlay(サウンド番号(0～7))
//	0x03 0x06 0x0506 0xBN(サウンド番号) 0x06
//	戻り値
//	0x03 0x06 0x0000 0xIS 0x06
//		0xIS
//		0: 鳴っていない
//		1: 鳴っている
//**************************************************
mrb_value mrb_sound_isPlay(mrb_state *mrb, mrb_value self)
{
int snum;

	mrb_get_args(mrb, "i", &snum );

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x06;		//全体サイズ
	SendData[2] = 0x05;		//命令コード H
	SendData[3] = 0x06;		//命令コード L
	
	SendData[4] = (char)(snum & 0xff);
	SendData[5] = 0x06;
	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}
	
	return mrb_fixnum_value( SendData[4] );
}

//**************************************************
// 2-5-7.ビープ音を鳴らします: sound.beep(0x0507)
//	sound.beep([周波数[,msec]])
//	0x03 0x09 0x0507 0xHZHZ 0xMMMM 0x06
//	引数 0xHZHZ
//		周波数 1～10000Hz 省略時(4kHz)
//	引数 0xMMMM
//		時間(msec) 省略時(200ms)
//**************************************************
mrb_value mrb_sound_beep(mrb_state *mrb, mrb_value self)
{
int	hz;
int msec;

	int n = mrb_get_args(mrb, "|ii", &hz, &msec );

   	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x09;		//全体サイズ
	SendData[2] = 0x05;		//命令コード H
	SendData[3] = 0x07;		//命令コード L

	if( n<1 ){
		SendData[4] = 0x0F;		//4000Hz
		SendData[5] = 0xA0;		//
		SendData[6] = 0x00;		//200ms
		SendData[7] = 0xC8;		//
    }
    else{
		if( hz>10000 ){	hz = 10000;	}	//10kHz以上は10kHz固定
		SendData[4] = (char)((hz>>8) & 0xff);
		SendData[5] = (char)(hz & 0xff);
	
		if( n<2 ){
			SendData[6] = 0x00;	//200ms
			SendData[7] = 0xC8;	//
	    }
	    else{
	   		SendData[6] = (char)((msec>>8) & 0xff);
			SendData[7] = (char)(msec & 0xff);
	    }
	}

	SendData[8] = 0x06;

	if( hz<=0 || msec<=0 ){
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		//return mrb_nil_value();			//戻り値は無しですよ。
	}
	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// 2-5-0.サウンドデータをクリアします: sound.clearAll(0x0500)
//	--clearAll()
//	0x03 0x05 0x0500 0x06
//	戻り値
//		-1: クリア失敗
//		0: クリア成功
//**************************************************
int	SoundClearAll( void )
{
	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x05;		//全体サイズ
	SendData[2] = 0x05;		//命令コード H
	SendData[3] = 0x00;		//命令コード L
	SendData[4] = 0x06;

	if( CommandSend( 1 )==0 ){
		return( -1 );
	}
	return( 0 );
}

//**************************************************
// ライブラリを定義します
//**************************************************
void sound_Init(mrb_state *mrb)
{
	struct RClass *soundModule = mrb_define_module(mrb, "Sound");

	mrb_define_module_function(mrb, soundModule, "start", mrb_sound_start, ARGS_REQ(1) | ARGS_OPT(1));
	mrb_define_module_function(mrb, soundModule, "setSoundFile", mrb_sound_setSoundFile, ARGS_REQ(3));
	mrb_define_module_function(mrb, soundModule, "restart", mrb_sound_restart, ARGS_REQ(1));
	mrb_define_module_function(mrb, soundModule, "stop", mrb_sound_stop, ARGS_REQ(1));
	mrb_define_module_function(mrb, soundModule, "pause", mrb_sound_pause, ARGS_REQ(1));
	mrb_define_module_function(mrb, soundModule, "isPlay", mrb_sound_isPlay, ARGS_REQ(1));
	mrb_define_module_function(mrb, soundModule, "beep", mrb_sound_beep, ARGS_OPT(2));
}