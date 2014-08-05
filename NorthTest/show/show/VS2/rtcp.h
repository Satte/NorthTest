#ifndef rtcp_h
#define rtcp_h

//#include "Types.h"
#include <string>
using namespace std;

struct rtcp_header_common
{
	uint8_t		cnt:5;		//报告条目数
	uint8_t		p:1;		//字节补齐标志
	uint8_t		ver:2;		//报文版本
	uint8_t		pt;			//负载类型
	uint16_t	length;		//以32-bit为单位的报文长度减去1
};

struct rtcp_header : public rtcp_header_common
{
	uint32_t	ssrc;		//发送源的SSRC
};

//necessary zq
struct  ntp64
{
	unsigned int upper;	/* more significant 32 bits */
	unsigned int lower;	/* less significant 32 bits */
};

/*
* Sender report.
*/
struct rtcp_sr 
{
	ntp64		ntp;	/* 64-bit ntp timestamp */
	uint32_t	ts;		/* reference media timestamp */
	uint32_t	np;		/* no. packets sent */
	uint32_t	nb;		/* no. bytes sent */
};

/*
* Receiver report.
* Time stamps are middle 32-bits of ntp timestamp.
*/
struct rtcp_rr 
{
	uint32_t	ssrc;	/* source to be reported */
	uint32_t	loss;	/* loss stats (8:fraction, 24:cumulative)*/
	uint32_t	ehsr;	/* ext. highest seqno received */
	uint32_t	dv;	    /* jitter (delay variance) */
	uint32_t	lsr;	/* orig. ts from last rr from this src  */
	uint32_t	dlsr;	/* time from recpt of last rr to xmit time */
};

struct SDES
{
	wstring m_sCName;
	wstring m_sName;
	wstring m_sEmail;
	wstring m_sPhone;
	wstring m_sLocation;
	wstring m_sTool;
	wstring m_sNote;
	wstring m_sPrivate;

	SDES() { }
	SDES(const wstring& cname, const wstring& name, const wstring& email, const wstring& phone, const wstring& loc, const wstring& tool, const wstring& note, const wstring& priv)
		: m_sCName(cname), m_sName(name), m_sEmail(email), m_sPhone(phone), m_sLocation(loc), m_sTool(tool), m_sNote(note), m_sPrivate(priv) { }
};

//Rtcp_Info and Rtcp_Info::UserInfo
//struct Rtcp_Info
//{
//	Rtcp_Info()
//		: dwIP(0x030808ee)	// 238.8.8.3
//		, iPort(8001)
//		, iTTL(127)
//		, bNat(false)
//	{
//	}
//
//	uint32_t dwIP;       //目的IP地址
//	int iPort;        //目的端口
//	int iTTL;         //报文存活时间
//	bool bNat;        //是否使用NAT模式
//
//	struct UserInfo
//	{
//		UserInfo()
//			: name(L"no name")
//			, addr(L"no address")
//			, phone(L"no phone")
//			, mail(L"no mail")
//			, notes(L"no note")
//			, username(L"test")
//			, cname(L"test@127.0.0.1")
//		{
//		}
//
//		CStringW name;	 //节点名
//		CStringW addr;	 //通讯地址
//		CStringW phone;	 //电话
//		CStringW mail;	 //电子邮件
//		CStringW cname;	 //用户名
//		CStringW notes;	 //备注       视频流名称：name(节点名).notes(用户名)
//		CStringW username;//用户名
//	} userinfo;
//};

#endif 
