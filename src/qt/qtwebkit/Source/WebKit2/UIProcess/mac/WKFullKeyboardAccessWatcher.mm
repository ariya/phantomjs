/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "WKFullKeyboardAccessWatcher.h"

#import "WebContext.h"

NSString * const KeyboardUIModeDidChangeNotification = @"com.apple.KeyboardUIModeDidChange";
const CFStringRef AppleKeyboardUIMode = CFSTR("AppleKeyboardUIMode");
const CFStringRef UniversalAccessDomain = CFSTR("com.apple.universalaccess");

using namespace WebKit;

@implementation WKFullKeyboardAccessWatcher

- (void)notifyAllWebContexts
{
    const Vector<WebContext*>& contexts = WebContext::allContexts();
    for (size_t i = 0; i < contexts.size(); ++i)
        contexts[i]->fullKeyboardAccessModeChanged(fullKeyboardAccessEnabled);
}

- (void)retrieveKeyboardUIModeFromPreferences:(NSNotification *)notification
{
    BOOL oldValue = fullKeyboardAccessEnabled;

    CFPreferencesAppSynchronize(UniversalAccessDomain);

    Boolean keyExistsAndHasValidFormat;
    int mode = CFPreferencesGetAppIntegerValue(AppleKeyboardUIMode, UniversalAccessDomain, &keyExistsAndHasValidFormat);
    if (keyExistsAndHasValidFormat) {
        // The keyboard access mode is reported by two bits:
        // Bit 0 is set if feature is on
        // Bit 1 is set if full keyboard access works for any control, not just text boxes and lists.
        fullKeyboardAccessEnabled = (mode & 0x2);
    }

    if (fullKeyboardAccessEnabled != oldValue)
        [self notifyAllWebContexts];
}

- (id)init
{
    self = [super init];
    if (!self)
        return nil;

    [self retrieveKeyboardUIModeFromPreferences:nil];

    [[NSDistributedNotificationCenter defaultCenter] 
        addObserver:self selector:@selector(retrieveKeyboardUIModeFromPreferences:) 
        name:KeyboardUIModeDidChangeNotification object:nil];

    return self;
}

+ (BOOL)fullKeyboardAccessEnabled
{
    static WKFullKeyboardAccessWatcher *watcher = [[WKFullKeyboardAccessWatcher alloc] init];
    return watcher->fullKeyboardAccessEnabled;
}

@end
