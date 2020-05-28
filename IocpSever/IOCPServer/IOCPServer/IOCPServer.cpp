#include"winerror.h"
#include"Winsock2.h"
#pragma comment(lib, "ws2_32")
#include"windows.h"
#include<iostream>
using namespace std;
#pragma warning(disable:4996)


/// 宏定义
#define PORT 5050
#define DATA_BUFSIZE 8192

#define OutErr(a) cout << (a) << endl \
      << "出错代码："<< WSAGetLastError() << endl \
      << "出错文件："<< __FILE__ << endl  \
      << "出错行数："<< __LINE__ << endl \

#define OutMsg(a) cout << (a) << endl;


/// 全局函数定义


///////////////////////////////////////////////////////////////////////
//
// 函数名       : InitWinsock
// 功能描述     : 初始化WINSOCK
// 返回值       : void
//
///////////////////////////////////////////////////////////////////////
void InitWinsock()
{
	// 初始化WINSOCK
	WSADATA wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		OutErr("WSAStartup()");
	}
}

///////////////////////////////////////////////////////////////////////
//
// 函数名       : BindServerOverlapped
// 功能描述     : 绑定端口，并返回一个 Overlapped 的ListenSocket
// 参数         : int nPort
// 返回值       : SOCKET
//
///////////////////////////////////////////////////////////////////////
SOCKET BindServerOverlapped(int nPort)
{
	// 创建socket
	SOCKET sServer = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	// 绑定端口
	struct sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(nPort);
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sServer, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
	{
		OutErr("bind Failed!");
		return NULL;
	}

	// 设置监听队列为200
	if (listen(sServer, 200) != 0)
	{
		OutErr("listen Failed!");
		return NULL;
	}
	return sServer;
}


/// 结构体定义
typedef struct
{
	OVERLAPPED Overlapped;
	WSABUF DataBuf;
	CHAR Buffer[DATA_BUFSIZE];
}PER_IO_OPERATION_DATA, *LPPER_IO_OPERATION_DATA;


typedef struct
{
	SOCKET Socket;
}PER_HANDLE_DATA, *LPPER_HANDLE_DATA;


DWORD WINAPI ProcessIO(LPVOID lpParam)
{
	HANDLE CompletionPort = (HANDLE)lpParam;
	DWORD BytesTransferred;
	LPPER_HANDLE_DATA PerHandleData;
	LPPER_IO_OPERATION_DATA PerIoData;
	bool bloop = true;
	while (bloop)
	{

		if (0 == GetQueuedCompletionStatus(CompletionPort, &BytesTransferred, (LPDWORD)&PerHandleData, (LPOVERLAPPED*)&PerIoData, INFINITE))
		{
			if ((GetLastError() == WAIT_TIMEOUT) || (GetLastError() == ERROR_NETNAME_DELETED))
			{
				cout << "closingsocket" << PerHandleData->Socket << endl;
				closesocket(PerHandleData->Socket);

				delete PerIoData;
				delete PerHandleData;
				continue;
			}
			else
			{
				OutErr("GetQueuedCompletionStatus failed!");
			}
			return 0;
		}

		// 说明客户端已经退出
		if (BytesTransferred == 0)
		{
			cout << "closing socket" << PerHandleData->Socket << endl;
			closesocket(PerHandleData->Socket);
			delete PerIoData;
			delete PerHandleData;
			continue;
		}

		// 取得数据并处理
		cout << PerHandleData->Socket << "发送过来的消息：" << PerIoData->Buffer << endl;

		if (0==strcmp(PerIoData->Buffer, "exit"))
			break;


		// 继续向 socket 投递WSARecv操作
		DWORD Flags = 0;
		DWORD dwRecv = 0;
		ZeroMemory(PerIoData, sizeof(PER_IO_OPERATION_DATA));
		PerIoData->DataBuf.buf = PerIoData->Buffer;
		PerIoData->DataBuf.len = DATA_BUFSIZE;
		WSARecv(PerHandleData->Socket, &PerIoData->DataBuf, 1, &dwRecv, &Flags, &PerIoData->Overlapped, NULL);
	}

	return 0;
}

void main()
{
	InitWinsock();
	HANDLE CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	//根据系统的CPU来创建工作者线程
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);

	//线程数目=系统进程数目的两倍.
	for (int i = 0; i < SystemInfo.dwNumberOfProcessors * 2; i++)
	{
		HANDLE hProcessIO = CreateThread(NULL, 0, ProcessIO, CompletionPort, 0, NULL);
		if (hProcessIO)
		{
			CloseHandle(hProcessIO);
		}
	}

	//创建侦听SOCKET
	SOCKET sListen = BindServerOverlapped(PORT);

	SOCKET sClient;
	LPPER_HANDLE_DATA PerHandleData;
	LPPER_IO_OPERATION_DATA PerIoData;
	while (true)
	{
		// 等待客户端接入
		//sClient = WSAAccept(sListen, NULL, NULL, NULL, 0);
		sClient = accept(sListen, 0, 0);
		cout << "Socket " << sClient << "连接进来" << endl;

		PerHandleData = new PER_HANDLE_DATA();
		PerHandleData->Socket = sClient;

		// 将接入的客户端和完成端口联系起来
		CreateIoCompletionPort((HANDLE)sClient, CompletionPort, (DWORD)PerHandleData, 0);

		// 建立一个Overlapped，并使用这个Overlapped结构对socket投递操作
		PerIoData = new PER_IO_OPERATION_DATA();

		ZeroMemory(PerIoData, sizeof(PER_IO_OPERATION_DATA));
		PerIoData->DataBuf.buf = PerIoData->Buffer;
		PerIoData->DataBuf.len = DATA_BUFSIZE;

		// 投递一个WSARecv操作
		DWORD Flags = 0;
		DWORD dwRecv = 0;
		WSARecv(sClient, &PerIoData->DataBuf, 1, &dwRecv, &Flags, &PerIoData->Overlapped, NULL);
		//WSASend(sClient, &PerIoData->DataBuf, 1, &dwRecv, MSG_OOB, &PerIoData->Overlapped, NULL);
	}

	DWORD dwByteTrans;
	//将一个已经完成的IO通知添加到IO完成端口的队列中.
	 //提供了与线程池中的所有线程通信的方式.
	PostQueuedCompletionStatus(CompletionPort, dwByteTrans, 0, 0);  //IO操作完成时接收的字节数.

	closesocket(sListen);
}