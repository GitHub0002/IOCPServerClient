#include"winerror.h"
#include"Winsock2.h"
#pragma comment(lib, "ws2_32")
#include"windows.h"
#include<iostream>
using namespace std;
#pragma warning(disable:4996)


/// �궨��
#define PORT 5050
#define DATA_BUFSIZE 8192

#define OutErr(a) cout << (a) << endl \
      << "������룺"<< WSAGetLastError() << endl \
      << "�����ļ���"<< __FILE__ << endl  \
      << "����������"<< __LINE__ << endl \

#define OutMsg(a) cout << (a) << endl;


/// ȫ�ֺ�������


///////////////////////////////////////////////////////////////////////
//
// ������       : InitWinsock
// ��������     : ��ʼ��WINSOCK
// ����ֵ       : void
//
///////////////////////////////////////////////////////////////////////
void InitWinsock()
{
	// ��ʼ��WINSOCK
	WSADATA wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		OutErr("WSAStartup()");
	}
}

///////////////////////////////////////////////////////////////////////
//
// ������       : BindServerOverlapped
// ��������     : �󶨶˿ڣ�������һ�� Overlapped ��ListenSocket
// ����         : int nPort
// ����ֵ       : SOCKET
//
///////////////////////////////////////////////////////////////////////
SOCKET BindServerOverlapped(int nPort)
{
	// ����socket
	SOCKET sServer = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	// �󶨶˿�
	struct sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(nPort);
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sServer, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
	{
		OutErr("bind Failed!");
		return NULL;
	}

	// ���ü�������Ϊ200
	if (listen(sServer, 200) != 0)
	{
		OutErr("listen Failed!");
		return NULL;
	}
	return sServer;
}


/// �ṹ�嶨��
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

		// ˵���ͻ����Ѿ��˳�
		if (BytesTransferred == 0)
		{
			cout << "closing socket" << PerHandleData->Socket << endl;
			closesocket(PerHandleData->Socket);
			delete PerIoData;
			delete PerHandleData;
			continue;
		}

		// ȡ�����ݲ�����
		cout << PerHandleData->Socket << "���͹�������Ϣ��" << PerIoData->Buffer << endl;

		if (0==strcmp(PerIoData->Buffer, "exit"))
			break;


		// ������ socket Ͷ��WSARecv����
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

	//����ϵͳ��CPU�������������߳�
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);

	//�߳���Ŀ=ϵͳ������Ŀ������.
	for (int i = 0; i < SystemInfo.dwNumberOfProcessors * 2; i++)
	{
		HANDLE hProcessIO = CreateThread(NULL, 0, ProcessIO, CompletionPort, 0, NULL);
		if (hProcessIO)
		{
			CloseHandle(hProcessIO);
		}
	}

	//��������SOCKET
	SOCKET sListen = BindServerOverlapped(PORT);

	SOCKET sClient;
	LPPER_HANDLE_DATA PerHandleData;
	LPPER_IO_OPERATION_DATA PerIoData;
	while (true)
	{
		// �ȴ��ͻ��˽���
		//sClient = WSAAccept(sListen, NULL, NULL, NULL, 0);
		sClient = accept(sListen, 0, 0);
		cout << "Socket " << sClient << "���ӽ���" << endl;

		PerHandleData = new PER_HANDLE_DATA();
		PerHandleData->Socket = sClient;

		// ������Ŀͻ��˺���ɶ˿���ϵ����
		CreateIoCompletionPort((HANDLE)sClient, CompletionPort, (DWORD)PerHandleData, 0);

		// ����һ��Overlapped����ʹ�����Overlapped�ṹ��socketͶ�ݲ���
		PerIoData = new PER_IO_OPERATION_DATA();

		ZeroMemory(PerIoData, sizeof(PER_IO_OPERATION_DATA));
		PerIoData->DataBuf.buf = PerIoData->Buffer;
		PerIoData->DataBuf.len = DATA_BUFSIZE;

		// Ͷ��һ��WSARecv����
		DWORD Flags = 0;
		DWORD dwRecv = 0;
		WSARecv(sClient, &PerIoData->DataBuf, 1, &dwRecv, &Flags, &PerIoData->Overlapped, NULL);
		//WSASend(sClient, &PerIoData->DataBuf, 1, &dwRecv, MSG_OOB, &PerIoData->Overlapped, NULL);
	}

	DWORD dwByteTrans;
	//��һ���Ѿ���ɵ�IO֪ͨ��ӵ�IO��ɶ˿ڵĶ�����.
	 //�ṩ�����̳߳��е������߳�ͨ�ŵķ�ʽ.
	PostQueuedCompletionStatus(CompletionPort, dwByteTrans, 0, 0);  //IO�������ʱ���յ��ֽ���.

	closesocket(sListen);
}