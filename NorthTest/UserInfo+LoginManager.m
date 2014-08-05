//
//  UserInfo+LoginManager.m
//  NorthTest
//
//  Created by ADMIRE on 14-8-4.
//  Copyright (c) 2014年 JIANGNAN. All rights reserved.
//

#import "UserInfo+LoginManager.h"
#import "JSONDelegate.h"

@implementation UserInfo (LoginManager)
/*
 * statecode=0:成功；
 * statecode=-1:失败-密码错误
 * statecode=-2:失败-网络错误
 */
-(int)requestforlogin
{
    NSLog(@"login check function called");
    int stateCode=1;
    if ([[JSONDelegate class] networkIsEnabled]>0) {
        if([self.username isEqualToString:@"admin"]||[self.passwd isEqualToString:@"123456"])
        {
            stateCode=0;
            NSDate *date = [NSDate dateWithTimeIntervalSinceReferenceDate:-30];
            self.loginTime = [date timeIntervalSinceReferenceDate];//登录时，记录下时间
        }else{
            stateCode=-1;
        }
    }else{
        stateCode=-2;
    }
    return stateCode;
}
-(void)requestforlogout
{
    //loginstate=: 0为已登录，-1为未登录
    if (self.loginstate ==0) {
        self.loginstate=-1;
    }
}
-(int)stateRequest
{
    NSDate *date = [NSDate dateWithTimeIntervalSinceReferenceDate:-30];
    NSTimeInterval currentTime=[date timeIntervalSinceReferenceDate];
    if(currentTime-self.loginTime>=300){
        self.loginstate=-2;
    }
    return self.loginstate;
}
@end
