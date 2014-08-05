#include "rtcpSender.h"
#include "mediaTimer.h"
#include "sessionStatsManager.h"
#include <math.h>
#include <time.h>
#include <iconv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma comment(lib, "Version.lib")

RTCPSender::RTCPSender(bool bSender)
	: m_sendThreadHdr(nullptr)
	, m_srcDpt(nullptr), m_wesend(bSender), m_bNat(false)
{
	for(int i = 0;i < RTCP_SDES_MAX; i++)	//描述信息
		m_sdes[i][0] = 0;
}

RTCPSender::~RTCPSender()
{
	stopSendRTCP();	
}

int RTCPSender::WideCharToMultiByte(std::wstring inbuf, int insize, char *outbuf, int maxoutsize)
{
    char *tocode = "UTF-8";
    char *fromcode;
    if ( sizeof(wchar_t) == 2)
        fromcode = "UTF-16LE";
    else
        fromcode = "UTF-32LE";
    iconv_t cd = iconv_open(tocode, fromcode);

    char *inbuf_p = (char *)inbuf.data();
    size_t insize_t = (size_t)insize*sizeof(wchar_t);
    size_t bytesleft;
    iconv(cd,&inbuf_p, &insize_t,&outbuf,&bytesleft);

    iconv_close(cd);

    return maxoutsize - bytesleft;

}
void RTCPSender::setRtcpInfo(const SDES& sdes, uint32_t dwIP, uint16_t iPort, uint8_t iTTL, bool bNat)
{
	changeRtcpInfo(dwIP, iPort, iTTL);
	m_bNat = bNat;

	// 要使用UTF-8编码！
	m_sdes[RTCP_SDES_CNAME - 1][0] = WideCharToMultiByte(sdes.m_sCName, sdes.m_sCName.length(), m_sdes[RTCP_SDES_CNAME - 1] + 1, 255);
	m_sdes[RTCP_SDES_NAME - 1][0] = WideCharToMultiByte(sdes.m_sName, sdes.m_sName.length(), m_sdes[RTCP_SDES_NAME - 1] + 1, 255);
	m_sdes[RTCP_SDES_EMAIL - 1][0] = WideCharToMultiByte(sdes.m_sEmail, sdes.m_sEmail.length(), m_sdes[RTCP_SDES_EMAIL - 1] + 1, 255);
	m_sdes[RTCP_SDES_PHONE - 1][0] = WideCharToMultiByte(sdes.m_sPhone, sdes.m_sPhone.length(), m_sdes[RTCP_SDES_PHONE - 1] + 1, 255);
	m_sdes[RTCP_SDES_LOC - 1][0] = WideCharToMultiByte(sdes.m_sLocation, sdes.m_sLocation.length(), m_sdes[RTCP_SDES_LOC - 1] + 1, 255);
	m_sdes[RTCP_SDES_NOTE - 1][0] = WideCharToMultiByte(sdes.m_sNote, sdes.m_sNote.length(), m_sdes[RTCP_SDES_NOTE - 1] + 1, 255);
	m_sdes[RTCP_SDES_TOOL - 1][0] = WideCharToMultiByte(sdes.m_sTool, sdes.m_sTool.length(), m_sdes[RTCP_SDES_TOOL - 1] + 1, 255);
	m_sdes[RTCP_SDES_PRIV - 1][0] = WideCharToMultiByte(sdes.m_sPrivate, sdes.m_sPrivate.length(), m_sdes[RTCP_SDES_PRIV - 1] + 1, 255);
}

void RTCPSender::changeRtcpInfo(uint32_t dwIP, uint16_t iPort, uint8_t iTTL)
{
	m_dwIP = dwIP;
	m_iPort = iPort;
	m_iTTL = iTTL;
}

//开始发送RTCP报文
void RTCPSender::startSendRTCP(srcDescriptor * src)
{
	//防止重复初始化
	if (m_sendThreadHdr != nullptr) return;

	//TRACE("The ctrl_inv_bw_ is %f, the bw if %d.\n",ctrl_inv_bw_, bw);
	//该视频源描述符结点指针
	m_srcDpt = src;
	//打开UDP Socket发送端
	m_socket.Open(m_dwIP, m_iPort, m_iTTL, m_bNat);

	//将线程停止的事件
	//m_stop = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    //启动RTCP报文发送线程
	//m_sendThreadHdr = (HANDLE)_beginthreadex(nullptr, 0, threadRTCPSend, this, 0, nullptr);
    pthread_create(&m_sendThreadHdr, NULL, threadRTCPSend, this);
}

bool RTCPSender::stopSendRTCP()
{
	if (m_sendThreadHdr == nullptr) return false ;

    //等待线程的关闭
	//SignalObjectAndWait(m_stop, m_sendThreadHdr, INFINITE, FALSE);
    pthread_cancel(m_sendThreadHdr);
    pthread_join(m_sendThreadHdr, NULL);
    //关闭线程句柄
//	CloseHandle(m_sendThreadHdr); m_sendThreadHdr = nullptr;
	//关闭事件句柄
//	CloseHandle(m_stop); m_stop = nullptr ;

    //关闭UDP Socket
	m_socket.Close();

	//安全关闭
	return true;
}

void RTCPSender::SendProc()
{
	TPacket packet;
	// 关于发送间隔：不再遵守RFC 3550中给出的算法，而是直接在 5.0s ~ 10.0s 中随机选择一个时间发送
	// 原因：（1）带宽已成倍增加，RTCP的发送无需再死卡带宽；（2）提高客户端刷新用户的速度
	int iDelay = rand() % (RTCP_MIN_TIME / 2); //Avoid a burst of rtcp ,when lots of rtcp senders startup


    while(1){
        pthread_testcancel();
        sleep(iDelay/1000);
        pthread_testcancel();
		//产生一个RTCP报文
		ProduceRTCP(packet);
		//发送一个SR报文
		m_socket.Send(packet);

		//TRACE("The next rtcp delay is %d.is we send? %d.\n",iDelay,m_wesend);
		//当一次时钟到时时将设置一个新的延迟delay
		iDelay = (rand() % (RTCP_MAX_TIME - RTCP_MIN_TIME)) + RTCP_MIN_TIME;
    }

	//TRACE("we send bye now.\n");
	//产生一个Bye报文
	ProduceBye(packet);
	//发送一个Bye报文
	m_socket.Send(packet);
}

//生成一个RTCP报文
void RTCPSender::ProduceRTCP(TPacket& packet)
{
	if (m_wesend)
		ProduceSR(packet);
	else
		ProduceRR(packet, true);
	
	//生成SDES
	ProduceSDES(packet);
}

//产生一个RTCP RR报文
void RTCPSender::ProduceRR(TPacket& packet, bool bHasReports)
{
	//初始化Report Header
	auto hdr = reinterpret_cast<rtcp_header*>(packet.Buffer);
	auto rr = reinterpret_cast<rtcp_rr*>(hdr + 1);

	//生成RR Report
	int nrr = bHasReports ? ProduceReport(rr) : 0;

	//RTCP 通用头部初始化
	hdr->ver = RTP_VERSION;
	hdr->p = 0;
	hdr->cnt = nrr;
	hdr->pt = RTCP_PT_RR;
	hdr->length = htons(((nrr * sizeof(rtcp_rr) + sizeof(rtcp_header)) >> 2) - 1);
	hdr->ssrc = htonl(m_srcDpt->m_ssrc);        //ssrc
	packet.Length = nrr * sizeof(rtcp_rr) + sizeof(rtcp_header);
}

//产生一个RTCP SR报文
void RTCPSender::ProduceSR(TPacket& packet)
{
	//初始化Report Header
	auto hdr = reinterpret_cast<rtcp_header*>(packet.Buffer);
	auto sr = reinterpret_cast<rtcp_sr*>(hdr + 1);
	auto rr = reinterpret_cast<rtcp_rr*>(sr + 1);

    //SR头部初始化
	//得到当前时间 
	sr->ntp = MediaTimer::ntp64time(MediaTimer::unixtime());      //the time , NTP
	sr->ntp.upper = htonl(sr->ntp.upper);
	sr->ntp.lower = htonl(sr->ntp.lower);
	sr->ts = htonl(MediaTimer::media_ts());    //media time
	
	sr->np = htonl(m_srcDpt->m_np);            //已经发送的RTP报文数量，字节数目
	sr->nb = htonl(m_srcDpt->m_nb);

	//SR报文中不再另外附带RR Report（因已有专门的RR Report在不停地发送）

	//RTCP 通用头部初始化
	hdr->ver = RTP_VERSION;
	hdr->p = 0;
	hdr->cnt = 0;
	hdr->pt = RTCP_PT_SR;
	hdr->length = htons(((sizeof(rtcp_sr) + sizeof(rtcp_header)) >> 2) - 1);
	hdr->ssrc = htonl(m_srcDpt->m_ssrc);        //ssrc
	packet.Length = sizeof(rtcp_sr) + sizeof(rtcp_header);
}

int RTCPSender::ProduceReport(rtcp_rr* rr)
{
	int nrr = 0;

	//得到读保护锁
	g_SessionStateMgr.getReadLockRevList();

	auto revList = g_SessionStateMgr.getRevList();

	for(auto iter = revList.begin(); iter != revList.end() && nrr < 32; iter++, rr++)
	{
		uint32_t extended_max = (*iter)->m_extended_max;
		uint32_t expected = extended_max - (*iter)->m_base_seq + 1;
		uint32_t lost = expected - (*iter)->m_received;
		uint32_t expected_interval = expected - (*iter)->m_expected_prior;
		uint32_t received_interval = (*iter)->m_received - (*iter)->m_received_prior;
		uint32_t recovered_interval = (*iter)->m_recovered - (*iter)->m_recovered_prior;
		(*iter)->m_expected_prior = expected;
		(*iter)->m_received_prior = (*iter)->m_received;
		(*iter)->m_recovered_prior = (*iter)->m_recovered;

		if (received_interval > 0) nrr++; else continue;

		uint32_t lost_interval = expected_interval - received_interval;
		uint8_t fraction;

		if (expected_interval > 0)
		{
			(*iter)->m_PacketLoss = (float)lost_interval / (float)expected_interval;
			(*iter)->m_PacketRecovered = (float)recovered_interval / (float)expected_interval;
			fraction = (lost_interval << 8) / expected_interval;
		}
		else
		{
			(*iter)->m_PacketLoss = (*iter)->m_PacketRecovered = 0.0f;
			fraction = 0;
		}

		//ssrc
		rr->ssrc = (*iter)->m_ssrc ;
		//lost
		rr->loss = htonl((fraction << 24) | (lost & 0xffffff));
		//extended highest sequence number received
		rr->ehsr = htonl(extended_max);
		//jitter: 
		rr->dv = htonl((*iter)->m_jitter);
		//LSR
		rr->lsr = htonl((((*iter)->m_iLastSRRevSec & 0x0000ffff) << 16) | (((*iter)->m_iLastSRRevFrac & 0xffff0000 ) >> 16));
		//DLSR
		rr->dlsr = htonl((MediaTimer::media_ts() - (*iter)->m_iLastSRRevTS)/90000*65536);
	}

	//释放读保护锁
	g_SessionStateMgr.releaseReadLockRevList();

	//传回rr的数量
	return nrr;
}

// 生成SDES部分，鉴于带宽过剩，所以发送策略改为全发送
void RTCPSender::ProduceSDES(TPacket& packet)
{
	static const int sdes_item[] = { RTCP_SDES_CNAME, RTCP_SDES_NAME, RTCP_SDES_NOTE, RTCP_SDES_TOOL };

	int len = 0, pad = 0, totalSDESlen = 0, oneSDESLen = 0;

	auto hdr = reinterpret_cast<rtcp_header*>(packet.Buffer + packet.Length);
	uint8_t* ptr = reinterpret_cast<uint8_t*>(hdr + 1);
    
	for(int i = 0; i < sizeof(sdes_item) / sizeof(int); i++)
	{
		if ((oneSDESLen = ProduceOneSDES(ptr, sdes_item[i])) > 0)
		{
			totalSDESlen += oneSDESLen;
			ptr += oneSDESLen;
		}
	}
	
	//计算SDES 部分的长度，必须填充成四字节的倍数
	pad = 4 - (totalSDESlen & 0x3);
	len = totalSDESlen + sizeof(rtcp_header) + pad;
	while (pad-- > 0) *ptr++ = 0;	//填充item sdes到达32－bits的对齐

	//RTCP 通用头部初始化；没有加密，不需要置“填充”位；这和 RTCP 本身需要的填充是不一样的。
	hdr->ver = RTP_VERSION;
	hdr->p = 0;
	hdr->cnt = 1;
	hdr->pt = RTCP_PT_SDES;
	hdr->length = htons((len >> 2) - 1);
	hdr->ssrc = htonl(m_srcDpt->m_ssrc);        //ssrc
	packet.Length += len;
}

int RTCPSender::ProduceOneSDES(uint8_t* ptr, uint8_t code)
{
	// code 从 1 开始，所以需要减去 1
	const char* value = m_sdes[code - 1];

	if (value[0] != 0)
	{
		*ptr++ = code;
		*ptr++ = value[0];
		memcpy(ptr, value + 1, value[0]);
		return value[0] + 2;
	}
	else
		return 0;
}

//产生一个Bye报文 
void RTCPSender::ProduceBye(TPacket& packet)
{
	//根据是否发送了RTP报文，判断发送SR或者RR
	if (m_wesend)
		ProduceSR(packet);
	else
		ProduceRR(packet, false);

	//附上Bye报文
	auto hdr = reinterpret_cast<rtcp_header*>(packet.Buffer + packet.Length);

	//RTCP 通用头部初始化
	hdr->ver = RTP_VERSION;
	hdr->p = 0;
	hdr->cnt = 1;
	hdr->pt = RTCP_PT_BYE;
	hdr->length = htons((sizeof(rtcp_header) >> 2) - 1);
	hdr->ssrc = htonl(m_srcDpt->m_ssrc);        //ssrc
	packet.Length += sizeof(rtcp_header);
}
