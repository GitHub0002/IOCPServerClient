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

	err = WSAStartup(wVersionRequested, &wsaData);//WSAStartup()�����׽��ֿ�
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
		 addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//���ػ�·��ַ127������һ̨�����ϲ��Ե�IP
		 addrSrv.sin_family = AF_INET;
		 addrSrv.sin_port = htons(5050);//�ͷ������˵Ķ˿ںű���һ��
		 connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));//���ӷ������ˣ��׽��֣���ַת�������ȣ�
	while (nCnt < MAXCNT)
	{
		


		sprintf(sendBuf, "This is TestNo : %d\n", ++nCnt);
		send(sockClient, sendBuf, strlen(sendBuf) + 1, 0);//��������˷������ݣ�"+1"��Ϊ�˸�'\0'���ռ�
		printf("send:%s", sendBuf);

		           //memset(recvBuf,0,100);
		           //recv(sockClient,recvBuf,100,0);//��������
		           //printf("%s\n",recvBuf);//��ӡ

		
		Sleep(1);
	}
	sprintf(sendBuf, "exit");
		send(sockClient, sendBuf, strlen(sendBuf) + 1, 0);//��������˷������ݣ�"+1"��Ϊ�˸�'\0'���ռ�
	printf("send:%s", sendBuf);
	closesocket(sockClient);//�ر��׽��֣��ͷ�Ϊ����׽��ַ������Դ
	WSACleanup();//��ֹ������׽��ֿ��ʹ��
}