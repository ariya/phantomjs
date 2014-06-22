/*
 * Copyright (C) 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *           (C) 2007 Graham Dennis (graham.dennis@gmail.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "DumpRenderTree.h"

#import "AccessibilityController.h"
#import "CheckedMalloc.h"
#import "DefaultPolicyDelegate.h"
#import "DumpRenderTreeDraggingInfo.h"
#import "DumpRenderTreePasteboard.h"
#import "DumpRenderTreeWindow.h"
#import "EditingDelegate.h"
#import "EventSendingController.h"
#import "FrameLoadDelegate.h"
#import "HistoryDelegate.h"
#import "JavaScriptThreading.h"
#import "TestRunner.h"
#import "MockGeolocationProvider.h"
#import "MockWebNotificationProvider.h"
#import "NavigationController.h"
#import "ObjCPlugin.h"
#import "ObjCPluginFunction.h"
#import "PixelDumpSupport.h"
#import "PolicyDelegate.h"
#import "ResourceLoadDelegate.h"
#import "StorageTrackerDelegate.h"
#import "UIDelegate.h"
#import "WebArchiveDumpSupport.h"
#import "WebCoreTestSupport.h"
#import "WorkQueue.h"
#import "WorkQueueItem.h"
#import <Carbon/Carbon.h>
#import <CoreFoundation/CoreFoundation.h>
#import <JavaScriptCore/HeapStatistics.h>
#import <JavaScriptCore/Options.h>
#import <WebCore/FoundationExtras.h>
#import <WebKit/DOMElement.h>
#import <WebKit/DOMExtensions.h>
#import <WebKit/DOMRange.h>
#import <WebKit/WebArchive.h>
#import <WebKit/WebBackForwardList.h>
#import <WebKit/WebCache.h>
#import <WebKit/WebCoreStatistics.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebDatabaseManagerPrivate.h>
#import <WebKit/WebDocumentPrivate.h>
#import <WebKit/WebDeviceOrientationProviderMock.h>
#import <WebKit/WebDynamicScrollBarsView.h>
#import <WebKit/WebEditingDelegate.h>
#import <WebKit/WebFrameView.h>
#import <WebKit/WebHistory.h>
#import <WebKit/WebHistoryItemPrivate.h>
#import <WebKit/WebInspector.h>
#import <WebKit/WebKitNSStringExtras.h>
#import <WebKit/WebPluginDatabase.h>
#import <WebKit/WebPreferences.h>
#import <WebKit/WebPreferencesPrivate.h>
#import <WebKit/WebPreferenceKeysPrivate.h>
#import <WebKit/WebResourceLoadDelegate.h>
#import <WebKit/WebStorageManagerPrivate.h>
#import <WebKit/WebTypesInternal.h>
#import <WebKit/WebViewPrivate.h>
#import <getopt.h>
#import <wtf/Assertions.h>
#import <wtf/FastMalloc.h>
#import <wtf/RetainPtr.h>
#import <wtf/Threading.h>
#import <wtf/ObjcRuntimeExtras.h>
#import <wtf/OwnPtr.h>

extern "C" {
#import <mach-o/getsect.h>
}

using namespace std;

@interface DumpRenderTreeApplication : NSApplication
@end

@interface DumpRenderTreeEvent : NSEvent
@end

@interface NSURLRequest (PrivateThingsWeShouldntReallyUse)
+(void)setAllowsAnyHTTPSCertificate:(BOOL)allow forHost:(NSString *)host;
@end

#if USE(APPKIT)
@interface NSSound (Details)
+ (void)_setAlertType:(NSUInteger)alertType;
@end
#endif

static void runTest(const string& testPathOrURL);

// Deciding when it's OK to dump out the state is a bit tricky.  All these must be true:
// - There is no load in progress
// - There is no work queued up (see workQueue var, below)
// - waitToDump==NO.  This means either waitUntilDone was never called, or it was called
//       and notifyDone was called subsequently.
// Note that the call to notifyDone and the end of the load can happen in either order.

volatile bool done;

NavigationController* gNavigationController = 0;
RefPtr<TestRunner> gTestRunner;

WebFrame *mainFrame = 0;
// This is the topmost frame that is loading, during a given load, or nil when no load is 
// in progress.  Usually this is the same as the main frame, but not always.  In the case
// where a frameset is loaded, and then new content is loaded into one of the child frames,
// that child frame is the "topmost frame that is loading".
WebFrame *topLoadingFrame = nil;     // !nil iff a load is in progress


CFMutableSetRef disallowedURLs = 0;
static CFRunLoopTimerRef waitToDumpWatchdog = 0;

// Delegates
static FrameLoadDelegate *frameLoadDelegate;
static UIDelegate *uiDelegate;
static EditingDelegate *editingDelegate;
static ResourceLoadDelegate *resourceLoadDelegate;
static HistoryDelegate *historyDelegate;
PolicyDelegate *policyDelegate;
DefaultPolicyDelegate *defaultPolicyDelegate;
StorageTrackerDelegate *storageDelegate;

static int dumpPixelsForAllTests = NO;
static bool dumpPixelsForCurrentTest = false;
static int threaded;
static int dumpTree = YES;
static int useTimeoutWatchdog = YES;
static int forceComplexText;
static int gcBetweenTests;
static BOOL printSeparators;
static RetainPtr<CFStringRef> persistentUserStyleSheetLocation;

static WebHistoryItem *prevTestBFItem = nil;  // current b/f item at the end of the previous test

#ifdef __OBJC2__
static void swizzleAllMethods(Class imposter, Class original)
{
    unsigned int imposterMethodCount;
    Method* imposterMethods = class_copyMethodList(imposter, &imposterMethodCount);

    unsigned int originalMethodCount;
    Method* originalMethods = class_copyMethodList(original, &originalMethodCount);

    for (unsigned int i = 0; i < imposterMethodCount; i++) {
        SEL imposterMethodName = method_getName(imposterMethods[i]);

        // Attempt to add the method to the original class.  If it fails, the method already exists and we should
        // instead exchange the implementations.
        if (class_addMethod(original, imposterMethodName, method_getImplementation(imposterMethods[i]), method_getTypeEncoding(imposterMethods[i])))
            continue;

        unsigned int j = 0;
        for (; j < originalMethodCount; j++) {
            SEL originalMethodName = method_getName(originalMethods[j]);
            if (sel_isEqual(imposterMethodName, originalMethodName))
                break;
        }

        // If class_addMethod failed above then the method must exist on the original class.
        ASSERT(j < originalMethodCount);
        method_exchangeImplementations(imposterMethods[i], originalMethods[j]);
    }

    free(imposterMethods);
    free(originalMethods);
}
#endif

static void poseAsClass(const char* imposter, const char* original)
{
    Class imposterClass = objc_getClass(imposter);
    Class originalClass = objc_getClass(original);

#ifndef __OBJC2__
    class_poseAs(imposterClass, originalClass);
#else

    // Swizzle instance methods
    swizzleAllMethods(imposterClass, originalClass);
    // and then class methods
    swizzleAllMethods(object_getClass(imposterClass), object_getClass(originalClass));
#endif
}

void setPersistentUserStyleSheetLocation(CFStringRef url)
{
    persistentUserStyleSheetLocation = url;
}

static bool shouldIgnoreWebCoreNodeLeaks(const string& URLString)
{
    static char* const ignoreSet[] = {
        // Keeping this infrastructure around in case we ever need it again.
    };
    static const int ignoreSetCount = sizeof(ignoreSet) / sizeof(char*);
    
    for (int i = 0; i < ignoreSetCount; i++) {
        // FIXME: ignore case
        string curIgnore(ignoreSet[i]);
        // Match at the end of the URLString
        if (!URLString.compare(URLString.length() - curIgnore.length(), curIgnore.length(), curIgnore))
            return true;
    }
    return false;
}

static NSSet *allowedFontFamilySet()
{
    static NSSet *fontFamilySet = [[NSSet setWithObjects:
        @"Ahem",
        @"Al Bayan",
        @"American Typewriter",
        @"Andale Mono",
        @"Apple Braille",
        @"Apple Color Emoji",
        @"Apple Chancery",
        @"Apple Garamond BT",
        @"Apple LiGothic",
        @"Apple LiSung",
        @"Apple Symbols",
        @"AppleGothic",
        @"AppleMyungjo",
        @"Arial Black",
        @"Arial Hebrew",
        @"Arial Narrow",
        @"Arial Rounded MT Bold",
        @"Arial Unicode MS",
        @"Arial",
        @"Ayuthaya",
        @"Baghdad",
        @"Baskerville",
        @"BiauKai",
        @"Big Caslon",
        @"Brush Script MT",
        @"Chalkboard",
        @"Chalkduster",
        @"Charcoal CY",
        @"Cochin",
        @"Comic Sans MS",
        @"Copperplate",
        @"Corsiva Hebrew",
        @"Courier New",
        @"Courier",
        @"DecoType Naskh",
        @"Devanagari MT",
        @"Didot",
        @"Euphemia UCAS",
        @"Futura",
        @"GB18030 Bitmap",
        @"Geeza Pro",
        @"Geneva CY",
        @"Geneva",
        @"Georgia",
        @"Gill Sans",
        @"Gujarati MT",
        @"GungSeo",
        @"Gurmukhi MT",
        @"HeadLineA",
        @"Hei",
        @"Heiti SC",
        @"Heiti TC",
        @"Helvetica CY",
        @"Helvetica Neue",
        @"Helvetica",
        @"Herculanum",
        @"Hiragino Kaku Gothic Pro",
        @"Hiragino Kaku Gothic ProN",
        @"Hiragino Kaku Gothic Std",
        @"Hiragino Kaku Gothic StdN",
        @"Hiragino Maru Gothic Monospaced",
        @"Hiragino Maru Gothic Pro",
        @"Hiragino Maru Gothic ProN",
        @"Hiragino Mincho Pro",
        @"Hiragino Mincho ProN",
        @"Hiragino Sans GB",
        @"Hoefler Text",
        @"Impact",
        @"InaiMathi",
        @"Kai",
        @"Kailasa",
        @"Kokonor",
        @"Krungthep",
        @"KufiStandardGK",
        @"LiHei Pro",
        @"LiSong Pro",
        @"Lucida Grande",
        @"Marker Felt",
        @"Menlo",
        @"Microsoft Sans Serif",
        @"Monaco",
        @"Mshtakan",
        @"Nadeem",
        @"New Peninim MT",
        @"Optima",
        @"Osaka",
        @"Papyrus",
        @"PCMyungjo",
        @"PilGi",
        @"Plantagenet Cherokee",
        @"Raanana",
        @"Sathu",
        @"Silom",
        @"Skia",
        @"Songti SC",
        @"Songti TC",
        @"STFangsong",
        @"STHeiti",
        @"STIXGeneral",
        @"STIXSizeOneSym",
        @"STKaiti",
        @"STSong",
        @"Symbol",
        @"Tahoma",
        @"Thonburi",
        @"Times New Roman",
        @"Times",
        @"Trebuchet MS",
        @"Verdana",
        @"Webdings",
        @"WebKit WeightWatcher",
        @"Wingdings 2",
        @"Wingdings 3",
        @"Wingdings",
        @"Zapf Dingbats",
        @"Zapfino",
        nil] retain];
    
    return fontFamilySet;
}

static NSSet *systemHiddenFontFamilySet()
{
    static NSSet *fontFamilySet = [[NSSet setWithObjects:
        @".LucidaGrandeUI",
        nil] retain];

    return fontFamilySet;
}

static IMP appKitAvailableFontFamiliesIMP;
static IMP appKitAvailableFontsIMP;

static NSArray *drt_NSFontManager_availableFontFamilies(id self, SEL _cmd)
{
    static NSArray *availableFontFamilies;
    if (availableFontFamilies)
        return availableFontFamilies;
    
    NSArray *availableFamilies = wtfCallIMP<id>(appKitAvailableFontFamiliesIMP, self, _cmd);

    NSMutableSet *prunedFamiliesSet = [NSMutableSet setWithArray:availableFamilies];
    [prunedFamiliesSet intersectSet:allowedFontFamilySet()];

    availableFontFamilies = [[prunedFamiliesSet allObjects] retain];
    return availableFontFamilies;
}

static NSArray *drt_NSFontManager_availableFonts(id self, SEL _cmd)
{
    static NSArray *availableFonts;
    if (availableFonts)
        return availableFonts;
    
    NSSet *allowedFamilies = allowedFontFamilySet();
    NSMutableArray *availableFontList = [[NSMutableArray alloc] initWithCapacity:[allowedFamilies count] * 2];
    for (NSString *fontFamily in allowedFontFamilySet()) {
        NSArray* fontsForFamily = [[NSFontManager sharedFontManager] availableMembersOfFontFamily:fontFamily];
        for (NSArray* fontInfo in fontsForFamily) {
            // Font name is the first entry in the array.
            [availableFontList addObject:[fontInfo objectAtIndex:0]];
        }
    }

    for (NSString *hiddenFontFamily in systemHiddenFontFamilySet()) {
        [availableFontList addObject:hiddenFontFamily];
    }

    availableFonts = availableFontList;
    return availableFonts;
}

static void swizzleNSFontManagerMethods()
{
    Method availableFontFamiliesMethod = class_getInstanceMethod(objc_getClass("NSFontManager"), @selector(availableFontFamilies));
    ASSERT(availableFontFamiliesMethod);
    if (!availableFontFamiliesMethod) {
        NSLog(@"Failed to swizzle the \"availableFontFamilies\" method on NSFontManager");
        return;
    }
    
    appKitAvailableFontFamiliesIMP = method_setImplementation(availableFontFamiliesMethod, (IMP)drt_NSFontManager_availableFontFamilies);

    Method availableFontsMethod = class_getInstanceMethod(objc_getClass("NSFontManager"), @selector(availableFonts));
    ASSERT(availableFontsMethod);
    if (!availableFontsMethod) {
        NSLog(@"Failed to swizzle the \"availableFonts\" method on NSFontManager");
        return;
    }
    
    appKitAvailableFontsIMP = method_setImplementation(availableFontsMethod, (IMP)drt_NSFontManager_availableFonts);
}

static void activateTestingFonts()
{
    static const char* fontFileNames[] = {
        "AHEM____.TTF",
        "WebKitWeightWatcher100.ttf",
        "WebKitWeightWatcher200.ttf",
        "WebKitWeightWatcher300.ttf",
        "WebKitWeightWatcher400.ttf",
        "WebKitWeightWatcher500.ttf",
        "WebKitWeightWatcher600.ttf",
        "WebKitWeightWatcher700.ttf",
        "WebKitWeightWatcher800.ttf",
        "WebKitWeightWatcher900.ttf",
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
        "SampleFont.sfont",
#endif
        0
    };

    NSMutableArray *fontURLs = [NSMutableArray array];
    NSURL *resourcesDirectory = [NSURL URLWithString:@"DumpRenderTree.resources" relativeToURL:[[NSBundle mainBundle] executableURL]];
    for (unsigned i = 0; fontFileNames[i]; ++i) {
        NSURL *fontURL = [resourcesDirectory URLByAppendingPathComponent:[NSString stringWithUTF8String:fontFileNames[i]]];
        [fontURLs addObject:[fontURL absoluteURL]];
    }

    CFArrayRef errors = 0;
    if (!CTFontManagerRegisterFontsForURLs((CFArrayRef)fontURLs, kCTFontManagerScopeProcess, &errors)) {
        NSLog(@"Failed to activate fonts: %@", errors);
        CFRelease(errors);
        exit(1);
    }
}

static void adjustFonts()
{
    swizzleNSFontManagerMethods();
    activateTestingFonts();
}

@interface DRTMockScroller : NSScroller
@end

@implementation DRTMockScroller

- (NSRect)rectForPart:(NSScrollerPart)partCode
{
    switch (partCode) {
    case NSScrollerKnob: {
        NSRect frameRect = [self frame];
        NSRect bounds = [self bounds];
        BOOL isHorizontal = frameRect.size.width > frameRect.size.height;
        CGFloat trackLength = isHorizontal ? bounds.size.width : bounds.size.height;
        CGFloat minKnobSize = isHorizontal ? bounds.size.height : bounds.size.width;
        CGFloat knobLength = max(minKnobSize, static_cast<CGFloat>(round(trackLength * [self knobProportion])));
        CGFloat knobPosition = static_cast<CGFloat>((round([self doubleValue] * (trackLength - knobLength))));
        
        if (isHorizontal)
            return NSMakeRect(bounds.origin.x + knobPosition, bounds.origin.y, knobLength, bounds.size.height);

        return NSMakeRect(bounds.origin.x, bounds.origin.y +  + knobPosition, bounds.size.width, knobLength);
    }
    }
    
    return [super rectForPart:partCode];
}

- (void)drawKnob
{
    if (![self isEnabled])
        return;

    NSRect knobRect = [self rectForPart:NSScrollerKnob];
    
    static NSColor *knobColor = [[NSColor colorWithDeviceRed:0x80 / 255.0 green:0x80 / 255.0 blue:0x80 / 255.0 alpha:1] retain];
    [knobColor set];

    NSRectFill(knobRect);
}

- (void)drawRect:(NSRect)dirtyRect
{
    static NSColor *trackColor = [[NSColor colorWithDeviceRed:0xC0 / 255.0 green:0xC0 / 255.0 blue:0xC0 / 255.0 alpha:1] retain];
    static NSColor *disabledTrackColor = [[NSColor colorWithDeviceRed:0xE0 / 255.0 green:0xE0 / 255.0 blue:0xE0 / 255.0 alpha:1] retain];

    if ([self isEnabled])
        [trackColor set];
    else
        [disabledTrackColor set];

    NSRectFill(dirtyRect);
    
    [self drawKnob];
}

@end

static void registerMockScrollbars()
{
    [WebDynamicScrollBarsView setCustomScrollerClass:[DRTMockScroller class]];
}

WebView *createWebViewAndOffscreenWindow()
{
    NSRect rect = NSMakeRect(0, 0, TestRunner::viewWidth, TestRunner::viewHeight);
    WebView *webView = [[WebView alloc] initWithFrame:rect frameName:nil groupName:@"org.webkit.DumpRenderTree"];
        
    [webView setUIDelegate:uiDelegate];
    [webView setFrameLoadDelegate:frameLoadDelegate];
    [webView setEditingDelegate:editingDelegate];
    [webView setResourceLoadDelegate:resourceLoadDelegate];
    [webView _setGeolocationProvider:[MockGeolocationProvider shared]];
    [webView _setDeviceOrientationProvider:[WebDeviceOrientationProviderMock shared]];
    [webView _setNotificationProvider:[MockWebNotificationProvider shared]];

    // Register the same schemes that Safari does
    [WebView registerURLSchemeAsLocal:@"feed"];
    [WebView registerURLSchemeAsLocal:@"feeds"];
    [WebView registerURLSchemeAsLocal:@"feedsearch"];
    
    [webView setContinuousSpellCheckingEnabled:YES];
    [webView setAutomaticQuoteSubstitutionEnabled:NO];
    [webView setAutomaticLinkDetectionEnabled:NO];
    [webView setAutomaticDashSubstitutionEnabled:NO];
    [webView setAutomaticTextReplacementEnabled:NO];
    [webView setAutomaticSpellingCorrectionEnabled:YES];
    [webView setGrammarCheckingEnabled:YES];

    [webView setDefersCallbacks:NO];
    [webView setInteractiveFormValidationEnabled:YES];
    [webView setValidationMessageTimerMagnification:-1];
    
    // To make things like certain NSViews, dragging, and plug-ins work, put the WebView a window, but put it off-screen so you don't see it.
    // Put it at -10000, -10000 in "flipped coordinates", since WebCore and the DOM use flipped coordinates.
    NSRect windowRect = NSOffsetRect(rect, -10000, [(NSScreen *)[[NSScreen screens] objectAtIndex:0] frame].size.height - rect.size.height + 10000);
    DumpRenderTreeWindow *window = [[DumpRenderTreeWindow alloc] initWithContentRect:windowRect styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:YES];

    [window setColorSpace:[[NSScreen mainScreen] colorSpace]];
    [window setCollectionBehavior:NSWindowCollectionBehaviorStationary];
    [[window contentView] addSubview:webView];
    [window orderBack:nil];
    [window setAutodisplay:NO];
    [window _setWindowResolution:1 displayIfChanged:YES];

    [window startListeningForAcceleratedCompositingChanges];
    
    // For reasons that are not entirely clear, the following pair of calls makes WebView handle its
    // dynamic scrollbars properly. Without it, every frame will always have scrollbars.
    NSBitmapImageRep *imageRep = [webView bitmapImageRepForCachingDisplayInRect:[webView bounds]];
    [webView cacheDisplayInRect:[webView bounds] toBitmapImageRep:imageRep];
        
    return webView;
}

static NSString *libraryPathForDumpRenderTree()
{
    //FIXME: This may not be sufficient to prevent interactions/crashes
    //when running more than one copy of DumpRenderTree.
    //See https://bugs.webkit.org/show_bug.cgi?id=10906
    char* dumpRenderTreeTemp = getenv("DUMPRENDERTREE_TEMP");
    if (dumpRenderTreeTemp)
        return [[NSFileManager defaultManager] stringWithFileSystemRepresentation:dumpRenderTreeTemp length:strlen(dumpRenderTreeTemp)];
    else
        return [@"~/Library/Application Support/DumpRenderTree" stringByExpandingTildeInPath];
}

// Called before each test.
static void resetDefaultsToConsistentValues()
{
    static const int NoFontSmoothing = 0;
    static const int BlueTintedAppearance = 1;

    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    [defaults setInteger:4 forKey:@"AppleAntiAliasingThreshold"]; // smallest font size to CG should perform antialiasing on
    [defaults setInteger:NoFontSmoothing forKey:@"AppleFontSmoothing"];
    [defaults setInteger:BlueTintedAppearance forKey:@"AppleAquaColorVariant"];
    [defaults setObject:@"0.709800 0.835300 1.000000" forKey:@"AppleHighlightColor"];
    [defaults setObject:@"0.500000 0.500000 0.500000" forKey:@"AppleOtherHighlightColor"];
    [defaults setObject:[NSArray arrayWithObject:@"en"] forKey:@"AppleLanguages"];
    [defaults setBool:YES forKey:WebKitEnableFullDocumentTeardownPreferenceKey];
    [defaults setBool:YES forKey:WebKitFullScreenEnabledPreferenceKey];
    [defaults setBool:YES forKey:@"UseWebKitWebInspector"];

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    [defaults setObject:[NSDictionary dictionaryWithObjectsAndKeys:
        @"notational", @"notationl",
        @"message", @"mesage",
        @"would", @"wouldn",
        @"welcome", @"wellcome",
        @"hello\nworld", @"hellolfworld",
        nil] forKey:@"NSTestCorrectionDictionary"];
#endif

    // Scrollbars are drawn either using AppKit (which uses NSUserDefaults) or using HIToolbox (which uses CFPreferences / kCFPreferencesAnyApplication / kCFPreferencesCurrentUser / kCFPreferencesAnyHost)
    [defaults setObject:@"DoubleMax" forKey:@"AppleScrollBarVariant"];
    RetainPtr<CFTypeRef> initialValue = CFPreferencesCopyValue(CFSTR("AppleScrollBarVariant"), kCFPreferencesAnyApplication, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
    CFPreferencesSetValue(CFSTR("AppleScrollBarVariant"), CFSTR("DoubleMax"), kCFPreferencesAnyApplication, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
#ifndef __LP64__
    // See <rdar://problem/6347388>.
    ThemeScrollBarArrowStyle style;
    GetThemeScrollBarArrowStyle(&style); // Force HIToolbox to read from CFPreferences
#endif


#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
    [defaults setBool:NO forKey:@"NSScrollAnimationEnabled"];
#else
    [defaults setBool:NO forKey:@"AppleScrollAnimationEnabled"];
#endif

    [defaults setBool:NO forKey:@"NSOverlayScrollersEnabled"];
    [defaults setObject:@"Always" forKey:@"AppleShowScrollBars"];

    if (initialValue)
        CFPreferencesSetValue(CFSTR("AppleScrollBarVariant"), initialValue.get(), kCFPreferencesAnyApplication, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);

    NSString *path = libraryPathForDumpRenderTree();
    [defaults setObject:[path stringByAppendingPathComponent:@"Databases"] forKey:WebDatabaseDirectoryDefaultsKey];
    [defaults setObject:[path stringByAppendingPathComponent:@"LocalStorage"] forKey:WebStorageDirectoryDefaultsKey];
    [defaults setObject:[path stringByAppendingPathComponent:@"LocalCache"] forKey:WebKitLocalCacheDefaultsKey];

    [defaults setBool:NO forKey:@"WebKitKerningAndLigaturesEnabledByDefault"];

    WebPreferences *preferences = [WebPreferences standardPreferences];

    [preferences setAllowUniversalAccessFromFileURLs:YES];
    [preferences setAllowFileAccessFromFileURLs:YES];
    [preferences setStandardFontFamily:@"Times"];
    [preferences setFixedFontFamily:@"Courier"];
    [preferences setSerifFontFamily:@"Times"];
    [preferences setSansSerifFontFamily:@"Helvetica"];
    [preferences setCursiveFontFamily:@"Apple Chancery"];
    [preferences setFantasyFontFamily:@"Papyrus"];
    [preferences setPictographFontFamily:@"Apple Color Emoji"];
    [preferences setDefaultFontSize:16];
    [preferences setDefaultFixedFontSize:13];
    [preferences setMinimumFontSize:0];
    [preferences setDefaultTextEncodingName:@"ISO-8859-1"];
    [preferences setJavaEnabled:NO];
    [preferences setJavaScriptEnabled:YES];
    [preferences setEditableLinkBehavior:WebKitEditableLinkOnlyLiveWithShiftKey];
    [preferences setTabsToLinks:NO];
    [preferences setDOMPasteAllowed:YES];
    [preferences setShouldPrintBackgrounds:YES];
    [preferences setCacheModel:WebCacheModelDocumentBrowser];
    [preferences setXSSAuditorEnabled:NO];
    [preferences setExperimentalNotificationsEnabled:NO];
    [preferences setPlugInsEnabled:YES];
    [preferences setTextAreasAreResizable:YES];

    [preferences setPrivateBrowsingEnabled:NO];
    [preferences setAuthorAndUserStylesEnabled:YES];
    [preferences setJavaScriptCanOpenWindowsAutomatically:YES];
    [preferences setJavaScriptCanAccessClipboard:YES];
    [preferences setOfflineWebApplicationCacheEnabled:YES];
    [preferences setDeveloperExtrasEnabled:NO];
    [preferences setJavaScriptExperimentsEnabled:YES];
    [preferences setLoadsImagesAutomatically:YES];
    [preferences setLoadsSiteIconsIgnoringImageLoadingPreference:NO];
    [preferences setFrameFlatteningEnabled:NO];
    [preferences setSpatialNavigationEnabled:NO];
    if (persistentUserStyleSheetLocation) {
        [preferences setUserStyleSheetLocation:[NSURL URLWithString:(NSString *)(persistentUserStyleSheetLocation.get())]];
        [preferences setUserStyleSheetEnabled:YES];
    } else
        [preferences setUserStyleSheetEnabled:NO];

    // The back/forward cache is causing problems due to layouts during transition from one page to another.
    // So, turn it off for now, but we might want to turn it back on some day.
    [preferences setUsesPageCache:NO];
    [preferences setAcceleratedCompositingEnabled:YES];
#if USE(CA) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    [preferences setCanvasUsesAcceleratedDrawing:YES];
    [preferences setAcceleratedDrawingEnabled:NO];
#endif
    [preferences setWebGLEnabled:NO];
    [preferences setCSSRegionsEnabled:YES];
    [preferences setCSSGridLayoutEnabled:NO];
    [preferences setUsePreHTML5ParserQuirks:NO];
    [preferences setAsynchronousSpellCheckingEnabled:NO];
    [preferences setMockScrollbarsEnabled:YES];
    [preferences setSeamlessIFramesEnabled:YES];

#if ENABLE(WEB_AUDIO)
    [preferences setWebAudioEnabled:YES];
#endif

    [preferences setScreenFontSubstitutionEnabled:YES];

    [WebPreferences _setCurrentNetworkLoaderSessionCookieAcceptPolicy:NSHTTPCookieAcceptPolicyOnlyFromMainDocumentDomain];
    
    TestRunner::setSerializeHTTPLoads(false);

    setlocale(LC_ALL, "");
}

// Called once on DumpRenderTree startup.
static void setDefaultsToConsistentValuesForTesting()
{
    // FIXME: We'd like to start with a clean state for every test, but this function can't be used more than once yet.
    [WebPreferences _switchNetworkLoaderToNewTestingSession];

    resetDefaultsToConsistentValues();

    NSString *path = libraryPathForDumpRenderTree();
    NSURLCache *sharedCache =
        [[NSURLCache alloc] initWithMemoryCapacity:1024 * 1024
                                      diskCapacity:0
                                          diskPath:[path stringByAppendingPathComponent:@"URLCache"]];
    [NSURLCache setSharedURLCache:sharedCache];
    [sharedCache release];
}

static void runThread(void* arg)
{
    static ThreadIdentifier previousId = 0;
    ThreadIdentifier currentId = currentThread();
    // Verify 2 successive threads do not get the same Id.
    ASSERT(previousId != currentId);
    previousId = currentId;
}

static void* runPthread(void* arg)
{
    runThread(arg);
    return 0;
}

static void testThreadIdentifierMap()
{
    // Imitate 'foreign' threads that are not created by WTF.
    pthread_t pthread;
    pthread_create(&pthread, 0, &runPthread, 0);
    pthread_join(pthread, 0);

    pthread_create(&pthread, 0, &runPthread, 0);
    pthread_join(pthread, 0);

    // Now create another thread using WTF. On OSX, it will have the same pthread handle
    // but should get a different ThreadIdentifier.
    createThread(runThread, 0, "DumpRenderTree: test");
}

static void allocateGlobalControllers()
{
    // FIXME: We should remove these and move to the ObjC standard [Foo sharedInstance] model
    gNavigationController = [[NavigationController alloc] init];
    frameLoadDelegate = [[FrameLoadDelegate alloc] init];
    uiDelegate = [[UIDelegate alloc] init];
    editingDelegate = [[EditingDelegate alloc] init];
    resourceLoadDelegate = [[ResourceLoadDelegate alloc] init];
    policyDelegate = [[PolicyDelegate alloc] init];
    historyDelegate = [[HistoryDelegate alloc] init];
    storageDelegate = [[StorageTrackerDelegate alloc] init];
    defaultPolicyDelegate = [[DefaultPolicyDelegate alloc] init];
}

// ObjC++ doens't seem to let me pass NSObject*& sadly.
static inline void releaseAndZero(NSObject** object)
{
    [*object release];
    *object = nil;
}

static void releaseGlobalControllers()
{
    releaseAndZero(&gNavigationController);
    releaseAndZero(&frameLoadDelegate);
    releaseAndZero(&editingDelegate);
    releaseAndZero(&resourceLoadDelegate);
    releaseAndZero(&uiDelegate);
    releaseAndZero(&policyDelegate);
    releaseAndZero(&storageDelegate);
}

static void initializeGlobalsFromCommandLineOptions(int argc, const char *argv[])
{
    struct option options[] = {
        {"notree", no_argument, &dumpTree, NO},
        {"pixel-tests", no_argument, &dumpPixelsForAllTests, YES},
        {"tree", no_argument, &dumpTree, YES},
        {"threaded", no_argument, &threaded, YES},
        {"complex-text", no_argument, &forceComplexText, YES},
        {"gc-between-tests", no_argument, &gcBetweenTests, YES},
        {"no-timeout", no_argument, &useTimeoutWatchdog, NO},
        {NULL, 0, NULL, 0}
    };
    
    int option;
    while ((option = getopt_long(argc, (char * const *)argv, "", options, NULL)) != -1) {
        switch (option) {
            case '?':   // unknown or ambiguous option
            case ':':   // missing argument
                exit(1);
                break;
        }
    }
}

static void addTestPluginsToPluginSearchPath(const char* executablePath)
{
    NSString *pwd = [[NSString stringWithUTF8String:executablePath] stringByDeletingLastPathComponent];
    [WebPluginDatabase setAdditionalWebPlugInPaths:[NSArray arrayWithObject:pwd]];
    [[WebPluginDatabase sharedDatabase] refresh];
}

static bool useLongRunningServerMode(int argc, const char *argv[])
{
    // This assumes you've already called getopt_long
    return (argc == optind+1 && strcmp(argv[optind], "-") == 0);
}

static void runTestingServerLoop()
{
    // When DumpRenderTree run in server mode, we just wait around for file names
    // to be passed to us and read each in turn, passing the results back to the client
    char filenameBuffer[2048];
    while (fgets(filenameBuffer, sizeof(filenameBuffer), stdin)) {
        char *newLineCharacter = strchr(filenameBuffer, '\n');
        if (newLineCharacter)
            *newLineCharacter = '\0';

        if (strlen(filenameBuffer) == 0)
            continue;

        runTest(filenameBuffer);
    }
}

static void prepareConsistentTestingEnvironment()
{
    poseAsClass("DumpRenderTreePasteboard", "NSPasteboard");
    poseAsClass("DumpRenderTreeEvent", "NSEvent");

    setDefaultsToConsistentValuesForTesting();
    adjustFonts();
    registerMockScrollbars();
    
    allocateGlobalControllers();
    
    makeLargeMallocFailSilently();

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    NSActivityOptions options = (NSActivityUserInitiatedAllowingIdleSystemSleep | NSActivityLatencyCritical) & ~(NSActivitySuddenTerminationDisabled | NSActivityAutomaticTerminationDisabled);
    static id assertion = [[[NSProcessInfo processInfo] beginActivityWithOptions:options reason:@"DumpRenderTree should not be subject to process suppression"] retain];
    ASSERT_UNUSED(assertion, assertion);
#endif
}

void dumpRenderTree(int argc, const char *argv[])
{
    initializeGlobalsFromCommandLineOptions(argc, argv);
    prepareConsistentTestingEnvironment();
    addTestPluginsToPluginSearchPath(argv[0]);

    if (forceComplexText)
        [WebView _setAlwaysUsesComplexTextCodePath:YES];

#if USE(APPKIT)
    [NSSound _setAlertType:0];
#endif

    WebView *webView = createWebViewAndOffscreenWindow();
    mainFrame = [webView mainFrame];

    [[NSURLCache sharedURLCache] removeAllCachedResponses];
    [WebCache empty];

    [NSURLRequest setAllowsAnyHTTPSCertificate:YES forHost:@"localhost"];
    [NSURLRequest setAllowsAnyHTTPSCertificate:YES forHost:@"127.0.0.1"];

    // http://webkit.org/b/32689
    testThreadIdentifierMap();

    if (threaded)
        startJavaScriptThreads();

    if (useLongRunningServerMode(argc, argv)) {
        printSeparators = YES;
        runTestingServerLoop();
    } else {
        printSeparators = optind < argc - 1;
        for (int i = optind; i != argc; ++i)
            runTest(argv[i]);
    }

    if (threaded)
        stopJavaScriptThreads();

    NSWindow *window = [webView window];
    [webView close];
    mainFrame = nil;

    // Work around problem where registering drag types leaves an outstanding
    // "perform selector" on the window, which retains the window. It's a bit
    // inelegant and perhaps dangerous to just blow them all away, but in practice
    // it probably won't cause any trouble (and this is just a test tool, after all).
    [NSObject cancelPreviousPerformRequestsWithTarget:window];
    
    [window close]; // releases when closed
    [webView release];
    
    releaseGlobalControllers();
    
    [DumpRenderTreePasteboard releaseLocalPasteboards];

    // FIXME: This should be moved onto TestRunner and made into a HashSet
    if (disallowedURLs) {
        CFRelease(disallowedURLs);
        disallowedURLs = 0;
    }
}

int main(int argc, const char *argv[])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [DumpRenderTreeApplication sharedApplication]; // Force AppKit to init itself
    dumpRenderTree(argc, argv);
    [WebCoreStatistics garbageCollectJavaScriptObjects];
    [WebCoreStatistics emptyCache]; // Otherwise SVGImages trigger false positives for Frame/Node counts
    if (JSC::Options::logHeapStatisticsAtExit())
        JSC::HeapStatistics::reportSuccess();
    [pool release];
    return 0;
}

static NSInteger compareHistoryItems(id item1, id item2, void *context)
{
    return [[item1 target] caseInsensitiveCompare:[item2 target]];
}

static NSData *dumpAudio()
{
    const char *encodedAudioData = gTestRunner->encodedAudioData().c_str();
    
    NSData *data = [NSData dataWithBytes:encodedAudioData length:gTestRunner->encodedAudioData().length()];
    return data;
}

static void dumpHistoryItem(WebHistoryItem *item, int indent, BOOL current)
{
    int start = 0;
    if (current) {
        printf("curr->");
        start = 6;
    }
    for (int i = start; i < indent; i++)
        putchar(' ');
    
    NSString *urlString = [item URLString];
    if ([[NSURL URLWithString:urlString] isFileURL]) {
        NSRange range = [urlString rangeOfString:@"/LayoutTests/"];
        urlString = [@"(file test):" stringByAppendingString:[urlString substringFromIndex:(range.length + range.location)]];
    }
    
    printf("%s", [urlString UTF8String]);
    NSString *target = [item target];
    if (target && [target length] > 0)
        printf(" (in frame \"%s\")", [target UTF8String]);
    if ([item isTargetItem])
        printf("  **nav target**");
    putchar('\n');
    NSArray *kids = [item children];
    if (kids) {
        // must sort to eliminate arbitrary result ordering which defeats reproducible testing
        kids = [kids sortedArrayUsingFunction:&compareHistoryItems context:nil];
        for (unsigned i = 0; i < [kids count]; i++)
            dumpHistoryItem([kids objectAtIndex:i], indent+4, NO);
    }
}

static void dumpFrameScrollPosition(WebFrame *f)
{
    WebScriptObject* scriptObject = [f windowObject];
    NSPoint scrollPosition = NSMakePoint(
        [[scriptObject valueForKey:@"pageXOffset"] floatValue],
        [[scriptObject valueForKey:@"pageYOffset"] floatValue]);
    if (ABS(scrollPosition.x) > 0.00000001 || ABS(scrollPosition.y) > 0.00000001) {
        if ([f parentFrame] != nil)
            printf("frame '%s' ", [[f name] UTF8String]);
        printf("scrolled to %.f,%.f\n", scrollPosition.x, scrollPosition.y);
    }

    if (gTestRunner->dumpChildFrameScrollPositions()) {
        NSArray *kids = [f childFrames];
        if (kids)
            for (unsigned i = 0; i < [kids count]; i++)
                dumpFrameScrollPosition([kids objectAtIndex:i]);
    }
}

static NSString *dumpFramesAsText(WebFrame *frame)
{
    DOMDocument *document = [frame DOMDocument];
    DOMElement *documentElement = [document documentElement];

    if (!documentElement)
        return @"";

    NSMutableString *result = [[[NSMutableString alloc] init] autorelease];

    // Add header for all but the main frame.
    if ([frame parentFrame])
        result = [NSMutableString stringWithFormat:@"\n--------\nFrame: '%@'\n--------\n", [frame name]];

    [result appendFormat:@"%@\n", [documentElement innerText]];

    if (gTestRunner->dumpChildFramesAsText()) {
        NSArray *kids = [frame childFrames];
        if (kids) {
            for (unsigned i = 0; i < [kids count]; i++)
                [result appendString:dumpFramesAsText([kids objectAtIndex:i])];
        }
    }

    return result;
}

static NSData *dumpFrameAsPDF(WebFrame *frame)
{
    if (!frame)
        return nil;

    // Sadly we have to dump to a file and then read from that file again
    // +[NSPrintOperation PDFOperationWithView:insideRect:] requires a rect and prints to a single page
    // likewise +[NSView dataWithPDFInsideRect:] also prints to a single continuous page
    // The goal of this function is to test "real" printing across multiple pages.
    // FIXME: It's possible there might be printing SPI to let us print a multi-page PDF to an NSData object
    NSString *path = [libraryPathForDumpRenderTree() stringByAppendingPathComponent:@"test.pdf"];

    NSMutableDictionary *printInfoDict = [NSMutableDictionary dictionaryWithDictionary:[[NSPrintInfo sharedPrintInfo] dictionary]];
    [printInfoDict setObject:NSPrintSaveJob forKey:NSPrintJobDisposition];
    [printInfoDict setObject:path forKey:NSPrintSavePath];

    NSPrintInfo *printInfo = [[NSPrintInfo alloc] initWithDictionary:printInfoDict];
    [printInfo setHorizontalPagination:NSAutoPagination];
    [printInfo setVerticalPagination:NSAutoPagination];
    [printInfo setVerticallyCentered:NO];

    NSPrintOperation *printOperation = [NSPrintOperation printOperationWithView:[frame frameView] printInfo:printInfo];
    [printOperation setShowPanels:NO];
    [printOperation runOperation];

    [printInfo release];

    NSData *pdfData = [NSData dataWithContentsOfFile:path];
    [[NSFileManager defaultManager] removeFileAtPath:path handler:nil];

    return pdfData;
}

static void dumpBackForwardListForWebView(WebView *view)
{
    printf("\n============== Back Forward List ==============\n");
    WebBackForwardList *bfList = [view backForwardList];

    // Print out all items in the list after prevTestBFItem, which was from the previous test
    // Gather items from the end of the list, the print them out from oldest to newest
    NSMutableArray *itemsToPrint = [[NSMutableArray alloc] init];
    for (int i = [bfList forwardListCount]; i > 0; i--) {
        WebHistoryItem *item = [bfList itemAtIndex:i];
        // something is wrong if the item from the last test is in the forward part of the b/f list
        assert(item != prevTestBFItem);
        [itemsToPrint addObject:item];
    }
            
    assert([bfList currentItem] != prevTestBFItem);
    [itemsToPrint addObject:[bfList currentItem]];
    int currentItemIndex = [itemsToPrint count] - 1;

    for (int i = -1; i >= -[bfList backListCount]; i--) {
        WebHistoryItem *item = [bfList itemAtIndex:i];
        if (item == prevTestBFItem)
            break;
        [itemsToPrint addObject:item];
    }

    for (int i = [itemsToPrint count]-1; i >= 0; i--)
        dumpHistoryItem([itemsToPrint objectAtIndex:i], 8, i == currentItemIndex);

    [itemsToPrint release];
    printf("===============================================\n");
}

static void sizeWebViewForCurrentTest()
{
    // W3C SVG tests expect to be 480x360
    bool isSVGW3CTest = (gTestRunner->testPathOrURL().find("svg/W3C-SVG-1.1") != string::npos);
    if (isSVGW3CTest)
        [[mainFrame webView] setFrameSize:NSMakeSize(TestRunner::w3cSVGViewWidth, TestRunner::w3cSVGViewHeight)];
    else
        [[mainFrame webView] setFrameSize:NSMakeSize(TestRunner::viewWidth, TestRunner::viewHeight)];
}

static const char *methodNameStringForFailedTest()
{
    const char *errorMessage;
    if (gTestRunner->dumpAsText())
        errorMessage = "[documentElement innerText]";
    else if (gTestRunner->dumpDOMAsWebArchive())
        errorMessage = "[[mainFrame DOMDocument] webArchive]";
    else if (gTestRunner->dumpSourceAsWebArchive())
        errorMessage = "[[mainFrame dataSource] webArchive]";
    else
        errorMessage = "[mainFrame renderTreeAsExternalRepresentation]";

    return errorMessage;
}

static void dumpBackForwardListForAllWindows()
{
    CFArrayRef openWindows = (CFArrayRef)[DumpRenderTreeWindow openWindows];
    unsigned count = CFArrayGetCount(openWindows);
    for (unsigned i = 0; i < count; i++) {
        NSWindow *window = (NSWindow *)CFArrayGetValueAtIndex(openWindows, i);
        WebView *webView = [[[window contentView] subviews] objectAtIndex:0];
        dumpBackForwardListForWebView(webView);
    }
}

static void invalidateAnyPreviousWaitToDumpWatchdog()
{
    if (waitToDumpWatchdog) {
        CFRunLoopTimerInvalidate(waitToDumpWatchdog);
        CFRelease(waitToDumpWatchdog);
        waitToDumpWatchdog = 0;
    }
}

void setWaitToDumpWatchdog(CFRunLoopTimerRef timer)
{
    ASSERT(timer);
    ASSERT(shouldSetWaitToDumpWatchdog());
    waitToDumpWatchdog = timer;
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), waitToDumpWatchdog, kCFRunLoopCommonModes);
}

bool shouldSetWaitToDumpWatchdog()
{
    return !waitToDumpWatchdog && useTimeoutWatchdog;
}

void dump()
{
    invalidateAnyPreviousWaitToDumpWatchdog();
    ASSERT(!gTestRunner->hasPendingWebNotificationClick());

    if (dumpTree) {
        NSString *resultString = nil;
        NSData *resultData = nil;
        NSString *resultMimeType = @"text/plain";

        if ([[[mainFrame dataSource] _responseMIMEType] isEqualToString:@"text/plain"]) {
            gTestRunner->setDumpAsText(true);
            gTestRunner->setGeneratePixelResults(false);
        }
        if (gTestRunner->dumpAsAudio()) {
            resultData = dumpAudio();
            resultMimeType = @"audio/wav";
        } else if (gTestRunner->dumpAsText()) {
            resultString = dumpFramesAsText(mainFrame);
        } else if (gTestRunner->dumpAsPDF()) {
            resultData = dumpFrameAsPDF(mainFrame);
            resultMimeType = @"application/pdf";
        } else if (gTestRunner->dumpDOMAsWebArchive()) {
            WebArchive *webArchive = [[mainFrame DOMDocument] webArchive];
            resultString = HardAutorelease(createXMLStringFromWebArchiveData((CFDataRef)[webArchive data]));
            resultMimeType = @"application/x-webarchive";
        } else if (gTestRunner->dumpSourceAsWebArchive()) {
            WebArchive *webArchive = [[mainFrame dataSource] webArchive];
            resultString = HardAutorelease(createXMLStringFromWebArchiveData((CFDataRef)[webArchive data]));
            resultMimeType = @"application/x-webarchive";
        } else
            resultString = [mainFrame renderTreeAsExternalRepresentationForPrinting:gTestRunner->isPrinting()];

        if (resultString && !resultData)
            resultData = [resultString dataUsingEncoding:NSUTF8StringEncoding];

        printf("Content-Type: %s\n", [resultMimeType UTF8String]);

        if (gTestRunner->dumpAsAudio())
            printf("Content-Transfer-Encoding: base64\n");

        WTF::FastMallocStatistics mallocStats = WTF::fastMallocStatistics();
        printf("DumpMalloc: %li\n", mallocStats.committedVMBytes);

        if (resultData) {
            fwrite([resultData bytes], 1, [resultData length], stdout);

            if (!gTestRunner->dumpAsText() && !gTestRunner->dumpDOMAsWebArchive() && !gTestRunner->dumpSourceAsWebArchive())
                dumpFrameScrollPosition(mainFrame);

            if (gTestRunner->dumpBackForwardList())
                dumpBackForwardListForAllWindows();
        } else
            printf("ERROR: nil result from %s", methodNameStringForFailedTest());

        // Stop the watchdog thread before we leave this test to make sure it doesn't
        // fire in between tests causing the next test to fail.
        // This is a speculative fix for: https://bugs.webkit.org/show_bug.cgi?id=32339
        invalidateAnyPreviousWaitToDumpWatchdog();

        if (printSeparators) {
            puts("#EOF");       // terminate the content block
            fputs("#EOF\n", stderr);
        }            
    }

    if (dumpPixelsForCurrentTest && gTestRunner->generatePixelResults())
        // FIXME: when isPrinting is set, dump the image with page separators.
        dumpWebViewAsPixelsAndCompareWithExpected(gTestRunner->expectedPixelHash());

    puts("#EOF");   // terminate the (possibly empty) pixels block

    fflush(stdout);
    fflush(stderr);

    done = YES;
}

static bool shouldLogFrameLoadDelegates(const char* pathOrURL)
{
    return strstr(pathOrURL, "loading/");
}

static bool shouldLogHistoryDelegates(const char* pathOrURL)
{
    return strstr(pathOrURL, "globalhistory/");
}

static bool shouldOpenWebInspector(const char* pathOrURL)
{
    return strstr(pathOrURL, "inspector/");
}

static bool shouldDumpAsText(const char* pathOrURL)
{
    return strstr(pathOrURL, "dumpAsText/");
}

static bool shouldEnableDeveloperExtras(const char* pathOrURL)
{
    return true;
}

static void resetWebViewToConsistentStateBeforeTesting()
{
    WebView *webView = [mainFrame webView];
    [webView setEditable:NO];
    [(EditingDelegate *)[webView editingDelegate] setAcceptsEditing:YES];
    [webView makeTextStandardSize:nil];
    [webView resetPageZoom:nil];
    [webView _scaleWebView:1.0 atOrigin:NSZeroPoint];
    [webView _setCustomBackingScaleFactor:0];
    [webView setTabKeyCyclesThroughElements:YES];
    [webView setPolicyDelegate:defaultPolicyDelegate];
    [policyDelegate setPermissive:NO];
    [policyDelegate setControllerToNotifyDone:0];
    [frameLoadDelegate resetToConsistentState];
    [webView _setDashboardBehavior:WebDashboardBehaviorUseBackwardCompatibilityMode to:NO];
    [webView _clearMainFrameName];
    [[webView undoManager] removeAllActions];
    [WebView _removeAllUserContentFromGroup:[webView groupName]];
    [[webView window] setAutodisplay:NO];
    [webView setTracksRepaints:NO];
    
    resetDefaultsToConsistentValues();

    if (gTestRunner) {
        WebCoreTestSupport::resetInternalsObject([mainFrame globalContext]);
        // in the case that a test using the chrome input field failed, be sure to clean up for the next test
        gTestRunner->removeChromeInputField();
    }

    [webView setContinuousSpellCheckingEnabled:YES];
    [webView setAutomaticQuoteSubstitutionEnabled:NO];
    [webView setAutomaticLinkDetectionEnabled:NO];
    [webView setAutomaticDashSubstitutionEnabled:NO];
    [webView setAutomaticTextReplacementEnabled:NO];
    [webView setAutomaticSpellingCorrectionEnabled:YES];
    [webView setGrammarCheckingEnabled:YES];

    [WebView _setUsesTestModeFocusRingColor:YES];
    [WebView _resetOriginAccessWhitelists];
    [WebView _setAllowsRoundingHacks:NO];

    [[MockGeolocationProvider shared] stopTimer];
    [[MockWebNotificationProvider shared] reset];
    
    // Clear the contents of the general pasteboard
    [[NSPasteboard generalPasteboard] declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];

    [mainFrame _clearOpener];
}

static void runTest(const string& inputLine)
{
    ASSERT(!inputLine.empty());

    TestCommand command = parseInputLine(inputLine);
    const string& pathOrURL = command.pathOrURL;
    dumpPixelsForCurrentTest = command.shouldDumpPixels || dumpPixelsForAllTests;

    NSString *pathOrURLString = [NSString stringWithUTF8String:pathOrURL.c_str()];
    if (!pathOrURLString) {
        fprintf(stderr, "Failed to parse \"%s\" as UTF-8\n", pathOrURL.c_str());
        return;
    }

    NSURL *url;
    if ([pathOrURLString hasPrefix:@"http://"] || [pathOrURLString hasPrefix:@"https://"] || [pathOrURLString hasPrefix:@"file://"])
        url = [NSURL URLWithString:pathOrURLString];
    else
        url = [NSURL fileURLWithPath:pathOrURLString];
    if (!url) {
        fprintf(stderr, "Failed to parse \"%s\" as a URL\n", pathOrURL.c_str());
        return;
    }

    const string testURL([[url absoluteString] UTF8String]);
    
    resetWebViewToConsistentStateBeforeTesting();

    gTestRunner = TestRunner::create(testURL, command.expectedPixelHash);
    topLoadingFrame = nil;
    ASSERT(!draggingInfo); // the previous test should have called eventSender.mouseUp to drop!
    releaseAndZero(&draggingInfo);
    done = NO;

    sizeWebViewForCurrentTest();
    gTestRunner->setIconDatabaseEnabled(false);

    if (disallowedURLs)
        CFSetRemoveAllValues(disallowedURLs);
    if (shouldLogFrameLoadDelegates(pathOrURL.c_str()))
        gTestRunner->setDumpFrameLoadCallbacks(true);

    if (shouldLogHistoryDelegates(pathOrURL.c_str()))
        [[mainFrame webView] setHistoryDelegate:historyDelegate];
    else
        [[mainFrame webView] setHistoryDelegate:nil];

    if (shouldEnableDeveloperExtras(pathOrURL.c_str())) {
        gTestRunner->setDeveloperExtrasEnabled(true);
        if (shouldOpenWebInspector(pathOrURL.c_str()))
            gTestRunner->showWebInspector();
        if (shouldDumpAsText(pathOrURL.c_str())) {
            gTestRunner->setDumpAsText(true);
            gTestRunner->setGeneratePixelResults(false);
        }
    }

    if ([WebHistory optionalSharedHistory])
        [WebHistory setOptionalSharedHistory:nil];
    lastMousePosition = NSZeroPoint;
    lastClickPosition = NSZeroPoint;

    [prevTestBFItem release];
    prevTestBFItem = [[[[mainFrame webView] backForwardList] currentItem] retain];

    WorkQueue::shared()->clear();
    WorkQueue::shared()->setFrozen(false);

    bool ignoreWebCoreNodeLeaks = shouldIgnoreWebCoreNodeLeaks(testURL);
    if (ignoreWebCoreNodeLeaks)
        [WebCoreStatistics startIgnoringWebCoreNodeLeaks];

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [mainFrame loadRequest:[NSURLRequest requestWithURL:url]];
    [pool release];

    while (!done) {
        pool = [[NSAutoreleasePool alloc] init];
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantPast]]; 
        [pool release];
    }

    pool = [[NSAutoreleasePool alloc] init];
    [EventSendingController clearSavedEvents];
    [[mainFrame webView] setSelectedDOMRange:nil affinity:NSSelectionAffinityDownstream];

    WorkQueue::shared()->clear();

    if (gTestRunner->closeRemainingWindowsWhenComplete()) {
        NSArray* array = [DumpRenderTreeWindow openWindows];

        unsigned count = [array count];
        for (unsigned i = 0; i < count; i++) {
            NSWindow *window = [array objectAtIndex:i];

            // Don't try to close the main window
            if (window == [[mainFrame webView] window])
                continue;
            
            WebView *webView = [[[window contentView] subviews] objectAtIndex:0];

            [webView close];
            [window close];
        }
    }

    // If developer extras enabled Web Inspector may have been open by the test.
    if (shouldEnableDeveloperExtras(pathOrURL.c_str())) {
        gTestRunner->closeWebInspector();
        gTestRunner->setDeveloperExtrasEnabled(false);
    }

    resetWebViewToConsistentStateBeforeTesting();

    [mainFrame loadHTMLString:@"<html></html>" baseURL:[NSURL URLWithString:@"about:blank"]];
    [mainFrame stopLoading];

    [pool release];

    // We should only have our main window left open when we're done
    ASSERT(CFArrayGetCount(openWindowsRef) == 1);
    ASSERT(CFArrayGetValueAtIndex(openWindowsRef, 0) == [[mainFrame webView] window]);

    gTestRunner.clear();

    if (ignoreWebCoreNodeLeaks)
        [WebCoreStatistics stopIgnoringWebCoreNodeLeaks];

    if (gcBetweenTests)
        [WebCoreStatistics garbageCollectJavaScriptObjects];
}

void displayWebView()
{
    WebView *webView = [mainFrame webView];
    [webView display];
    
    [webView setTracksRepaints:YES];
    [webView resetTrackedRepaints];
}

@implementation DumpRenderTreeEvent

+ (NSPoint)mouseLocation
{
    return [[[mainFrame webView] window] convertBaseToScreen:lastMousePosition];
}

@end

@implementation DumpRenderTreeApplication

- (BOOL)isRunning
{
    // <rdar://problem/7686123> Java plug-in freezes unless NSApplication is running
    return YES;
}

@end
