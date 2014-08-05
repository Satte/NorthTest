//
//  LoginViewController.m
//  NorthTest
//
//  Created by JIANGNAN on 13-9-6.
//  Copyright (c) 2013年 JIANGNAN. All rights reserved.
//

#import "LoginViewController.h"
#import "AboutUsViewController.h"


@interface LoginViewController ()

@end

@implementation LoginViewController

@synthesize txtName=_txtName;
@synthesize txtPassword=_txtPassword;
@synthesize checkAcivityIndicator=_checkAcivityIndicator;

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
    
    UITapGestureRecognizer *tapGesture=[[UITapGestureRecognizer alloc]initWithTarget:self action:@selector(dismisskeyboard)];
    [self.view addGestureRecognizer:tapGesture];
    tapGesture.delegate=self;
}
-(void)viewDidDisappear:(BOOL)animated
{
     [self.checkAcivityIndicator stopAnimating];
}

-(void) dismisskeyboard
{
    [self.view endEditing:YES];
    [self.txtPassword resignFirstResponder];
    [self.txtName resignFirstResponder];
}


- (IBAction)generalLoginClicked:(id)sender {
   
   
    NSLog(@"button clicked");
    NSString *username=self.txtName.text;
    NSString * password=self.txtPassword.text;
    NSLog(@"%@ %@",username,password);
    if([username isEqualToString:@"admin"] &&[password isEqualToString:@"123456"]){
        NSLog(@"login sucessfully");
        [self performSegueWithIdentifier:@"mainView" sender:self];
    }else{
        UIAlertView *alert=[[UIAlertView alloc] initWithTitle:@"输入错误" message:@"用户名或密码错误" delegate:nil cancelButtonTitle:@"确定" otherButtonTitles:nil, nil];
        [alert show];
        
    }
}
- (IBAction)anonymousLoginClicked:(id)sender {

    NSLog(@"匿名登陆成功");
    [self performSegueWithIdentifier:@"mainView" sender:self];
}




@end
