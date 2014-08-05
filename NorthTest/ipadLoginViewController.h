//
//  ipadLoginViewController.h
//  NorthTest
//
//  Created by ADMIRE on 14-7-30.
//  Copyright (c) 2014年 JIANGNAN. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "UserInfo+LoginManager.h"

@interface ipadLoginViewController : UIViewController<UIGestureRecognizerDelegate,NSFetchedResultsControllerDelegate>
@property(nonatomic,strong) UIManagedDocument* userInfoDatabase;
@property (weak, nonatomic) IBOutlet UITextField *txtName;
@property (weak, nonatomic) IBOutlet UITextField *txtPassword;
@property (weak, nonatomic) IBOutlet UIActivityIndicatorView *checkAcivityIndicator;

- (IBAction)generalLoginClicked:(id)sender;

- (IBAction)anonymousLoginClicked:(id)sender;
-(void) dismisskeyboard;
@end
