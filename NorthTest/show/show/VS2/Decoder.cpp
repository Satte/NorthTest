#include "Types.h"
#include "Decoder.h"

bool CDecoder::CheckSingleRTPPacket(const TRTPPacket* packet)
{
	//第一步，检查协议版本号，必须为2
	if (packet->Header()->ver != RTP_VERSION) return false;

	//第二步，检查X标志
	if (!packet->Header()->x) return false;

	//第三步，检查扩展头
	if (packet->Extension()->profile != 0x5356) return false;

	//第四步，负载类型检测
    if (packet->Header()->pt != RTP_PT_H264) return false;

	const h264_header* hdr = packet->H264();
	if (hdr->type == H264_NAL_FU)
		return !(reinterpret_cast<const h264_header_fu*>(hdr)->s == true && reinterpret_cast<const h264_header_fu*>(hdr)->e == true);
	else if ((hdr->type >= 1 && hdr->type <= 23) || hdr->type == H264_NAL_STAP_A)
		return true;
	else
		return false;
}

int CDecoder::GetPayloadLength(const TRTPPacket* packet)
{
	const h264_header* hdr = packet->H264();
	int length = 0;
	switch(hdr->type)
	{
	case H264_NAL_FU:
		length = packet->PayloadLength() - sizeof(h264_header_fu) + ((reinterpret_cast<const h264_header_fu*>(hdr)->s) ? 5 : 0);
		break;
	case H264_NAL_STAP_A:
		for(const uint8_t* p = reinterpret_cast<const uint8_t*>(hdr + 1); p < packet->Data() + packet->PayloadLength(); )
		{
			int len = ntohs(*reinterpret_cast<const uint16_t*>(p));
			length += 4 + len;
			p += sizeof(uint16_t) + len;
		}
		break;
	default:
		length = packet->PayloadLength() + 4;
		break;
	}
	return length;
}


//初始化解码器
void CDecoder::Init(int iWidth, int iHeight)
{
	if (m_bInited) Close();
	
	m_decoder = get_ffmpeg_video_decoder("h264");
	if (m_decoder) m_bInited = m_decoder->initialize(0, 0);         // #define PIX_FMT_YUV420P 0
	if (m_bInited)
	{
		m_iWidth = iWidth; m_iHeight = iHeight;
		m_iBufferLength = iWidth * iHeight * 3 / 2;
		m_pBuffer = (uint8_t*)malloc(m_iBufferLength);
	}
}

void CDecoder::Close()
{
	if (m_bInited)
	{
		m_decoder->release(); m_decoder = nullptr;
		m_iWidth = m_iHeight = m_iBufferLength = 0;
		free(m_pBuffer); m_pBuffer = nullptr;
		m_bInited = false;
	}
}

//对一帧解码
bool CDecoder::Decode(const RTPFrame& src, TFrame* pDst)
{
	if (!m_bInited) return false;

	if (m_iBufferLength < src.FrameLength())
	{
		m_pBuffer = (uint8_t*)realloc(m_pBuffer, src.FrameLength());
		m_iBufferLength = src.FrameLength();
	}
	Unpack(src);

	VideoFrame frame;

    if (m_decoder->decode_frame(m_pBuffer, src.FrameLength(), frame, pDst->width, pDst->height))	// 解码图像
    {
        // 将输入数据复制到编码缓冲区中
        int height[3] = { pDst->height, pDst->height >> 1, pDst->height >> 1 };
        for(int i = 0; i < 3; i++)
        {
            uint8_t* src = frame.data[i];
            uint8_t* dst = pDst->data[i];
            for(int j = 0; j < height[i]; j++, src += frame.linesize[i], dst += pDst->line[i])
                memcpy(dst, src, pDst->line[i]);
        }
        return true;
    }
    else
        return false;
}

void CDecoder::Unpack(const RTPFrame& src)
{
	static uint8_t start_code[4] = { 0x00, 0x00, 0x00, 0x01 };

	auto packets = src.Packets();
	uint8_t* ptr = (uint8_t*)m_pBuffer;
	for(auto i = 0; i < src.Size(); i++)
	{
		const h264_header* hdr = packets[i]->H264();
		switch(hdr->type)
		{
		case H264_NAL_FU:
			if (reinterpret_cast<const h264_header_fu*>(hdr)->s)
			{
				memcpy(ptr, start_code, sizeof(start_code));
				ptr += sizeof(start_code);
				h264_header* h = reinterpret_cast<h264_header*>(ptr++);
				h->type = reinterpret_cast<const h264_header_fu*>(hdr)->nal_type;
				h->nri = reinterpret_cast<const h264_header_fu*>(hdr)->nri;
				h->f = reinterpret_cast<const h264_header_fu*>(hdr)->f;
			}
			memcpy(ptr, reinterpret_cast<const h264_header_fu*>(hdr) + 1, packets[i]->PayloadLength() - sizeof(h264_header_fu));
			ptr += packets[i]->PayloadLength() - sizeof(h264_header_fu);
			break;
		case H264_NAL_STAP_A:
			for(const uint8_t* p = reinterpret_cast<const uint8_t*>(hdr + 1); p < packets[i]->Data() + packets[i]->PayloadLength(); )
			{
				int len = ntohs(*reinterpret_cast<const uint16_t*>(p));
				p += sizeof(uint16_t);
				memcpy(ptr, start_code, sizeof(start_code));
				ptr += sizeof(start_code);
				memcpy(ptr, p, len);
				ptr += len; p += len;
			}
			break;
		default:
			memcpy(ptr, start_code, sizeof(start_code));
			ptr += sizeof(start_code);
			memcpy(ptr, hdr, packets[i]->PayloadLength());
			ptr += packets[i]->PayloadLength();
			break;
		}
	}
}


