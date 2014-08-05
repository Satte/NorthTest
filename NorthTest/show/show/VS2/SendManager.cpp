#include "SendManager.h"
#include "Capture.h"
#include "Render.h"
#include "Codec.h"
#include "mediaTimer.h"
#include "sessionStatsManager.h"
#include "RWLock.h"
#include <sys/time.h>

SendManager::SendManager(const ChannelInfo& info, VIEW *view)
	: m_channelInfo(info)
    , m_pView(view)
	, m_pRender(nullptr)
	, m_rtcpSend(true)
	, m_iActualBW(0)
	, m_iRtpSeq(0)
	, m_FrameBuf(nullptr)            //待发送的帧缓冲区
	, m_bRendVideo(false)           //是否渲染视频
	, m_bSendVideo(false)           //是否发送视频
	, m_iPacketBufferTime(MAX_FRAME_TIME_RANGE)
{
	m_rtcpSend.setRtcpInfo(
		SDES(
			g_SessionStateMgr.getCName(),
			g_SessionStateMgr.getName(),
			g_SessionStateMgr.getEmail(),
			g_SessionStateMgr.getPhone(),
			g_SessionStateMgr.getLocation(),
			g_SessionStateMgr.getTool(),
			info.m_strNote,
			g_SessionStateMgr.getPrivate()
		),
		g_SessionStateMgr.getNetAddr(),
		g_SessionStateMgr.getRtcpPort(),
		g_SessionStateMgr.getTTL(),
		g_SessionStateMgr.IsUseNat()
	);

	//生成一个新的视频源描述符，加入全局唯一列表中，对该视频源的RTP报文统计数据都写入
	//该描述符中，ssrc将由该描述符，唯一保存，sessionStatsManager统一管理，应对ssrc改变时只要更改一处即可
	m_srcDpt         = g_SessionStateMgr.addSrc();

	//创建互斥锁
	InitializeCriticalSection(&m_renderLock);
	InitializeCriticalSection(&m_ResendLock);

    //initialize semaphore for resend 
    sem_init(&m_hSemaResend, 0, 0);
}

SendManager::~SendManager( void ) 
{
	Close();

	//销毁互斥锁
	DeleteCriticalSection(&m_ResendLock);
	DeleteCriticalSection(&m_renderLock);

	//删除全局唯一队列中该描述符结点
	g_SessionStateMgr.deleteSrc(m_srcDpt->m_ssrc);
}



void SendManager::OnFrame( void )
{
   
    while(sem_wait(&m_hSemaFrame) == 0){
        pthread_testcancel();
		std::shared_ptr<TFrame> pRaw(m_pCapture->GetFrame());       //从采集器提取一帧

		m_FrameBuf = pRaw;						//清除上一帧

		if (m_bRendVideo)
		{
			EnterCriticalSection(&m_renderLock);   //要加互斥锁，防治此时UI发命令要停止一路视频绘制
			m_pRender->Render(pRaw.get());	//渲染
			LeaveCriticalSection(&m_renderLock);
		}
    }
  
    m_FrameBuf = nullptr;
}

void SendManager::SendProc()
{
	std::list<uint16_t> resend_list;
	std::vector<std::unique_ptr<TRTPPacket>> packets(65536);
	std::vector<int> frame_sizes(m_channelInfo.m_iFPS);

	int64_t dueTime, start, now, frametime, delaytime;
	int64_t frame_interval = 10000000 / m_channelInfo.m_iFPS;
	int total = 0, pos = 0, count = 0, counted_frame_size = 0, now_bw = 0;
	uint16_t min_seq = m_iRtpSeq;

    struct timeval tv; 
    gettimeofday(&tv, NULL);
    start = tv.tv_sec * 1e7 + tv.tv_usec*10;                   // 时间单位都是100ns
    delaytime = 10000;
    dueTime = start + delaytime;
    while(1){
        usleep(delaytime/10);
        pthread_testcancel();
        if (sem_trywait(&m_hSemaResend) == 0){
			//记录要重发的报文
			EnterCriticalSection(&m_ResendLock);
			resend_list = m_ResendList;
            m_ResendList.clear();
			LeaveCriticalSection(&m_ResendLock);

			for(auto i = resend_list.begin(); i != resend_list.end(); i++)
			{
				if (packets[*i] != nullptr)
					m_send.Send(*packets[*i]);
			}
        }
        else{
            int frame_size = 0;
            //编码&发送
            std::shared_ptr<TFrame> frame = m_FrameBuf;
            if (m_bSendVideo && frame)  //允许发送视频，若视频流缓冲区不为空就编码发送（该缓冲区必须互斥访问）
            {
                int cPkt;
                uint16_t start_seq = m_iRtpSeq;
                if ((cPkt = m_pEncoder->Encode(frame.get(), packets)) > 0)
                {
                    //发送各个封包
                    //edit by xlong_liu
                    for(auto i = 0; i < cPkt; i++)
                    {
                        TRTPPacket& pkt = *packets[(uint16_t)(start_seq + i)];
                        m_send.Send(pkt);
                        frame_size += pkt.Length;	//统计发送的字节数
                        pkt.Extension()->resend = true;	// 设置重发标志
                    }

                    //调整发送缓存队列
                    uint32_t ts = ntohl(packets[start_seq]->Header()->ts);
                    while(min_seq != start_seq && ts - ntohl(packets[min_seq]->Header()->ts) > m_iPacketBufferTime)
                        packets[min_seq++] = nullptr;
                }
            }

            if (count < m_channelInfo.m_iFPS)
            {
                frame_sizes[pos++] = frame_size;
                counted_frame_size += frame_size;
                frametime = frame_interval;
                count++;
            }
            else
            {
                pos %= m_channelInfo.m_iFPS;
                counted_frame_size += frame_size - frame_sizes[pos];	// 平滑帧大小
                frame_sizes[pos++] = frame_size;
                frametime = (int64_t)80000 * counted_frame_size / m_channelInfo.m_iFPS / m_channelInfo.m_iBW;	// 计算帧的发送时间
                if (frametime < frame_interval) frametime = frame_interval;
            }
            total += frame_size;
            
            gettimeofday(&tv, NULL);
            now = tv.tv_sec * 1e7 + tv.tv_usec*10;
            if (now - start >= 10000000)
            {
                now_bw = (int)((int64_t)total * 80000000 / (now - start));
                m_iActualBW = (m_iActualBW * 3 + now_bw) / 4;	// 历史平均值
                start = now; total = 0;
            }

            dueTime += frametime;

            delaytime = dueTime - now;
            delaytime = delaytime > 0 ? delaytime:0;
        }
    }

}

void SendManager::Close()
{
	stopSender();      //关闭发送线程
    stopRender();
}

bool SendManager::beginRender()
{ 
	if (!m_bRendVideo)
	{
        m_pRender.reset (  new CRender(m_pView ));
        m_pRender->Attach(m_channelInfo.m_iWidth, m_channelInfo.m_iHeight);
        sem_init(&m_hSemaFrame, 0, 0); 

		//启动主控线程，此时其应该开始等待采集到的第一帧数据
        pthread_create(&m_hThreadOnFrame, NULL, threadOnFrame, this);

		//采集器开始采集
        m_pCapture.reset(new CCapture(m_channelInfo));
		m_bRendVideo = m_pCapture->Start(&m_hSemaFrame);
	}
    return m_bRendVideo;
}

bool SendManager::beginSender()
{
	if (!m_bSendVideo)
	{
		//本地增加了一个发送源
		g_SessionStateMgr.incrSrc();

		//编码器初始化
		m_pEncoder.reset(new CEncoder(m_srcDpt, m_iRtpSeq));
		m_pEncoder->Init(m_channelInfo.m_iWidth, m_channelInfo.m_iHeight, m_channelInfo.m_iQuality, m_channelInfo.m_iBW, m_channelInfo.m_iFPS, m_channelInfo.m_iKeyframeFreq);

		//UDPSocket初始化
		m_send.Open(g_SessionStateMgr.getNetAddr(), g_SessionStateMgr.getRtpPort(), g_SessionStateMgr.getTTL(), g_SessionStateMgr.IsUseNat());

		//RTCP Sender 初始化
		m_rtcpSend.changeRtcpInfo(g_SessionStateMgr.getNetAddr(), g_SessionStateMgr.getRtcpPort(), g_SessionStateMgr.getTTL());

		m_bSendVideo = true;                //设置已经发送视频标志位

		//启动RTCP报文发送线程
		m_rtcpSend.startSendRTCP(m_srcDpt);

		//启动绘制线程初始化
        if( !m_bRendVideo)
            beginRender();

        pthread_create(&m_hThreadSend, NULL, threadSend, this);

	}
	return true;
}


// 停止该视频的绘制
bool SendManager::stopRender()
{
	if (m_bRendVideo)
	{
        m_pRender = nullptr;
		m_bRendVideo = false;
		if (!m_bSendVideo) commonClose();   //如果没有Sender和Render就清理资源
	}
	return true;
}

bool SendManager::stopSender()
{
	
	
	if (m_bSendVideo)
	{
		m_bSendVideo = false;       //停止发送

        pthread_cancel(m_hThreadSend);
        pthread_join(m_hThreadSend, NULL);

		m_rtcpSend.stopSendRTCP();	 //关闭RTCP报文发送线程

		//关闭网络发送器。此调用必须在主控线程关闭后，否则可能导致主控线程调用已关闭的网络发送器发送数据
		m_send.Close();

		//本地减少了一个发送源
		g_SessionStateMgr.decrSrc();

		m_pEncoder = nullptr;

		Critical_Section lock(&m_ResendLock);
		m_ResendList.clear();

	}

	if (!m_bRendVideo) commonClose();	//如果没有Render
	m_iActualBW = 0;
	return true;
}

//Sender和Render相同部分的清理工作
void SendManager::commonClose()
{
    pthread_cancel(m_hThreadOnFrame);
    pthread_join(m_hThreadOnFrame, NULL);
    //关闭采集器。此调用必须再主控线程关闭后，否则可能导致采集器清除缓冲时与主控线程发生冲突访问
    m_pCapture->Stop();
    m_pCapture.reset();

}

void SendManager::ResendPackets(std::list<uint16_t> seqs)
{
	Critical_Section lock(&m_ResendLock);
	m_ResendList.merge(seqs);
	m_ResendList.unique();
    sem_post(&m_hSemaResend);
}
