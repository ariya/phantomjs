/*
 * Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#import "WebIconDatabaseInternal.h"

#import "WebIconDatabaseClient.h"
#import "WebIconDatabaseDelegate.h"
#import "WebKitLogging.h"
#import "WebKitNSStringExtras.h"
#import "WebNSFileManagerExtras.h"
#import "WebNSNotificationCenterExtras.h"
#import "WebNSURLExtras.h"
#import "WebPreferencesPrivate.h"
#import "WebTypesInternal.h"
#import <WebCore/IconDatabase.h>
#import <WebCore/Image.h>
#import <WebCore/IntSize.h>
#import <WebCore/RunLoop.h>
#import <WebCore/SharedBuffer.h>
#import <WebCore/ThreadCheck.h>
#import <runtime/InitializeThreading.h>
#import <wtf/MainThread.h>

using namespace WebCore;

NSString * const WebIconDatabaseVersionKey =    @"WebIconDatabaseVersion";
NSString * const WebURLToIconURLKey =           @"WebSiteURLToIconURLKey";

NSString *WebIconDatabaseDidAddIconNotification =          @"WebIconDatabaseDidAddIconNotification";
NSString *WebIconNotificationUserInfoURLKey =              @"WebIconNotificationUserInfoURLKey";
NSString *WebIconDatabaseDidRemoveAllIconsNotification =   @"WebIconDatabaseDidRemoveAllIconsNotification";

NSString *WebIconDatabaseDirectoryDefaultsKey = @"WebIconDatabaseDirectoryDefaultsKey";
NSString *WebIconDatabaseEnabledDefaultsKey =   @"WebIconDatabaseEnabled";

NSString *WebIconDatabasePath = @"~/Library/Icons";

NSSize WebIconSmallSize = {16, 16};
NSSize WebIconMediumSize = {32, 32};
NSSize WebIconLargeSize = {128, 128};

#define UniqueFilePathSize (34)

static WebIconDatabaseClient* defaultClient()
{
#if ENABLE(ICONDATABASE)
    static WebIconDatabaseClient* defaultClient = new WebIconDatabaseClient();
    return defaultClient;
#else
    return 0;
#endif
}

@interface WebIconDatabase (WebReallyInternal)
- (void)_sendNotificationForURL:(NSString *)URL;
- (void)_sendDidRemoveAllIconsNotification;
- (NSImage *)_iconForFileURL:(NSString *)fileURL withSize:(NSSize)size;
- (void)_resetCachedWebPreferences:(NSNotification *)notification;
- (NSImage *)_largestIconFromDictionary:(NSMutableDictionary *)icons;
- (NSMutableDictionary *)_iconsBySplittingRepresentationsOfIcon:(NSImage *)icon;
- (NSImage *)_iconFromDictionary:(NSMutableDictionary *)icons forSize:(NSSize)size cache:(BOOL)cache;
- (void)_scaleIcon:(NSImage *)icon toSize:(NSSize)size;
- (NSString *)_databaseDirectory;
@end

@implementation WebIconDatabase

+ (void)initialize
{
    JSC::initializeThreading();
    WTF::initializeMainThreadToProcessMainThread();
    WebCore::RunLoop::initializeMainRunLoop();
}

+ (WebIconDatabase *)sharedIconDatabase
{
    static WebIconDatabase *database = nil;
    if (!database)
        database = [[WebIconDatabase alloc] init];
    return database;
}

- (id)init
{
    self = [super init];
    if (!self)
        return nil;
    WebCoreThreadViolationCheckRoundOne();
        
    _private = [[WebIconDatabasePrivate alloc] init];
    
    // Check the user defaults and see if the icon database should even be enabled.
    // Inform the bridge and, if we're disabled, bail from init right here
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    // <rdar://problem/4741419> - IconDatabase should be disabled by default
    NSDictionary *initialDefaults = [[NSDictionary alloc] initWithObjectsAndKeys:[NSNumber numberWithBool:YES], WebIconDatabaseEnabledDefaultsKey, nil];
    [defaults registerDefaults:initialDefaults];
    [initialDefaults release];
    BOOL enabled = [defaults boolForKey:WebIconDatabaseEnabledDefaultsKey];
    iconDatabase().setEnabled(enabled);
    if (enabled)
        [self _startUpIconDatabase];
    return self;
}

- (NSImage *)iconForURL:(NSString *)URL withSize:(NSSize)size cache:(BOOL)cache
{
    ASSERT_MAIN_THREAD();
    ASSERT(size.width);
    ASSERT(size.height);

    if (!URL || ![self isEnabled])
        return [self defaultIconForURL:URL withSize:size];

    // FIXME - <rdar://problem/4697934> - Move the handling of FileURLs to WebCore and implement in ObjC++
    if ([URL _webkit_isFileURL])
        return [self _iconForFileURL:URL withSize:size];
    
    if (Image* image = iconDatabase().synchronousIconForPageURL(URL, IntSize(size)))
        if (NSImage *icon = webGetNSImage(image, size))
            return icon;
    return [self defaultIconForURL:URL withSize:size];
}

- (NSImage *)iconForURL:(NSString *)URL withSize:(NSSize)size
{
    return [self iconForURL:URL withSize:size cache:YES];
}

- (NSString *)iconURLForURL:(NSString *)URL
{
    if (![self isEnabled])
        return nil;
    ASSERT_MAIN_THREAD();

    return iconDatabase().synchronousIconURLForPageURL(URL);
}

- (NSImage *)defaultIconWithSize:(NSSize)size
{
    ASSERT_MAIN_THREAD();
    ASSERT(size.width);
    ASSERT(size.height);
    
    Image* image = iconDatabase().defaultIcon(IntSize(size));
    return image ? image->getNSImage() : nil;
}

- (NSImage *)defaultIconForURL:(NSString *)URL withSize:(NSSize)size
{
    if (_private->delegateImplementsDefaultIconForURL)
        return [_private->delegate webIconDatabase:self defaultIconForURL:URL withSize:size];
    return [self defaultIconWithSize:size];
}

- (void)retainIconForURL:(NSString *)URL
{
    ASSERT_MAIN_THREAD();
    ASSERT(URL);
    if (![self isEnabled])
        return;

    iconDatabase().retainIconForPageURL(URL);
}

- (void)releaseIconForURL:(NSString *)pageURL
{
    ASSERT_MAIN_THREAD();
    ASSERT(pageURL);
    if (![self isEnabled])
        return;

    iconDatabase().releaseIconForPageURL(pageURL);
}

+ (void)delayDatabaseCleanup
{
    ASSERT_MAIN_THREAD();

    IconDatabase::delayDatabaseCleanup();
}

+ (void)allowDatabaseCleanup
{
    ASSERT_MAIN_THREAD();

    IconDatabase::allowDatabaseCleanup();
}

- (void)setDelegate:(id)delegate
{
    _private->delegate = delegate;
    _private->delegateImplementsDefaultIconForURL = [delegate respondsToSelector:@selector(webIconDatabase:defaultIconForURL:withSize:)];
}

- (id)delegate
{
    return _private->delegate;
}

@end


@implementation WebIconDatabase (WebPendingPublic)

- (BOOL)isEnabled
{
    return iconDatabase().isEnabled();
}

- (void)setEnabled:(BOOL)flag
{
    BOOL currentlyEnabled = [self isEnabled];
    if (currentlyEnabled && !flag) {
        iconDatabase().setEnabled(false);
        [self _shutDownIconDatabase];
    } else if (!currentlyEnabled && flag) {
        iconDatabase().setEnabled(true);
        [self _startUpIconDatabase];
    }
}

- (void)removeAllIcons
{
    ASSERT_MAIN_THREAD();
    if (![self isEnabled])
        return;

    // Via the IconDatabaseClient interface, removeAllIcons() will send the WebIconDatabaseDidRemoveAllIconsNotification
    iconDatabase().removeAllIcons();
}

@end

@implementation WebIconDatabase (WebPrivate)

+ (void)_checkIntegrityBeforeOpening
{
    IconDatabase::checkIntegrityBeforeOpening();
}

@end

@implementation WebIconDatabase (WebInternal)

- (void)_sendNotificationForURL:(NSString *)URL
{
    ASSERT(URL);
    
    NSDictionary *userInfo = [NSDictionary dictionaryWithObject:URL
                                                         forKey:WebIconNotificationUserInfoURLKey];
                                                         
    [[NSNotificationCenter defaultCenter] postNotificationOnMainThreadWithName:WebIconDatabaseDidAddIconNotification
                                                        object:self
                                                      userInfo:userInfo];
}

- (void)_sendDidRemoveAllIconsNotification
{
    [[NSNotificationCenter defaultCenter] postNotificationOnMainThreadWithName:WebIconDatabaseDidRemoveAllIconsNotification
                                                        object:self
                                                      userInfo:nil];
}

- (void)_startUpIconDatabase
{
    iconDatabase().setClient(defaultClient());
    
    // Figure out the directory we should be using for the icon.db
    NSString *databaseDirectory = [self _databaseDirectory];
    
    // Rename legacy icon database files to the new icon database name
    BOOL isDirectory = NO;
    NSString *legacyDB = [databaseDirectory stringByAppendingPathComponent:@"icon.db"];
    NSFileManager *defaultManager = [NSFileManager defaultManager];
    if ([defaultManager fileExistsAtPath:legacyDB isDirectory:&isDirectory] && !isDirectory) {
        NSString *newDB = [databaseDirectory stringByAppendingPathComponent:IconDatabase::defaultDatabaseFilename()];
        if (![defaultManager fileExistsAtPath:newDB])
            rename([legacyDB fileSystemRepresentation], [newDB fileSystemRepresentation]);
    }
    
    // Set the private browsing pref then open the WebCore icon database
    iconDatabase().setPrivateBrowsingEnabled([[WebPreferences standardPreferences] privateBrowsingEnabled]);
    if (!iconDatabase().open(databaseDirectory, IconDatabase::defaultDatabaseFilename()))
        LOG_ERROR("Unable to open icon database");
    
    // Register for important notifications
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(_applicationWillTerminate:)
                                                 name:NSApplicationWillTerminateNotification
                                               object:NSApp];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(_resetCachedWebPreferences:)
                                                 name:WebPreferencesChangedInternalNotification
                                               object:nil];
}

- (void)_shutDownIconDatabase
{
    // Unregister for important notifications
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:NSApplicationWillTerminateNotification
                                                  object:NSApp];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:WebPreferencesChangedInternalNotification
                                                  object:nil];
}

- (void)_applicationWillTerminate:(NSNotification *)notification
{
    iconDatabase().close();
}

- (NSImage *)_iconForFileURL:(NSString *)file withSize:(NSSize)size
{
    ASSERT_MAIN_THREAD();
    ASSERT(size.width);
    ASSERT(size.height);

    NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
    NSString *path = [[NSURL _web_URLWithDataAsString:file] path];
    NSString *suffix = [path pathExtension];
    NSImage *icon = nil;
    
    if ([suffix _webkit_isCaseInsensitiveEqualToString:@"htm"] || [suffix _webkit_isCaseInsensitiveEqualToString:@"html"]) {
        if (!_private->htmlIcons) {
            icon = [workspace iconForFileType:@"html"];
            _private->htmlIcons = [[self _iconsBySplittingRepresentationsOfIcon:icon] retain];
        }
        icon = [self _iconFromDictionary:_private->htmlIcons forSize:size cache:YES];
    } else {
        if (!path || ![path isAbsolutePath]) {
            // Return the generic icon when there is no path.
            icon = [workspace iconForFileType:NSFileTypeForHFSTypeCode(kGenericDocumentIcon)];
        } else {
            icon = [workspace iconForFile:path];
        }
        [self _scaleIcon:icon toSize:size];
    }

    return icon;
}

- (void)_resetCachedWebPreferences:(NSNotification *)notification
{
    BOOL privateBrowsingEnabledNow = [[WebPreferences standardPreferences] privateBrowsingEnabled];
    iconDatabase().setPrivateBrowsingEnabled(privateBrowsingEnabledNow);
}

- (NSImage *)_largestIconFromDictionary:(NSMutableDictionary *)icons
{
    ASSERT(icons);
    
    NSEnumerator *enumerator = [icons keyEnumerator];
    NSValue *currentSize, *largestSize=nil;
    float largestSizeArea=0;

    while ((currentSize = [enumerator nextObject]) != nil) {
        NSSize currentSizeSize = [currentSize sizeValue];
        float currentSizeArea = currentSizeSize.width * currentSizeSize.height;
        if(!largestSizeArea || (currentSizeArea > largestSizeArea)){
            largestSize = currentSize;
            largestSizeArea = currentSizeArea;
        }
    }

    return [icons objectForKey:largestSize];
}

- (NSMutableDictionary *)_iconsBySplittingRepresentationsOfIcon:(NSImage *)icon
{
    ASSERT(icon);

    NSMutableDictionary *icons = [NSMutableDictionary dictionary];
    NSEnumerator *enumerator = [[icon representations] objectEnumerator];
    NSImageRep *rep;

    while ((rep = [enumerator nextObject]) != nil) {
        NSSize size = [rep size];
        NSImage *subIcon = [[NSImage alloc] initWithSize:size];
        [subIcon addRepresentation:rep];
        [icons setObject:subIcon forKey:[NSValue valueWithSize:size]];
        [subIcon release];
    }

    if([icons count] > 0)
        return icons;

    LOG_ERROR("icon has no representations");
    
    return nil;
}

- (NSImage *)_iconFromDictionary:(NSMutableDictionary *)icons forSize:(NSSize)size cache:(BOOL)cache
{
    ASSERT(size.width);
    ASSERT(size.height);

    NSImage *icon = [icons objectForKey:[NSValue valueWithSize:size]];

    if(!icon){
        icon = [[[self _largestIconFromDictionary:icons] copy] autorelease];
        [self _scaleIcon:icon toSize:size];

        if(cache){
            [icons setObject:icon forKey:[NSValue valueWithSize:size]];
        }
    }

    return icon;
}

- (void)_scaleIcon:(NSImage *)icon toSize:(NSSize)size
{
    ASSERT(size.width);
    ASSERT(size.height);
    
#if !LOG_DISABLED
    double start = CFAbsoluteTimeGetCurrent();
#endif
    
    [icon setScalesWhenResized:YES];
    [icon setSize:size];
    
#if !LOG_DISABLED
    double duration = CFAbsoluteTimeGetCurrent() - start;
    LOG(Timing, "scaling icon took %f seconds.", duration);
#endif
}

- (NSString *)_databaseDirectory
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

    // Figure out the directory we should be using for the icon.db
    NSString *databaseDirectory = [defaults objectForKey:WebIconDatabaseDirectoryDefaultsKey];
    if (!databaseDirectory) {
        databaseDirectory = WebIconDatabasePath;
        [defaults setObject:databaseDirectory forKey:WebIconDatabaseDirectoryDefaultsKey];
    }
    
    return [[databaseDirectory stringByExpandingTildeInPath] stringByStandardizingPath];
}

@end

@implementation WebIconDatabasePrivate
@end

NSImage *webGetNSImage(Image* image, NSSize size)
{
    ASSERT_MAIN_THREAD();
    ASSERT(size.width);
    ASSERT(size.height);

    // FIXME: We're doing the resize here for now because WebCore::Image doesn't yet support resizing/multiple representations
    // This makes it so there's effectively only one size of a particular icon in the system at a time. We should move this
    // to WebCore::Image at some point.
    if (!image)
        return nil;
    NSImage* nsImage = image->getNSImage();
    if (!nsImage)
        return nil;
    if (!NSEqualSizes([nsImage size], size)) {
        [nsImage setScalesWhenResized:YES];
        [nsImage setSize:size];
    }
    return nsImage;
}
