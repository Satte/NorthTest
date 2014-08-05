
#pragma once
#include <string>
#include "Constants.h"
#include "rtcp.h"
#include "mediaTimer.h"


struct TPacket
{
	static const int MaxPacketLength = 1440;
    
	uint8_t Buffer[MaxPacketLength];
	int Length;
};


struct SendInfo
{
//	GUID     m_ColorSpace;
//	CRect    m_cropRect;          //裁剪区域，对CaptureScreen来说即是捕捉区域，对其他类型来说是对原始面面的裁剪
	int32_t  m_iCapWidth;
	int32_t  m_iCapHeight;
	int32_t  m_iWidth;            //视频长
	int32_t  m_iHeight;           //视频宽
	int32_t  m_iQuality;          //视频质量
	int32_t  m_iFPS;              //帧率
	int32_t  m_iKeyframeFreq;     //I帧率
	int32_t  m_iBW;               //发送的视频流的带宽上限(Kbps)
	RTPPayloadType m_format;      //编码格式：MPEG4,H263,FGS,H261.etc
	StereoFrameMode m_StereoMode;

	SendInfo()
//		 m_cropRect(0, 0, 352, 288)
    : m_iCapWidth(352), m_iCapHeight(288)
		, m_iWidth(352), m_iHeight(288)
		, m_iQuality(50), m_iFPS(25)
		, m_iKeyframeFreq(10), m_iBW(300)
		, m_format(RTPPayloadType::RTP_PT_H264)
//		, m_StereoMode(StereoFrameMode::STEREO_NONE)
//		, m_ColorSpace(GUID_NULL)
	{ }
};
typedef void* VIEW;
struct ChannelInfo : public SendInfo
{
    std::wstring    m_deviceName; //视频硬件设备的描述路径
    std::wstring    m_strNote;    //备注
	VIEW            *m_hWindow;    //需要捕捉的窗口句柄
};



//视频源描述符，记录接受到的视频源信息
struct srcDescriptor
{
public:
	srcDescriptor(uint32_t ssrc) : m_ssrc(ssrc) { reset(); }

	void reset() { m_fs = m_cs = m_np = m_nb = 0; }

	uint32_t m_ssrc;  /* the ssrc identifier of the source */
	uint16_t m_fs;	 /* first seq. no send */
	uint16_t m_cs;	 /* current (most recent) seq. no send */
	uint32_t m_np;	 /* no. packets RTP send  */
	uint32_t m_nb;	 /* no. bytes RTP send */
};

struct revDescriptor
{
public:
	revDescriptor(uint32_t ssrc)
		: m_ssrc(ssrc)
		, m_bDelete(false)
		, m_bIsSender(false)
		, m_iLastSRRevSec(0)
		, m_iLastSRRevFrac(0)
		, m_iLastSRRevTS(0)
		, m_tLastRTCPActive(0)
		, m_nPacket(0)
		, m_nByte(0)
	{
		reset();
	}

	inline void reset()
	{
		m_iFrameRecv = m_iBytesRecv = m_received = m_recovered = m_transit = m_jitter = 0;
		m_FrameRate = m_BandWidth = m_PacketLoss = m_PacketRecovered = 0.0f;
		m_iLastCalTS = 0;
		m_bCounted = false;
		m_base_seq = m_extended_max = 0;
		m_expected_prior = m_received_prior = m_recovered_prior = 0;
	}

	void UpdateStatics()
	{
		uint32_t curTS = MediaTimer::GetTickCount(); //提取当前时间
		uint32_t ts_elapsed = (curTS - m_iLastCalTS > 0) ? curTS - m_iLastCalTS : 1;	//防止curTS和m_iLastCalTS相同，我在使用时碰到了相同的情况
		float curFrameRate = (uint32_t)__sync_lock_test_and_set((long*)&m_iFrameRecv, 0) * 1000.0f / ts_elapsed;	//当前帧率
		float curBandWidth = (uint32_t)__sync_lock_test_and_set((long*)&m_iBytesRecv, 0) * 8.0f / ts_elapsed;	//当前带宽(kbps)
        m_iLastCalTS = curTS;
		m_FrameRate = (m_FrameRate * 3.0f + curFrameRate) / 4;		//历史平均值
		m_BandWidth = (m_BandWidth * 3.0f + curBandWidth) / 4;		//历史平均值
	}

    SDES				m_sdes;				/* SDES */

	uint32_t			m_ssrc;				/* the unique identifier of a source */
	
	/* For RTCP SR */
	uint32_t			m_iLastSRRevSec;	/* 上一SR报文接收到的发送时间高位 */
	uint32_t			m_iLastSRRevFrac;	/* 上一SR报文接收到的发送时间低位 */
	uint32_t			m_iLastSRRevTS;		/* 上一SR报文接收时间 */
	uint32_t			m_nPacket;			/* 发送源已经发送的RTP个数 */
	uint32_t			m_nByte;			/* 发送源已经发送的字节数 */ 

	volatile uint32_t	m_iFrameRecv;		//目前为止收到了几个视频帧
	volatile uint32_t	m_iBytesRecv;		//已经收到的字节数
	//volatile uint32_t	m_iLostPack;		//检测到的丢失的报文数

	union
	{
		struct
		{
			uint16_t	m_max_seq;        /* highest seq. number seen */
			uint16_t	m_cycles;         /* shifted count of seq. number cycles */
		};
		uint32_t		m_extended_max;
	};
	uint32_t			m_base_seq;       /* base seq number */
	uint32_t			m_received;       /* packets received */
	uint32_t			m_recovered;
	uint32_t			m_expected_prior; /* packet expected at last interval */
	uint32_t			m_received_prior; /* packet received at last interval */
	uint32_t			m_recovered_prior;
	uint32_t			m_transit;        /* relative trans time for prev pkt */
	uint32_t			m_jitter;         /* estimated jitter */

	//added by zyc
	float				m_FrameRate;		//当前帧率(统计值）
	float				m_BandWidth;		//当前带宽(统计值)
	float				m_PacketLoss;		//当前丢包率(统计值)
	float				m_PacketRecovered;
	uint32_t			m_iLastCalTS;		//上次计算帧率和带宽的时间（从开始接收视频数据RTP报文的时刻统计）
	long                m_tLastRTPActive;	//最后一次接收到RTP报文的时间
	long                m_tLastRTCPActive;	/* 最近一次接受到RR或者SR报文的时间 */
	
	bool				m_bIsSender;		/* 是否是视频发送源 */
	bool				m_bDelete;			/* delete flags */
	RTPPayloadType		m_videoCode;		/* 视频编码方式 */
	bool				m_bCounted;
}; 
