#pragma once

#include <vector>
#include <memory>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "Types.h"
#include "UDPSocket.h"
#include "rtcpSender.h"
#include "Encoder.h"

class CRender;                    //视频渲染器
class CCapture;                   //视频捕捉器


//功能：捕捉、编码、发送、渲染（原始图像）
class SendManager
{
public:
	SendManager(const ChannelInfo& info, VIEW *view);
	~SendManager();

	bool beginRender();	//开始在m_pView指定的窗口里绘制该视频, 如果已经在绘制了,会停止以前的, 在新指定的窗口里绘制
	bool stopRender();	//停止该视频的绘制
	bool beginSender();				//开始视频发送
	bool stopSender();				//停止视频发送
	void Close();					//停止发送、绘制后做清理工作
	void ResendPackets(std::list<uint16_t> seqs);	//重发指定序号的报文，序号应以升序排列（遇到回绕时，应将回绕的序列置在后方）
	bool isRender() const { return this ? m_bRendVideo : false; }	//在视频渲染吗？
	bool isSender() const { return this ? m_bSendVideo : false; }	//在视频发送吗?
	bool beginSender(const SendInfo& info) { setSendInfo(info); return beginSender(); }
	unsigned int getSSRC() const { return (this && m_srcDpt) ? m_srcDpt->m_ssrc : -1; }	//得到该发送源的ssrc
	int GetActualBW() const { return this ? m_iActualBW : 0; }
	void setChannelInfo(const ChannelInfo& info) { m_channelInfo = info; }
	const ChannelInfo& getChannelInfo() const { return m_channelInfo; }
	ChannelInfo& getChannelInfo() { return m_channelInfo; }
	void setSendInfo(const SendInfo& info) { static_cast<SendInfo&>(m_channelInfo) = info; }

private:
	ChannelInfo					m_channelInfo;
	RTCPSender					m_rtcpSend;			//rtcp报文发送器
	CUDPSocketSend				m_send;				//网络发送器
	CRITICAL_SECTION			m_renderLock;		//渲染器区互斥锁
	CRITICAL_SECTION			m_ResendLock;		//
	std::list<uint16_t>			m_ResendList;
	std::unique_ptr<CRender>	m_pRender;			//渲染器,only one
	std::unique_ptr<CCapture>	m_pCapture;			//采集器
	std::unique_ptr<CEncoder>	m_pEncoder;			//编码器
	std::shared_ptr<TFrame>		m_FrameBuf;			//待发送的帧
	srcDescriptor*				m_srcDpt;			//关于该视频发送源的描述符结点指针
	sem_t                       m_hSemaFrame;		//标志采集器已经捕捉的帧数
	pthread_t                   m_hThreadOnFrame;	//threadOnFrame线程的句柄，检查该线程退出时使用
	pthread_t                   m_hThreadSend;		//发送线程
	uint32_t					m_iPacketBufferTime;//报文缓存队列的长度（以90KHz的时间戳形式表示）
	uint32_t					m_iRtpSeq;
	int							m_iActualBW;		//设定发送最大带宽和实际带宽(实际带宽以bps为单位)
	volatile bool				m_bRendVideo;		//是否渲染视频
	volatile bool				m_bSendVideo;		//是否发送视频

    VIEW                        *m_pView;
    sem_t                       m_hSemaResend;

	static void *threadOnFrame(void* param) { reinterpret_cast<SendManager*>(param)->OnFrame(); return 0; }	//采集到视频后激活
	void OnFrame();
    

	static  void *threadSend(void* param) { reinterpret_cast<SendManager*>(param)->SendProc(); return 0; }
	void SendProc();

	//Sender和Render相同部分的清理工作
	inline void commonClose();
};
