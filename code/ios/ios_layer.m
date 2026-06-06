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

@class SGGameOverlay;
@class SGMenuTouchView;

static UIWindow *gSdlWindow = nil;
static UIWindow *gOverlayWindow = nil;
static UIViewController *gOverlayRoot = nil;
static SGGameOverlay *gOverlay = nil;
static SGMenuTouchView *gMenuTouch = nil;
static qboolean gForceHideOverlay = qfalse;
static int gLayoutWidth = 0;
static int gLayoutHeight = 0;

static UIWindowScene *IOS_ActiveWindowScene( void );
static UIWindowScene *IOS_Layer_ActiveScene( void );
static CGRect IOS_Layer_ScreenBounds( void );
static void IOS_Layer_SyncOverlayFrame( void );

static void IOS_Layer_ShowKeyboard( void )
{
	IN_TouchToggleConsole();
	SDL_StartTextInput();
	if ( gSdlWindow )
		[gSdlWindow makeKeyAndVisible];
	if ( gOverlayWindow )
	{
		gOverlayWindow.hidden = YES;
		gOverlayWindow.userInteractionEnabled = NO;
	}
}

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
@property (nonatomic, assign) touchButton_t buttonId;
@property (nonatomic, assign) BOOL pressed;
@end

@implementation SGButtonView

- (instancetype)initWithFrame:(CGRect)frame title:(NSString *)title buttonId:(touchButton_t)bid
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
	IN_TouchButton( _buttonId, pressed ? qtrue : qfalse );
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

@interface SGKeyboardButton : UIView
@end

@implementation SGKeyboardButton

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
	[@"KB" drawAtPoint:CGPointMake( 11, 13 )
		withAttributes:@{
			NSFontAttributeName: [UIFont boldSystemFontOfSize:12],
			NSForegroundColorAttributeName: [UIColor whiteColor]
		}];
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	(void)touches;
	(void)event;
	IOS_Layer_ShowKeyboard();
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	(void)touches;
	(void)event;
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
@property (nonatomic, strong) SGButtonView *aimBtn;
@property (nonatomic, strong) SGButtonView *runBtn;
@property (nonatomic, strong) SGButtonView *w1Btn;
@property (nonatomic, strong) SGButtonView *w2Btn;
@property (nonatomic, strong) SGButtonView *w3Btn;
@property (nonatomic, strong) SGButtonView *w4Btn;
@property (nonatomic, strong) SGButtonView *dropBtn;
@property (nonatomic, strong) SGButtonView *reloadBtn;
@property (nonatomic, strong) SGButtonView *useBtn;
@property (nonatomic, strong) SGButtonView *buyBtn;
@property (nonatomic, strong) SGButtonView *eBtn;
@property (nonatomic, strong) SGButtonView *escBtn;
@property (nonatomic, strong) SGSettingsButton *settingsBtn;
@property (nonatomic, strong) SGKeyboardButton *keyboardBtn;
@end

@implementation SGGameOverlay

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event
{
	UIView *hit = [super hitTest:point withEvent:event];
	if ( hit == self )
		return nil;
	return hit;
}

- (void)layoutButton:(SGButtonView *)btn frame:(CGRect)frame pressed:(qboolean)held syncHeld:(BOOL)syncHeld
{
	if ( !btn )
		return;
	btn.hidden = NO;
	btn.frame = frame;
	if ( syncHeld )
		btn.pressed = held ? YES : NO;
	[btn setNeedsDisplay];
}

- (void)layoutFromOverlay:(const touchOverlay_t *)ignored
{
	touchOverlay_t layout;
	const touchOverlay_t *o;
	CGRect screen;
	int bw, bh;

	(void)ignored;

	screen = IOS_Layer_ScreenBounds();
	bw = (int)screen.size.width;
	bh = (int)screen.size.height;
	if ( bw < 1 )
		bw = 1;
	if ( bh < 1 )
		bh = 1;

	self.frame = CGRectMake( 0.0f, 0.0f, (CGFloat)bw, (CGFloat)bh );
	gLayoutWidth = bw;
	gLayoutHeight = bh;

	IN_TouchGetOverlayForSize( &layout, bw, bh );
	if ( !layout.visible )
	{
		self.hidden = YES;
		return;
	}
	o = &layout;

	self.hidden = NO;

	CGFloat d = o->moveR * 2.0f;
	_moveStick.hidden = NO;
	_moveStick.frame = CGRectMake( o->moveCx - o->moveR, o->moveCy - o->moveR, d, d );
	_moveStick.knobOffset = CGPointMake( o->moveKnobX - o->moveCx, o->moveKnobY - o->moveCy );
	[_moveStick setNeedsDisplay];

	d = o->lookR * 2.0f;
	_lookStick.hidden = NO;
	_lookStick.frame = CGRectMake( o->lookCx - o->lookR, o->lookCy - o->lookR, d, d );
	_lookStick.knobOffset = CGPointMake( o->lookKnobX - o->lookCx, o->lookKnobY - o->lookCy );
	[_lookStick setNeedsDisplay];

	_fireBtn.hidden = NO;
	_fireBtn.frame = CGRectMake( o->fireX, o->fireY, o->fireW, o->fireH );
	_fireBtn.pressed = o->fireHeld ? YES : NO;
	[_fireBtn setNeedsDisplay];

	_jumpBtn.hidden = NO;
	_jumpBtn.frame = CGRectMake( o->jumpX, o->jumpY, o->jumpW, o->jumpH );
	_jumpBtn.pressed = o->jumpHeld ? YES : NO;
	[_jumpBtn setNeedsDisplay];

	[self layoutButton:_aimBtn frame:CGRectMake( o->aimX, o->aimY, o->aimW, o->aimH ) pressed:o->aimHeld syncHeld:YES];
	[self layoutButton:_runBtn frame:CGRectMake( o->runX, o->runY, o->runW, o->runH ) pressed:o->runHeld syncHeld:YES];
	[self layoutButton:_w1Btn frame:CGRectMake( o->w1X, o->w1Y, o->w1W, o->w1H ) pressed:qfalse syncHeld:NO];
	[self layoutButton:_w2Btn frame:CGRectMake( o->w2X, o->w2Y, o->w2W, o->w2H ) pressed:qfalse syncHeld:NO];
	[self layoutButton:_w3Btn frame:CGRectMake( o->w3X, o->w3Y, o->w3W, o->w3H ) pressed:qfalse syncHeld:NO];
	[self layoutButton:_w4Btn frame:CGRectMake( o->w4X, o->w4Y, o->w4W, o->w4H ) pressed:qfalse syncHeld:NO];
	[self layoutButton:_dropBtn frame:CGRectMake( o->dropX, o->dropY, o->dropW, o->dropH ) pressed:qfalse syncHeld:NO];
	[self layoutButton:_reloadBtn frame:CGRectMake( o->reloadX, o->reloadY, o->reloadW, o->reloadH ) pressed:qfalse syncHeld:NO];
	[self layoutButton:_useBtn frame:CGRectMake( o->useX, o->useY, o->useW, o->useH ) pressed:o->useHeld syncHeld:YES];
	[self layoutButton:_buyBtn frame:CGRectMake( o->buyX, o->buyY, o->buyW, o->buyH ) pressed:qfalse syncHeld:NO];
	[self layoutButton:_eBtn frame:CGRectMake( o->eX, o->eY, o->eW, o->eH ) pressed:o->eHeld syncHeld:YES];
	[self layoutButton:_escBtn frame:CGRectMake( o->escX, o->escY, o->escW, o->escH ) pressed:qfalse syncHeld:NO];

	_settingsBtn.hidden = NO;
	_settingsBtn.frame = CGRectMake( o->cfgX, o->cfgY, o->cfgW, o->cfgH );
	[_settingsBtn setNeedsDisplay];

	_keyboardBtn.hidden = NO;
	_keyboardBtn.frame = CGRectMake( o->kbdX, o->kbdY, o->kbdW, o->kbdH );
	[_keyboardBtn setNeedsDisplay];
}

@end

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
	if ( @available( iOS 11.0, * ) )
		gOverlayRoot.view.insetsLayoutMarginsFromSafeArea = NO;
	gOverlayRoot.extendedLayoutIncludesOpaqueBars = YES;
	window.rootViewController = gOverlayRoot;

	gOverlay.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	[gOverlayRoot.view addSubview:gOverlay];

	gOverlayWindow = window;
}

static UIWindowScene *IOS_Layer_ActiveScene( void )
{
	if ( gOverlayWindow && gOverlayWindow.windowScene )
		return gOverlayWindow.windowScene;
	if ( gSdlWindow && gSdlWindow.windowScene )
		return gSdlWindow.windowScene;
	return IOS_ActiveWindowScene();
}

/* Bounds physiques complets (points), indépendants du viewport SDL / letterbox */
static CGRect IOS_Layer_ScreenBounds( void )
{
	UIWindowScene *scene;

	scene = IOS_Layer_ActiveScene();
	if ( scene && scene.screen )
		return scene.screen.bounds;

	return UIScreen.mainScreen.bounds;
}

static void IOS_Layer_SyncOverlayFrame( void )
{
	CGRect bounds;
	CGRect full;

	if ( !gOverlay )
		return;

	bounds = IOS_Layer_ScreenBounds();
	full = CGRectMake( 0.0f, 0.0f, bounds.size.width, bounds.size.height );

	if ( gOverlayWindow )
	{
		gOverlayWindow.frame = full;
		[gOverlayWindow layoutIfNeeded];
	}

	if ( gOverlayRoot )
	{
		gOverlayRoot.view.frame = full;
		[gOverlayRoot.view layoutIfNeeded];
	}

	gOverlay.frame = full;
	[gOverlay layoutIfNeeded];

	gLayoutWidth = (int)full.size.width;
	gLayoutHeight = (int)full.size.height;
	if ( gLayoutWidth < 1 )
		gLayoutWidth = 1;
	if ( gLayoutHeight < 1 )
		gLayoutHeight = 1;
}

void IOS_Layer_GetLayoutSize( int *width, int *height )
{
	if ( width )
		*width = gLayoutWidth;
	if ( height )
		*height = gLayoutHeight;
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
	gOverlay.hidden = NO;
	[gOverlayWindow makeKeyAndVisible];
}

void IOS_Layer_SetGameOverlayVisible( qboolean visible )
{
	gForceHideOverlay = visible ? qfalse : qtrue;
	if ( gForceHideOverlay )
	{
		if ( gOverlay )
			gOverlay.hidden = YES;
		if ( gOverlayWindow )
		{
			gOverlayWindow.hidden = YES;
			gOverlayWindow.userInteractionEnabled = NO;
		}
		IOS_Layer_RestoreSdlKeyWindow();
		return;
	}

	if ( gOverlayWindow )
	{
		gOverlayWindow.hidden = NO;
		gOverlayWindow.userInteractionEnabled = YES;
	}
}

void IOS_Layer_OpenTouchSettings( void )
{
	dispatch_async( dispatch_get_main_queue(), ^{
		UIViewController *host = nil;

		if ( gSdlWindow && gSdlWindow.rootViewController )
			host = gSdlWindow.rootViewController;
		else if ( gOverlayRoot )
			host = gOverlayRoot;

		if ( !host )
			return;

		/* Masquer les sticks sans présenter la feuille sur une fenêtre cachée */
		gForceHideOverlay = qtrue;
		if ( gOverlay )
			gOverlay.hidden = YES;
		if ( gOverlayWindow )
		{
			gOverlayWindow.hidden = YES;
			gOverlayWindow.userInteractionEnabled = NO;
		}

		if ( gSdlWindow )
			[gSdlWindow makeKeyAndVisible];

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
	gOverlay.fireBtn = [[SGButtonView alloc] initWithFrame:CGRectZero title:@"FIRE" buttonId:TOUCH_BTN_FIRE];
	gOverlay.jumpBtn = [[SGButtonView alloc] initWithFrame:CGRectZero title:@"JUMP" buttonId:TOUCH_BTN_JUMP];
	gOverlay.aimBtn = [[SGButtonView alloc] initWithFrame:CGRectZero title:@"AIM" buttonId:TOUCH_BTN_AIM];
	gOverlay.runBtn = [[SGButtonView alloc] initWithFrame:CGRectZero title:@"RUN" buttonId:TOUCH_BTN_RUN];
	gOverlay.w1Btn = [[SGButtonView alloc] initWithFrame:CGRectZero title:@"1" buttonId:TOUCH_BTN_WEAPON1];
	gOverlay.w2Btn = [[SGButtonView alloc] initWithFrame:CGRectZero title:@"2" buttonId:TOUCH_BTN_WEAPON2];
	gOverlay.w3Btn = [[SGButtonView alloc] initWithFrame:CGRectZero title:@"3" buttonId:TOUCH_BTN_WEAPON3];
	gOverlay.w4Btn = [[SGButtonView alloc] initWithFrame:CGRectZero title:@"4" buttonId:TOUCH_BTN_WEAPON4];
	gOverlay.dropBtn = [[SGButtonView alloc] initWithFrame:CGRectZero title:@"L" buttonId:TOUCH_BTN_DROP];
	gOverlay.reloadBtn = [[SGButtonView alloc] initWithFrame:CGRectZero title:@"R" buttonId:TOUCH_BTN_RELOAD];
	gOverlay.useBtn = [[SGButtonView alloc] initWithFrame:CGRectZero title:@"F" buttonId:TOUCH_BTN_USE];
	gOverlay.buyBtn = [[SGButtonView alloc] initWithFrame:CGRectZero title:@"B" buttonId:TOUCH_BTN_BUY];
	gOverlay.eBtn = [[SGButtonView alloc] initWithFrame:CGRectZero title:@"E" buttonId:TOUCH_BTN_E];
	gOverlay.escBtn = [[SGButtonView alloc] initWithFrame:CGRectZero title:@"ESC" buttonId:TOUCH_BTN_ESC];
	gOverlay.settingsBtn = [[SGSettingsButton alloc] initWithFrame:CGRectMake( 8, 8, 44, 44 )];
	gOverlay.keyboardBtn = [[SGKeyboardButton alloc] initWithFrame:CGRectMake( 8, 58, 44, 44 )];

	[gOverlay addSubview:gOverlay.moveStick];
	[gOverlay addSubview:gOverlay.lookStick];
	[gOverlay addSubview:gOverlay.fireBtn];
	[gOverlay addSubview:gOverlay.jumpBtn];
	[gOverlay addSubview:gOverlay.aimBtn];
	[gOverlay addSubview:gOverlay.runBtn];
	[gOverlay addSubview:gOverlay.w1Btn];
	[gOverlay addSubview:gOverlay.w2Btn];
	[gOverlay addSubview:gOverlay.w3Btn];
	[gOverlay addSubview:gOverlay.w4Btn];
	[gOverlay addSubview:gOverlay.dropBtn];
	[gOverlay addSubview:gOverlay.reloadBtn];
	[gOverlay addSubview:gOverlay.useBtn];
	[gOverlay addSubview:gOverlay.buyBtn];
	[gOverlay addSubview:gOverlay.eBtn];
	[gOverlay addSubview:gOverlay.escBtn];
	[gOverlay addSubview:gOverlay.settingsBtn];
	[gOverlay addSubview:gOverlay.keyboardBtn];

	IOS_Layer_SyncOverlayFrame();
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

void IOS_Layer_SyncScreen( void )
{
	dispatch_async( dispatch_get_main_queue(), ^{
		IOS_Layer_SyncOverlayFrame();
	} );
}

static void IOS_Layer_TickMain( void )
{
	touchOverlay_t o;
	qboolean uiMode;
	qboolean showOverlay;

	if ( !gOverlay )
		return;

	IOS_Layer_SyncOverlayFrame();

	uiMode = IN_TouchInUIMode();
	IN_TouchGetOverlayForSize( &o, gLayoutWidth, gLayoutHeight );
	showOverlay = ( !gForceHideOverlay && o.visible ) ? qtrue : qfalse;

	if ( !showOverlay )
	{
		IOS_Layer_HideOverlayWindow();
		IOS_Layer_SetMenuTouchActive( uiMode && !gForceHideOverlay ? qtrue : qfalse );
		return;
	}

	IOS_Layer_SetMenuTouchActive( qfalse );
	IOS_Layer_ShowOverlayWindow();
	[gOverlay layoutFromOverlay:NULL];
}

void IOS_Layer_Tick( void )
{
	if ( [NSThread isMainThread] )
	{
		IOS_Layer_TickMain();
		return;
	}

	dispatch_sync( dispatch_get_main_queue(), ^{
		IOS_Layer_TickMain();
	} );
}
