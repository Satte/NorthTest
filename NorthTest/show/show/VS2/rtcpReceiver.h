#pragma once

#include "rtcp.h"
#include "Types.h"

class RTCPReceiver
{
public:
	RTCPReceiver(revDescriptor* revDpt) : m_revDpt(revDpt) { }	//描述符指针置空
	~RTCPReceiver() { }

	//解析一个RTCP报文
	unsigned short parseRtcp(const TPacket* packet) ;

protected:
    //描述符指针
	revDescriptor* m_revDpt ;

	//检查RTCP报文头的合法性
    bool check_rtcp_header(const TPacket* packet);

	//以下分别对四类RTCP报文解码
	void decode_rtcp_sr(const uint8_t* pBuffer);
	void decode_rtcp_rr(const uint8_t* pBuffer);
	void decode_rtcp_sdes(const uint8_t* pBuffer);
	void decode_rtcp_bye(const uint8_t* pBuffer);
	//解析一个rr
	void decode_one_rrreport(const rtcp_rr* rr_hdr);

	//存储指定SSRC的SR信息
	void storeRevSR(const rtcp_sr* sr , uint32_t ssrc);
    int MultiByteToWideChar(const char* inbuf, uint8_t insize, wchar_t *outbuf, int maxoutsize);
};
