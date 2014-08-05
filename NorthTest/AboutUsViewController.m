//
//  AboutUsViewController.m
//  NorthTest
//
//  Created by JIANGNAN on 13-9-6.
//  Copyright (c) 2013年 JIANGNAN. All rights reserved.
//

#import "AboutUsViewController.h"

@interface AboutUsViewController ()

@end

@implementation AboutUsViewController


- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}


- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view.

}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)returnkeyClicked:(UIBarButtonItem *)sender {
    [self dismissViewControllerAnimated:YES completion:^{
        NSLog(@"modal segue dismissed");
    }];
}
@end
