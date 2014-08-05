#pragma once

#include <vector>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <unistd.h>
#include "Types.h"
#include "rtp.h"
#include "rtpReceiver.h"
#include "rtcpReceiver.h"

struct revDescriptor;                      //视频源接收描述符
class CRender;                             //视频渲染器
class CDecoder;                            //视频解码器
struct TFrame;

//功能：接收、解码、渲染（解码图像）
class CChannelRecv
{
public:
	CChannelRecv(revDescriptor* revDpt);
	~CChannelRecv();

    //Render相关
	void startOneRender(VIEW *hWnd);	//开始一个渲染器
	void stopOneRender();	//根据绘制窗口句柄，关闭一个Render；如果所有Render都已经关闭，停止绘制线程，停止接收rtp

	void parseRtp(const TRTPPacket* packet);	//解析rtp报文
	void parseRtcp(const TPacket* packet) { m_RtcpRecv.parseRtcp(packet); delete packet; }	//解析rtcp报文

	void GetLostPackets(std::list<uint16_t>& list, uint32_t delay) { m_RtpRecv.GetLostPackets(list, delay); }	//统计并得到丢包队列（不统计 dealy 毫秒以内帧的丢包）

	bool isStartRecvRtp() const { return m_isStart_Rtp; }		//已经开始RTP接收了？
	uint32_t getSSRC() const { return m_revDpt->m_ssrc; }		//得到给信道的ssrc
	revDescriptor* getRevDpt() const { return m_revDpt; }
private:

	std::unique_ptr<CRender> m_pRender;		//渲染器
	std::unique_ptr<CDecoder> m_pDecoder;	//解码器
	sem_t   m_hSema_Render;					//反映帧缓冲队列已缓冲完整的帧的信号量
	sem_t   m_hEvtReadyForRender;			//通知绘制线程可以开始绘制事件
	pthread_t   m_hThreadOnRender;				//threadOnRender的线程句柄
	revDescriptor* m_revDpt;				//接收源描述符，包括对接收源的一些参数描述
	TFrame* m_pFrame;						//
	RTCPReceiver m_RtcpRecv;				//RTCP报文解析器
	RTPReceiver m_RtpRecv;					//RTP报文解析器
	int m_iWidth, m_iHeight;				//视频长和宽
	volatile long m_lLock;					//
	volatile bool m_isStart_Rtp;			//开始接收RTCP报文的标志位
	RTPPayloadType m_payLoadType;			//RTP负载类型，以决定使用的编码器类型
//====================add by gs=================== 
    volatile bool m_bRender;
//===============================================

	static void *threadOnRender(void* param) { reinterpret_cast<CChannelRecv*>(param)->OnRender(); return 0; }	//绘制线程
	void OnRender();	//负责实际的绘制工作
};
