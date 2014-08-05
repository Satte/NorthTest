#include "Encoder.h"
#include "mediaTimer.h"

#pragma comment(lib, "codecs.lib")

void CEncoder::Init(int iWidth, int iHeight, int iQuality, int iBandWidth, int iFrameRate, int iKeyframeFreq)
{
	if (m_bInited) Close();

	setQuality(iQuality);
	m_width = iWidth;
	m_height = iHeight;

	m_encoder = get_x264_video_encoder();
	if (m_encoder) m_bInited = m_encoder->initialize(X264_CSP_I420, iWidth, iHeight, iBandWidth, iFrameRate, iKeyframeFreq, "ultrafast", "zerolatency", "baseline");
	if (m_bInited) InitPack();
}

int CEncoder::Encode(TFrame* pSrc, std::vector<std::unique_ptr<TRTPPacket>>& vPackets)
{
	if (!m_bInited) return 0;

	VideoFrame frame = { { pSrc->data[0], pSrc->data[1], pSrc->data[2] }, { pSrc->line[0], pSrc->line[1], pSrc->line[2] } };
	nal_t* nals;
	int nbytes, n;
	if ((nbytes = m_encoder->encode_frame(frame, m_iQuality, nals, n, m_bIFrameCoded)) > 0)
		return Pack(nals, n, vPackets);
	else
		return 0;
}

void CEncoder::Close()
{
	if (!m_bInited) return;
	m_encoder->release();
	m_encoder = nullptr;
}

int CEncoder::Pack(nal_t* nal, int count,  std::vector<std::unique_ptr<TRTPPacket>>& vPackets)
{
	//视频报文的时间戳频率固定为90kHz
	uint32_t iTS = htonl(MediaTimer::media_ts());
	m_iTS = (iTS == m_iTS) ? iTS + 0x10000 : iTS;

	int index = 0, frag_offset = 5;	// 分片包的初始偏移为5，即跳过开始码和NAL头
	int nPkts = 0;
	uint16_t start_seq = m_iSeq;
	while(index < count)
	{
		vPackets[(uint16_t)m_iSeq].reset(new TRTPPacket);
		auto& pkt = *vPackets[(uint16_t)m_iSeq];

		// 初始化包头和扩展头
		InitializeRTPHeader(pkt.Header(), RTPPayloadType::RTP_PT_H264);
		InitializeRTPExtension(pkt.Extension(), m_width, m_height, nPkts++);
		uint8_t* data = pkt.Data();
		if (nal[index].i_payload - 4 > TRTPPacket::MaxPayloadLength)
		{
			// NAL长度大于最大包长，需要分片（FU-A，Type 28）
			h264_header* hdr = reinterpret_cast<h264_header*>(nal[index].p_payload + 4);	// 跳过 NAL 起始码
			h264_header_fu* fu_hdr = reinterpret_cast<h264_header_fu*>(data);

			// 以下FU头部设置的值依据 RFC 3984
			fu_hdr->type = H264_NAL_FU;
			fu_hdr->nri = hdr->nri;
			fu_hdr->f = hdr->f;
			fu_hdr->nal_type = hdr->type;
			fu_hdr->r = false;
			fu_hdr->s = (frag_offset == 5);
			fu_hdr->e = (nal[index].i_payload - frag_offset <= TRTPPacket::MaxPayloadLength - 2);
			if (fu_hdr->e)
			{
				pkt.Length = nal[index].i_payload - frag_offset + 2;
				memcpy(fu_hdr + 1, nal[index].p_payload + frag_offset, pkt.Length - 2);
				frag_offset = 5; index++;
			}
			else
			{
				pkt.Length = TRTPPacket::MaxPayloadLength;
				memcpy(fu_hdr + 1, nal[index].p_payload + frag_offset, pkt.Length - 2);
				frag_offset += TRTPPacket::MaxPayloadLength - 2;
			}
		}
		else if (index < count - 1 && nal[index].i_payload + nal[index + 1].i_payload - 3 <= TRTPPacket::MaxPayloadLength)
		{
			// 两个以上的NAL总长度小于包长，打成聚合包（STAP-A，Type 24）
			int n = 2, total = nal[index].i_payload + nal[index + 1].i_payload - 3;
			// 首先计算可以打到包内的最大长度和NAL个数
			while(index + n < count && total + nal[index + n].i_payload - 2 <= TRTPPacket::MaxPayloadLength)
				total += nal[index + n++].i_payload - 2;

			// 填充AU头部
			h264_header* au_hdr = reinterpret_cast<h264_header*>(data);
			au_hdr->type = H264_NAL_STAP_A;	// STAP-A
			au_hdr->f = false;
			au_hdr->nri = 0;

			// 依次将各个NAL打入包中
			uint8_t* ptr = reinterpret_cast<uint8_t*>(au_hdr + 1);
			for(int i = 0; i < n; i++, index++)
			{
				h264_header* hdr = reinterpret_cast<h264_header*>(nal[index].p_payload + 4);
				*reinterpret_cast<uint16_t*>(ptr) = htons(nal[index].i_payload - 4);
				memcpy(ptr + sizeof(uint16_t), hdr, nal[index].i_payload - 4);
				au_hdr->f |= hdr->f;
				if (au_hdr->nri < hdr->nri) au_hdr->nri = hdr->nri;
				ptr += nal[index].i_payload - 4 + sizeof(uint16_t);
			}
			pkt.Length = total;
		}
		else
		{
			// 单一包
			pkt.Length = nal[index].i_payload - 4;
			memcpy(pkt.Data(), nal[index].p_payload + 4, pkt.Length);
			index++;
		}
		pkt.Length += sizeof(rtp_header) + sizeof(rtp_extension);
		m_srcDpt->m_nb += pkt.Length;
	}
	return FinishPack(vPackets, start_seq);
}
