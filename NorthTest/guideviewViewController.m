//
//  guideviewViewController.m
//  NorthTest
//
//  Created by ADMIRE on 14-7-27.
//  Copyright (c) 2014年 JIANGNAN. All rights reserved.
//

#import "guideviewViewController.h"
#import "LoginViewController.h"


@interface guideviewViewController ()


@end

@implementation guideviewViewController


@synthesize acitivityindcator=_acitivityindcator;

- (instancetype)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}
-(void)animate{
    [self performSegueWithIdentifier:@"loginView" sender:self];
    [self.acitivityindcator stopAnimating];
    
}


-(void)viewDidDisappear:(BOOL)animated
{
    [super viewDidDisappear:animated];
    [self.acitivityindcator stopAnimating];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self performSelector:@selector(animate) withObject:nil afterDelay:4];
    [self.acitivityindcator startAnimating];
    
}


@end
