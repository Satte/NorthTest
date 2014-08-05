//
//  JSONHelper.m
//  NorthTest
//
//  Created by JIANGNAN on 13-8-30.
//  Copyright (c) 2013年 JIANGNAN. All rights reserved.
//

#import "JSONDelegate.h"

@implementation JSONDelegate

+(int)networkIsEnabled
{
    Reachability *CurReach = [Reachability reachabilityForInternetConnection];
    switch ([CurReach currentReachabilityStatus]) {
        case NotReachable://没有网络
            return 0;
        case ReachableViaWiFi://有wifi
            return 1;
        case ReachableViaWWAN://有3G
            return 2;
    }
}

+(NSDictionary *)loadJSONDataFromURL:(NSString *)urlString
{
    NSLog(@"request remote json-format data\n");

    NSError *error;
    NSURL * url=[NSURL URLWithString:urlString];
    NSMutableURLRequest *request=[NSMutableURLRequest requestWithURL:url];
    [request setHTTPMethod:@"GET"];
    [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
    
    NSData * data=[NSURLConnection sendSynchronousRequest:request returningResponse:nil error: &error];
    if(!data)
    {
        NSLog(@"Download Error: %@",error.localizedDescription);
        return nil;
    }
    id dictionary=[NSJSONSerialization JSONObjectWithData:data options:kNilOptions error: &error];
    if (dictionary == nil) {
        NSLog(@"JSON Error: %@",error);
        return nil;
    }
    NSLog(@"json data;%@",dictionary);
    return dictionary;
}

+(BOOL)postJSONDataToURL:(NSString *)urlString JSONdata:(NSString*)JSONdata
{
    // Attempt to send some data to a "POST" JSON Web Service, then parse the JSON results
    // which get returned, into a SQLResult record.
    //
    BOOL result = YES;
    
    NSURL *url = [NSURL URLWithString:urlString];
    
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    NSData *postData = [JSONdata dataUsingEncoding:NSASCIIStringEncoding allowLossyConversion:YES];
    NSString *postLength = [NSString stringWithFormat:@"%d", [postData length]];
    [request setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
    [request setHTTPMethod:@"POST"];
    [request setValue:postLength forHTTPHeaderField:@"Content-Length"];
    [request setHTTPBody:postData];
    
    NSError *error;
    NSData *data = [ NSURLConnection sendSynchronousRequest:request returningResponse: nil error:&error ];
    if (!data)
    {
        // An error occurred while calling the JSON web service.
        // Let's return a [SQLResult] record, to tell the user the web service couldn't be called.
        result=NO;
        return result;
    }
    
    
    // The JSON web call did complete, but perhaps it hit an error (such as a foreign key
    // constraint, if we've accidentally sent an unknown User_ID to the [Survey] INSERT command).
    //
    // The ResultString will return a "WasSuccessful" value, and an Exception value:
    //     { "Exception":"", "WasSuccesful":1 }
    // or this
    //     { "Exception:":"The INSERT statement conflicted with ...", "WasSuccesful":0 }"
    //
    
    NSString *resultString = [[NSString alloc] initWithBytes: [data bytes] length:[data length] encoding: NSUTF8StringEncoding];
    
    if (resultString == nil)
    {
        result= NO;
        NSLog(@"An exception occurred: %@", error.localizedDescription);
        return result;
    }
    
    // We need to convert our JSON string into a NSDictionary object
    NSData *data2 = [resultString dataUsingEncoding:NSUTF8StringEncoding];
    NSDictionary *dictionary = [NSJSONSerialization JSONObjectWithData:data2 options:kNilOptions error:&error];
    if (dictionary == nil)
    {
        result= NO;
        NSLog(@"Unable to parse the JSON return string.");
        return result;
    }
    
    result=YES;
    
    return result;
}

+(void)showError:(NSString *)errorMessage
{
    dispatch_async(dispatch_get_main_queue(), ^{
        UIAlertView *alertView = [[UIAlertView alloc]
                                  initWithTitle:@"Error"
                                  message:errorMessage
                                  delegate:nil
                                  cancelButtonTitle:@"OK"
                                  otherButtonTitles:nil];
        
        [alertView show];
    });
}

@end
