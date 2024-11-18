//
// Created by Administrator on 2024/11/12.
//
#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>
#include <stdbool.h>

#pragma comment(lib,"ws2_32.lib")

#define SPORT 8888

#define PACKET_SIZE (1024 - sizeof(int)*3)

//定义标识
enum MSGTAG {
	MSG_FileName = 1,
	MSG_FileSize = 2,
	MSG_Ready_Read = 3, //客户端准备接受
	MSG_Send = 4, // 发送
	MSG_Successed = 5, // 传输完成
	MSG_OpenFile_Failed = 6 //告诉客户端文件找不到
};
#pragma pack(1) // 设置结构体1字节对齐
struct MsgHeader 
{
	enum MSGTAG msgID; //当前标记
	union MyUnion 
	{
		struct 
		{
			int filesize; //文件大小    4
			char filename[256]; //文件名   256
		} fileInfo; // 260

		struct 
		{
			int nsize;  //包的数据大小
			int nStart; //包的编号
			char buf[PACKET_SIZE];
		} packet;
	};
};
#pragma pack()


bool InitSocket();

bool ColseSocket();

bool ConnectToHost();

bool processMessage(SOCKET serfd);

void DownloadFileName(SOCKET serfd);

void ReadyRead(SOCKET serfd, struct MsgHeader* pmsg);


// 写文件
bool writeFile(SOCKET serfd, struct MsgHeader* pmsg);

#endif

