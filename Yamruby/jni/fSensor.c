//************************************************************************
// センサ関連
//************************************************************************
#include <string.h>
#include <math.h>

#include "mruby.h"
#include "mruby/string.h"
#include "mruby/array.h"

#include "fvm.h"
#include "fvmExec.h"
#include "fvmClient.h"

extern int	SocketNumber;			//接続ソケット番号
extern char	SendData[];				//送信データ(512byte)

//************************************************************************
// センサ情報取得のサブルーチン
//************************************************************************
mrb_value SubSensor( mrb_state *mrb, int comnum )
{
int	lenok;
unsigned char *s;

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x05;		//全体サイズ
	SendData[2] = 0x06;		//命令コード H
	SendData[3] = (char)(comnum & 0xff);		//命令コード L
	SendData[4] = 0x06;

	CommandSend( 0 );
	lenok = getRecvData( SocketNumber, SendData, 64, 0, 2000000L );
	//2sec待って何も返ってこなかったり、終了コードが返ってきたら、LuaVMを終了させる
	if( lenok<1 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	mrb_value arv[3];

	s = (unsigned char*)SendData;
	s += 3;
	arv[0] = mrb_float_value(btof( s, 0 ));

	s += 4;
	arv[1] = mrb_float_value(btof( s, 0 ));

	s += 4;
	arv[2] = mrb_float_value(btof( s, 0 ));

	return mrb_ary_new_from_values(mrb, 3, arv);
}

//************************************************************************
// 2-6-0.加速度センサ情報を取得します: sensor.getAccel(0x0600)
//	sensor.getAccel()
//	0x03 0x05 0x0600 0x06
//	受信
//	0x03 0x10 0xAK 0xXXXXXXXX 0xYYYYYYYY 0xZZZZZZZZ 0x06
//		XXXXXXXX: 4バイトがX方向の加速度 float 値を持っている。
//		YYYYYYYY: 4バイトがX方向の加速度 float 値を持っている。
//		ZZZZZZZZ: 4バイトがX方向の加速度 float 値を持っている。
//************************************************************************
mrb_value mrb_sensor_getAccel(mrb_state *mrb, mrb_value self)
{
	return SubSensor( mrb, 0 );
}

//************************************************************************
// 2-6-1.傾斜センサ情報を取得します: sensor.getOrient(0x0601)
//	sensor.getOrient()
//	0x03 0x05 0x0601 0x06
//	受信
//	0x03 0x10 0xAK 0xYYYYYYYY 0xPPPPPPPP 0xRRRRRRRR 0x06
//		YYYYYYYY: 4バイトが回転の角度 float 値を持っている。
//		PPPPPPPP: 4バイトが上下のピッチ成分 float 値を持っている。
//		RRRRRRRR: 4バイトがロール成分 float 値を持っている。
//************************************************************************
mrb_value mrb_sensor_getOrient(mrb_state *mrb, mrb_value self)
{
	return( SubSensor( mrb, 1 ) );
}

//************************************************************************
// 2-6-6.重力方向と重力成分を取得します: sensor.getGdirection(0x0606)
//	sensor.getGdirection()
//	0x03 0x05 0x0600 0x06
//	受信
//	0x03 0x10 0xAK 0xXXXXXXXX 0xYYYYYYYY 0xZZZZZZZZ 0x06
//		XXXXXXXX: 4バイトがX方向の加速度 float 値を持っている。
//		YYYYYYYY: 4バイトがX方向の加速度 float 値を持っている。
//		ZZZZZZZZ: 4バイトがX方向の加速度 float 値を持っている。
//	0x0600コードを送って、X,Y,Zデータを取得し、関数内で計算する
//************************************************************************
mrb_value mrb_sensor_getGdirection(mrb_state *mrb, mrb_value self)
{
int	lenok;
unsigned char *s;
float fx,fy,fz;

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x05;		//全体サイズ
	SendData[2] = 0x06;		//命令コード H
	SendData[3] = 0x00;		//命令コード L
	SendData[4] = 0x06;

	CommandSend( 0 );
	lenok = getRecvData( SocketNumber, SendData, 64, 0, 2000000L );
	//2sec待って何も返ってこなかったり、終了コードが返ってきたら、LuaVMを終了させる
	if( lenok<1 ){
		VmFinishMes(mrb);
		return mrb_nil_value();			//戻り値は無しですよ。
	}

	s = (unsigned char*)SendData;
	s += 3;
	fx = btof( s, 0 );
	s += 4;
	fy = btof( s, 0 );
	s += 4;
	fz = btof( s, 0 );

	mrb_value arv[3];

	arv[0] = mrb_float_value(atan2( -fy, fx )*57.29577951);

	arv[1] = mrb_float_value(sqrt( fy*fy+fx*fx ));

	arv[2] = mrb_float_value(fz);

	return mrb_ary_new_from_values(mrb, 3, arv);
}

//************************************************************************
// 2-6-2.磁気センサ情報を取得します: sensor.getMagnet(0x0602)
//	sensor.getMagnet()
//	0x03 0x05 0x0602 0x06
//	受信
//	0x03 0x10 0xAK 0xXXXXXXXX 0xYYYYYYYY 0xZZZZZZZZ 0x06
//		XXXXXXXX: 4バイトがX方向の磁気成分 float 値を持っている。
//		YYYYYYYY: 4バイトがX方向の磁気成分 float 値を持っている。
//		ZZZZZZZZ: 4バイトがX方向の磁気成分 float 値を持っている。
//************************************************************************
mrb_value mrb_sensor_getMagnet(mrb_state *mrb, mrb_value self)
{
	return( SubSensor( mrb, 2 ) );
}

//************************************************************************
// センサの起動のサブルーチン
//************************************************************************
mrb_value SubSensorSetDev( mrb_state *mrb, int comnum )
{
int snum;

	mrb_get_args(mrb, "i", &snum);

	SendData[0] = 0x03;		//先頭コード
	SendData[1] = 0x06;		//全体サイズ
	SendData[2] = 0x06;		//命令コード H
	SendData[3] = (char)(comnum & 0xff);		//命令コード L
	
	SendData[4] = (char)(snum & 0xff);
	SendData[5] = 0x06;

	CommandSend( 0 );

	//ack待ち
	int lenok = getRecvData( SocketNumber, SendData, 10, 0, 10000000L );

	int ret = 0;
	if( lenok<1 || SendData[4]==1 ){
		//10sec待って何も返ってこなかったら
		ret = 1;
	}

	return mrb_fixnum_value( ret );
}

//************************************************************************
// 2-6-3.加速度センサの動作を切り替えます: sensor.setdevAccel(0x0603)
//	sensor.setdevAccel()
//	0x03 0x06 0x0603 0xSS 0x06
//	引数 0xSS
//		0: 加速度センサ動作終了
//		1: 加速度センサ動作開始
//	受信
//	0x03 0x06 0x00 0x00 0xRR 0x06
//	戻り値 0xRR	
//		0: 設定成功
//		1: 設定失敗
//************************************************************************
mrb_value mrb_sensor_setdevAccel(mrb_state *mrb, mrb_value self)
{
	return( SubSensorSetDev( mrb, 3 ) );
}

//************************************************************************
// 2-6-4.傾斜センサの動作を切り替えます: sensor.setdevOrient(0x0604)
//	sensor.setdevOrient()
//	0x03 0x06 0x0604 0xSS 0x06
//	引数 0xSS
//		0: 傾斜センサ動作終了
//		1: 傾斜センサ動作開始
//	受信
//	0x03 0x06 0x00 0x00 0xRR 0x06
//	戻り値
//		0: 設定成功
//		1: 設定失敗
//************************************************************************
mrb_value mrb_sensor_setdevOrient(mrb_state *mrb, mrb_value self)
{
	return( SubSensorSetDev( mrb, 4 ) );
}

//************************************************************************
// 2-6-5.磁気センサの動作を切り替えます: sensor.setdevMagnet(0x0605)
//	sensor.setdevMagnet()
//	0x03 0x06 0x0605 0xSS 0x06
//	引数 0xSS
//		0: 磁気センサ動作終了
//		1: 磁気センサ動作開始
//	受信
//	0x03 0x06 0x00 0x00 0xRR 0x06
//	戻り値
//		0: 設定成功
//		1: 設定失敗
//************************************************************************
mrb_value mrb_sensor_setdevMagnet(mrb_state *mrb, mrb_value self)
{
	return( SubSensorSetDev( mrb, 5 ) );
}

//**************************************************
// ライブラリを定義します
//**************************************************
void sensor_Init(mrb_state *mrb)
{
	struct RClass *sensorModule = mrb_define_module(mrb, "Sensor");

	mrb_define_module_function(mrb, sensorModule, "getAccel", mrb_sensor_getAccel, ARGS_NONE());
	mrb_define_module_function(mrb, sensorModule, "getOrient", mrb_sensor_getOrient, ARGS_NONE());
	mrb_define_module_function(mrb, sensorModule, "getGdirection", mrb_sensor_getGdirection, ARGS_NONE());
	mrb_define_module_function(mrb, sensorModule, "getMagnet", mrb_sensor_getMagnet, ARGS_NONE());
	mrb_define_module_function(mrb, sensorModule, "setdevAccel", mrb_sensor_setdevAccel, ARGS_REQ(1));
	mrb_define_module_function(mrb, sensorModule, "setdevOrient", mrb_sensor_setdevOrient, ARGS_REQ(1));
	mrb_define_module_function(mrb, sensorModule, "setdevMagnet", mrb_sensor_setdevMagnet, ARGS_REQ(1));
}