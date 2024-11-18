
#include <stdio.h>
#include "server.h"

#pragma comment(lib,"ws2_32.lib")

char g_recvBuffer[1024]; //�������ܿͻ��˷��͵���Ϣ
int g_fileSize; //�ļ���С
char* g_FileBuf; //�洢�ļ�����

int main() {
	InitSocket();

	ListenToClient();

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

bool ListenToClient() {
	// ����socket�׽���	��ַ,�˿ں�
	SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == serfd) {
		printf("socket failed:%d\n", WSAGetLastError());
		return false;
	}
	// ��ip�Ͷ˿ں�
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(SPORT); // �ֽ���ת��Ϊ�����ֽ���
	serAddr.sin_addr.S_un.S_addr = ADDR_ANY; // ����������������

	if (0 != bind(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr))) {
		printf("socket failed:%d\n", WSAGetLastError());
		return false;
	}

	// �����ͻ�������
	if (0 != listen(serfd, 10)) {
		printf("socket failed:%d\n", WSAGetLastError());
		return false;
	}

	// �пͻ���,��������
	struct sockaddr_in cliaddr;
	int len = sizeof(cliaddr);
	SOCKET clifd = accept(serfd, (struct sockaddr*)&cliaddr, &len);
	if (INVALID_SOCKET == clifd) {
		printf("socket failed:%d\n", WSAGetLastError());
		return false;
	}

	printf("server has started, waiting conection....\n");

	// ������Ϣ
	while (processMessage(clifd)) {
		Sleep(5000);
	}
}

bool processMessage(SOCKET clifd) {
	// �ɹ�������Ϣ,���ؽ����ֽ���,ʧ�ܷ���0
	int nRes = recv(clifd, g_recvBuffer, 1024, 0);
	struct MsgHeader* msg = (struct MsgHeader*)g_recvBuffer;

	if (nRes <= 0)
	{
		printf("�ͻ�������:%d\n", WSAGetLastError());
	}

	switch (msg->msgID)
	{
	case MSG_FileName:
		printf("%s\n", msg->fileInfo.filename);
		readFile(clifd, msg);
		break;
	case MSG_Send:
		printf("Sending file....\n");
		SendFile(clifd, msg);
		break;
	case MSG_Successed:
		printf("file hsa send!\n");
		break;
	}
	return true;
}

bool readFile(SOCKET clifd, struct MsgHeader* pmsg)
{
	FILE* pread = fopen(pmsg->fileInfo.filename, "rb");

	if (pread == NULL)
	{
		printf("fopen failed, can't find this file...: %s\n", pmsg->fileInfo.filename);
		struct MsgHeader msg;
		msg.msgID = MSG_OpenFile_Failed;
		if (SOCKET_ERROR == send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0))
		{
			printf("send failed:%d\n", WSAGetLastError());
		}
		return false;
	}

	// ��ȡ�ļ���С
	fseek(pread, 0, SEEK_END);
	g_fileSize = ftell(pread);
	fseek(pread, 0, SEEK_SET);

	// ���ļ���С�����ͻ���
	struct MsgHeader msg;
	msg.msgID = MSG_FileSize;
	msg.fileInfo.filesize = g_fileSize;

	// ��ȡ�ļ�·��
	char tfname[200] = { 0 }, text[100] = { 0 };
	_splitpath(pmsg->fileInfo.filename, NULL, NULL, tfname, text);

	//ƴ��
	strcat(tfname, text);

	// ���ļ����������ṹ��
	strcpy(msg.fileInfo.filename, tfname);

	// ���͸��ͻ���
	if (SOCKET_ERROR == send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0))
	{
		printf("send file(readFile()): %d\n", WSAGetLastError());
		return false;
	}

	//���ļ�����
	g_FileBuf = (char*)calloc(g_fileSize + 1, sizeof(char));

	if (g_FileBuf == NULL)
	{
		printf("calloc failed\n");
		return false;
	}
	fread(g_FileBuf, (size_t)sizeof(char), (size_t)g_fileSize, pread);
	g_FileBuf[g_fileSize] = '\0';

	fclose(pread);
	return true;
}

bool SendFile(SOCKET clifd, struct MsgHeader* pmsg)
{
	struct MsgHeader msg;
	msg.msgID = MSG_Ready_Read;

	// ����ļ����ȴ���ÿ�����ݰ��ܴ��ݵĴ�С(PACKET_SIZE) ��ô�ֿ鴫��
	for (size_t i = 0; i < g_fileSize; i += PACKET_SIZE)
	{
		msg.packet.nStart = i;

		//���Ĵ�С�����ļ������ݵĴ�С
		if (i + PACKET_SIZE + 1 > g_fileSize)
		{
			msg.packet.nsize = g_fileSize - i;
		}
		else
		{
			//���Ĵ�СС���ļ������ݵĴ�С
			msg.packet.nsize = PACKET_SIZE;
		}

		memcpy(msg.packet.buf, g_FileBuf + msg.packet.nStart, msg.packet.nsize);

		if (msg.packet.buf == NULL)
		{
			printf("memcpy failed(send file())\n");
			return false;
		}

		if (SOCKET_ERROR == send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0))
		{
			printf("file send failed(send file()):%d\n", WSAGetLastError());
			return false;
		}
	}
	return true;
}

/*
 1.�ͻ������������ļ�->���ļ������͸�������
 2.���������տͻ����͵��ļ���->�����ļ���,�ҵ��ļ�;���ļ���С���͸��ͻ���
 3.�ͻ��˽����ļ���С->׼����ʼ����,�����ڴ�,���߷�����
 4.����������ָ��,��������ʼ����
 5.�ͻ��˽����ļ�,���� ->�������,���߷�������
 6.�ر�����
 */