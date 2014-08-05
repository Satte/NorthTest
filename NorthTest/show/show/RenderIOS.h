//
//  render.h
//  show
//
//  Created by ADMIRE_2 on 13-6-1.
//  Copyright (c) 2013年 ADMIRE_2. All rights reserved.
//
#pragma once
#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <QuartzCore/QuartzCore.h>
#include "Frame.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
    
}


@interface RenderIOS : NSObject{
    UIImageView *imageView;
    AVPicture picture;
    
    UIImage *image;
    
    struct SwsContext *yuv2rgb_sws_context;
    AVFrame *YUVFrame;

    
}
-(id)initWithUIImageView:(UIImageView *)_imageView;
-(void)RenderWithTFrame:(TFrame*)_tFrame;

@end
