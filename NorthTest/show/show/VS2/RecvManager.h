#pragma once

#include <list>
//#include <hash_map>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include "UDPSocket.h"
#include "ChannelRecv.h"
#include "RWLock.h"


class RecvManager
{

public:
	RecvManager();
	~RecvManager();

	void startRecv();														//总的启动函数
	void stopRecv();														//总的停止函数
	bool startRecvOneChannel(unsigned int ssrc, VIEW *pWin);					//启动具体的某一路视频
	bool stopRecvOneChannel(unsigned int ssrc);					//停止具体的某一路视频
	void test(){printf("i am in RecvManager!\n");}

protected:
    CUDPSocketRecv m_revRtcp;					//RTCP报文接收器
	CUDPSocketRecv m_revRtp;					//RTP报文接收器
	std::list<CChannelRecv*> m_channelRevList;	//接收信道列表
    RWLock m_lockChannelList;					//对接收信道列表加锁，保持删除和读取的互斥
	//以下两个HANDLE由RTCP接收所用
	sem_t m_hSemaPacket_Rtcp;					//反映m_recv已缓冲报文多少的信号量
	pthread_t m_hThreadOnPacket_Rtcp;				//threadOnPacket线程的句柄，检查该线程退出时使用
	//以下两个HANDLE由RTP接收所用
	sem_t m_hSemaPacket_Rtp;					//反映m_recv已缓冲报文多少的信号量
	pthread_t m_hThreadOnPacket_Rtp;				//threadOnPacket线程的句柄，检查该线程退出时使用
	//定期检查已经退出的接收源
	sem_t m_deleteIM;							//立即执行检查例程
    pthread_t m_hThreadOnCheck;					//执行检查的线程句柄
	int   m_delay;								//过多久检查一次链表中的各个接收源的存活情况, 毫秒
    uint32_t m_ipGroup;							//监听IP地址
	uint16_t m_iPort;								//监听的端口号
	bool  m_bNat;								//是否使用NAT模式

//	HWND hwnd;									//VS2Dlg窗体句柄            !!!!!!!!!!!!!!!!!!alter by gs

//=============== state variable==================
    volatile bool m_bCheck;
    volatile bool m_bRecvRtcp;
    volatile bool m_bRecvRtp;

//===============================================

	//RTCP报文的接收线程
	static void *threadOnRtcpPacket(void* param) { reinterpret_cast<RecvManager*>(param)->OnRtcpPacket(); return 0; }	//有RTCP报文到达后该线程激活
	void   OnRtcpPacket();				                        //threadOnRtcpPacket调用该函数，完成实际的功能

	//RTP报文的接收线程
	static void * threadOnRtpPacket(void* param) { reinterpret_cast<RecvManager*>(param)->OnRtpPacket(); return 0; }	//有Rtp报文到达后该线程激活
	void   OnRtpPacket();				                        //threadOnRtpPacket调用该函数，完成实际的功能

	//定期检查接收源存活情况的线程
	static void *threadOnActiveCheck(void * param) { reinterpret_cast<RecvManager*>(param)->OnActiveCheck(); return 0; }	//当定时器一到就被激活，检查sesionStatsManager中的revList
	void   OnActiveCheck();                                      //threadOnActiveCheck调用该函数，检查各个信道的存活情况(各个信道必须有周期性的rtcp报文到达)


};

