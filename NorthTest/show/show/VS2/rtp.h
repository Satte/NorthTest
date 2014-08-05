#pragma once

#include "Types.h"
#include <stdint.h>


//sizeof(rtp_header) = 12 Bytes, BIGENDIAN
struct rtp_header
{
	uint8_t		cc:4;		//CSRC个数
	uint8_t		x:1;		//头部扩展标志
	uint8_t		p:1;		//字节补齐标志
	uint8_t		ver:2;		//报文版本
	uint8_t		pt:7;		//负载类型
	uint8_t		m:1;		//标记位
	uint16_t	seq;		//报文序列号
	uint32_t	ts;			//报文发送的RTP时间
	uint32_t	ssrc;		//报文源标识
};

//sizeof(rtp_header) = 12 Bytes
struct rtp_extension
{
	uint16_t	profile;    //扩展头标识
	uint16_t	length;     //扩展头（除标识和本身）的长度,以4字节为单位，所以0也是有效值
	uint16_t	width;      //视频的宽，我们在扩展头里包括我们视频的格式信息
	uint16_t	height;		//视频的高，我们在扩展头里包括我们视频的格式信息
	uint8_t		iframe:1;	//标识本帧是否是I帧
	uint8_t		resend:1;	//重发标志
	uint8_t		reserved:2;
	uint8_t		stereo:4;	//标识立体帧类型
	uint8_t		index;		//该RTP包是本帧的第几个包
	uint8_t		total;		//本帧RTP包的数量-1
	uint8_t		indexh:4;
	uint8_t		totalh:4;	//留给后人使用

	uint16_t Index() const { return ((uint16_t)indexh << 8) | (uint16_t)index; }
	void Index(uint16_t value) { index = (uint8_t)value; indexh = value >> 8; }

	uint16_t Total() const { return (((uint16_t)totalh << 8) | (uint16_t)total) + 1; }
	void Total(uint16_t value) { value--; total = (uint8_t)value; totalh = value >> 8; }
};

struct h264_header
{
	uint8_t		type:5;		//nal_unit_type :表示NAL的类型，从0～31
	uint8_t		nri:2;		//nal_ref_bits :表示该NAL是否被用来构造参考帧
	uint8_t		f:1;		//forbidden_zero_bit :强制0位
};

//fragmentation unit packet的header
struct h264_header_fu
{
	uint8_t		type:5;		//该fragment的类型，28或29
	uint8_t		nri:2;		//该fragment所属于的nal的nri位的值
	uint8_t		f:1;		//该fragment所属于的nal的f位的值
	uint8_t		nal_type:5;	//该fragment所属于的nal的类型
	uint8_t		r:1;		//该保留位须置为0，接收者可以忽略
	uint8_t		e:1;		//该fragment是否为所属于的nal的最后一个分片
	uint8_t		s:1;		//该fragment是否为所属于的nal的第一个分片
};

struct h263_header
{
	uint8_t		r0:1;
	uint8_t		v:1;
	uint8_t		p:1;		//是否省略GBSC
	uint8_t		rr:5;
	uint8_t		r1;
};

struct h263_header_full_le
{
	uint16_t	pebit:3;
	uint16_t	plen:6;
	uint16_t	v:1;
	uint16_t	p:1;
	uint16_t	rr:5;
};

struct h261_header
{
	uint8_t		v:1;
	uint8_t		i:1;
	uint8_t		ebit:3;
	uint8_t		sbit:3;
	uint8_t		r1;
	uint16_t	r2;
};

struct h261_header_full_le
{
	uint32_t	vmvd:5;
	uint32_t	hmvd:5;
	uint32_t	quant:5;
	uint32_t	mbap:5;
	uint32_t	gobn:4;
	uint32_t	v:1;
	uint32_t	i:1;
	uint32_t	ebit:3;
	uint32_t	sbit:3;
};

struct TRTPPacket : public TPacket
{
	static const int MaxPayloadLength = 1400;
    
	rtp_header* Header() { return reinterpret_cast<rtp_header*>(Buffer); }
	rtp_extension* Extension() { return reinterpret_cast<rtp_extension*>(Buffer + sizeof(rtp_header)); }
	uint8_t* Data() { return Buffer + sizeof(rtp_header) + sizeof(rtp_extension); }
    
	const rtp_header* Header() const { return reinterpret_cast<const rtp_header*>(Buffer); }
	const rtp_extension* Extension() const { return reinterpret_cast<const rtp_extension*>(Buffer + sizeof(rtp_header)); }
	const uint8_t* Data() const { return Buffer + sizeof(rtp_header) + sizeof(rtp_extension); }
    
	int PayloadLength() const { return Length - (sizeof(rtp_header) + sizeof(rtp_extension)); }
    
	const h261_header* H261() const { return reinterpret_cast<const h261_header*>(Data()); }
	const h263_header* H263() const { return reinterpret_cast<const h263_header*>(Data()); }
	const h264_header* H264() const { return reinterpret_cast<const h264_header*>(Data()); }
};

class RTPFrame
{
public:
	RTPFrame(const TRTPPacket* packet);
	~RTPFrame();

	bool InsertPacket(const TRTPPacket* packet);

	bool KeyFrame() const { return m_keyframe; }
	bool Completed() const { return m_iCount == m_iSize; }
	int Size() const { return m_iSize; }
	int ReceivedCount() const { return m_iCount; }
	uint16_t Start() const { return m_wStartSeq; }
	int FrameLength() const { return m_iFrameLength; }
	uint32_t TS() const { return m_ts; }
	RTPPayloadType PT() const { return (RTPPayloadType)m_pt; }
	StereoFrameMode StereoFlag() const { return m_StereoFlag; }
	const TRTPPacket* const * Packets() const { return m_Buffer; }

private:
	const TRTPPacket**	m_Buffer;		//该帧的rtp报文buffer
	const int			m_iSize;		//RTP报文缓冲区的大小(数量)
	int					m_iCount;		//已经接收的RTP报文数量
	int					m_iFrameLength;	//该帧的编码数据大小
	int					m_iRetransmit;
	uint32_t			m_ts;			//该帧的时间戳(发送端，封包发送时的时间，单个帧内个RTP包都一样)
	uint16_t			m_wStartSeq;	//该帧的起始序列号
	StereoFrameMode		m_StereoFlag;
	int8_t				m_pt;
	bool				m_keyframe;
};

//=========================================================================================================
