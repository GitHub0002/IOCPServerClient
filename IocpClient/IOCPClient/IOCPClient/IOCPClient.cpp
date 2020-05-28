#pragma warning(disable:4996)

#include<stdio.h>
#include<Winsock2.h>
#pragma comment(lib, "ws2_32")
#define MAXCNT 30000


void main()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);//WSAStartup()加载套接字库
	if (err != 0) {

		return;
	}

	if (LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 2) {
		WSACleanup();
		return;
	}

	static int nCnt = 0;
	char sendBuf[2000];
	     char recvBuf[100];

		 SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);
		 SOCKADDR_IN addrSrv;
		 addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//本地回路地址127，用于一台机器上测试的IP
		 addrSrv.sin_family = AF_INET;
		 addrSrv.sin_port = htons(5050);//和服务器端的端口号保持一致
		 connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));//连接服务器端（套接字，地址转换，长度）
	while (nCnt < MAXCNT)
	{
		


		sprintf(sendBuf, "This is TestNo : %d\n", ++nCnt);
		send(sockClient, sendBuf, strlen(sendBuf) + 1, 0);//向服务器端发送数据，"+1"是为了给'\0'留空间
		printf("send:%s", sendBuf);

		           //memset(recvBuf,0,100);
		           //recv(sockClient,recvBuf,100,0);//接收数据
		           //printf("%s\n",recvBuf);//打印

		
		Sleep(1);
	}
	sprintf(sendBuf, "exit");
		send(sockClient, sendBuf, strlen(sendBuf) + 1, 0);//向服务器端发送数据，"+1"是为了给'\0'留空间
	printf("send:%s", sendBuf);
	closesocket(sockClient);//关闭套接字，释放为这个套接字分配的资源
	WSACleanup();//终止对这个套接字库的使用
}