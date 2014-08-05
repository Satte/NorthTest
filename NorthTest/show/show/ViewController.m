//
//  ViewController.m
//  show
//
//  Created by ADMIRE_2 on 13-6-1.
//  Copyright (c) 2013年 ADMIRE_2. All rights reserved.
//

#import "ViewController.h"
#include "RecvManager.h"
#include "renderIOS.h"


@interface ViewController ()

@end



@implementation ViewController 


@synthesize imageView;



- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
   
   // RecvManager *a = new RecvManager();
    //a->test();
    
    RenderIOS *r = [[RenderIOS alloc]init];
    [NSThread detachNewThreadSelector:@selector(test) toTarget:r withObject:nil];
    sleep(20);
     //  [c start];
   // dispatch_async(dispatch_get_main_queue(),^{[c start];});
  //  [NSThread detachNewThreadSelector:@selector(fun) toTarget:self withObject:nil];
  //  [self fun];
}


-(void) fun{
    for(int i = 0 ;i< 100 ;++ i)
        printf("fun1 loop\n");
}
- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}



@end
