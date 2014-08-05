#include "Render.h"
#include "codec.h"
#include <algorithm>

#pragma comment(lib, "codecs.lib")



bool CRender::Attach(int iWidthSrc, int iHeightSrc)
{
	return (m_bInited = true);
}

void CRender::Detach()
{
	if (!this) return;

	free(m_pTempBuffer);
	m_pTempBuffer = nullptr;
	m_bInited = false;

}


void CRender::Render(TFrame* frame)
{
}

