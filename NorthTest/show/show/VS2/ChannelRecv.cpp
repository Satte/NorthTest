#include "ChannelRecv.h"
#include "Render.h"
#include "Decoder.h"
#include "Frame.h"

#include <algorithm>
#include <string.h>

CChannelRecv::CChannelRecv(revDescriptor* revDpt)
	: m_pRender(nullptr)
	, m_pFrame(nullptr), m_revDpt(revDpt)
	, m_RtpRecv(revDpt)									//创建rtp报文接收器
	, m_RtcpRecv(revDpt)												//RTCP报文解析器
	, m_iWidth(0), m_iHeight(0), m_lLock(0)
	, m_isStart_Rtp(false)												//还没有开始接收RTP报文
	, m_payLoadType(RTPPayloadType::RTP_PT_UNKNOWN)						//视频流负载类型
    , m_bRender(false)
{
    sem_init(&m_hSema_Render, 0, 0);
    m_RtpRecv.setSema(&m_hSema_Render);

}

CChannelRecv::~CChannelRecv()
{
    stopOneRender();
	sem_destroy(&m_hEvtReadyForRender);
	sem_destroy(&m_hSema_Render);
}

//视频刷新

//开始一个渲染器
void CChannelRecv::startOneRender(VIEW *view)
{
    m_pRender.reset(new CRender(view));
    m_pRender->Attach(m_iWidth, m_iHeight);

	if (!m_bRender)
	{
        sem_init(&m_hEvtReadyForRender, 0, 0);
        pthread_create(&m_hThreadOnRender, NULL, threadOnRender, this);
	}
	m_isStart_Rtp = true;
    m_bRender = true;
}

//根据绘制窗口句柄，关闭一个Render；如果所有Render都已经关闭，停止绘制线程，停止接收rtp
void CChannelRecv::stopOneRender()
{
	if (m_bRender)
	{
        //结束接收线程
        pthread_cancel(m_hThreadOnRender);
        pthread_join(m_hThreadOnRender, NULL);
        m_pRender = nullptr;
        m_isStart_Rtp = false;			//停止接收

        //关闭编解码器
        m_pDecoder= nullptr;

        //清除参数
        m_iWidth = m_iHeight = 0;
        m_payLoadType = RTPPayloadType::RTP_PT_UNKNOWN;

        m_bRender = false;

        m_revDpt->reset();				//相应的RTP报文统计信息置零
        m_RtpRecv.ClearFrameBuffer();	//清除RTP报文缓存
	}
}


void CChannelRecv::OnRender()
{
	std::unique_ptr<TFrame> curFrame, preFrame;

    while( sem_wait(&m_hEvtReadyForRender) == 0){
        pthread_testcancel();

		std::unique_ptr<TFrame> curFrame(new TFrame(m_iWidth, m_iHeight)), preFrame(new TFrame(m_iWidth, m_iHeight));

		memset(curFrame->data[0], 0, m_iWidth * m_iHeight * 3 / 2);
		memset(curFrame->data[0], 0, m_iWidth * m_iHeight * 3 / 2);

        while( sem_wait(&m_hSema_Render) == 0){
            pthread_testcancel();
			//从RTP报文解析器取出一帧
			RTPFrame* pVideoData = m_RtpRecv.GetOneFrame();

			if (pVideoData)
			{
				//解码
				if (m_pDecoder->Decode(*pVideoData, curFrame.get()))
				{
					//保存上一帧图象
					m_pFrame = curFrame.release();

					curFrame.reset(preFrame.release());
					preFrame.reset(m_pFrame);
 
					//绘制
                    m_pRender->Render(m_pFrame);

				}
				delete pVideoData;
			}
        
        }

    }
}

//解析rtp报文
void CChannelRecv::parseRtp(const TRTPPacket* packet)
{
	__sync_lock_test_and_set(&m_lLock, 1);
	//测试视频流的格式
	if (m_isStart_Rtp && CDecoder::CheckSingleRTPPacket(packet))
	{
		RTPPayloadType pt = (RTPPayloadType)packet->Header()->pt;
		uint16_t iWidth = ntohs(packet->Extension()->width);
		uint16_t iHeight = ntohs(packet->Extension()->height);
		m_revDpt->m_videoCode = pt;

		//1.媒体类型和已经记载的不一样，说明发送方中途改变了编码格式
		//2.视频的长宽发生变化， 则我们需要重启编码器和绘制线程
		//3.刚收到第一个报文，变量还没有初始化
		if (m_payLoadType != pt || iWidth != m_iWidth || iHeight != m_iHeight)
		{
			m_RtpRecv.ClearFrameBuffer();													//清空帧缓冲

			m_payLoadType = pt;
			m_iWidth = iWidth;
			m_iHeight = iHeight;

			//解码器初始化
			m_pDecoder.reset(new CDecoder());
			m_pDecoder->Init(m_iWidth, m_iHeight);

			//绘制器重置
			
            m_pRender->Detach();


            sem_post(&m_hEvtReadyForRender);
		}

		m_RtpRecv.InsertRtp(packet);

		//added by zyc
        struct timeval tv;
        gettimeofday(&tv, NULL);
		m_revDpt->m_tLastRTPActive = tv.tv_sec;
		// 报文将在Decode之后释放
	}
	else
		delete packet;
	__sync_lock_test_and_set(&m_lLock, 0);
}

