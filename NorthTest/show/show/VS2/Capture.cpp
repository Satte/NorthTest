#include "Capture.h"
#include "RWLock.h"


void CCapture::ClearFrameBuffer()
{
	//丢弃已捕获的数据
	Critical_Section lock(&m_csList);
	while(!m_frame.empty())
	{
		delete m_frame.front();
		m_frame.pop_front();
	}
}

TFrame* CCapture::GetFrame()
{
	Critical_Section lock(&m_csList);

	TFrame* p = m_frame.front();
	m_frame.pop_front();

	return p;
}

void CCapture::AppendFrame(TFrame* frame)
{
	//将捕获的数据放入缓冲队列
	Critical_Section lock(&m_csList);

	m_frame.push_back(frame);
	if (m_frame.size() > 4)
	{
//		OutputDebugString(L"Drop frame!"));

		//队列长度超过一定数值，说明外部模块来不及完成后续操作，尝试放弃最早捕获帧以免造成更严重的数据堆积
		delete m_frame.front();
		m_frame.pop_front();
	}
	else
	{
	//	ReleaseSemaphore(m_hSemaFrame, 1, nullptr);
        sem_post(m_hSemaFrame);
	}
}


bool CCapture::Start(sem_t *hSemaFrame)
{
    return true;
}

void CCapture::Stop()
{
    return ;
}
