#include "rtcpReceiver.h"
#include "sessionStatsManager.h"
#include <iconv.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//检查RTCP报文头
bool RTCPReceiver::check_rtcp_header(const TPacket* packet)
{
	auto hdr = reinterpret_cast<const rtcp_header_common*>(packet->Buffer);

	bool bPadded = false ;
	int iTotalLen = 0;

	//复合报文的首报文必须为SR或RR报文
	if (hdr->pt != RTCP_PT_SR && hdr->pt != RTCP_PT_RR) return false;
    
	//记录这是各SR或RR，便于UI判断对方是否是视频发送源
	m_revDpt->m_bIsSender = (hdr->pt == RTCP_PT_SR);

	auto ptr = packet->Buffer;
	for(iTotalLen = 0; iTotalLen < packet->Length; )
	{
		uint32_t len = (ntohs(hdr->length) + 1) << 2 ;

		//版本号必须为2
		if (hdr->ver != RTP_VERSION) return false ;

		//只有最后一个报文允许有填充字节
		if (bPadded)
			return false;
		else
			bPadded = hdr->p;

		iTotalLen += len; ptr += len;
		hdr = reinterpret_cast<const rtcp_header_common*>(ptr); //指向下一个类型报文的通用头部
	}

	//检验总长度与标识是否一致
	return (iTotalLen == packet->Length);
}

//解析一个RTCP报文
unsigned short RTCPReceiver::parseRtcp(const TPacket* packet)
{
	//设置接收时间，又收到一个新的rtcp啦
	m_revDpt->m_tLastRTCPActive = MediaTimer::GetTickCount();

	//首先进行RTCP报文头的检查
	if (!check_rtcp_header(packet))
		return 1;       //没有通过rtcp头部检查

	//按rtcp的各个类别进行解析
	auto hdr = reinterpret_cast<const rtcp_header_common*>(packet->Buffer);
	auto ptr = packet->Buffer;
	int iTotalLen = 0 ;

	//以rtcp_packet为单位的长度检查已经在RTCPCheck中做了，无需重复
	while(iTotalLen < packet->Length)
	{
		int len = (ntohs(hdr->length) + 1) << 2;

		//按报文类型分别处理
		switch(hdr->pt)
		{
		case RTCP_PT_SR:   decode_rtcp_sr(ptr);   break;
		case RTCP_PT_RR:   decode_rtcp_rr(ptr);   break;
		case RTCP_PT_SDES: decode_rtcp_sdes(ptr); break;
		case RTCP_PT_BYE:  decode_rtcp_bye(ptr);  break;
		case RTCP_PT_APP:  break;
		default:           return 2;	        //无法解析的报文类型，为错误报文
		}

		//移动指针到下一报文
		ptr += len; iTotalLen += len;
		hdr  = reinterpret_cast<const rtcp_header_common*>(ptr);
	}

	return 3;	//解析报文的工作顺利结束，返回
}

//解析一个RTCP SR报文
void RTCPReceiver::decode_rtcp_sr(const uint8_t* pBuffer )
{
	//RTCP通用头部
	auto hdr = reinterpret_cast<const rtcp_header*>(pBuffer);
	unsigned int len = (ntohs(hdr->length) + 1) << 2;

	
	//对SSRC作检查，看是否存在冲突，是则修改自己本地视频源的SSRC
	unsigned int iSSRC = ntohl(hdr->ssrc);
	if (g_SessionStateMgr.findSrc(iSSRC))
		g_SessionStateMgr.changeSSRC(iSSRC);

	if (g_SessionStateMgr.findRev(iSSRC))
	{
		auto sr = reinterpret_cast<const rtcp_sr*>(hdr + 1);
		storeRevSR(sr, iSSRC);	//通过sessionStatsManager将SR信息存储起来

		auto rr = reinterpret_cast<const rtcp_rr*>(sr + 1);
		for(uint8_t i = 0; i < hdr->cnt; i++, rr++)
			decode_one_rrreport(rr);	// 处理RR Report
	}
	else
		return;		// 收到一个新的SSRC发来的RTCP，不在这里加新成员，decode_rtcp_sdes中会处理
}


//解析一个RTCP RR报文
void RTCPReceiver::decode_rtcp_rr(const uint8_t* pBuffer)
{
	//RTCP通用头部
	auto hdr = reinterpret_cast<const rtcp_header*>(pBuffer);
	unsigned int len = (ntohs(hdr->length) + 1) << 2;

	//对SSRC作检查，看是否存在冲突，是则修改自己的SSRC
	unsigned int iSSRC = ntohl(hdr->ssrc);
	if (g_SessionStateMgr.findSrc(iSSRC))
		g_SessionStateMgr.changeSSRC(iSSRC);

	if (g_SessionStateMgr.findRev(iSSRC))
	{
		auto rr = reinterpret_cast<const rtcp_rr*>(hdr + 1);
		for(uint8_t i = 0; i < hdr->cnt; i++, rr++)
			decode_one_rrreport(rr);	// 处理RR Report
	}
	else
		return;		// 收到一个新的SSRC发来的RTCP，不在这里加新成员，decode_rtcp_sdes中会处理
}

//解析一个RR Report Block部分
void RTCPReceiver::decode_one_rrreport(const rtcp_rr * rr)
{
	//I do nothing here ,because I don't know the usage of the statisics
	//写入revList对应结点时，注意互斥保护链表
}


int RTCPReceiver::MultiByteToWideChar(const char* inbuf, uint8_t insize, wchar_t *outbuf, int maxoutsize)
{
    char *tocode;
    if( sizeof(wchar_t) == 2)
        tocode = "UTF16-LE";
    else
        tocode = "UTF32-LE";

    char *fromcode = "UTF-8";
    iconv_t cd = iconv_open(tocode, fromcode);


    char *inbuf_p = (char *)inbuf;
    size_t insize_t = (size_t)insize;
    char *outbuf_p = (char *)outbuf;
    size_t bytesleft;
    iconv(cd, &inbuf_p, &insize_t, &outbuf_p, &bytesleft);
    iconv_close(cd);

    return maxoutsize - bytesleft/sizeof(wchar_t);
}

//解析SDES部分
void RTCPReceiver::decode_rtcp_sdes(const uint8_t* pBuffer)
{
	//一个SDES报文可以同时说明多个源，但当前版本只说明一个源

	//RTCP 通用头部
	auto hdr = reinterpret_cast<const rtcp_header_common *>(pBuffer);
	//减去头部长度，剩下的就是各个SDES的总长
	int len = ntohs(hdr->length) << 2;
    
	auto ptr = reinterpret_cast<const uint8_t*>(hdr + 1) ;//指向第一个SSDC的SDES集合
	for(uint8_t i = 0; i < hdr->cnt && len >= 4; i++)
	{
		//对SSRC作检查，看是否存在冲突，是则修改自己本地发送源的SSRC
		uint32_t iSSRC = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
        if (g_SessionStateMgr.findSrc(iSSRC))
			g_SessionStateMgr.changeSSRC(iSSRC);

		//接收到SDES报文，是发现一个合法成员的标志,VS2中只有收到一个源的rtcp报文，才认为该源存在
		//if (!( g_SessionStateMgr.findRev(iSSRC)))
		//    g_SessionStateMgr.addRev(iSSRC);

		ptr += sizeof(uint32_t); len -= sizeof(uint32_t);

		wchar_t buf[256];
		//对一个源的多个说明条目逐一解释
		while(len >= 1)
		{
			//SDES的类型，描述字符串长度
			uint8_t type = *ptr++; len--;
			if (type == RTCP_SDES_NONE || type > RTCP_SDES_PRIV || len <= 0) break;

			uint8_t size = *ptr++; len--;
			if (len < size) break;

			// 字符串是UTF-8编码，转成Unicode串
			int cch = MultiByteToWideChar(reinterpret_cast<const char*>(ptr), size, buf, 256);
			if (cch > 0 && cch < 256) buf[cch] = 0; else buf[0] = 0;
            
			//按类型复制到适当条目
			switch(type)
			{
			case RTCP_SDES_CNAME: m_revDpt->m_sdes.m_sCName.assign(buf,cch);    break;
			case RTCP_SDES_NAME:  m_revDpt->m_sdes.m_sName.assign(buf,cch);     break;
			case RTCP_SDES_EMAIL: m_revDpt->m_sdes.m_sEmail.assign(buf,cch);    break;
			case RTCP_SDES_PHONE: m_revDpt->m_sdes.m_sPhone.assign(buf,cch);    break;
			case RTCP_SDES_LOC:   m_revDpt->m_sdes.m_sLocation.assign(buf,cch); break;
			case RTCP_SDES_TOOL:  m_revDpt->m_sdes.m_sTool.assign(buf,cch);     break;
			case RTCP_SDES_NOTE:  m_revDpt->m_sdes.m_sNote.assign(buf,cch);     break;
			case RTCP_SDES_PRIV:  m_revDpt->m_sdes.m_sPrivate.assign(buf,cch);  break;
			}

			//指向下一条目
			ptr += size; len -= size;
		}
	}
}

//解析一个RTCP BYE报文
void RTCPReceiver::decode_rtcp_bye(const uint8_t* pBuffer)
{
	//RTCP 通用头部
	auto hdr = reinterpret_cast<const rtcp_header_common*>(pBuffer);
	int len = (ntohs(hdr->length) + 1) << 2;

	auto ssrc = reinterpret_cast<const uint32_t *>(hdr + 1);
	for(uint8_t i = 0; i < hdr->cnt && len >= sizeof(uint32_t); i++, ssrc++, len -= sizeof(uint32_t))
		g_SessionStateMgr.setDeleteRev(ntohl(*ssrc));	//删除对应的接收源描述符，如果根据SSRC找不到对应的描述符的话，则不删除
    
    g_SessionStateMgr.fastDelete();	//立即执行检查接收源存活情况例程
}

void RTCPReceiver::storeRevSR(const rtcp_sr* sr, uint32_t ssrc)
{    
	//保存最近收到SR的NTP 64bit时间
	m_revDpt->m_iLastSRRevFrac  = ntohl(sr->ntp.lower);
	m_revDpt->m_iLastSRRevSec   = ntohl(sr->ntp.upper);
	//保存最近收到SR的reference media timestamp
	m_revDpt->m_iLastSRRevTS    = ntohl(sr->ts);
	//记录 nPacket 和 nBytes ，不过不知道有什么用
	m_revDpt->m_nPacket         = ntohl(sr->np);
	m_revDpt->m_nByte           = ntohl(sr->nb);
}
