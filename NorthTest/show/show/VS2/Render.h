#pragma once

#include <vector>
#include "Types.h"
#include "Decoder.h"

class CRender
{
public:
	CRender(VIEW hWnd)
		: m_pTempBuffer(nullptr)
		, m_hWnd(hWnd)
        , m_bInited(false)
	{ }
	virtual ~CRender() { Detach(); free(m_pTempBuffer); }

	bool Attach(int iWidthSrc, int iHeightSrc);
	void Detach();
	void Render(TFrame* frame);

protected:
	uint8_t* m_pTempBuffer;
	VIEW m_hWnd;							//目标窗口句柄
	int m_iWidthSrc, m_iHeightSrc;			//源图像的大小
	CDecoder* m_pcbDecoder;
    bool m_bInited;
};
