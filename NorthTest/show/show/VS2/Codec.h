//
//  code.h
//  video
//
//  Created by ADMIRE_2 on 13-5-17.
//  Copyright (c) 2013年 ADMIRE_2. All rights reserved.
//

#ifndef video_code_h
#define video_code_h


#pragma once
#include <stdint.h>


#define	MAX_PICTURE_COUNT		2
#define	INTERNAL_BUFFER_SIZE	MAX_PICTURE_COUNT
#ifndef CODECS_BUILD
#	define FF_LAMBDA_MAX		(256*128-1)
#	define X264_QP_AUTO			0
#	define FF_DEBUG_PICT_INFO   1
#	define FF_DEBUG_STARTCODE   0x00000100
#	define X264_CSP_I420		0x0001

#	define ILBC_MODE_20ms		20
#	define ILBC_MODE_30ms		30
#endif

struct VideoFrame
{
    uint8_t *data[4];
    int linesize[4];
};

struct nal_t
{
	uint8_t*	p_payload;
	int			i_payload;
};

// 用于编码 H.264 视频流
class x264_video_encoder
{
public:
	// 以给定参数初始化编码器
	virtual bool initialize(int csp, int width, int height, int bitrate, int fps, int keyint, const char* preset, const char* tune, const char* profile) = 0;
	// 编码一帧并返回 NAL 引用
	virtual int encode_frame(const VideoFrame& frame, int quality, nal_t*& p_nals, int& n_nals, bool& keyframe) = 0;
	// 释放本对象
	virtual void release() = 0;
};

// 得到一个 x264_video_encoder 对象的指针
extern "C" x264_video_encoder* get_x264_video_encoder(void);

// 用于解码 H.263/H.264/MPEG4 视频流
class ffmpeg_video_decoder
{
public:
	// 以给定参数初始化解码器
	virtual bool initialize(int csp, int debug) = 0;
	// 以给定参数初始化 DXVA 解码器
	//virtual bool initialize_dxva(struct IDirect3DSurface9 **surface, struct IDirectXVideoDecoder *decoder, const struct _DXVA2_ConfigPictureDecode *cfg, unsigned int surface_count, int width) = 0;
	// 解码一帧
	virtual bool decode_frame(const void* buffer, int length, VideoFrame& frame, int& width, int& height) = 0;
	// 释放本对象
	virtual void release() = 0;
};
// 根据指定的解码器名称得到一个 ffmpeg_video_decoder 对象的指针
extern "C" ffmpeg_video_decoder* get_ffmpeg_video_decoder(const char* codec_name);

#endif
