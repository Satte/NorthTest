#pragma once
#include <stdint.h>
#include <vector>
#include <memory>
#include <list>
#include "rtp.h"
#include "Types.h"
#include "Frame.h"
#include "codec.h"

class CEncoder
{
public:
	CEncoder(srcDescriptor* srcDpt, uint32_t& iSeq) : m_bInited(false), m_iSeq(iSeq), m_srcDpt(srcDpt), m_iTS(0), m_encoder(nullptr) { }
	~CEncoder() {Close();}

	void Init(int iWidth, int iHeight, int iQuality, int iBandWidth, int iFrameRate, int iKeyframeFreq);	//启动编码器
	int Encode(TFrame* pSrc, std::vector<std::unique_ptr<TRTPPacket>>& vPackets);	//对一帧编码，返回编码块的大小
	void setQuality(int iQuality) { m_iQuality = (iQuality == 100) ? X264_QP_AUTO : (100 - iQuality) / 3 + 19; }  //设定编码器编码质量
protected:
	srcDescriptor* m_srcDpt;
	uint32_t& m_iSeq;
	uint32_t m_iTS;
	int m_iQuality;				//压缩质量
	bool m_bIFrameCoded;		//标识刚刚编码的帧是否是I帧，如果是，则为true
	bool m_bInited;				//是否完成初始化

//==================arange by gs================
    x264_video_encoder *m_encoder;
    int m_width, m_height;

	int Pack(nal_t* nal, int count,  std::vector<std::unique_ptr<TRTPPacket>>& vPackets);
//==============================================


	void Close();	//关闭编码器。不需要显式调用，再次Init或析构时自动调用

	void InitPack()
	{
		m_srcDpt->m_fs = (uint16_t)m_iSeq;
	}

	int FinishPack(std::vector<std::unique_ptr<TRTPPacket>>& vPackets, uint16_t start)
	{
		uint16_t count = (uint16_t)m_iSeq - start;
		vPackets[(uint16_t)(m_iSeq - 1)]->Header()->m = true;
		for(uint16_t i = 0; i < count; i++)
			vPackets[(uint16_t)(start + i)]->Extension()->Total(count);
		m_srcDpt->m_cs = (uint16_t)(m_iSeq - 1);
		m_srcDpt->m_np += count;
		return count;
	}

	void InitializeRTPHeader(rtp_header* pHeader, RTPPayloadType pt)
	{
		pHeader->cc = 0;
		pHeader->x = 1;
		pHeader->p = 0;
		pHeader->ver = 2;
		pHeader->pt = (uint8_t)pt;
		pHeader->ssrc = htonl(m_srcDpt->m_ssrc);
		pHeader->seq = htons(m_iSeq++);
		pHeader->ts = m_iTS;
		pHeader->m = false;
	}

	void InitializeRTPExtension(rtp_extension* ext, uint16_t width, uint16_t height, int16_t index)
	{
		ext->profile =  0x5356; //'SV';
		ext->length = htons((sizeof(rtp_extension) >> 2) - 1);
		ext->width = htons(width);
		ext->height = htons(height);
		ext->iframe = m_bIFrameCoded;
		ext->resend = false;
		ext->stereo = (uint8_t)0;
		ext->Index(index);
		ext->Total(0);
		ext->reserved = 0;
	}
};
