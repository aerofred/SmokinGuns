/*
===========================================================================
Réglages iOS (UIKit) — tactile + affichage via cl_touch.c
===========================================================================
*/
#import <UIKit/UIKit.h>

#include "../client/cl_touch.h"
#include "ios_layer.h"

typedef NS_ENUM( NSInteger, SGTouchRow ) {
	SGTouchRowEnabled = 0,
	SGTouchRowLookSens,
	SGTouchRowUISens,
	SGTouchRowDeadzone,
	SGTouchRowGamma,
	SGTouchRowIntensity,
	SGTouchRowOverbright,
	SGTouchRowStickSize,
	SGTouchRowBtnSize,
	SGTouchRowMoveX,
	SGTouchRowMoveY,
	SGTouchRowLookX,
	SGTouchRowLookY,
	SGTouchRowRightX,
	SGTouchRowFireY,
	SGTouchRowJumpY,
	SGTouchRowCount
};

@interface SGTouchSettingsController : UITableViewController
@property (nonatomic, assign) touchConfig_t cfg;
@end

@implementation SGTouchSettingsController

- (instancetype)init
{
	return [super initWithStyle:UITableViewStyleInsetGrouped];
}

- (void)viewDidLoad
{
	[super viewDidLoad];
	self.title = @"Réglages";

	UIBarButtonItem *done = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone
		target:self action:@selector(onDone)];
	UIBarButtonItem *reset = [[UIBarButtonItem alloc] initWithTitle:@"Défauts"
		style:UIBarButtonItemStylePlain target:self action:@selector(onReset)];
	self.navigationItem.rightBarButtonItem = done;
	self.navigationItem.leftBarButtonItem = reset;

	IN_TouchReadConfig( &_cfg );
}

- (void)viewWillDisappear:(BOOL)animated
{
	[super viewWillDisappear:animated];
	if ( self.isBeingDismissed || self.navigationController.isBeingDismissed )
		IOS_Layer_SetGameOverlayVisible( qtrue );
}

- (void)onDone
{
	[self dismissViewControllerAnimated:YES completion:nil];
}

- (void)onReset
{
	IN_TouchApplyDefaults();
	IN_TouchReadConfig( &_cfg );
	[self.tableView reloadData];
}

- (void)pushConfig
{
	IN_TouchWriteConfig( &_cfg );
}

- (NSString *)titleForRow:(SGTouchRow)row
{
	switch ( row )
	{
	case SGTouchRowEnabled: return @"Activer";
	case SGTouchRowLookSens: return @"Sensibilité visée";
	case SGTouchRowUISens: return @"Sensibilité menus";
	case SGTouchRowDeadzone: return @"Zone morte";
	case SGTouchRowGamma: return @"Gamma";
	case SGTouchRowIntensity: return @"Intensité";
	case SGTouchRowOverbright: return @"Luminosité cartes";
	case SGTouchRowStickSize: return @"Taille sticks";
	case SGTouchRowBtnSize: return @"Taille boutons";
	case SGTouchRowMoveX: return @"Déplacement X";
	case SGTouchRowMoveY: return @"Déplacement Y";
	case SGTouchRowLookX: return @"Visée X";
	case SGTouchRowLookY: return @"Visée Y";
	case SGTouchRowRightX: return @"Colonne droite X";
	case SGTouchRowFireY: return @"Tir Y";
	case SGTouchRowJumpY: return @"Saut Y";
	default: return @"";
	}
}

- (void)min:(float *)mn max:(float *)mx forRow:(SGTouchRow)row
{
	switch ( row )
	{
	case SGTouchRowLookSens: *mn = 0.5f; *mx = 6.0f; break;
	case SGTouchRowUISens: *mn = 0.5f; *mx = 8.0f; break;
	case SGTouchRowDeadzone: *mn = 0.0f; *mx = 0.5f; break;
	case SGTouchRowGamma: *mn = 0.5f; *mx = 3.0f; break;
	case SGTouchRowIntensity: *mn = 1.0f; *mx = 3.0f; break;
	case SGTouchRowOverbright: *mn = 0.0f; *mx = 2.0f; break;
	case SGTouchRowStickSize: *mn = 0.08f; *mx = 0.22f; break;
	case SGTouchRowBtnSize: *mn = 0.06f; *mx = 0.18f; break;
	default: *mn = 0.05f; *mx = 0.95f; break;
	}
}

- (float *)valuePtrForRow:(SGTouchRow)row
{
	switch ( row )
	{
	case SGTouchRowLookSens: return &_cfg.lookSensitivity;
	case SGTouchRowUISens: return &_cfg.uiSensitivity;
	case SGTouchRowDeadzone: return &_cfg.deadzone;
	case SGTouchRowGamma: return &_cfg.gamma;
	case SGTouchRowIntensity: return &_cfg.intensity;
	case SGTouchRowOverbright: return &_cfg.overBrightBits;
	case SGTouchRowStickSize: return &_cfg.stickSize;
	case SGTouchRowBtnSize: return &_cfg.btnSize;
	case SGTouchRowMoveX: return &_cfg.moveX;
	case SGTouchRowMoveY: return &_cfg.moveY;
	case SGTouchRowLookX: return &_cfg.lookX;
	case SGTouchRowLookY: return &_cfg.lookY;
	case SGTouchRowRightX: return &_cfg.rightX;
	case SGTouchRowFireY: return &_cfg.fireY;
	case SGTouchRowJumpY: return &_cfg.jumpY;
	default: return NULL;
	}
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
	(void)tableView;
	return 4;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
	(void)tableView;
	switch ( section )
	{
	case 0: return @"Contrôles";
	case 1: return @"Affichage";
	case 2: return @"Tailles";
	default: return @"Positions (0–1)";
	}
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
	(void)tableView;
	if ( section == 1 )
		return @"Augmentez gamma et intensité si le monde paraît trop sombre ou noir.";
	return nil;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
	(void)tableView;
	switch ( section )
	{
	case 0: return 4;
	case 1: return 3;
	case 2: return 2;
	default: return 7;
	}
}

- (SGTouchRow)rowForIndexPath:(NSIndexPath *)ip
{
	if ( ip.section == 0 )
		return (SGTouchRow)ip.row;
	if ( ip.section == 1 )
		return (SGTouchRow)( SGTouchRowGamma + ip.row );
	if ( ip.section == 2 )
		return (SGTouchRow)( SGTouchRowStickSize + ip.row );
	return (SGTouchRow)( SGTouchRowMoveX + ip.row );
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)ip
{
	SGTouchRow row = [self rowForIndexPath:ip];
	NSString *cid = ( row == SGTouchRowEnabled ) ? @"switch" : @"slider";
	UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:cid];
	if ( !cell )
		cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:cid];

	cell.textLabel.text = [self titleForRow:row];
	cell.selectionStyle = UITableViewCellSelectionStyleNone;
	cell.accessoryView = nil;

	if ( row == SGTouchRowEnabled )
	{
		UISwitch *sw = [[UISwitch alloc] init];
		sw.on = _cfg.enabled ? YES : NO;
		[sw addTarget:self action:@selector(onSwitch:) forControlEvents:UIControlEventValueChanged];
		cell.accessoryView = sw;
	}
	else
	{
		float mn, mx;
		float *val = [self valuePtrForRow:row];
		[self min:&mn max:&mx forRow:row];

		UISlider *sl = [[UISlider alloc] initWithFrame:CGRectMake( 0, 0, 160, 31 )];
		sl.minimumValue = mn;
		sl.maximumValue = mx;
		sl.value = val ? *val : 0.0f;
		sl.tag = (NSInteger)row;
		[sl addTarget:self action:@selector(onSlider:) forControlEvents:UIControlEventValueChanged];
		cell.accessoryView = sl;
	}

	return cell;
}

- (void)onSwitch:(UISwitch *)sw
{
	_cfg.enabled = sw.on ? qtrue : qfalse;
	[self pushConfig];
}

- (void)onSlider:(UISlider *)sl
{
	SGTouchRow row = (SGTouchRow)sl.tag;
	float *val = [self valuePtrForRow:row];
	if ( !val )
		return;
	*val = sl.value;
	[self pushConfig];
}

@end

void IOS_TouchSettings_Present( UIViewController *host )
{
	if ( !host )
		return;

	SGTouchSettingsController *root = [[SGTouchSettingsController alloc] init];
	UINavigationController *nav = [[UINavigationController alloc] initWithRootViewController:root];
	nav.modalPresentationStyle = UIModalPresentationFormSheet;
	if ( @available( iOS 13.0, * ) )
		nav.modalInPresentation = YES;

	[host presentViewController:nav animated:YES completion:nil];
}
