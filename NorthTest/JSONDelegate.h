//
//  JSONDelegate.h
//  NorthTest
//
//  Created by JIANGNAN on 13-8-30.
//  Copyright (c) 2013年 JIANGNAN. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "Reachability.h"

@interface JSONDelegate : NSObject

+(NSDictionary *)loadJSONDataFromURL:(NSString *) urlString;

+(BOOL)postJSONDataToURL:(NSString *)urlString JSONdata:(NSString*)JSONdata;

+(void)showError:(NSString *)errorMessage;
+(int)networkIsEnabled;

@end
