//
//  CustomersTableViewController.h
//  NorthTest
//
//  Created by JIANGNAN on 13-8-30.
//  Copyright (c) 2013年 JIANGNAN. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface SongsTableViewController : UITableViewController
@property (strong, nonatomic) IBOutlet UITableView *tvCustomers;

@property (strong,nonatomic) NSMutableArray * listOfCustomers;


+(UIImage*)getImageFromURL:(NSString*)url;
@end
