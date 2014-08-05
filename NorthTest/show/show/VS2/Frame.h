
#pragma once
#include <stdint.h>
#include "Types.h"

// 表示一个视频帧
struct TFrame
{
	int			width;	// 帧宽度
	int			height;	// 帧高度
	uint8_t*	data[4];// 帧数据
	int			line[4];
	void*		extra;

	TFrame(int width, int height)
		: width(width), height(height), extra(nullptr)
	{
		data[0] = (uint8_t*)malloc(width * height * 3 / 2); line[0] = width;
		data[1] = data[0] + width * height;                 line[1] = width / 2;
		data[2] = data[1] + width * height / 4;             line[2] = width / 2;
		data[3] = nullptr;                                  line[3] = 0;
	}

	~TFrame() { free(data[0]); }

};
