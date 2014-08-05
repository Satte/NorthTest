//
//  UserInfo.h
//  NorthTest
//
//  Created by ADMIRE on 14-8-4.
//  Copyright (c) 2014年 JIANGNAN. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>


@interface UserInfo : NSManagedObject

@property (nonatomic) int16_t language;
@property (nonatomic) int16_t loginstate;
@property (nonatomic) NSTimeInterval loginTime;
@property (nonatomic, retain) NSString * mailaddress;
@property (nonatomic, retain) NSString * nickname;
@property (nonatomic, retain) NSString * passwd;
@property (nonatomic, retain) NSString * userid;
@property (nonatomic, retain) NSString * username;
@property (nonatomic, retain) NSString * userpictureurl;

@end
