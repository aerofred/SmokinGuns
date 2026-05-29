/*
===========================================================================
Couche UIKit iOS — overlay de jeu + pont vers cl_touch.c (moteur)
Fenêtre UIKit dédiée au-dessus de SDL (comme Quake3-iOS).
===========================================================================
*/
#import <UIKit/UIKit.h>
#import <SDL.h>
#import <SDL_syswm.h>

#include "../client/cl_touch.h"
#include "ios_layer.h"

void IOS_TouchSettings_Present( UIViewController *host );

@interface SGStickView : UIView
@property (nonatomic, assign) int stickId;
@property (nonatomic, assign) CGPoint knobOffset;
@end

@implementation SGStickView

- (instancetype)initWithFrame:(CGRect)frame stickId:(int)sid
{
	self = [super initWithFrame:frame];
	if ( self )
	{
		_stickId = sid;
		self.backgroundColor = [UIColor clearColor];
		self.multipleTouchEnabled = NO;
		self.opaque = NO;
	}
	return self;
}

- (void)drawRect:(CGRect)rect
{
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	CGFloat r = CGRectGetWidth( self.bounds ) * 0.5f;
	CGPoint c = CGPointMake( CGRectGetMidX( self.bounds ), CGRectGetMidY( self.bounds ) );

	[[[UIColor whiteColor] colorWithAlphaComponent:0.30f] setFill];
	CGContextAddArc( ctx, c.x, c.y, r, 0, (CGFloat)( 2.0 * M_PI ), 0 );
	CGContextFillPath( ctx );

	[[[UIColor whiteColor] colorWithAlphaComponent:0.65f] setStroke];
	CGContextSetLineWidth( ctx, 2.0f );
	CGContextAddArc( ctx, c.x, c.y, r, 0, (CGFloat)( 2.0 * M_PI ), 0 );
	CGContextStrokePath( ctx );

	CGPoint k = CGPointMake( c.x + _knobOffset.x, c.y + _knobOffset.y );
	[[[UIColor whiteColor] colorWithAlphaComponent:0.80f] setFill];
	CGContextAddArc( ctx, k.x, k.y, r * 0.38f, 0, (CGFloat)( 2.0 * M_PI ), 0 );
	CGContextFillPath( ctx );
}

- (void)reportStick:(CGPoint)norm active:(BOOL)active
{
	if ( _stickId == 0 )
		IN_TouchSetMoveStick( norm.x, norm.y, active ? qtrue : qfalse );
	else
		IN_TouchSetLookStick( norm.x, norm.y, active ? qtrue : qfalse );
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	[self handleTouches:touches ended:NO];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	[self handleTouches:touches ended:NO];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	[self handleTouches:touches ended:YES];
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	[self handleTouches:touches ended:YES];
}

- (void)handleTouches:(NSSet<UITouch *> *)touches ended:(BOOL)ended
{
	UITouch *t = touches.anyObject;
	CGPoint p = [t locationInView:self];
	CGFloat r = CGRectGetWidth( self.bounds ) * 0.5f;
	CGPoint c = CGPointMake( r, r );
	CGPoint d = CGPointMake( p.x - c.x, p.y - c.y );
	CGFloat len = (CGFloat)sqrt( d.x * d.x + d.y * d.y );

	if ( ended )
	{
		_knobOffset = CGPointZero;
		[self setNeedsDisplay];
		[self reportStick:CGPointZero active:NO];
		return;
	}

	if ( len > r * 0.85f && len > 0.01f )
	{
		d.x = d.x / len * r * 0.85f;
		d.y = d.y / len * r * 0.85f;
		len = r * 0.85f;
	}
	_knobOffset = d;
	[self setNeedsDisplay];

	CGPoint norm = CGPointZero;
	if ( len > 0.01f )
	{
		norm.x = d.x / ( r * 0.85f );
		norm.y = d.y / ( r * 0.85f );
	}
	[self reportStick:norm active:YES];
}

@end

@interface SGButtonView : UIView
@property (nonatomic, copy) NSString *title;
@property (nonatomic, assign) int buttonId;
@property (nonatomic, assign) BOOL pressed;
@end

@implementation SGButtonView

- (instancetype)initWithFrame:(CGRect)frame title:(NSString *)title buttonId:(int)bid
{
	self = [super initWithFrame:frame];
	if ( self )
	{
		_title = [title copy];
		_buttonId = bid;
		self.backgroundColor = [UIColor clearColor];
		self.opaque = NO;
	}
	return self;
}

- (void)drawRect:(CGRect)rect
{
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	CGRect b = CGRectInset( self.bounds, 2, 2 );
	UIColor *fill = _pressed
		? [UIColor colorWithRed:1.0 green:0.75 blue:0.1 alpha:0.9]
		: [UIColor colorWithRed:0.15 green:0.15 blue:0.15 alpha:0.65];
	[fill setFill];
	CGContextFillEllipseInRect( ctx, b );
	[[[UIColor whiteColor] colorWithAlphaComponent:0.5] setStroke];
	CGContextSetLineWidth( ctx, 2.0f );
	CGContextStrokeEllipseInRect( ctx, b );

	NSDictionary *attrs = @{
		NSFontAttributeName: [UIFont boldSystemFontOfSize:14],
		NSForegroundColorAttributeName: [UIColor whiteColor]
	};
	CGSize ts = [_title sizeWithAttributes:attrs];
	CGPoint tp = CGPointMake(
		CGRectGetMidX( b ) - ts.width * 0.5f,
		CGRectGetMidY( b ) - ts.height * 0.5f );
	[_title drawAtPoint:tp withAttributes:attrs];
}

- (void)setPressed:(BOOL)pressed
{
	if ( _pressed == pressed )
		return;
	_pressed = pressed;
	[self setNeedsDisplay];
	if ( _buttonId == 0 )
		IN_TouchSetFire( pressed ? qtrue : qfalse );
	else
		IN_TouchSetJump( pressed ? qtrue : qfalse );
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	(void)touches;
	(void)event;
	self.pressed = YES;
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	(void)touches;
	(void)event;
	self.pressed = NO;
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	(void)touches;
	(void)event;
	self.pressed = NO;
}

@end

@interface SGSettingsButton : UIView
@end

@implementation SGSettingsButton

- (instancetype)initWithFrame:(CGRect)frame
{
	self = [super initWithFrame:frame];
	if ( self )
	{
		self.backgroundColor = [UIColor clearColor];
		self.opaque = NO;
	}
	return self;
}

- (void)drawRect:(CGRect)rect
{
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	CGRect b = CGRectInset( self.bounds, 2, 2 );
	[[[UIColor whiteColor] colorWithAlphaComponent:0.25f] setFill];
	CGContextFillEllipseInRect( ctx, b );
	[[[UIColor whiteColor] colorWithAlphaComponent:0.6f] setStroke];
	CGContextSetLineWidth( ctx, 1.5f );
	CGContextStrokeEllipseInRect( ctx, b );
	[@"CFG" drawAtPoint:CGPointMake( 10, 8 )
		withAttributes:@{
			NSFontAttributeName: [UIFont boldSystemFontOfSize:11],
			NSForegroundColorAttributeName: [UIColor whiteColor]
		}];
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	(void)touches;
	(void)event;
	IOS_Layer_OpenTouchSettings();
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	(void)touches;
	(void)event;
}

@end

/* Souris virtuelle style RDP pour les menus Quake (UIKit → moteur) */
@interface SGMenuTouchView : UIView
@end

@implementation SGMenuTouchView

- (void)forwardTouches:(NSSet<UITouch *> *)touches move:(BOOL)move ended:(BOOL)ended
{
	UITouch *t = touches.anyObject;
	CGPoint p;

	if ( !t )
		return;

	p = [t locationInView:self];
	IN_TouchPointer( p.x, p.y, ended ? qfalse : qtrue, move ? qtrue : qfalse );
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	(void)event;
	[self forwardTouches:touches move:NO ended:NO];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	(void)event;
	[self forwardTouches:touches move:YES ended:NO];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	(void)event;
	[self forwardTouches:touches move:NO ended:YES];
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	(void)event;
	[self forwardTouches:touches move:NO ended:YES];
}

@end

@interface SGGameOverlay : UIView
@property (nonatomic, strong) SGStickView *moveStick;
@property (nonatomic, strong) SGStickView *lookStick;
@property (nonatomic, strong) SGButtonView *fireBtn;
@property (nonatomic, strong) SGButtonView *jumpBtn;
@property (nonatomic, strong) SGSettingsButton *settingsBtn;
@end

@implementation SGGameOverlay

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event
{
	UIView *hit = [super hitTest:point withEvent:event];
	if ( hit == self )
		return nil;
	return hit;
}

- (void)layoutFromOverlay:(const touchOverlay_t *)o
{
	if ( !o || !o->visible )
	{
		self.hidden = YES;
		return;
	}
	self.hidden = NO;

	CGFloat d = o->moveR * 2.0f;
	_moveStick.frame = CGRectMake( o->moveCx - o->moveR, o->moveCy - o->moveR, d, d );
	_moveStick.knobOffset = CGPointMake( o->moveKnobX - o->moveCx, o->moveKnobY - o->moveCy );
	[_moveStick setNeedsDisplay];

	d = o->lookR * 2.0f;
	_lookStick.frame = CGRectMake( o->lookCx - o->lookR, o->lookCy - o->lookR, d, d );
	_lookStick.knobOffset = CGPointMake( o->lookKnobX - o->lookCx, o->lookKnobY - o->lookCy );
	[_lookStick setNeedsDisplay];

	_fireBtn.frame = CGRectMake( o->fireX, o->fireY, o->fireW, o->fireH );
	_jumpBtn.frame = CGRectMake( o->jumpX, o->jumpY, o->jumpW, o->jumpH );
	_settingsBtn.frame = CGRectMake( 8, 8, 44, 44 );
}

@end

static UIWindow *gSdlWindow = nil;
static UIWindow *gOverlayWindow = nil;
static UIViewController *gOverlayRoot = nil;
static SGGameOverlay *gOverlay = nil;
static SGMenuTouchView *gMenuTouch = nil;
static qboolean gForceHideOverlay = qfalse;

static UIWindowScene *IOS_ActiveWindowScene( void )
{
	for ( UIScene *scene in UIApplication.sharedApplication.connectedScenes )
	{
		if ( [scene isKindOfClass:[UIWindowScene class]] &&
			scene.activationState == UISceneActivationStateForegroundActive )
		{
			return (UIWindowScene *)scene;
		}
	}
	for ( UIScene *scene in UIApplication.sharedApplication.connectedScenes )
	{
		if ( [scene isKindOfClass:[UIWindowScene class]] )
			return (UIWindowScene *)scene;
	}
	return nil;
}

static void IOS_Layer_CreateOverlayIfNeeded( void )
{
	UIWindow *window;
	UIWindowScene *scene;
	CGFloat baseLevel;

	if ( gOverlayWindow || !gOverlay )
		return;

	scene = gSdlWindow ? gSdlWindow.windowScene : IOS_ActiveWindowScene();
	if ( scene )
		window = [[UIWindow alloc] initWithWindowScene:scene];
	else
		window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];

	window.backgroundColor = [UIColor clearColor];
	window.opaque = NO;
	window.userInteractionEnabled = YES;

	baseLevel = gSdlWindow ? (CGFloat)gSdlWindow.windowLevel : (CGFloat)UIWindowLevelNormal;
	window.windowLevel = (UIWindowLevel)( MAX( (CGFloat)UIWindowLevelAlert, baseLevel ) + 1.0 );

	gOverlayRoot = [[UIViewController alloc] init];
	gOverlayRoot.view.backgroundColor = [UIColor clearColor];
	gOverlayRoot.view.userInteractionEnabled = YES;
	window.rootViewController = gOverlayRoot;

	gOverlay.frame = gOverlayRoot.view.bounds;
	gOverlay.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	[gOverlayRoot.view addSubview:gOverlay];

	gOverlayWindow = window;
}

static void IOS_Layer_SyncOverlayFrame( void )
{
	CGRect bounds;

	if ( !gOverlayWindow || !gOverlay )
		return;

	if ( gSdlWindow )
		bounds = gSdlWindow.bounds;
	else
		bounds = UIScreen.mainScreen.bounds;

	gOverlayWindow.frame = bounds;
	gOverlay.frame = gOverlayRoot.view.bounds;
}

static void IOS_Layer_RestoreSdlKeyWindow( void )
{
	if ( gSdlWindow && !gSdlWindow.hidden )
		[gSdlWindow makeKeyWindow];
}

static void IOS_Layer_SetMenuTouchActive( qboolean active )
{
	UIView *host;

	if ( !gSdlWindow )
		return;

	host = gSdlWindow.rootViewController ? gSdlWindow.rootViewController.view : gSdlWindow;

	if ( active )
	{
		if ( !gMenuTouch )
		{
			gMenuTouch = [[SGMenuTouchView alloc] initWithFrame:host.bounds];
			gMenuTouch.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
			gMenuTouch.backgroundColor = [UIColor clearColor];
			gMenuTouch.opaque = NO;
			gMenuTouch.multipleTouchEnabled = NO;
		}
		if ( gMenuTouch.superview != host )
			[host addSubview:gMenuTouch];
		gMenuTouch.frame = host.bounds;
		gMenuTouch.hidden = NO;
	}
	else if ( gMenuTouch )
	{
		gMenuTouch.hidden = YES;
		[gMenuTouch removeFromSuperview];
	}
}

static void IOS_Layer_HideOverlayWindow( void )
{
	if ( gOverlayWindow )
	{
		gOverlayWindow.hidden = YES;
		gOverlayWindow.userInteractionEnabled = NO;
	}
	if ( gOverlay )
		gOverlay.hidden = YES;
	IOS_Layer_RestoreSdlKeyWindow();
}

static void IOS_Layer_ShowOverlayWindow( void )
{
	IOS_Layer_CreateOverlayIfNeeded();
	IOS_Layer_SyncOverlayFrame();

	if ( !gOverlayWindow )
		return;

	if ( gForceHideOverlay )
	{
		IOS_Layer_HideOverlayWindow();
		return;
	}

	gOverlayWindow.hidden = NO;
	gOverlayWindow.userInteractionEnabled = YES;
	[gOverlayWindow makeKeyAndVisible];
}

void IOS_Layer_SetGameOverlayVisible( qboolean visible )
{
	gForceHideOverlay = visible ? qfalse : qtrue;
	if ( gOverlayWindow )
		gOverlayWindow.hidden = gForceHideOverlay ? YES : NO;
}

void IOS_Layer_OpenTouchSettings( void )
{
	UIViewController *host = gOverlayRoot ? gOverlayRoot : gSdlWindow.rootViewController;
	if ( !host )
		return;

	dispatch_async( dispatch_get_main_queue(), ^{
		IOS_Layer_SetGameOverlayVisible( qfalse );
		IOS_TouchSettings_Present( host );
	} );
}

void IOS_Layer_AttachToWindow( void )
{
	dispatch_async( dispatch_get_main_queue(), ^{
		/* Laisser SDL recevoir les touches (menus) ; IOS_Layer_Tick affiche l’overlay en jeu */
		IOS_Layer_RestoreSdlKeyWindow();
	} );
}

void IOS_Layer_Init( SDL_Window *window )
{
	SDL_SysWMinfo wmInfo;

	if ( gOverlay )
		return;

	if ( window )
	{
		SDL_VERSION( &wmInfo.version );
		if ( SDL_GetWindowWMInfo( window, &wmInfo ) )
			gSdlWindow = wmInfo.info.uikit.window;
	}

	gOverlay = [[SGGameOverlay alloc] initWithFrame:CGRectZero];
	gOverlay.backgroundColor = [UIColor clearColor];
	gOverlay.userInteractionEnabled = YES;
	gOverlay.hidden = YES;

	gOverlay.moveStick = [[SGStickView alloc] initWithFrame:CGRectZero stickId:0];
	gOverlay.lookStick = [[SGStickView alloc] initWithFrame:CGRectZero stickId:1];
	gOverlay.fireBtn = [[SGButtonView alloc] initWithFrame:CGRectZero title:@"FIRE" buttonId:0];
	gOverlay.jumpBtn = [[SGButtonView alloc] initWithFrame:CGRectZero title:@"JUMP" buttonId:1];
	gOverlay.settingsBtn = [[SGSettingsButton alloc] initWithFrame:CGRectMake( 8, 8, 44, 44 )];

	[gOverlay addSubview:gOverlay.moveStick];
	[gOverlay addSubview:gOverlay.lookStick];
	[gOverlay addSubview:gOverlay.fireBtn];
	[gOverlay addSubview:gOverlay.jumpBtn];
	[gOverlay addSubview:gOverlay.settingsBtn];
}

void IOS_Layer_Shutdown( void )
{
	IOS_Layer_SetMenuTouchActive( qfalse );
	gMenuTouch = nil;
	if ( gOverlayWindow )
	{
		gOverlayWindow.hidden = YES;
		gOverlayWindow.rootViewController = nil;
		gOverlayWindow = nil;
	}
	gOverlayRoot = nil;
	gOverlay = nil;
	gSdlWindow = nil;
	gForceHideOverlay = qfalse;
}

void IOS_Layer_Tick( void )
{
	touchOverlay_t o;
	qboolean uiMode;

	if ( !gOverlay )
		return;

	uiMode = IN_TouchInUIMode();
	IN_TouchGetOverlay( &o );

	/* Menus Quake : souris virtuelle au doigt, pas d’overlay jeu */
	if ( gForceHideOverlay || !o.visible )
	{
		IOS_Layer_HideOverlayWindow();
		IOS_Layer_SetMenuTouchActive( uiMode && !gForceHideOverlay ? qtrue : qfalse );
		return;
	}

	IOS_Layer_SetMenuTouchActive( qfalse );
	IOS_Layer_ShowOverlayWindow();
	[gOverlay layoutFromOverlay:&o];
}
