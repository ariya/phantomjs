/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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

#import "WebIconDatabaseClient.h"

#import "WebIconDatabaseInternal.h"
#import <wtf/text/WTFString.h>

#if ENABLE(ICONDATABASE)

void WebIconDatabaseClient::didRemoveAllIcons()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [[WebIconDatabase sharedIconDatabase] _sendDidRemoveAllIconsNotification];
    [pool drain];
}

void WebIconDatabaseClient::didImportIconURLForPageURL(const String& pageURL)
{
    // This is a quick notification that is likely to fire in a rapidly iterating loop
    // Therefore we let WebCore handle autorelease by draining its pool "from time to time"
    // instead of us doing it every iteration
    [[WebIconDatabase sharedIconDatabase] _sendNotificationForURL:pageURL];
}

void WebIconDatabaseClient::didImportIconDataForPageURL(const String& pageURL)
{
    // WebKit1 only has a single "icon did change" notification.
    didImportIconURLForPageURL(pageURL);
}
void WebIconDatabaseClient::didChangeIconForPageURL(const String& pageURL)
{
    // WebKit1 only has a single "icon did change" notification.
    didImportIconURLForPageURL(pageURL);
}

void WebIconDatabaseClient::didFinishURLImport()
{
}

#endif // ENABLE(ICONDATABASE)
