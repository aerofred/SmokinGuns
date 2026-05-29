/*
===========================================================================
Écran de chargement iOS — visible pendant l’initialisation du moteur
===========================================================================
*/
#import <UIKit/UIKit.h>
#include "ios_loading.h"
#include "ios_layer.h"

@interface SGLoadingViewController : UIViewController
@property (nonatomic, strong) UILabel *statusLabel;
@property (nonatomic, strong) UIProgressView *progressBar;
@property (nonatomic, strong) UIActivityIndicatorView *spinner;
@end

@implementation SGLoadingViewController

- (void)viewDidLoad
{
	[super viewDidLoad];
	self.view.backgroundColor = [UIColor colorWithRed:0.08 green:0.08 blue:0.10 alpha:1.0];

	UILabel *title = [[UILabel alloc] initWithFrame:CGRectZero];
	title.text = @"Smokin' Guns";
	title.textColor = [UIColor colorWithWhite:0.95 alpha:1.0];
	title.font = [UIFont boldSystemFontOfSize:28];
	title.textAlignment = NSTextAlignmentCenter;
	title.translatesAutoresizingMaskIntoConstraints = NO;
	[self.view addSubview:title];

	_spinner = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleLarge];
	_spinner.color = [UIColor colorWithRed:1.0 green:0.75 blue:0.15 alpha:1.0];
	_spinner.translatesAutoresizingMaskIntoConstraints = NO;
	[_spinner startAnimating];
	[self.view addSubview:_spinner];

	_progressBar = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault];
	_progressBar.progressTintColor = [UIColor colorWithRed:1.0 green:0.75 blue:0.15 alpha:1.0];
	_progressBar.trackTintColor = [UIColor colorWithWhite:0.25 alpha:1.0];
	_progressBar.progress = 0.0f;
	_progressBar.translatesAutoresizingMaskIntoConstraints = NO;
	[self.view addSubview:_progressBar];

	_statusLabel = [[UILabel alloc] initWithFrame:CGRectZero];
	_statusLabel.text = @"Démarrage…";
	_statusLabel.textColor = [UIColor colorWithWhite:0.75 alpha:1.0];
	_statusLabel.font = [UIFont systemFontOfSize:15];
	_statusLabel.textAlignment = NSTextAlignmentCenter;
	_statusLabel.numberOfLines = 2;
	_statusLabel.translatesAutoresizingMaskIntoConstraints = NO;
	[self.view addSubview:_statusLabel];

	UILayoutGuide *g = self.view.safeAreaLayoutGuide;
	[NSLayoutConstraint activateConstraints:@[
		[title.centerXAnchor constraintEqualToAnchor:g.centerXAnchor],
		[title.centerYAnchor constraintEqualToAnchor:g.centerYAnchor constant:-60],
		[title.leadingAnchor constraintGreaterThanOrEqualToAnchor:g.leadingAnchor constant:24],
		[title.trailingAnchor constraintLessThanOrEqualToAnchor:g.trailingAnchor constant:-24],

		[_spinner.centerXAnchor constraintEqualToAnchor:g.centerXAnchor],
		[_spinner.topAnchor constraintEqualToAnchor:title.bottomAnchor constant:28],

		[_progressBar.leadingAnchor constraintEqualToAnchor:g.leadingAnchor constant:48],
		[_progressBar.trailingAnchor constraintEqualToAnchor:g.trailingAnchor constant:-48],
		[_progressBar.topAnchor constraintEqualToAnchor:_spinner.bottomAnchor constant:32],

		[_statusLabel.leadingAnchor constraintEqualToAnchor:g.leadingAnchor constant:24],
		[_statusLabel.trailingAnchor constraintEqualToAnchor:g.trailingAnchor constant:-24],
		[_statusLabel.topAnchor constraintEqualToAnchor:_progressBar.bottomAnchor constant:16],
	]];
}

@end

static UIWindow *gLoadWindow = nil;
static SGLoadingViewController *gLoadVC = nil;
static float gLastProgress = 0.0f;

static void IOS_Loading_OnMain( void (^block)( void ) )
{
	if ( [NSThread isMainThread] )
		block();
	else
		dispatch_sync( dispatch_get_main_queue(), block );
}

static void IOS_Loading_Apply( float progress, const char *status )
{
	if ( !gLoadVC )
		return;

	if ( progress >= 0.0f )
	{
		if ( progress < gLastProgress )
			progress = gLastProgress;
		else
			gLastProgress = progress;
		gLoadVC.progressBar.progress = progress;
	}

	if ( status && status[0] )
		gLoadVC.statusLabel.text = [NSString stringWithUTF8String:status];
}

void IOS_Loading_PumpUI( void )
{
	SInt32 result;
	do {
		result = CFRunLoopRunInMode( kCFRunLoopDefaultMode, 0.0, TRUE );
	} while ( result == kCFRunLoopRunHandledSource );
}

void IOS_Loading_Begin( void )
{
	if ( gLoadWindow )
		return;

	IOS_Loading_OnMain( ^{
		UIWindowScene *scene = nil;
		for ( UIScene *s in UIApplication.sharedApplication.connectedScenes )
		{
			if ( [s isKindOfClass:[UIWindowScene class]] && s.activationState == UISceneActivationStateForegroundActive )
			{
				scene = (UIWindowScene *)s;
				break;
			}
		}
		if ( !scene )
		{
			for ( UIScene *s in UIApplication.sharedApplication.connectedScenes )
			{
				if ( [s isKindOfClass:[UIWindowScene class]] )
				{
					scene = (UIWindowScene *)s;
					break;
				}
			}
		}

		if ( scene )
			gLoadWindow = [[UIWindow alloc] initWithWindowScene:scene];
		else
			gLoadWindow = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];

		gLoadWindow.windowLevel = UIWindowLevelAlert + 1.0;
		gLoadVC = [[SGLoadingViewController alloc] init];
		gLoadWindow.rootViewController = gLoadVC;
		[gLoadWindow makeKeyAndVisible];
		IOS_Loading_Apply( 0.02f, "Démarrage…" );
	} );

	IOS_Loading_PumpUI();
}

void IOS_Loading_SetProgress( float progress, const char *status )
{
	if ( !gLoadWindow )
		return;

	IOS_Loading_OnMain( ^{
		IOS_Loading_Apply( progress, status );
	} );
	IOS_Loading_PumpUI();
}

void IOS_Loading_End( void )
{
	if ( !gLoadWindow )
		return;

	IOS_Loading_OnMain( ^{
		IOS_Loading_Apply( 1.0f, "Prêt" );
		[UIView animateWithDuration:0.25 animations:^{
			gLoadWindow.alpha = 0.0;
		} completion:^(BOOL finished) {
			(void)finished;
			gLoadWindow.hidden = YES;
			gLoadWindow.rootViewController = nil;
			gLoadWindow = nil;
			gLoadVC = nil;
			gLastProgress = 0.0f;
			IOS_Layer_AttachToWindow();
		}];
	} );
	IOS_Loading_PumpUI();
}
