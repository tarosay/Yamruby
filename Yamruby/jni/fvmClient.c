// dalvikと通信するモジュール

#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <linux/in.h>
#include <unistd.h>
#include <endian.h>
	
#include "fvm.h"

int		SocketNumber;				//接続ソケット番号
char	SendData[530];				//送信データ

//**************************************************
// 受信チェックありの受信処理
// socket, *buf, len, flags は、recv(int socket, void *buf, size_t len, int flags);
// us は、受信チェックの待ち時間、単位はμsec
//**************************************************
ssize_t getRecvData( int socket, void *buf, size_t len, int flags, suseconds_t us )
{
ssize_t	recvd_len = 0;
int		ret;
fd_set	fds;
struct	timeval tv;

	FD_ZERO( &fds );			//ファイルディスクリプタの識別子初期化
	FD_SET( socket, &fds );		//socketのファイルディスクリプタ識別子に1をセット
	tv.tv_sec = 0;
	tv.tv_usec = us;

	//ポートの監視、（受信のみ監視、他はNULL)
	ret = select( (int)socket+1, &fds, NULL, NULL, &tv );
	if( ret!=0 ){
		recvd_len = recv( socket, buf, len, flags );
	}
	return( recvd_len );
}


//*************************************************************************
//
// ソケットを開いて接続します
//
//*************************************************************************
int	SocketOpen( void )
{
struct hostent	*accessIP;		//接続先IP
struct sockaddr_in accessAddr;	//アクセスソケット
int	ret;

	SocketNumber = socket( AF_INET, SOCK_STREAM, 0 );    //接続用ソケットの生成
	if( SocketNumber&0x80000000 ){
		//LOGE("Error Socket could not Create.");
		return( VM_SOCKET_OPEN_ERROR );
	}	

	// サーバのIPアドレスを検索します
	accessIP = gethostbyname( (char*)VM_HOSTIP );
	accessAddr.sin_family = AF_INET;
	accessAddr.sin_addr.s_addr = *( (unsigned long *)(accessIP->h_addr_list[0]) );		//IP設定
	accessAddr.sin_port = htons( VM_PORTNUMBER );									//ポート設定
	ret = connect( SocketNumber, (struct sockaddr *)&accessAddr, sizeof(accessAddr) );	//接続する

	if( ret!=0 ){
		close( SocketNumber );
		//LOGE("Error Do not Connect.");
		return( VM_CONNECT_ERROR );
	}
	return( 0 );
}

//*************************************************************************
// ソケットを閉じます
//*************************************************************************
void	SocketClose( void )
{
	shutdown( SocketNumber, 0 );	//ソケットをシャットダウンします
	close( SocketNumber );			//ソケットを閉じます
}

//*************************************************************************
// コマンドデータをサーバに送信します
// recvFlg: 0:ACKを待たない, 1:ACKを待つ
//*************************************************************************
int		CommandSend( int recvFlg )
{
	int lenok = send( SocketNumber, SendData, (unsigned char)SendData[1], 0 );

	if( recvFlg>0 ){
		//ack待ち
		lenok = getRecvData( SocketNumber, SendData, 10, 0, 2000000L );
		//2sec待って何も返ってこなかったり、終了コードが返ってきたら、ScriptVMを終了させる
		if( lenok<1 ){
			//LOGI( "2 Seconds Break" );
			return( 0 );
		}
	}
	return( 1 );
}

//*************************************************************************
// 255バイト以上のコマンドデータをサーバに送信します
// recvFlg: 0:ACKを待たない, 1:ACKを待つ
// size: 送信サイズ
//*************************************************************************
int		CommandSend2( int recvFlg, int size )
{
	int lenok = send( SocketNumber, SendData, size, 0 );

	if( recvFlg>0 ){
		//ack待ち
		lenok = getRecvData( SocketNumber, SendData, 10, 0, 2000000L );
		//2sec待って何も返ってこなかったり、終了コードが返ってきたら、ScriptVMを終了させる
		if( lenok<1 ){
			return( 0 );
		}
	}
	return( 1 );
}
