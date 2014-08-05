//
//  ipadLoginViewController.m
//  NorthTest
//
//  Created by ADMIRE on 14-7-30.
//  Copyright (c) 2014年 JIANGNAN. All rights reserved.
//

#import "ipadLoginViewController.h"

@implementation ipadLoginViewController
@synthesize userInfoDatabase=_userInfoDatabase;
@synthesize txtName=_txtName;
@synthesize txtPassword=_txtPassword;
@synthesize checkAcivityIndicator=_checkAcivityIndicator;


-(void)useDocument
{
    if(![[NSFileManager defaultManager]fileExistsAtPath:[self.userInfoDatabase.fileURL path]]){
        [self.userInfoDatabase saveToURL:self.userInfoDatabase.fileURL forSaveOperation:UIDocumentSaveForCreating completionHandler:^(BOOL success) {
            if(success) NSLog(@"create database file");
        }];
    }else if(self.userInfoDatabase.documentState ==UIDocumentStateClosed){
     [self.userInfoDatabase openWithCompletionHandler:^(BOOL success) {
         if(success) NSLog(@"open database");
     }];
        
    }else if(self.userInfoDatabase.documentState ==UIDocumentStateNormal){
        
    }
}

-(UIManagedDocument*)userInfoDatabase
{
    if(!_userInfoDatabase){
        NSURL *url=[[[NSFileManager defaultManager] URLsForDirectory:NSDocumentationDirectory inDomains:NSUserDomainMask ] lastObject];
        url=[url URLByAppendingPathComponent:@"defult user database"];
        _userInfoDatabase=[[UIManagedDocument alloc]initWithFileURL:url];
    }
    return _userInfoDatabase;
}

-(void)setUserInfoDatabase:(UIManagedDocument *)userInfoDatabase
{
    if(_userInfoDatabase!=userInfoDatabase){
        _userInfoDatabase=userInfoDatabase;
    }
}
- (instancetype)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
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
    
    //
    UITapGestureRecognizer *tapGesture=[[UITapGestureRecognizer alloc]initWithTarget:self action:@selector(dismisskeyboard)];
    [self.view addGestureRecognizer:tapGesture];
    tapGesture.delegate=self;
    
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
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
    UserInfo *user = (UserInfo *)[NSEntityDescription insertNewObjectForEntityForName:@"UserInfo" inManagedObjectContext:[self.userInfoDatabase managedObjectContext]];
    [user setUsername:self.txtName.text];
    [user setPasswd:self.txtPassword.text];
    int ret=[user requestforlogin];
    NSLog(@"ret=%d",ret);
    if(ret==0){
        [self performSegueWithIdentifier:@"mainView" sender:self];
    }else {
        NSString *message=(ret==-1?@"用户名或密码错误":@"网络连接故障");
        UIAlertView *alert=[[UIAlertView alloc] initWithTitle:@"输入错误" message:message delegate:nil cancelButtonTitle:@"确定" otherButtonTitles:nil, nil];
        [alert show];
    }
}
- (IBAction)anonymousLoginClicked:(id)sender {
    
    NSLog(@"匿名登陆成功");
    [self performSegueWithIdentifier:@"mainView" sender:self];
}

@end
