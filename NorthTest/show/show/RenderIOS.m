//
//  render.m
//  show
//
//  Created by ADMIRE_2 on 13-6-1.
//  Copyright (c) 2013年 ADMIRE_2. All rights reserved.
//

#import "RenderIOS.h"
#include "RecvManager.h"
#include <pthread.h>

@implementation RenderIOS

-(id)initWithUIImageView:(UIImageView *)_imageView{
    imageView = _imageView;
    return self;
}
-(void)RenderWithTFrame:(TFrame *)_tFrame{
    
    dispatch_async(dispatch_get_main_queue(), ^{
        if(image == NULL)
        {
            NSLog(@"image == null");
        }
        [imageView setBackgroundColor:[UIColor greenColor]];
        
    });

    
    
    if (_tFrame == NULL) {
        printf("null\n");
        return;
    }
    
    YUVFrame = avcodec_alloc_frame();
    for(int i = 0;i <4;i++)
    {
        YUVFrame->linesize[i] = _tFrame->line[i];
        YUVFrame->data[i] = _tFrame->data[i];
    }
    
    
    avpicture_alloc((AVPicture *)&picture, PIX_FMT_RGB24,_tFrame->width, _tFrame->height);
    
    
    sws_getCachedContext(yuv2rgb_sws_context, _tFrame->width, _tFrame->height, PIX_FMT_YUV420P, _tFrame->width, _tFrame->height, PIX_FMT_RGB24, SWS_POINT, NULL, NULL, NULL);
    sws_scale(yuv2rgb_sws_context, YUVFrame->data, YUVFrame->linesize, 0, _tFrame->height, picture.data, picture.linesize);
    
    
    image = [[UIImage alloc] init];
    image = [self imageFromAVPicture:picture width:_tFrame->width height:_tFrame->height];
    
    
    dispatch_async(dispatch_get_main_queue(), ^{
        if(image == NULL)
        {
            NSLog(@"image == null");
        }
        [imageView setBackgroundColor:[UIColor greenColor]];
    
    });
    
    
    
    /*   if there is something error,try this:
     *
     *   NSData *data = UIImageJPEGRepresentation(image, 0.5);
     *
     *   UIImage *testImage = [UIImage imageWithData:data];
     *
     *   [imageView setImage:image]
     *
     *
     */
     
     
    avpicture_free(&picture);
    av_free(YUVFrame);
    sws_freeContext(yuv2rgb_sws_context);
}


//only RGB24
-(UIImage *)imageFromAVPicture:(AVPicture )pict width:(int)width_ height:(int)height_ {
    CGBitmapInfo bitmapInfo =kCGBitmapByteOrderDefault;
    CFDataRef data =CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, pict.data[0], pict.linesize[0]*height_,kCFAllocatorNull);
    CGDataProviderRef provider =CGDataProviderCreateWithCFData(data);
    CGColorSpaceRef colorSpace =CGColorSpaceCreateDeviceRGB();
    int bitsPerComponent = 8;
    int bitsPerPixel = 3 * bitsPerComponent;
    int bytesPerRow =3 * width_;
    CGImageRef cgImage =CGImageCreate(width_,
                                      height_,
                                      bitsPerComponent,
                                      bitsPerPixel,
                                      bytesPerRow,//pict.linesize[0],等效
                                      colorSpace,
                                      bitmapInfo,
                                      provider,
                                      NULL,
                                      NO,
                                      kCGRenderingIntentDefault);
    CGColorSpaceRelease(colorSpace);
    UIImage *imageTemp = [UIImage imageWithCGImage:cgImage];
    CGImageRelease(cgImage);
    CGDataProviderRelease(provider);
    CFRelease(data);
    return imageTemp;
}


@end
