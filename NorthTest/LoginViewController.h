//
//  LoginViewController.h
//  NorthTest
//
//  Created by JIANGNAN on 13-9-6.
//  Copyright (c) 2013年 JIANGNAN. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface LoginViewController : UIViewController<UIGestureRecognizerDelegate>
@property (weak, nonatomic) IBOutlet UITextField *txtName;
@property (weak, nonatomic) IBOutlet UITextField *txtPassword;
@property (weak, nonatomic) IBOutlet UIActivityIndicatorView *checkAcivityIndicator;

- (IBAction)generalLoginClicked:(id)sender;

- (IBAction)anonymousLoginClicked:(id)sender;
-(void) dismisskeyboard;
@end
