//
//  UserInfo+LoginManager.h
//  NorthTest
//
//  Created by ADMIRE on 14-8-4.
//  Copyright (c) 2014年 JIANGNAN. All rights reserved.
//

#import "UserInfo.h"


@interface UserInfo (LoginManager)

-(int)requestforlogin;
-(void)requestforlogout;
-(int)stateRequest;
@end
