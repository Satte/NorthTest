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
	void Init(int iWidth, int iHeight);	//��ʼ��������
	bool Decode(const RTPFrame& src, TFrame* pDst);							//��һ֡����

	static bool CheckSingleRTPPacket(const TRTPPacket* packet);
	static int GetPayloadLength(const TRTPPacket* packet);

private:
	void Close();																	//�رս�����������Ҫ��ʽ���ã��ٴ�Init������ʱ�Զ�����
	void Unpack(const RTPFrame& src);

	bool m_bInited;
	int m_iWidth, m_iHeight, m_iBufferLength;
	void* m_pBuffer;
	ffmpeg_video_decoder* m_decoder;
};
