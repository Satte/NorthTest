#include "sessionStatsManager.h"
#include "MediaTimer.h"
#include "RTPReceiver.h"
#include "rtp.h"
#include "Decoder.h"
#include "Types.h"
#include <algorithm>

//============================================================================

RTPFrame::RTPFrame(const TRTPPacket* packet)
	: m_Buffer(new const TRTPPacket*[packet->Extension()->Total()])
	, m_iSize(packet->Extension()->Total())
	, m_iCount(0), m_iRetransmit(0)
	, m_ts(ntohl(packet->Header()->ts))
	, m_wStartSeq(ntohs(packet->Header()->seq) - packet->Extension()->Index())
	, m_StereoFlag((StereoFrameMode)packet->Extension()->stereo)
	, m_pt(packet->Header()->pt)
	, m_keyframe(packet->Extension()->iframe)
	, m_iFrameLength(0)
{
	memset(m_Buffer, 0, sizeof(TRTPPacket*) * m_iSize);
	InsertPacket(packet);
}

RTPFrame::~RTPFrame()
{
	for(int i = 0; i < m_iSize; i++)
		delete (TPacket*)m_Buffer[i];
	delete[] m_Buffer;
}

bool RTPFrame::InsertPacket(const TRTPPacket* packet)
{
	uint16_t index = packet->Extension()->Index();
	if (packet->Header()->pt == m_pt &&
		packet->Extension()->iframe == m_keyframe &&
		packet->Extension()->Total() == m_iSize &&
		ntohs(packet->Header()->seq) == m_wStartSeq + index)
	{
		if (index == m_iSize - 1 && !packet->Header()->m) return false;

		if (m_Buffer[index] == nullptr)
		{
			m_Buffer[index] = packet;
			m_iFrameLength += CDecoder::GetPayloadLength(packet);
			m_iCount++; m_iRetransmit += packet->Extension()->resend;
			return true;
		}
	}
	return false;
}

//============================================================================

RTPReceiver::RTPReceiver(sem_t *hSema, revDescriptor* revDpt)
	: m_FrameSema(hSema)
	, m_bWaitNextIFrame(false)
	, m_revDpt(revDpt)							//描述符指针
	, m_iFrameQueueTimeDelay(MAX_FRAME_TIME_RANGE)
{
	//创建互斥锁
	InitializeCriticalSection(&m_FrameCritical);
}

RTPReceiver::RTPReceiver(revDescriptor* revDpt)
{
    RTPReceiver(NULL, revDpt);
}
RTPReceiver::~RTPReceiver()
{
	ClearFrameBuffer();

	//销毁互斥锁
	DeleteCriticalSection(&m_FrameCritical);
}

// 插入RTP报文
void RTPReceiver::InsertRtp(const TRTPPacket* packet)
{
	//如果这是第一次调用该函数，则设置第一次接收到RTP报文的时间
	if (m_revDpt->m_iLastCalTS == 0)
		m_revDpt->m_iLastCalTS = MediaTimer::GetTickCount();

	//计算收到的字节数
	__sync_fetch_and_add((long*)&m_revDpt->m_iBytesRecv, packet->Length); //已经收到的字节数

	//RTP报文的时延，和传输抖动估计（RFC 3550，p.38）
	uint32_t transit = MediaTimer::media_ts() - ntohl(packet->Header()->ts);
	uint32_t d = (transit >= m_revDpt->m_transit) ? transit - m_revDpt->m_transit : m_revDpt->m_transit - transit;
	m_revDpt->m_transit = transit;							//记录传输时延
	m_revDpt->m_jitter += (d - m_revDpt->m_jitter) / 16;	//更改抖动

	//将一个检查完毕，合法的RTP报文放入对应的帧缓冲队列中去
	//每个帧内的RTP报文拥有一样的时间戳
	uint32_t iTS = ntohl(packet->Header()->ts);
	uint16_t pkt_seq = ntohs(packet->Header()->seq);

	//当前报文所属帧的插入位置
	std::list<RTPFrameEntry>::iterator iter;

	//准备获取修改帧缓冲区的保护锁
	Critical_Section Lock(&m_FrameCritical);

	//先比较帧的时间戳，看看是否在已经接收的范围之外
	if (m_receiveQueue.empty())
	{
		iter = m_receiveQueue.insert(m_receiveQueue.begin(), RTPFrameEntry(new RTPFrame(packet)));	// 队列为空，直接插入
	}
	else
	{
		iter = m_receiveQueue.end();
		auto j = ++m_receiveQueue.begin();
		for(auto i = m_receiveQueue.begin(); i != m_receiveQueue.end(); i++, j++)
		{
			if (i->TS() == iTS)
			{
				//找到了，已经有同样时间戳的RTP报文到达了
				if (!i->Completed() && i->Get()->InsertPacket(packet))
				{
					iter = i; break;		//只有在这一帧没有完成的情况下才插入
				}
				else
				{
					delete packet; return;	// 如果是已完成的帧，或者插入失败，那这个包就不要了
				}
			}
			else if (j != m_receiveQueue.end() &&
				((i->TS() < iTS && j->TS() > iTS) ||						// 普通插入点
				((j->TS() < i->TS()) && (iTS > i->TS() || iTS < j->TS()))))	// 回绕处插入点
			{
				iter = m_receiveQueue.insert(j, RTPFrameEntry(new RTPFrame(packet))); break;	//建立一个新帧并插入
			}
		}
		// 时间戳落在帧缓存的范围外，插在最前面 or 最后面 ？
		if (iter == m_receiveQueue.end())
		{
			// 这里采用一种简单的比较办法，即当前时间戳离哪边近就插入到哪边
			// 这里要满足的假设是：队列的长度不会超过所有可能时间戳总数的一半，显然这个假设是成立的
			if (m_receiveQueue.front().TS() - iTS < iTS - m_receiveQueue.back().TS())
			{
				// 插在前面的报文应当保证时间戳不小于最小可容忍值，太早的帧就直接丢弃
				if (m_receiveQueue.back().TS() - iTS <= m_iFrameQueueTimeDelay)
				{
					iter = m_receiveQueue.insert(m_receiveQueue.begin(), RTPFrameEntry(new RTPFrame(packet)));
				}
				else
				{
					delete packet; return;
				}
			}
			else
			{
				iter = m_receiveQueue.insert(m_receiveQueue.end(), RTPFrameEntry(new RTPFrame(packet)));
			}
		}
	}

	if (iter->Completed()) __sync_add_and_fetch((long*)&m_revDpt->m_iFrameRecv, 1);

	// 处理统计信息
	if (m_revDpt->m_bCounted)
	{
		if (pkt_seq < m_revDpt->m_max_seq)
		{
			if (m_revDpt->m_max_seq - pkt_seq <= 4096)
			{
				m_revDpt->m_cycles++;
				m_revDpt->m_max_seq = pkt_seq;
			}
		}
		else
			m_revDpt->m_max_seq = pkt_seq;
	}
	else
	{
		m_revDpt->m_max_seq = m_revDpt->m_base_seq = pkt_seq;
		m_revDpt->m_bCounted = true;
	}
	m_revDpt->m_received++;
	m_revDpt->m_recovered += packet->Extension()->resend;

	// 队列长度已经达到延迟要求
	while(m_receiveQueue.back().TS() > m_receiveQueue.front().TS() + m_iFrameQueueTimeDelay)
	{
		if (m_receiveQueue.front().Completed())
		{
			RTPFrame* frame = m_receiveQueue.front().Pop();

			if (m_preparedQueue.size() >= MAX_FRAME_COUNT)
			{
				// 如果队列满了，需要删掉一帧，则需要根据几种情况来决定删掉哪一帧
				auto i = m_preparedQueue.begin();
				while(i != m_preparedQueue.end() && !(*i)->KeyFrame()) i++;
				if (i == m_preparedQueue.begin() || i == m_preparedQueue.end())
				{
					// 队列里没有关键帧，或者关键帧是队列里的第一帧
					if (frame->KeyFrame())
						i = --m_preparedQueue.end();	// 如果本帧是关键帧，那么删掉最后一帧（这样前面的帧可以尽量多地成功解码）
					else
						i = m_preparedQueue.end();	// 否则的话，本帧就不要放进去了
				}
				else
					i--;								// 队列里有关键帧就把关键帧的前一帧给删掉

				// 无论是上面哪种情况，都不需要去动信号量了，因此队列长度不变化
				if (i != m_preparedQueue.end())
				{
					delete *i;
					m_preparedQueue.erase(i);
					m_preparedQueue.push_back(frame);
				}
				else
				{
					delete frame;
					frame = nullptr;
				}
			}
			else
			{
				m_preparedQueue.push_back(frame);			// 插入本帧
//				ReleaseSemaphore(m_FrameSema, 1, nullptr);	// 向提取线程发送信号
                sem_post(m_FrameSema);
			}
		}
		m_receiveQueue.pop_front();
	}
}

RTPFrame* RTPReceiver::GetOneFrame()
{
	Critical_Section Lock(&m_FrameCritical);
	if (m_preparedQueue.size() > 0)
	{
		auto ret = m_preparedQueue.front();
		m_preparedQueue.pop_front();
		return ret;
	}
	else
		return nullptr;
}

void RTPReceiver::ClearFrameBuffer()
{
	Critical_Section Lock(&m_FrameCritical);

	//清空帧缓冲队列
	m_receiveQueue.clear();

	//删除准备好的帧
	for(auto i = m_preparedQueue.begin(); i != m_preparedQueue.end(); i++)
	{
//		WaitForSingleObject(m_FrameSema, 0);
        sem_wait(m_FrameSema);
		delete *i;
	}
	m_preparedQueue.clear();
}

//统计并得到丢包队列（不统计 dealy 毫秒以内帧的丢包）
void RTPReceiver::GetLostPackets(std::list<uint16_t>& list, uint32_t delay)
{
	delay *= 90;	// 转换成 90KHz 为单位
	Critical_Section Lock(&m_FrameCritical);

	uint32_t ts = m_receiveQueue.back().Get()->TS();
	auto i = m_receiveQueue.begin();
	uint16_t max_seq = (i != m_receiveQueue.end()) ? 0 : i->Get()->Start();
	while(i != m_receiveQueue.end() && ts - i->Get()->TS() >= delay)
	{
		auto frame = i->Get();
		int count = frame->Size();
		
		if (frame->Start() != max_seq)
		{
			// 序号出现中断，说明有一些帧的所有报文都没有收到
			auto start = frame->Start();
			for(uint16_t j = max_seq; j != start; j++)
				list.push_back(j);
		}

		if (!frame->Completed())
		{
			// 只需统计那些没有完成的帧
			auto packets = frame->Packets();
			auto start = frame->Start();
			for(int j = 0; j < count; j++)
				if (packets[j] == nullptr)
					list.push_back(start + j);
		}

		max_seq = frame->Start() + frame->Size();
		i++;
	}
}

void RTPReceiver::setBufferMaxDelay(uint32_t delay)
{
	m_iFrameQueueTimeDelay = delay;
}

void RTPReceiver::setSema(sem_t *hSema)
{
    m_FrameSema = hSema;
}
