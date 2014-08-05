//
//  capture.h
//  show
//
//  Created by ADMIRE_2 on 13-6-1.
//  Copyright (c) 2013年 ADMIRE_2. All rights reserved.
//
#pragma once 
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
#import <CoreGraphics/CoreGraphics.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>
#include "Frame.h"

#include <list>


extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
    
}

#include <semaphore.h>
#include "RWLock.h"

@interface CaptureIOS : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
{
    int32_t m_iWidth;
    int32_t m_iHeight;
    AVFrame *captureFrame;
    AVFrame *YUVFrame;
    struct SwsContext *rgb2yuv_sws_context;
    uint8_t *YUVbuffer;
    
    CRITICAL_SECTION m_csList;
    std::list<TFrame * >m_frame;
    sem_t *m_hSemaFrame;
}




@property (nonatomic,strong) AVCaptureDevice *device;
@property (nonatomic,strong) AVCaptureDeviceInput *captureInput;
@property (nonatomic,strong) AVCaptureVideoDataOutput *captureOutput;
@property (nonatomic,strong) AVCaptureConnection *connect;
@property (nonatomic,strong) AVCaptureSession *captureSession;

- (void)start;
- (void)appendFrame:(TFrame *)frame;
- (void)clearFrameBuffer;
- (TFrame*)getFrame;
- (void)stop;
@end



