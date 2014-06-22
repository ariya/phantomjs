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

#import <WebKit/WebKitErrors.h>

#import "WebLocalizableStringsInternal.h"
#import <WebKit/WebKitErrorsPrivate.h>
#import <WebKit/WebNSURLExtras.h>

#import <pthread.h>

NSString *WebKitErrorDomain = @"WebKitErrorDomain";

NSString * const WebKitErrorMIMETypeKey =               @"WebKitErrorMIMETypeKey";
NSString * const WebKitErrorPlugInNameKey =             @"WebKitErrorPlugInNameKey";
NSString * const WebKitErrorPlugInPageURLStringKey =    @"WebKitErrorPlugInPageURLStringKey";

// Policy errors
#define WebKitErrorDescriptionCannotShowMIMEType UI_STRING_INTERNAL("Content with specified MIME type can’t be shown", "WebKitErrorCannotShowMIMEType description")
#define WebKitErrorDescriptionCannotShowURL UI_STRING_INTERNAL("The URL can’t be shown", "WebKitErrorCannotShowURL description")
#define WebKitErrorDescriptionFrameLoadInterruptedByPolicyChange UI_STRING_INTERNAL("Frame load interrupted", "WebKitErrorFrameLoadInterruptedByPolicyChange description")
#define WebKitErrorDescriptionCannotUseRestrictedPort UI_STRING_INTERNAL("Not allowed to use restricted network port", "WebKitErrorCannotUseRestrictedPort description")

// Plug-in and java errors
#define WebKitErrorDescriptionCannotFindPlugin UI_STRING_INTERNAL("The plug-in can’t be found", "WebKitErrorCannotFindPlugin description")
#define WebKitErrorDescriptionCannotLoadPlugin UI_STRING_INTERNAL("The plug-in can’t be loaded", "WebKitErrorCannotLoadPlugin description")
#define WebKitErrorDescriptionJavaUnavailable UI_STRING_INTERNAL("Java is unavailable", "WebKitErrorJavaUnavailable description")
#define WebKitErrorDescriptionPlugInCancelledConnection UI_STRING_INTERNAL("Plug-in cancelled", "WebKitErrorPlugInCancelledConnection description")
#define WebKitErrorDescriptionPlugInWillHandleLoad UI_STRING_INTERNAL("Plug-in handled load", "WebKitErrorPlugInWillHandleLoad description")

// Geolocations errors

#define WebKitErrorDescriptionGeolocationLocationUnknown UI_STRING_INTERNAL("The current location cannot be found.", "WebKitErrorGeolocationLocationUnknown description")

static pthread_once_t registerErrorsControl = PTHREAD_ONCE_INIT;
static void registerErrors(void);

@implementation NSError (WebKitExtras)

static NSMutableDictionary *descriptions = nil;

+ (void)_registerWebKitErrors
{
    pthread_once(&registerErrorsControl, registerErrors);
}

-(id)_webkit_initWithDomain:(NSString *)domain code:(int)code URL:(NSURL *)URL
{
    NSDictionary *descriptionsDict;
    NSString *localizedDesc;
    NSDictionary *dict;
    // insert a localized string here for those folks not savvy to our category methods
    descriptionsDict = [descriptions objectForKey:domain];
    localizedDesc = descriptionsDict ? [descriptionsDict objectForKey:[NSNumber numberWithInt:code]] : nil;
    dict = [NSDictionary dictionaryWithObjectsAndKeys:
        URL, @"NSErrorFailingURLKey",
        [URL absoluteString], @"NSErrorFailingURLStringKey",
        localizedDesc, NSLocalizedDescriptionKey,
        nil];
    return [self initWithDomain:domain code:code userInfo:dict];
}

+(id)_webkit_errorWithDomain:(NSString *)domain code:(int)code URL:(NSURL *)URL
{
    return [[[self alloc] _webkit_initWithDomain:domain code:code URL:URL] autorelease];
}

+ (NSError *)_webKitErrorWithDomain:(NSString *)domain code:(int)code URL:(NSURL *)URL
{
    [self _registerWebKitErrors];
    return [self _webkit_errorWithDomain:domain code:code URL:URL];
}

+ (NSError *)_webKitErrorWithCode:(int)code failingURL:(NSString *)URLString
{
    return [self _webKitErrorWithDomain:WebKitErrorDomain code:code URL:[NSURL _web_URLWithUserTypedString:URLString]];
}

- (id)_initWithPluginErrorCode:(int)code
                    contentURL:(NSURL *)contentURL
                 pluginPageURL:(NSURL *)pluginPageURL
                    pluginName:(NSString *)pluginName
                      MIMEType:(NSString *)MIMEType
{
    [[self class] _registerWebKitErrors];
    
    NSMutableDictionary *userInfo = [[NSMutableDictionary alloc] init];
    NSDictionary *descriptionsForWebKitErrorDomain = [descriptions objectForKey:WebKitErrorDomain];
    NSString *localizedDescription = [descriptionsForWebKitErrorDomain objectForKey:[NSNumber numberWithInt:code]];
    if (localizedDescription)
        [userInfo setObject:localizedDescription forKey:NSLocalizedDescriptionKey];
    if (contentURL) {
        [userInfo setObject:contentURL forKey:@"NSErrorFailingURLKey"];
        [userInfo setObject:[contentURL _web_userVisibleString] forKey:NSURLErrorFailingURLStringErrorKey];
    }
    if (pluginPageURL) {
        [userInfo setObject:[pluginPageURL _web_userVisibleString] forKey:WebKitErrorPlugInPageURLStringKey];
    }
    if (pluginName) {
        [userInfo setObject:pluginName forKey:WebKitErrorPlugInNameKey];
    }
    if (MIMEType) {
        [userInfo setObject:MIMEType forKey:WebKitErrorMIMETypeKey];
    }

    NSDictionary *userInfoCopy = [userInfo count] > 0 ? [[NSDictionary alloc] initWithDictionary:userInfo] : nil;
    [userInfo release];
    NSError *error = [self initWithDomain:WebKitErrorDomain code:code userInfo:userInfoCopy];
    [userInfoCopy release];
    
    return error;
}

+ (void)_webkit_addErrorsWithCodesAndDescriptions:(NSDictionary *)dictionary inDomain:(NSString *)domain
{
    if (!descriptions)
        descriptions = [[NSMutableDictionary alloc] init];

    [descriptions setObject:dictionary forKey:domain];
}

static void registerErrors()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:
        // Policy errors
        WebKitErrorDescriptionCannotShowMIMEType,                   [NSNumber numberWithInt: WebKitErrorCannotShowMIMEType],
        WebKitErrorDescriptionCannotShowURL,                        [NSNumber numberWithInt: WebKitErrorCannotShowURL],
        WebKitErrorDescriptionFrameLoadInterruptedByPolicyChange,   [NSNumber numberWithInt: WebKitErrorFrameLoadInterruptedByPolicyChange],
        WebKitErrorDescriptionCannotUseRestrictedPort,              [NSNumber numberWithInt: WebKitErrorCannotUseRestrictedPort],
        
        // Plug-in and java errors
        WebKitErrorDescriptionCannotFindPlugin,                     [NSNumber numberWithInt: WebKitErrorCannotFindPlugIn],
        WebKitErrorDescriptionCannotLoadPlugin,                     [NSNumber numberWithInt: WebKitErrorCannotLoadPlugIn],
        WebKitErrorDescriptionJavaUnavailable,                      [NSNumber numberWithInt: WebKitErrorJavaUnavailable],
        WebKitErrorDescriptionPlugInCancelledConnection,            [NSNumber numberWithInt: WebKitErrorPlugInCancelledConnection],
        WebKitErrorDescriptionPlugInWillHandleLoad,                 [NSNumber numberWithInt: WebKitErrorPlugInWillHandleLoad],

        // Geolocation errors
        WebKitErrorDescriptionGeolocationLocationUnknown,           [NSNumber numberWithInt: WebKitErrorGeolocationLocationUnknown],
        nil];

    [NSError _webkit_addErrorsWithCodesAndDescriptions:dict inDomain:WebKitErrorDomain];

    [pool drain];
}

@end
