#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>
#include <stdbool.h>

#define PACKET_SIZE (1024 - sizeof(int)*3)

#define	SPORT 8888 // �������˿�


//�����ʶ
enum MSGTAG {
	MSG_FileName = 1,
	MSG_FileSize = 2,
	MSG_Ready_Read = 3, //�ͻ���׼������
	MSG_Send = 4, // ����
	MSG_Successed = 5, // �������
	MSG_OpenFile_Failed = 6 //���߿ͻ����ļ��Ҳ���
};

// ��װ��Ϣͷ
#pragma pack(1) // ���ýṹ��1�ֽڶ���
struct MsgHeader
{
	enum MSGTAG msgID; //��ǰ���
	union MyUnion
	{
		struct
		{
			int filesize; //�ļ���С    4
			char filename[256]; //�ļ���   256
		} fileInfo; // 260

		struct
		{
			int nsize;  //�������ݴ�С
			int nStart; //���ı��
			char buf[PACKET_SIZE];
		} packet;
	};
};
#pragma pack()

//struct MsgHeader {
//    enum MSGTAG msgID; //��ǰ���
//    struct {
//        char filename[256]; //�ļ���
//        int filesize; //�ļ���С
//    } fileInfo;
//};

bool InitSocket();

bool ColseSocket();

bool ListenToClient();

bool processMessage(SOCKET clifd);

bool readFile(SOCKET, struct MsgHeader*);

// �����ļ�
bool SendFile(SOCKET, struct MsgHeader*);


#endif //SERV
