// valvikと通信するモジュール
//#include <stdio.h>
//#include <string.h>
//#include <netdb.h>
//#include <linux/in.h>
//#include <unistd.h>
#include <endian.h>

//*************************************************************************
// ソケットを開いて接続します
//*************************************************************************
int	SocketOpen( void );

//*************************************************************************
// ソケットを閉じます
//*************************************************************************
void	SocketClose( void );

//*************************************************************************
// コマンドデータをサーバに送信します
// recvFlg: 0:ACKを待たない, 1:ACKを待つ
//*************************************************************************
int		CommandSend( int recvFlg );

//*************************************************************************
// 255バイト以上のコマンドデータをサーバに送信します
// recvFlg: 0:ACKを待たない, 1:ACKを待つ
// size: 送信サイズ
//*************************************************************************
int		CommandSend2( int recvFlg, int size );

//**************************************************
// 受信チェックありの受信処理
// socket, *buf, len, flags は、recv(int socket, void *buf, size_t len, int flags);
// us は、受信チェックの待ち時間、単位はμsec
//**************************************************
ssize_t getRecvData( int socket, void *buf, size_t len, int flags, suseconds_t us );