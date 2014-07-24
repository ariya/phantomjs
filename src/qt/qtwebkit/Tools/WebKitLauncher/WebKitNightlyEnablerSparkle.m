/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#if !ENABLE_SPARKLE

void initializeSparkle()
{
    // No-op.
}

#else // ENABLE_SPARKLE

#import <Cocoa/Cocoa.h>
#import <Sparkle/SUUpdater.h>
#import <objc/objc-runtime.h>
#import "WebKitNightlyEnabler.h"

// We need to tweak the wording of the prompt to make sense in the context of WebKit and Safari.
static NSString* updatePermissionPromptDescription(id self, SEL _cmd)
{
    return @"Should WebKit automatically check for updates? You can always check for updates manually from the Safari menu.";
}

static NSPanel *updateAlertPanel(id updateItem, id host)
{
    NSString *hostName = objc_msgSend(host, @selector(name));
    NSPanel *panel = NSGetInformationalAlertPanel([NSString stringWithFormat:@"Would you like to download and install %@ %@ now?", hostName, objc_msgSend(updateItem, @selector(displayVersionString))],
                                                  [NSString stringWithFormat:@"You are currently running %@ %@.", hostName, objc_msgSend(host, @selector(displayVersion))],
                                                  @"Install Update", @"Skip This Version", @"Remind Me Later");
    NSArray *subviews = [[panel contentView] subviews];
    NSEnumerator *e = [subviews objectEnumerator];
    NSView *view;
    while ((view = [e nextObject])) {
        if (![view isKindOfClass:[NSButton class]])
            continue;

        NSButton *button = (NSButton *)view;
        [button setAction:@selector(webKitHandleButtonPress:)];
        if ([button tag] == NSAlertOtherReturn)
            [button setKeyEquivalent:@"\033"];
    }
    [panel center];
    return panel;
}

// Sparkle's udpate alert panel looks odd with the release notes hidden, so we
// swap it out with a standard NSAlert-style panel instead.
static id updateAlertInitForAlertPanel(id self, SEL _cmd, id updateItem, id host)
{
    NSPanel *panel = updateAlertPanel(updateItem, host);
    [panel setDelegate:self];

    self = [self initWithWindow:panel];
    if (!self)
        return nil;

    [updateItem retain];
    [host retain];

    object_setInstanceVariable(self, "updateItem", (void*)updateItem);
    object_setInstanceVariable(self, "host", (void*)host);

    [self setShouldCascadeWindows:NO];

    return self;
}

@implementation NSAlert (WebKitLauncherExtensions)

- (void)webKitHandleButtonPress:(id)sender
{
    // We rely on the fact that NSAlertOtherReturn == -1, NSAlertAlternateReturn == 0 and NSAlertDefaultReturn == 1
    // to map the button tag to the corresponding selector
    SEL selectors[] = { @selector(remindMeLater:), @selector(skipThisVersion:), @selector(installUpdate:) };
    SEL selector = selectors[[sender tag] + 1];

    id delegate = [[sender window] delegate];
    objc_msgSend(delegate, selector, sender);
}

@end

static NSString *userAgentStringForSparkle()
{
    NSBundle *safariBundle = [NSBundle mainBundle];
    NSString *safariVersion = [[safariBundle localizedInfoDictionary] valueForKey:@"CFBundleShortVersionString"];
    NSString *safariBuild = [[[safariBundle infoDictionary] valueForKey:(NSString *)kCFBundleVersionKey] substringFromIndex:1];
    NSString *webKitRevision = [[webKitLauncherBundle() infoDictionary] valueForKey:(NSString *)kCFBundleVersionKey];
    NSString *applicationName = [NSString stringWithFormat:@"Version/%@ Safari/%@ WebKitRevision/%@", safariVersion, safariBuild, webKitRevision];
    Class WebView = objc_lookUpClass("WebView");
    return objc_msgSend(WebView, @selector(_standardUserAgentWithApplicationName:), applicationName);
}

void initializeSparkle()
{
    // Override some Sparkle behaviour
    Method methodToPatch = class_getInstanceMethod(objc_getRequiredClass("SUUpdatePermissionPrompt"), @selector(promptDescription));
    method_setImplementation(methodToPatch, (IMP)updatePermissionPromptDescription);

    methodToPatch = class_getInstanceMethod(objc_getRequiredClass("SUUpdateAlert"), @selector(initWithAppcastItem:host:));
    method_setImplementation(methodToPatch, (IMP)updateAlertInitForAlertPanel);

    SUUpdater *updater = [SUUpdater updaterForBundle:webKitLauncherBundle()];
    [updater setUserAgentString:userAgentStringForSparkle()];

    // Find the first separator on the Safari menu…
    NSMenu *applicationSubmenu = [[[NSApp mainMenu] itemAtIndex:0] submenu];
    int i = 0;
    for (; i < [applicationSubmenu numberOfItems]; i++) {
        if ([[applicationSubmenu itemAtIndex:i] isSeparatorItem])
            break;
    }

    // … and insert a menu item that can be used to manually trigger update checks.
    NSMenuItem *updateMenuItem = [[NSMenuItem alloc] initWithTitle:@"Check for WebKit Updates…" action:@selector(checkForUpdates:) keyEquivalent:@""];
    [updateMenuItem setTarget:updater];
    [applicationSubmenu insertItem:updateMenuItem atIndex:i];
    [updateMenuItem release];
}

#endif // ENABLE_SPARKLE
