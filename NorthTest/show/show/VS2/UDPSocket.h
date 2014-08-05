#pragma once

#include <list>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <poll.h>
#include <unistd.h> /* for close() */
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include "Types.h"
#include "RWLock.h"

#define INVALID_SOCKET  (~0)
#define SOCKET_ERROR    (-1)
#define TIME_WAITFOR_PACKET 30
//#define IN_CLASSD(i)            (((long)(i) & 0xf0000000) == 0xe0000000)

class CUDPSocketBase
{
public:
	CUDPSocketBase() : m_sock(INVALID_SOCKET) { }

protected:
	void OpenNATSocket(uint32_t ipServer, uint16_t iPort);
	void OpenNormalSocket();

	int m_sock;							//socket
};

//功能：发送UDP报文
class CUDPSocketSend
	: public CUDPSocketBase
{
public:
	~CUDPSocketSend(){ Close(); }

	void Open(uint32_t ipDst, uint16_t iPortDst, int iTTL = 127, bool bNat = false);//打开socket，设置目标地址
	void Close();						    //关闭socket
	int Send(const TPacket& packet);

protected:
	sockaddr_in m_saDst;					//目标地址
};

//功能：接收UDP报文，将其保存在内部的缓冲区中，等待外部模块提取
class CUDPSocketRecv
	: public CUDPSocketBase
{
	static const int MAX_UDP_QUEUE = 1024;
public:
	CUDPSocketRecv()
		: m_hThreadOnPacket(nullptr)
	{
		InitializeCriticalSection(&m_csList);
        m_bEndRecv = false;
	}

	~CUDPSocketRecv()
	{
		Close();
		DeleteCriticalSection(&m_csList);
	}

	void Open(int iPortListen, uint32_t ipGroup, sem_t *hSemaPacket, bool bNat = false);//打开socket，启动监听
	void Close();						    //停止监听，关闭socket
	TPacket* GetPacket();					//提取一个报文

protected:
	sem_t   *m_hSemaPacket;					//标志“已接收报文缓冲队列”内有无报文的信号量
	pthread_t  m_hThreadOnPacket;				//threadOnPacket线程句柄，检查线程退出时使用
	CRITICAL_SECTION m_csList;				//保护“已接收报文缓冲队列”的互斥量
    volatile bool m_bEndRecv;
	std::list<TPacket*> m_Packets;			//已接收报文缓冲队列

	static void * threadOnPacket(void* param) { reinterpret_cast<CUDPSocketRecv*>(param)->ReceiveProc(); return 0; }	//报文到达后该线程执行一次

	void ReceiveProc();
};
