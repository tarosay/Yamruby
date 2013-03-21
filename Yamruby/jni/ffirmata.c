//************************************************************************
// Firmata関連
//************************************************************************
#include "mruby.h"
#include "mruby/string.h"
#include "mruby/variable.h"
#include "mruby/array.h"

#include "fvm.h"
#include "fvmExec.h"
#include "fvmClient.h"

extern char	SendData[];				//送信データ(512byte)
extern int	SocketNumber;			//接続ソケット番号

//**************************************************
//2-3-10.デジタルライト: digitalWrite(0x0310)
//	digitalWrite(pin, value)
//	0x03 0x08 0x0310 0xXXXX 0xYY 0x06
//	0xXXXXの値
//		ピンの番号
//	0xYYの値
//		0: LOW
//		1: HIGH
//**************************************************
mrb_value mrb_kernel_digitalWrite(mrb_state *mrb, mrb_value self)
{
int pin, value;

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x08;		//全体サイズ
	SendData[2] = 0x03;		//命令コード H
	SendData[3] = 0x10;		//命令コード L

	mrb_get_args(mrb, "ii", &pin, &value);

	SendData[4] = (char)((pin>>8) & 0xff);
	SendData[5] = (char)(pin & 0xff);

	SendData[6] = (char)value;

	SendData[7] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();	//戻り値は無しですよ。
}

//**************************************************
//2-3-12.アナログPWM出力: analogWrite(0x0312)
//	analogWrite(pin, value)
//	0x03 0x08 0x0312 0xXXXX 0xYY 0x06
//	0xXXXXの値
//		ピンの番号
//	0xYYの値
//		出力PWM比率(0～255)
//**************************************************
mrb_value mrb_kernel_analogWrite(mrb_state *mrb, mrb_value self)
{
int pin, value;

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x08;		//全体サイズ
	SendData[2] = 0x03;		//命令コード H
	SendData[3] = 0x12;		//命令コード L

	mrb_get_args(mrb, "ii", &pin, &value);

	SendData[4] = (char)((pin>>8) & 0xff);
	SendData[5] = (char)(pin & 0xff);

	if( value>=0 && value<256 ){
		SendData[6] = (char)value;
	}
	else{
		SendData[6] = 0;
	}
	SendData[7] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
//2-3-11.デジタルリード: digitalRead(0x0311)
//	digitalRead(pin)
//	0x03 0x07 0x0311 0xXXXX 0x06
//	0xXXXXの値
//		ピンの番号
//	
//	受信
//	0x03 0x06 0x0311 0xXX 0x06
//	0xXXの値
//		0:LOW
//		1:HIGH
//**************************************************
mrb_value mrb_kernel_digitalRead(mrb_state *mrb, mrb_value self)
{
int pin;

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x07;		//全体サイズ
	SendData[2] = 0x03;		//命令コード H
	SendData[3] = 0x11;		//命令コード L

	mrb_get_args(mrb, "i", &pin);

	SendData[4] = (char)((pin>>8) & 0xff);
	SendData[5] = (char)(pin & 0xff);

	SendData[6] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	if(SendData[2]==0x00 && SendData[3]==0x00 && SendData[4]==0x46){
		ErrorSend( 10, (char*)"digitalRead:Communication Err" );
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}
	
	return mrb_fixnum_value( (int)SendData[4] );
}

//**************************************************
//2-3-13.アナログリード: analogRead(0x0313)
//	analogRead(pin)
//	0x03 0x07 0x0313 0xXXXX 0x06
//	0xXXXXの値
//		ピンの番号
//	
//	受信
//	0x03 0x07 0x0313 0xHHHH 0x06
//	0xHHHHの値
//		10ビットの値(0～1023)
//**************************************************
mrb_value mrb_kernel_analogRead(mrb_state *mrb, mrb_value self)
{
int pin;

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x07;		//全体サイズ
	SendData[2] = 0x03;		//命令コード H
	SendData[3] = 0x13;		//命令コード L

	mrb_get_args(mrb, "i", &pin);

	SendData[4] = (char)((pin>>8) & 0xff);
	SendData[5] = (char)(pin & 0xff);

	SendData[6] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	if(SendData[2]==0x00 && SendData[3]==0x00 && SendData[4]==0x46){
		ErrorSend( 10, (char*)"analogRead:Communication Err" );
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_fixnum_value((int)((((unsigned char)SendData[4] & 0x3)<<8) + (unsigned char)SendData[5]));
}

//**************************************************
//2-3-14.PINのモード設定: pinMode(0x0314)
//	pinMode(pin, mode)
//	0x03 0x08 0x0314 0xXXXX 0xYY 0x06
//	0xXXXXの値
//		ピンの番号
//	0xYYの値
//		0: INPUTモード
//		1: OUTPUTモード
//**************************************************
mrb_value mrb_kernel_pinMode(mrb_state *mrb, mrb_value self)
{
int pin, value;

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x08;		//全体サイズ
	SendData[2] = 0x03;		//命令コード H
	SendData[3] = 0x14;		//命令コード L

	mrb_get_args(mrb, "ii", &pin, &value);

	SendData[4] = (char)((pin>>8) & 0xff);
	SendData[5] = (char)(pin & 0xff);

	SendData[6] = (char)value;

	SendData[7] = 0x06;

	if( CommandSend( 1 )==0 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	return mrb_nil_value();			//戻り値は無しですよ。
}

//**************************************************
// ライブラリを定義します
//**************************************************
void firmata_Init(mrb_state *mrb)
{
	mrb_define_method(mrb, mrb->kernel_module, "digitalWrite", mrb_kernel_digitalWrite, ARGS_REQ(2));
	mrb_define_method(mrb, mrb->kernel_module, "analogWrite", mrb_kernel_analogWrite, ARGS_REQ(2));
	mrb_define_method(mrb, mrb->kernel_module, "digitalRead", mrb_kernel_digitalRead, ARGS_REQ(1));
	mrb_define_method(mrb, mrb->kernel_module, "analogRead", mrb_kernel_analogRead, ARGS_REQ(1));
	mrb_define_method(mrb, mrb->kernel_module, "pinMode", mrb_kernel_pinMode, ARGS_REQ(2));

	//lua_register( LuaLinkP, "analogWriteDAC", adkAnalogWriteDAC );
	//lua_register( LuaLinkP, "analogWriteFrequency", adkAnalogWriteFrequency );
	//lua_register( LuaLinkP, "pulseIn", adkPulseIn );
	//lua_register( LuaLinkP, "analogReference", adkAnalogReference );
}
