#pragma once
#include <list>
#include "Types.h"
#include "Frame.h"
#include <semaphore.h>
#include "RWLock.h"
class CCapture
{
public:
	CCapture(const ChannelInfo& info) : m_channelInfo(info), m_hSemaFrame(nullptr) { InitializeCriticalSection(&m_csList); }
	virtual ~CCapture() { DeleteCriticalSection(&m_csList); }

	//启动捕捉
	bool Start(sem_t *hSemaFrame) ;            //return ture !!!!
	void Stop() ;		//停止捕捉
	TFrame* GetFrame();				//提取一帧

protected:
	CRITICAL_SECTION m_csList;	//保护“已捕获帧缓冲”的互斥量
	std::list<TFrame*> m_frame;	//已捕获帧缓冲，队尾进，队首出
	sem_t *m_hSemaFrame;	    //标志“已捕获帧缓冲”内有无数据的信号量。由外部模块提供，本类仅负责对其的增加操作

	const ChannelInfo& m_channelInfo;

	void ClearFrameBuffer();
	void AppendFrame(TFrame* frame);
};
