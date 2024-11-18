#include <stdio.h>
#include "string.h"
#include "client.h"


char g_recvBuf[1024]; //接收消息的缓冲区
char* g_fileBuf;    // 接收存储文件内容
int g_fileSize;     // 文件分配的大小
char g_FileName[256];    // 保存服务器发送过来的文件名

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
	// 创建socket套节字	地址,端口号
	SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == serfd)
	{
		printf("socket faild:%d\n", WSAGetLastError());
		return false;
	}
	// 绑定ip和端口号
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(SPORT); // 字节序转换为网络字节序
	serAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // 监听本机所有网口

	// 链接到服务器
	if (0 != connect(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr)))
	{
		printf("socket faild:%d\n", WSAGetLastError());
		return false;
	}

	DownloadFileName(serfd);

	// 处理消息
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
	char FileName[1024] = "你好,我是隔壁的泰山~";
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
	// 准备内存  
	strcpy(g_FileName, pmsg->fileInfo.filename);
	g_fileSize = pmsg->fileInfo.filesize;
	g_fileBuf = (char*)calloc(g_fileSize + 1, sizeof(char)); //申请空间
	if (g_fileBuf == NULL)
	{
		// 内存申请失败
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
	// 给服务器发送 MSG_Ready_Read
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

	// 计算进度并显示
	int bytesReceived = nStart + nsize; // 已接收的字节数
	double progress = (double)bytesReceived / g_fileSize * 100;

	// 显示进度条
	printf("\rProgress: %.2f%%", progress);
	fflush(stdout); // 确保立即输出到控制台

	// 如果接收的是完整的包
	if (nStart + nsize >= g_fileSize)
	{
		FILE* pwirte = fopen(g_FileName, "wb");

		if (pwirte == NULL)
		{
			// 打开文件错误
			printf("write file failed..\n");
			return false;
		}

		fwrite(g_fileBuf, sizeof(char), g_fileSize, pwirte);

		fclose(pwirte);
		free(g_fileBuf);    // 释放内存
		g_fileBuf = NULL;

		struct MsgHeader msg;
		msg.msgID = MSG_Successed;	//下载完成
		send(serfd, (char*)&msg, sizeof(struct MsgHeader), 0);
	}
	return true;
}