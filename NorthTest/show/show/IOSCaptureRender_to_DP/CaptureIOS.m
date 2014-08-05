//
//  capture.m
//  show
//
//  Created by ADMIRE_2 on 13-6-1.
//  Copyright (c) 2013年 ADMIRE_2. All rights reserved.
//

#import "CaptureIOS.h"

@implementation CaptureIOS


@synthesize device;
@synthesize captureInput;
@synthesize captureOutput;
@synthesize connect;
@synthesize captureSession;


-(id)init
{
    InitializeCriticalSection(&m_csList);
    
    captureSession = [[AVCaptureSession alloc] init];
    
    
    //camera init
    
    device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    
    for(AVCaptureDevice *cam in [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo])
    {
        if(cam.position == AVCaptureDevicePositionFront)
            device = cam;
    }
    
    
    captureInput = [AVCaptureDeviceInput deviceInputWithDevice:device error:nil];
    
    
    
    captureOutput = [[AVCaptureVideoDataOutput alloc] init];
    captureOutput.alwaysDiscardsLateVideoFrames = YES;
    
    connect = [captureOutput connectionWithMediaType:AVMediaTypeVideo]; //fps
    connect.videoMinFrameDuration = CMTimeMake(1, 20);
    
    
    
    
    
    NSDictionary *videoSettings = @{(__bridge NSString *) kCVPixelBufferPixelFormatTypeKey:[NSNumber numberWithUnsignedInt:kCVPixelFormatType_32BGRA]};
    [captureOutput setVideoSettings:videoSettings];
    
    
    
    dispatch_queue_t queue;
    queue = dispatch_queue_create("camaraQueue", DISPATCH_QUEUE_SERIAL);
    
    [captureOutput setSampleBufferDelegate:self queue:queue];
    
    
    [captureSession addInput:captureInput];
    [captureSession addOutput:captureOutput];
    [captureSession setSessionPreset:AVCaptureSessionPresetMedium];
    
    
    NSLog(@"initial captureSession ok");
    return self;
}

-(void)startWithSema:(sem_t *)_sema{
    m_hSemaFrame = _sema;
    [captureSession startRunning];
    NSLog(@"startrunning");
}

-(void)stop{
    DeleteCriticalSection(&m_csList);
    [captureSession stopRunning];
}

-(void)appendFrame:(TFrame*) frame
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

-(TFrame*) getFrame
{
	Critical_Section lock(&m_csList);
    
	TFrame* p = m_frame.front();
	m_frame.pop_front();
    
	return p;
}

-(void )clearFrameBuffer
{
	//丢弃已捕获的数据
	Critical_Section lock(&m_csList);
	while(!m_frame.empty())
	{
		delete m_frame.front();
		m_frame.pop_front();
	}
}


- (void) captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
    
    NSLog(@"hello");
    
    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    CVPixelBufferLockBaseAddress(imageBuffer, 0);
    
    
    
       int width = CVPixelBufferGetWidth(imageBuffer);
       int height = CVPixelBufferGetHeight(imageBuffer);
    
    
      
    
    
    //how to release rawPixelBase?? 
    unsigned char *rawPixelBase = (unsigned char *) CVPixelBufferGetBaseAddress(imageBuffer);    
    captureFrame = avcodec_alloc_frame(); //remeber release
    YUVFrame = avcodec_alloc_frame() ;
    
    YUVbuffer = (uint8_t *)malloc(width*height*3/2);
    avpicture_fill((AVPicture *)captureFrame, rawPixelBase, PIX_FMT_RGB32, width, height);
    
    avpicture_fill((AVPicture *)YUVFrame, YUVbuffer, PIX_FMT_YUV420P, width, height);
    
    sws_getCachedContext(rgb2yuv_sws_context, width, height, PIX_FMT_RGB32, width, height, PIX_FMT_YUV420P, SWS_POINT, NULL, NULL, NULL);
    
    
    captureFrame->data[0] += captureFrame->linesize[0] * (height - 1);
    captureFrame->linesize[0] *= -1;
    
    captureFrame->data[1] += captureFrame->linesize[1] * (height/2 - 1);
    captureFrame->linesize[1] *= -1;
    
    captureFrame->data[2] += captureFrame->linesize[2] * (height/2 - 1);
    captureFrame->linesize[3] *= -1;

    
    
    
    sws_scale(rgb2yuv_sws_context, captureFrame->data, captureFrame->linesize, 0, height, YUVFrame->data, YUVFrame->linesize);
    
    
    TFrame *myTFrame = new TFrame(width,height);
    
    for(int i=0;i<4;i++){
        myTFrame->line[i] = YUVFrame->linesize[i];
        memcpy(myTFrame->data[i], YUVFrame->data[i],YUVFrame->linesize[i]);
    }
    
    [self appendFrame:myTFrame];
    
    
    dispatch_async(dispatch_get_main_queue(), ^{
        
        NSLog(@"capture out pu");
        
    });
    av_free(captureFrame);
    av_free(YUVFrame);
    YUVFrame = NULL;
    captureFrame = NULL;
    
    free(YUVbuffer);
    //NSLog(@"ok");
    
    CVPixelBufferUnlockBaseAddress(imageBuffer,0);
    
    
     
}









































@end


