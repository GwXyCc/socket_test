#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>
#include <stdbool.h>

#define PACKET_SIZE (1024 - sizeof(int)*3)

#define	SPORT 8888 // 服务器端口


//定义标识
enum MSGTAG {
	MSG_FileName = 1,
	MSG_FileSize = 2,
	MSG_Ready_Read = 3, //客户端准备接受
	MSG_Send = 4, // 发送
	MSG_Successed = 5, // 传输完成
	MSG_OpenFile_Failed = 6 //告诉客户端文件找不到
};

// 分装消息头
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

//struct MsgHeader {
//    enum MSGTAG msgID; //当前标记
//    struct {
//        char filename[256]; //文件名
//        int filesize; //文件大小
//    } fileInfo;
//};

bool InitSocket();

bool ColseSocket();

bool ListenToClient();

bool processMessage(SOCKET clifd);

bool readFile(SOCKET, struct MsgHeader*);

// 发送文件
bool SendFile(SOCKET, struct MsgHeader*);


#endif //SERV
