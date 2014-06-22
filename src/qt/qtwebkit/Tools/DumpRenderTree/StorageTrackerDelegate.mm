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

#import "config.h"
#import "StorageTrackerDelegate.h"

#import "TestRunner.h"
#import <WebKit/WebSecurityOriginPrivate.h>
#import <WebKit/WebStorageManagerPrivate.h>

@implementation StorageTrackerDelegate

- (id)init
{
    self = [super init];
    if (!self)
        return nil;

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(originModified:) name:WebStorageDidModifyOriginNotification object:nil];

    return self;
}

- (void)logNotifications:(unsigned)number controller:(TestRunner*)controller
{
    controllerToNotifyDone = controller;

    numberOfNotificationsToLog = number;
}

- (void)originModified:(NSNotification *)notification
{
    if (!numberOfNotificationsToLog)
        return;

    numberOfNotificationsToLog--;
    
    if (numberOfNotificationsToLog == 0 && controllerToNotifyDone) {
        NSArray *origins = [[WebStorageManager sharedWebStorageManager] origins];
        for (WebSecurityOrigin *origin in origins)
            printf("Origin identifier: '%s'\n", [[origin databaseIdentifier] UTF8String]);
        
        controllerToNotifyDone->notifyDone();
    }
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self name:WebStorageDidModifyOriginNotification object:nil];
    
    [super dealloc];
}

- (void)setControllerToNotifyDone:(TestRunner*)controller
{
    controllerToNotifyDone = controller;
}


@end
