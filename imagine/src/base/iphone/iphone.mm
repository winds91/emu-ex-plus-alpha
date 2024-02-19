/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

static_assert(__has_feature(objc_arc), "This file requires ARC");
#define LOGTAG "Base"
#import "MainApp.hh"
#import <dlfcn.h>
#import <unistd.h>
#include <imagine/base/Application.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Screen.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include "private.hh"
#include <imagine/fs/FS.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/coreFoundation.h>
#include "ios.hh"
#if __IPHONE_OS_VERSION_MIN_REQUIRED < 110000
#include <variant>
#endif

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <Foundation/NSPathUtilities.h>

#if defined __ARM_ARCH_6K__
// firstObject is availible since iOS 4.0
// but not declared in the 5.1 SDK
@interface NSArray (FirstObject)
- (id)firstObject;
@end
#endif

namespace IG::Input
{
	int GSEVENTKEY_KEYCODE = sizeof(NSInteger) == 8 ? GSEVENTKEY_KEYCODE_64_BIT : 15;
}

namespace IG
{

MainApp *mainApp{};
Application *appPtr{};
bool isIPad = false;
static bool isRunningAsSystemApp = false;
UIApplication *sharedApp{};
static const char *docPath{};
static FS::PathString appPath{};
static id onOrientationChangedObserver = nil;

#ifdef IPHONE_IMG_PICKER
static UIImagePickerController* imagePickerController;
static IPhoneImgPickerCallback imgPickCallback = NULL;
static void *imgPickUserPtr = NULL;
static NSData *imgPickData[2];
static uint8_t imgPickDataElements = 0;
#include "imagePicker.h"
#endif

#ifdef IPHONE_MSG_COMPOSE
static MFMailComposeViewController *composeController;
#include "mailCompose.h"
#endif

#ifdef IPHONE_GAMEKIT
#include "gameKit.h"
#endif

void releaseCFObject(void *ptr)
{
	if(!ptr)
		return;
	CFRelease(ptr);
}

static Screen &setupUIScreen(ApplicationContext ctx, UIScreen *screen, bool setOverscanCompensation)
{
	// prevent overscan compensation
	if(setOverscanCompensation)
		screen.overscanCompensation = UIScreenOverscanCompensationInsetApplicationFrame;
	IOSScreen::InitParams initParams{(__bridge void*)screen};
	auto s = std::make_unique<Screen>(ctx, initParams);
	[s->displayLink() addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
	return ctx.application().addScreen(ctx, std::move(s), true);
}

IOSApplication::IOSApplication(ApplicationInitParams initParams):
	BaseApplication{({appPtr = static_cast<Application*>(this); (__bridge UIApplication*)initParams.uiAppPtr;})}
{
	ApplicationContext ctx{(__bridge UIApplication*)initParams.uiAppPtr};
	auto sharedApp = ctx.uiApp();
	if(Config::DEBUG_BUILD)
	{
		//logMsg("in didFinishLaunchingWithOptions(), UUID %s", [[[UIDevice currentDevice] uniqueIdentifier] cStringUsingEncoding: NSASCIIStringEncoding]);
		logMsg("iOS version %s", [[[UIDevice currentDevice] systemVersion] cStringUsingEncoding: NSASCIIStringEncoding]);
	}
	if(UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
	{
		isIPad = 1;
		logMsg("running on iPad");
	}
	Input::init(ctx);
	if(hasAtLeastIOS7())
	{
		#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 70000
		[sharedApp setStatusBarStyle:UIStatusBarStyleLightContent animated:YES];
		#endif
		if(sizeof(NSInteger) == 4)
			Input::GSEVENTKEY_KEYCODE = Input::GSEVENTKEY_KEYCODE_IOS7;
	}
	else
	{
		#if __IPHONE_OS_VERSION_MIN_REQUIRED < 70000
		[sharedApp setStatusBarStyle:UIStatusBarStyleBlackOpaque animated:YES];
		#endif
	}

	//[nCenter addObserver:self selector:@selector(keyboardWasShown:) name:UIKeyboardDidShowNotification object:nil];
	//[nCenter addObserver:self selector:@selector(keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];
	#ifdef CONFIG_BASE_MULTI_SCREEN
	{
		NSNotificationCenter *nCenter = [NSNotificationCenter defaultCenter];
		[nCenter addObserverForName:UIScreenDidConnectNotification
			object:nil queue:nil usingBlock:
			^(NSNotification *note)
			{
				logMsg("screen connected");
				UIScreen *screen = [note object];
				if(findScreen((__bridge void*)screen))
				{
					logMsg("screen %p already in list", screen);
					return;
				}
				auto &s = setupUIScreen(ctx, screen, true);
			}];
		[nCenter addObserverForName:UIScreenDidDisconnectNotification
			object:nil queue:nil usingBlock:
			^(NSNotification *note)
			{
				logMsg("screen disconnected");
				UIScreen *screen = [note object];
				if(auto removedScreen = removeScreen(ctx, (__bridge void*)screen, true);
					removedScreen)
				{
					[removedScreen->displayLink() removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
				}
			}];
		if(Config::DEBUG_BUILD)
		{
			[nCenter addObserverForName:UIScreenModeDidChangeNotification
				object:nil queue:nil usingBlock:
				^(NSNotification *note)
				{
					logMsg("screen mode change");
				}];
		}
	}
	for(UIScreen *screen in [UIScreen screens])
	{
		setupUIScreen(ctx, screen, screens().size());
	}
	#else
	mainScreen().init([UIScreen mainScreen]);
	[mainScreen().displayLink() addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
	#endif
}

}

using namespace IG;

@implementation MainApp

#if 0
- (void)keyboardWasShown:(NSNotification *)notification
{
	return;
	using namespace IG;
	#ifndef NDEBUG
	CGSize keyboardSize = [[[notification userInfo] objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;
	logMsg("keyboard shown with size %d", (int)keyboardSize.height * pointScale);
	int visibleY = IG::max(1, int(mainWin.rect.y2 - keyboardSize.height * pointScale));
	float visibleFraction = visibleY / mainWin.rect.y2;
	/*if(isIPad)
		Gfx::viewMMHeight_ = 197. * visibleFraction;
	else
		Gfx::viewMMHeight_ = 75. * visibleFraction;*/
	//generic_resizeEvent(mainWin.rect.x2, visibleY);
	#endif
	mainWin.postDraw();
}

- (void) keyboardWillHide:(NSNotification *)notification
{
	return;
	using namespace IG;
	logMsg("keyboard hidden");
	mainWin.postDraw();
}
#endif

static Rotation iOSOrientationToGfx(UIDeviceOrientation orientation)
{
	switch(orientation)
	{
		case UIDeviceOrientationPortrait: return Rotation::UP;
		case UIDeviceOrientationLandscapeLeft: return Rotation::LEFT;
		case UIDeviceOrientationLandscapeRight: return Rotation::RIGHT;
		case UIDeviceOrientationPortraitUpsideDown: return Rotation::DOWN;
		default : return Rotation::ANY; // TODO: handle Face-up/down
	}
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	// TODO: use NSProcessInfo
	using namespace IG;
	mainApp = self;
	auto uiApp = [UIApplication sharedApplication];
	ApplicationInitParams initParams{.uiAppPtr = (__bridge void*)uiApp};
	ApplicationContext ctx{uiApp};
	ctx.dispatchOnInit(initParams);
	if(!ctx.windows().size())
		logWarn("didn't create a window");
	logMsg("exiting didFinishLaunchingWithOptions");
	return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
	logMsg("resign active");
	ApplicationContext ctx{application};
	for(auto &w : ctx.windows())
	{
		w->dispatchFocusChange(false);
	}
	ctx.application().deinitKeyRepeatTimer();
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
	logMsg("became active");
	ApplicationContext ctx{application};
	for(auto &w : ctx.windows())
	{
		w->dispatchFocusChange(true);
	}
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	logMsg("app exiting");
	ApplicationContext ctx{application};
	ctx.application().setExitingActivityState();
	ctx.dispatchOnExit(false);
	logMsg("app exited");
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
	logMsg("entering background");
	ApplicationContext ctx{application};
	ctx.application().setPausedActivityState();
	ctx.dispatchOnExit(true);
	ctx.application().setActiveForAllScreens(false);
	ctx.application().deinitKeyRepeatTimer();
	logMsg("entered background");
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
	logMsg("entered foreground");
	ApplicationContext ctx{application};
	ctx.application().setRunningActivityState();
	ctx.application().setActiveForAllScreens(true);
	for(auto &w : ctx.windows())
	{
		w->postDraw();
	}
	ctx.dispatchOnResume(true);
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
	logMsg("got memory warning");
	ApplicationContext ctx{application};
	ctx.application().dispatchOnFreeCaches(ctx, ctx.isRunning());
}

@end

@interface UIApplication ()
- (void)handleKeyUIEvent:(UIEvent *)event;
@end

@implementation MainUIApp

#ifndef __aarch64__
- (void)sendEvent:(UIEvent *)event
{
	[super sendEvent:event];
	if(sizeof(NSInteger) == 4 && Input::GSEVENTKEY_KEYCODE != Input::GSEVENTKEY_KEYCODE_IOS7)
		Input::handleKeyEvent({self}, event);
}
#endif

#ifndef __ARM_ARCH_6K__
- (void)handleKeyUIEvent:(UIEvent *)event
{
	[super handleKeyUIEvent:event];
	Input::handleKeyEvent({self}, event);
}
#endif

@end

namespace IG
{

static void setStatusBarHidden(ApplicationContext ctx, bool hidden)
{
	logMsg("setting status bar hidden: %d", (int)hidden);
	[ctx.uiApp() setStatusBarHidden: (hidden ? YES : NO) withAnimation: UIStatusBarAnimationFade];
	if(ctx.deviceWindow())
	{
		auto &win = *ctx.deviceWindow();
		win.updateContentRect(win.width(), win.height(), win.softOrientation());
		win.postDraw();
	}
}

void ApplicationContext::setSysUIStyle(SystemUIStyleFlags flags)
{
	setStatusBarHidden(uiApp(), flags.hideStatus);
}

bool ApplicationContext::hasTranslucentSysUI() const
{
	return hasAtLeastIOS7();
}

UIInterfaceOrientation gfxOrientationToUIInterfaceOrientation(Rotation orientation)
{
	switch(orientation)
	{
		default: return UIInterfaceOrientationPortrait;
		case Rotation::LEFT: return UIInterfaceOrientationLandscapeLeft;
		case Rotation::RIGHT: return UIInterfaceOrientationLandscapeRight;
		case Rotation::DOWN: return UIInterfaceOrientationPortraitUpsideDown;
	}
}

void ApplicationContext::setDeviceOrientationChangeSensor(bool enable)
{
	UIDevice *currDev = [UIDevice currentDevice];
	auto notificationsAreOn = currDev.generatesDeviceOrientationNotifications;
	if(enable && !notificationsAreOn)
	{
		logMsg("enabling device orientation notifications");
		[currDev beginGeneratingDeviceOrientationNotifications];
	}
	else if(!enable && notificationsAreOn)
	{
		logMsg("disabling device orientation notifications");
		[currDev endGeneratingDeviceOrientationNotifications];
	}
}

void ApplicationContext::setOnDeviceOrientationChanged(DeviceOrientationChangedDelegate onOrientationChanged)
{
	if(onOrientationChanged)
	{
		NSNotificationCenter *nCenter = [NSNotificationCenter defaultCenter];
		if(onOrientationChangedObserver)
		{
			[nCenter removeObserver:onOrientationChangedObserver];
		}
		onOrientationChangedObserver = [nCenter addObserverForName:UIDeviceOrientationDidChangeNotification
		                               object:nil queue:nil usingBlock:
		                               ^(NSNotification *note)
		                               {
		                              	auto o = iOSOrientationToGfx([[UIDevice currentDevice] orientation]);
		                              	if(o != Rotation::ANY)
		                              	{
		                              		onOrientationChanged(*this, o);
		                              	}
		                               }];
	}
	else if(!onOrientationChanged && onOrientationChangedObserver)
	{
		[[NSNotificationCenter defaultCenter] removeObserver:onOrientationChangedObserver];
		onOrientationChangedObserver = nil;
	}

}

void ApplicationContext::setSystemOrientation(Rotation o)
{
	logMsg("setting system orientation %s", wise_enum::to_string(o).data());
	auto sharedApp = uiApp();
	[sharedApp setStatusBarOrientation:gfxOrientationToUIInterfaceOrientation(o) animated:YES];
	if(deviceWindow())
	{
		auto &win = *deviceWindow();
		win.updateContentRect(win.width(), win.height(), win.softOrientation());
		win.postDraw();
	}
}

Orientations ApplicationContext::defaultSystemOrientations() const
{
	return isIPad ? Orientations::all() : Orientations::allButUpsideDown();
}

void ApplicationContext::setOnSystemOrientationChanged(SystemOrientationChangedDelegate del)
{
	// TODO
}

void ApplicationContext::exit(int returnVal)
{
	application().setExitingActivityState();
	dispatchOnExit(false);
	::exit(returnVal);
}

void ApplicationContext::openURL(CStringView url) const
{
	[uiApp() openURL:[NSURL URLWithString:
		[NSString stringWithCString:url encoding:NSASCIIStringEncoding]]];
}

void ApplicationContext::setIdleDisplayPowerSave(bool on)
{
	uiApp().idleTimerDisabled = on ? NO : YES;
	if(Config::DEBUG_BUILD)
	{
		logMsg("set idleTimerDisabled %d", (int)uiApp().idleTimerDisabled);
	}
}

void ApplicationContext::endIdleByUserActivity()
{
	if(!uiApp().idleTimerDisabled)
	{
		uiApp().idleTimerDisabled = YES;
		uiApp().idleTimerDisabled = NO;
	}
}

static FS::PathString makeSearchPath(NSSearchPathDirectory dir, NSSearchPathDomainMask domainMask, const char *appName = nullptr)
{
	NSArray *paths = NSSearchPathForDirectoriesInDomains(dir, domainMask, YES);
	NSString *dirStr = [paths firstObject];
	if(!dirStr)
		return {};
	NSFileManager *fm = [NSFileManager defaultManager];
	if(appName)
	{
		dirStr = [dirStr stringByAppendingPathComponent:[NSString stringWithUTF8String:appName]];
	}
	[fm createDirectoryAtPath:dirStr withIntermediateDirectories:YES attributes:nil error:nil];
	return dirStr.UTF8String;
}

FS::PathString ApplicationContext::assetPath(const char *) const { return appPath; }

FS::PathString ApplicationContext::supportPath(const char *appName) const
{
	return makeSearchPath(NSApplicationSupportDirectory, NSUserDomainMask, isRunningAsSystemApp ? appName : nullptr);
}

FS::PathString ApplicationContext::cachePath(const char *appName) const
{
	return makeSearchPath(NSCachesDirectory, NSUserDomainMask, isRunningAsSystemApp ? appName : nullptr);
}

FS::PathString ApplicationContext::sharedStoragePath() const
{
	if(isRunningAsSystemApp)
		return {"/User/Media"};
	else
		return makeSearchPath(NSDocumentDirectory, NSUserDomainMask);
}

FS::PathLocation ApplicationContext::sharedStoragePathLocation() const
{
	auto path = sharedStoragePath();
	if(isRunningAsSystemApp)
		return {path, "Storage Media", "Media"};
	else
		return {path, "Documents", "Documents"};
}

std::vector<FS::PathLocation> ApplicationContext::rootFileLocations() const
{
	return
		{
			sharedStoragePathLocation()
		};
}

FS::PathString ApplicationContext::libPath(const char *) const
{
	return appPath;
}

void IOSApplicationContext::setApplicationPtr(Application *appPtr_)
{
	appPtr = appPtr_;
}

Application &IOSApplicationContext::application() const
{
	return *appPtr;
}

bool IOSApplicationContext::deviceIsIPad() const
{
	return isIPad;
}

bool IOSApplicationContext::isSystemApp() const
{
	return isRunningAsSystemApp;
}

bool hasAtLeastIOS7()
{
	return kCFCoreFoundationVersionNumber >= kCFCoreFoundationVersionNumber_iOS_7_0;
}

bool hasAtLeastIOS8()
{
	return kCFCoreFoundationVersionNumber >= kCFCoreFoundationVersionNumber_iOS_8_0;
}

void ApplicationContext::exitWithMessage(int exitVal, const char *msg)
{
	logErr("%s", msg);
	::exit(exitVal);
}

#ifdef CONFIG_BASE_IOS_SETUID

uid_t realUID = 0, effectiveUID = 0;
static void setupUID()
{
	realUID = getuid();
	effectiveUID = geteuid();
	seteuid(realUID);
}

void setUIDReal()
{
	seteuid(realUID);
}

bool setUIDEffective()
{
	return seteuid(effectiveUID) == 0;
}

#endif

}

#if __IPHONE_OS_VERSION_MIN_REQUIRED < 110000
void *operator new(unsigned long size, std::align_val_t align)
{
	void *ptr{};
	::posix_memalign(&ptr, (size_t)align, size);
	return ptr;
}

void *operator new[](unsigned long size, std::align_val_t align)
{
	return ::operator new[](size);
}

void operator delete(void* ptr, std::align_val_t align) noexcept
{
	::free(ptr);
}

void operator delete[](void* ptr, std::align_val_t align) noexcept
{
	::operator delete[](ptr);
}

namespace std
{

const char *bad_variant_access::what() const noexcept { return ""; };

};
#endif

int main(int argc, char *argv[])
{
	using namespace IG;
	// check if running from system apps directory
	if(strlen(argv[0]) >= 14 && memcmp(argv[0], "/Applications/", 14) == 0)
	{
		logMsg("launched as system app from: %s", argv[0]);
		isRunningAsSystemApp = true;
	}
	#ifdef CONFIG_BASE_IOS_SETUID
	setupUID();
	#endif
	logger_setLogDirectoryPrefix("/var/mobile");
	appPath = FS::makeAppPathFromLaunchCommand(argv[0]);
	
	#ifdef CONFIG_BASE_IOS_SETUID
	logMsg("real UID %d, effective UID %d", realUID, effectiveUID);
	if(access("/Library/MobileSubstrate/DynamicLibraries/Backgrounder.dylib", F_OK) == 0)
	{
		logMsg("manually loading Backgrounder.dylib");
		dlopen("/Library/MobileSubstrate/DynamicLibraries/Backgrounder.dylib", RTLD_LAZY | RTLD_GLOBAL);
	}
	#endif

	@autoreleasepool
	{
		return UIApplicationMain(argc, argv, @"MainUIApp", @"MainApp");
	}
}
