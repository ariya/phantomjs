/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
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

#import <WebKit/WebPluginPackage.h>

#import <WebKit/WebKitLogging.h>
#import <WebKit/WebKitNSStringExtras.h>

NSString *WebPlugInBaseURLKey =                 @"WebPlugInBaseURLKey";
NSString *WebPlugInAttributesKey =              @"WebPlugInAttributesKey";
NSString *WebPlugInContainerKey =               @"WebPlugInContainerKey";
NSString *WebPlugInModeKey =                    @"WebPlugInModeKey";
NSString *WebPlugInShouldLoadMainResourceKey =  @"WebPlugInShouldLoadMainResourceKey";
NSString *WebPlugInContainingElementKey =       @"WebPlugInContainingElementKey";

@implementation WebPluginPackage

- (id)initWithPath:(NSString *)pluginPath
{
    if (!(self = [super initWithPath:pluginPath]))
        return nil;

    nsBundle = [[NSBundle alloc] initWithPath:path];

    if (!nsBundle) {
        [self release];
        return nil;
    }
    
    if (![[pluginPath pathExtension] _webkit_isCaseInsensitiveEqualToString:@"webplugin"]) {
        UInt32 type = 0;
        CFBundleGetPackageInfo(cfBundle.get(), &type, NULL);
        if (type != FOUR_CHAR_CODE('WBPL')) {
            [self release];
            return nil;
        }
    }
    
    NSFileHandle *executableFile = [NSFileHandle fileHandleForReadingAtPath:[nsBundle executablePath]];
    NSData *data = [executableFile readDataOfLength:512];
    [executableFile closeFile];
    if (![self isNativeLibraryData:data]) {
        [self release];
        return nil;
    }

    if (![self getPluginInfoFromPLists]) {
        [self release];
        return nil;
    }

    return self;
}

- (void)dealloc
{
    [nsBundle release];

    [super dealloc];
}

- (Class)viewFactory
{
    return [nsBundle principalClass];
}

- (BOOL)load
{
#if !LOG_DISABLED
    CFAbsoluteTime start = CFAbsoluteTimeGetCurrent();
#endif
    
    // Load the bundle
    if (![nsBundle isLoaded]) {
        if (![nsBundle load])
            return NO;
    }
    
#if !LOG_DISABLED
    CFAbsoluteTime duration = CFAbsoluteTimeGetCurrent() - start;
    LOG(Plugins, "principalClass took %f seconds for: %@", duration, (NSString *)[self pluginInfo].name);
#endif
    return [super load];
}

@end

@implementation NSObject (WebScripting)

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)aSelector
{
    return YES;
}

+ (BOOL)isKeyExcludedFromWebScript:(const char *)name
{
    return YES;
}

@end
