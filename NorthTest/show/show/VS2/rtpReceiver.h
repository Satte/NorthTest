#pragma once

#include <list>
#include <stdint.h>
#include "rtp.h"
#include "RWLock.h"

struct revDescriptor;                    //视频源接收描述符

class RTPReceiver
{
	class RTPFrameEntry
	{
	public:
		RTPFrameEntry(RTPFrame* frame) : m_frame(frame), m_ts(frame->TS()) { }
		RTPFrameEntry(uint32_t ts) : m_frame(nullptr), m_ts(ts) { }
		RTPFrameEntry(RTPFrameEntry&& other) : m_frame(other.m_frame), m_ts(other.m_ts) { other.m_frame = nullptr; }
		~RTPFrameEntry() { delete m_frame; }

		bool operator<(const RTPFrameEntry& other) const { return m_ts < other.m_ts; }

		uint32_t TS() const { return m_ts; }
		bool Completed() const { return m_frame->Completed(); }
		RTPFrame* Get() { return m_frame; }
		RTPFrame* Pop() { RTPFrame* ret = m_frame; m_frame = nullptr; return ret; }

		std::list<uint16_t> getMissQueue();
	private:
		RTPFrame* m_frame;
		uint32_t m_ts;
	};
public:
	RTPReceiver(sem_t *hSema, revDescriptor* revDpt);
    RTPReceiver(revDescriptor* revDpt);
	~RTPReceiver();

	void InsertRtp(const TRTPPacket* packet);					//RTP解包
	void GetLostPackets(std::list<uint16_t>& list, uint32_t delay);	//统计并得到丢包队列（不统计 dealy 毫秒以内帧的丢包）
	RTPFrame* GetOneFrame();						//取得准备好的帧
	void ClearFrameBuffer();						//清空帧队列
	void setBufferMaxDelay(uint32_t delay);			//设置缓冲最大延迟
    void setSema(sem_t *hSema);

private:
	std::list<RTPFrameEntry> m_receiveQueue;//收到的帧数据队列
	std::list<RTPFrame*> m_preparedQueue;
	CRITICAL_SECTION m_FrameCritical;		//保护帧缓冲的临界区
	sem_t *m_FrameSema;						//反映帧缓冲队列中已缓冲的帧（这些帧可能是不完整的）有多少的信号量
	revDescriptor* m_revDpt;				//接收描述符指针
	uint32_t m_iFrameQueueTimeDelay;		//帧缓存延迟/队列的长度（以90KHz的时间戳形式表示）
	bool m_bWaitNextIFrame;					//是否等待下一个I帧到来（当前I帧未能顺利接收解码时设置）
};
