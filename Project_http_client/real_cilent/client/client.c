#include <stdio.h>
#include "string.h"
#include "client.h"


char g_recvBuf[1024]; //������Ϣ�Ļ�����
char* g_fileBuf;    // ���մ洢�ļ�����
int g_fileSize;     // �ļ�����Ĵ�С
char g_FileName[256];    // ������������͹������ļ���

int main() {
	InitSocket();

	ConnectToHost();

	ColseSocket();
	return 0;
}

bool InitSocket() {
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		printf("WSAStartup failed:%d\n", WSAGetLastError());
		return false;
	}
	return true;
}

bool ColseSocket() {
	if (0 != WSACleanup()) {
		printf("WSACleanup failed:%d\n", WSAGetLastError());
		return false;
	}
	return true;
}

bool ConnectToHost()
{
	// ����socket�׽���	��ַ,�˿ں�
	SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == serfd)
	{
		printf("socket faild:%d\n", WSAGetLastError());
		return false;
	}
	// ��ip�Ͷ˿ں�
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(SPORT); // �ֽ���ת��Ϊ�����ֽ���
	serAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // ����������������

	// ���ӵ�������
	if (0 != connect(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr)))
	{
		printf("socket faild:%d\n", WSAGetLastError());
		return false;
	}

	DownloadFileName(serfd);

	// ������Ϣ
	while (processMessage(serfd))
	{
	}
}

bool processMessage(SOCKET serfd)
{
	recv(serfd, g_recvBuf, 1024, 0);
	struct MsgHeader* msg = (struct MsgHeader*)g_recvBuf;

	if (msg->msgID < 0 || msg->msgID > MSG_OpenFile_Failed) {
		printf("Invalid msgID: %d\n", msg->msgID);
		return false;
	}

	switch (msg->msgID)
	{
	case MSG_OpenFile_Failed:
		DownloadFileName(serfd);
		break;
	case MSG_FileSize:
		ReadyRead(serfd, msg);
		break;
	case MSG_Ready_Read:
		writeFile(serfd, msg);
		break;
	}
	return true;
}

void DownloadFileName(SOCKET serfd)
{
	char FileName[1024] = "���,���Ǹ��ڵ�̩ɽ~";
	printf("Please input file: ");
	gets_s(FileName, 1023);
	struct MsgHeader file;
	file.msgID = MSG_FileName;
	strcpy(file.fileInfo.filename, FileName);

	if (SOCKET_ERROR == send(serfd, (char*)&file, sizeof(struct MsgHeader), 0))
	{
		printf("DownloadFileName send error!");
		return;
	}
}

void ReadyRead(SOCKET serfd, struct MsgHeader* pmsg)
{
	// ׼���ڴ�  
	strcpy(g_FileName, pmsg->fileInfo.filename);
	g_fileSize = pmsg->fileInfo.filesize;
	g_fileBuf = (char*)calloc(g_fileSize + 1, sizeof(char)); //����ռ�
	if (g_fileBuf == NULL)
	{
		// �ڴ�����ʧ��
		printf("Out of memory,please try again!\n");
	}
	else
	{
		struct MsgHeader msg;
		msg.msgID = MSG_Send;
		if (SOCKET_ERROR == send(serfd, (char*)&msg, sizeof(struct MsgHeader), 0))
		{
			printf("send error:%d\n", WSAGetLastError());
			return;
		}
	}
	printf("FileSize: %d  FileName: %s\n", pmsg->fileInfo.filesize, pmsg->fileInfo.filename);
	// ������������ MSG_Ready_Read
}

bool writeFile(SOCKET serfd, struct MsgHeader* pmsg)
{
	if (g_fileBuf == NULL)
	{
		return false;
	}
	int nStart = pmsg->packet.nStart;
	int nsize = pmsg->packet.nsize;
	memcpy(g_fileBuf + nStart, pmsg->packet.buf, nsize);

	// ������Ȳ���ʾ
	int bytesReceived = nStart + nsize; // �ѽ��յ��ֽ���
	double progress = (double)bytesReceived / g_fileSize * 100;

	// ��ʾ������
	printf("\rProgress: %.2f%%", progress);
	fflush(stdout); // ȷ���������������̨

	// ������յ��������İ�
	if (nStart + nsize >= g_fileSize)
	{
		FILE* pwirte = fopen(g_FileName, "wb");

		if (pwirte == NULL)
		{
			// ���ļ�����
			printf("write file failed..\n");
			return false;
		}

		fwrite(g_fileBuf, sizeof(char), g_fileSize, pwirte);

		fclose(pwirte);
		free(g_fileBuf);    // �ͷ��ڴ�
		g_fileBuf = NULL;

		struct MsgHeader msg;
		msg.msgID = MSG_Successed;	//�������
		send(serfd, (char*)&msg, sizeof(struct MsgHeader), 0);
	}
	return true;
}