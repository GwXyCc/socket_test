
#include <stdio.h>
#include "server.h"

#pragma comment(lib,"ws2_32.lib")

char g_recvBuffer[1024]; //用来接受客户端发送的消息
int g_fileSize; //文件大小
char* g_FileBuf; //存储文件内容

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
	// 创建socket套节字	地址,端口号
	SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == serfd) {
		printf("socket failed:%d\n", WSAGetLastError());
		return false;
	}
	// 绑定ip和端口号
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(SPORT); // 字节序转换为网络字节序
	serAddr.sin_addr.S_un.S_addr = ADDR_ANY; // 监听本机所有网口

	if (0 != bind(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr))) {
		printf("socket failed:%d\n", WSAGetLastError());
		return false;
	}

	// 监听客户端链接
	if (0 != listen(serfd, 10)) {
		printf("socket failed:%d\n", WSAGetLastError());
		return false;
	}

	// 有客户端,接受链接
	struct sockaddr_in cliaddr;
	int len = sizeof(cliaddr);
	SOCKET clifd = accept(serfd, (struct sockaddr*)&cliaddr, &len);
	if (INVALID_SOCKET == clifd) {
		printf("socket failed:%d\n", WSAGetLastError());
		return false;
	}

	printf("server has started, waiting conection....\n");

	// 处理消息
	while (processMessage(clifd)) {
		Sleep(5000);
	}
}

bool processMessage(SOCKET clifd) {
	// 成功接收消息,返回接收字节数,失败返回0
	int nRes = recv(clifd, g_recvBuffer, 1024, 0);
	struct MsgHeader* msg = (struct MsgHeader*)g_recvBuffer;

	if (nRes <= 0)
	{
		printf("客户端下线:%d\n", WSAGetLastError());
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

	// 获取文件大小
	fseek(pread, 0, SEEK_END);
	g_fileSize = ftell(pread);
	fseek(pread, 0, SEEK_SET);

	// 把文件大小发给客户端
	struct MsgHeader msg;
	msg.msgID = MSG_FileSize;
	msg.fileInfo.filesize = g_fileSize;

	// 获取文件路径
	char tfname[200] = { 0 }, text[100] = { 0 };
	_splitpath(pmsg->fileInfo.filename, NULL, NULL, tfname, text);

	//拼接
	strcat(tfname, text);

	// 将文件名拷贝到结构体
	strcpy(msg.fileInfo.filename, tfname);

	// 发送给客户端
	if (SOCKET_ERROR == send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0))
	{
		printf("send file(readFile()): %d\n", WSAGetLastError());
		return false;
	}

	//读文件内容
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

	// 如果文件长度大于每个数据包能传递的大小(PACKET_SIZE) 那么分块传输
	for (size_t i = 0; i < g_fileSize; i += PACKET_SIZE)
	{
		msg.packet.nStart = i;

		//包的大小大于文件总数据的大小
		if (i + PACKET_SIZE + 1 > g_fileSize)
		{
			msg.packet.nsize = g_fileSize - i;
		}
		else
		{
			//包的大小小于文件总数据的大小
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
 1.客户端请求下载文件->把文件名发送给服务器
 2.服务器接收客户发送的文件名->根据文件名,找到文件;把文件大小发送给客户端
 3.客户端接收文件大小->准备开始接受,开辟内存,告诉服务器
 4.服务器接收指令,服务器开始发送
 5.客户端接收文件,保存 ->接受完成,告诉服务器、
 6.关闭链接
 */