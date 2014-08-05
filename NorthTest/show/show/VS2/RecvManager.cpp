#include <algorithm>
//#include <hash_map>
#include "Codec.h"
#include "ChannelRecv.h"
#include "RecvManager.h"
#include "mediaTimer.h"
#include "sessionStatsManager.h"
#include "rtp.h"


RecvManager::RecvManager()
	: m_delay(10000)      // m_delay 毫秒
	, m_ipGroup(0x0b0aa8c0), m_iPort(8000), m_bNat(false)
    , m_bCheck(false), m_bRecvRtcp(false), m_bRecvRtp(false)
{
}

RecvManager::~RecvManager()
{
//	TRACE("In ~RecvManager!\n");
    stopRecv();

}

//总的启动函数
void RecvManager::startRecv()
{
	if (m_bCheck|| m_bRecvRtcp|| m_bRecvRtp) stopRecv();

	m_ipGroup = g_SessionStateMgr.getNetAddr();	//监听IP地址
	m_iPort   = g_SessionStateMgr.getRtpPort();	//监听的端口号
	m_bNat    = g_SessionStateMgr.IsUseNat();		//是否使用NAT模式


	//初始化各个成员变量
    sem_init(&m_hSemaPacket_Rtcp, 0, 0);
    sem_init(&m_hSemaPacket_Rtp, 0, 0);
    sem_init(&m_deleteIM, 0, 0);

	m_revRtcp.Open(g_SessionStateMgr.getRtcpPort(), m_ipGroup, &m_hSemaPacket_Rtcp, m_bNat);
	m_revRtp.Open(m_iPort, m_ipGroup, &m_hSemaPacket_Rtp, m_bNat);
	g_SessionStateMgr.setDeleteIMEvent(&m_deleteIM);					//设置消息句柄

    pthread_create(&m_hThreadOnPacket_Rtcp, NULL, threadOnRtcpPacket, this);
    m_bRecvRtcp = true;

    pthread_create(&m_hThreadOnPacket_Rtp, NULL, threadOnRtpPacket, this);
    m_bRecvRtp = true;

    pthread_create(&m_hThreadOnCheck, NULL, threadOnActiveCheck, this);
    m_bCheck = true;
}

//总停止函数
void RecvManager::stopRecv()
{

	if (m_bCheck)
	{
		//结束检查线程
        pthread_cancel(m_hThreadOnCheck);
        pthread_join(m_hThreadOnCheck, NULL);
		g_SessionStateMgr.setDeleteIMEvent(nullptr);
        sem_destroy(&m_deleteIM);
        m_bCheck = false;

	}

	if (m_bRecvRtp)
	{
		//结束接收线程
        pthread_cancel(m_hThreadOnPacket_Rtp);
        pthread_join(m_hThreadOnPacket_Rtp, NULL);
		//关闭接收器(注意各个量关闭的先后顺序)
		m_revRtp.Close();
        sem_destroy(&m_hSemaPacket_Rtp);
        m_bRecvRtp = false;
	}

	if (m_bRecvRtcp)
	{
		//结束接收线程
        pthread_cancel(m_hThreadOnPacket_Rtcp);
        pthread_join(m_hThreadOnPacket_Rtcp, NULL);
		//关闭接收器
		m_revRtcp.Close();
        sem_destroy(&m_hSemaPacket_Rtcp);
        m_bRecvRtcp = false;

	}

	//清空接收信道队列
	while(!m_channelRevList.empty())
	{		   
		uint32_t ssrc = m_channelRevList.front()->getSSRC();
	    delete m_channelRevList.front();
		m_channelRevList.pop_front();
		g_SessionStateMgr.deleteRev(ssrc);
	}
}

//启动具体的某一路视频
bool RecvManager::startRecvOneChannel(unsigned int ssrc, VIEW *pWin)
{
	bool ret = true;
	//保护锁
	m_lockChannelList.acquireReadLock();

	auto channel = std::find_if(m_channelRevList.begin(), m_channelRevList.end(), [ssrc](CChannelRecv* channel)->bool { return channel->getSSRC() == ssrc; });
	if (channel != m_channelRevList.end())
		(*channel)->startOneRender(pWin);	//开始绘制
	else
		ret = false;

	m_lockChannelList.releaseReadLock();

	return ret;
}

//停止具体的某一路视频
bool RecvManager::stopRecvOneChannel(unsigned int ssrc)
{
	bool ret = true;
	//保护锁
	m_lockChannelList.acquireReadLock();

	auto channel = std::find_if(m_channelRevList.begin(), m_channelRevList.end(), [ssrc](CChannelRecv* channel)->bool { return channel->getSSRC() == ssrc; });
	if (channel != m_channelRevList.end())
		(*channel)->stopOneRender();	//停止绘制
	else
		ret = false;

	m_lockChannelList.releaseReadLock();
    return ret;
}

//threadOnRtcpPacket调用该函数，完成实际的功能
void RecvManager::OnRtcpPacket()
{
    while(sem_wait(&m_hSemaPacket_Rtcp) == 0){
        pthread_testcancel();
		//接收到一个RTCP报文
		TPacket* packet = m_revRtcp.GetPacket();
		uint32_t ssrc = ntohl(reinterpret_cast<rtcp_header*>(packet->Buffer)->ssrc);

		//是自己发出的RTCP报文，扔掉!
		if (g_SessionStateMgr.findSrc(ssrc))
		{
			delete packet;
			continue;
		}
		else if (!(g_SessionStateMgr.findRev(ssrc)))
		{
			//接收到一个新的视频源发来的RTCP报文
			//保护锁
			m_lockChannelList.acquireWriteLock();

			//增加新的接收信道
			//因为rtcp是一定要收的，以便在UI中可以看到有多少发送源，因此不需要特地开启
			auto channel = new CChannelRecv(g_SessionStateMgr.addRev(ssrc));
			m_channelRevList.push_front(channel);
			channel->parseRtcp(packet);

			m_lockChannelList.releaseWriteLock();
		}
		else
		{
			//解析一个RTCP报文
			//保护锁
			m_lockChannelList.acquireReadLock();

			//先找到对应的revList中的描述符结点指针，利用lambda表达式替换原先的函数对象
			auto channel = std::find_if(m_channelRevList.begin(), m_channelRevList.end(), [ssrc](CChannelRecv* channel)->bool { return channel->getSSRC() == ssrc; });

			if (channel != m_channelRevList.end())
   			   (*channel)->parseRtcp(packet);
			else
				delete packet;

			m_lockChannelList.releaseReadLock();
		}
    }
}

//threadOnRtpPacket调用该函数，完成实际的功能
void RecvManager::OnRtpPacket()
{
	//首先检查sessionStatsManager中接收源描述符链表中是否存在相应ssrc的结点
	//如果不存在，则在revList中新增加一个结点，表示又探测到一路视频

    while(sem_wait(&m_hSemaPacket_Rtp) == 0){
        pthread_testcancel();
		//接收到一个RTP报文
		TRTPPacket* packet = static_cast<TRTPPacket*>(m_revRtp.GetPacket());
		uint32_t ssrc = ntohl(packet->Header()->ssrc);

		
		if (g_SessionStateMgr.findSrc(ssrc))
		{
			//这是自己发出的RTP报文还，扔掉!
			delete packet;
			continue;
		}
		else
		{
			//这里还应该检查该路视频是不是UI指定要的那几路
			//将rtp报文放置到对应的接收信道缓冲区，由对应的rtp接收信道负责rtp解包
			//保护锁
			m_lockChannelList.acquireReadLock();

			//先找到对应的revList中的描述符结点指针，利用lambda表达式替换原先的函数对象
			auto channel = std::find_if(m_channelRevList.begin(), m_channelRevList.end(), [ssrc](CChannelRecv* channel)->bool { return channel->getSSRC() == ssrc; });

			//应该是UI已经指定了RTP报文，因此对应的RTP接收线程应该已经启动了
			if (channel != m_channelRevList.end())
				(*channel)->parseRtp(packet);
			else
				delete packet;
	
			m_lockChannelList.releaseReadLock();
		}

    }
}

//threadOnActiveCheck调用该函数，完成实际的功能
void RecvManager::OnActiveCheck()
{

    while(1){
        pthread_testcancel();
        sleep(m_delay/1000);
		unsigned int tRTCPDeadline = MediaTimer::GetTickCount() -  MAX_RTCP_IDLE_TIME;
		unsigned int tRTPDeadline = MediaTimer::GetTickCount() - MAX_RTP_IDLE_TIME;	//added by zyc

		m_lockChannelList.acquireWriteLock();
		for(auto channel = m_channelRevList.begin(); channel != m_channelRevList.end(); )
		{
			if (((*channel)->getRevDpt()->m_tLastRTCPActive <= tRTCPDeadline && (*channel)->getRevDpt()->m_tLastRTPActive <= tRTPDeadline) || (*channel)->getRevDpt()->m_bDelete)
			{
				unsigned int ssrc = (*channel)->getSSRC();
				bool bIsRtpRev = (*channel)->isStartRecvRtp();
				//要删除一个信道时，一定要先删除channel这样就停止了各个读写线程，
				//再在sessionManager的m_revList中删除对应revDpt，防止各个读写线程
				//对已经删除的revDpt进行读写
				delete (*channel);
				channel = m_channelRevList.erase(channel);

				g_SessionStateMgr.deleteRev(ssrc);
			}
			else
				channel++;
		} 
		m_lockChannelList.releaseWriteLock();
    }
	//主控线程已经发出停止的信号了
}

