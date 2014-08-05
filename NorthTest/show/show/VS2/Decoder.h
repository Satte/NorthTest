#pragma once
#include <stdint.h>
#include <vector>
#include <memory>
#include <list>
#include "rtp.h"
#include "Types.h"
#include "Frame.h"
#include "codec.h"

class CDecoder
{
public:
	CDecoder() :m_bInited(false), m_decoder(nullptr), m_iWidth(0), m_iHeight(0), m_iBufferLength(0), m_pBuffer(nullptr) {}
	~CDecoder() { Close();}
	void Init(int iWidth, int iHeight);	//初始化解码器
	bool Decode(const RTPFrame& src, TFrame* pDst);							//对一帧解码

	static bool CheckSingleRTPPacket(const TRTPPacket* packet);
	static int GetPayloadLength(const TRTPPacket* packet);

private:
	void Close();																	//关闭解码器。不需要显式调用，再次Init或析构时自动调用
	void Unpack(const RTPFrame& src);

	bool m_bInited;
	int m_iWidth, m_iHeight, m_iBufferLength;
	void* m_pBuffer;
	ffmpeg_video_decoder* m_decoder;
};
