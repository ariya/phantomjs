/*
 * Copyright (C) 2005, 2006, 2007 Apple Inc. All rights reserved.
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

#if ENABLE(NETSCAPE_PLUGIN_API)
#import "WebNetscapePluginPackage.h"

#import "WebTypesInternal.h"
#import "WebKitLogging.h"
#import "WebKitNSStringExtras.h"
#import "WebNSFileManagerExtras.h"
#import "WebNSObjectExtras.h"
#import <WebCore/npruntime_impl.h>
#import <wtf/RetainPtr.h>

#if USE(PLUGIN_HOST_PROCESS)
#import "NetscapePluginHostManager.h"

using namespace WebKit;
#endif

using namespace WebCore;

#define PluginNameOrDescriptionStringNumber     126
#define MIMEDescriptionStringNumber             127
#define MIMEListStringStringNumber              128

@interface WebNetscapePluginPackage (Internal)
- (void)_unloadWithShutdown:(BOOL)shutdown;
@end

@implementation WebNetscapePluginPackage

- (ResFileRefNum)openResourceFile
{
    return CFBundleOpenBundleResourceMap(cfBundle.get());
}

- (void)closeResourceFile:(ResFileRefNum)resRef
{
    CFBundleCloseBundleResourceMap(cfBundle.get(), resRef);
}

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

- (NSString *)stringForStringListID:(SInt16)stringListID andIndex:(SInt16)index
{
    // Get resource, and dereference the handle.
    Handle stringHandle = Get1Resource('STR#', stringListID);
    if (stringHandle == NULL) {
        return nil;
    }
    unsigned char *p = (unsigned char *)*stringHandle;
    if (!p)
        return nil;
    
    // Check the index against the length of the string list, then skip the length.
    if (index < 1 || index > *(SInt16 *)p)
        return nil;
    p += sizeof(SInt16);
    
    // Skip any strings that come before the one we are looking for.
    while (--index)
        p += 1 + *p;
    
    // Convert the one we found into an NSString.
    return [[[NSString alloc] initWithBytes:(p + 1) length:*p encoding:[NSString _web_encodingForResource:stringHandle]] autorelease];
}

- (BOOL)getPluginInfoFromResources
{
    SInt16 resRef = [self openResourceFile];
    if (resRef == -1)
        return NO;
    
    UseResFile(resRef);
    if (ResError() != noErr)
        return NO;

    NSString *MIME, *extensionsList, *description;
    NSArray *extensions;
    unsigned i;
    
    for (i=1; 1; i+=2) {
        MIME = [[self stringForStringListID:MIMEListStringStringNumber
                                   andIndex:i] lowercaseString];
        if (!MIME)
            break;

        MimeClassInfo mimeClassInfo;
        mimeClassInfo.type = String(MIME).lower();

        extensionsList = [[self stringForStringListID:MIMEListStringStringNumber andIndex:i+1] lowercaseString];
        if (extensionsList) {
            extensions = [extensionsList componentsSeparatedByString:@","];
            for (NSUInteger j = 0; j < [extensions count]; ++j)
                mimeClassInfo.extensions.append((NSString *)[extensions objectAtIndex:j]);
        }
        
        description = [self stringForStringListID:MIMEDescriptionStringNumber
                                         andIndex:pluginInfo.mimes.size() + 1];
        mimeClassInfo.desc = description;

        pluginInfo.mimes.append(mimeClassInfo);
    }

    NSString *filename = [(NSString *)path lastPathComponent];
    pluginInfo.file = filename;
    
    description = [self stringForStringListID:PluginNameOrDescriptionStringNumber andIndex:1];
    if (!description)
        description = filename;
    pluginInfo.desc = description;
    
    
    NSString *theName = [self stringForStringListID:PluginNameOrDescriptionStringNumber andIndex:2];
    if (!theName)
        theName = filename;
    pluginInfo.name = theName;

    pluginInfo.isApplicationPlugin = false;
    
    [self closeResourceFile:resRef];
    
    return YES;
}
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

- (BOOL)_initWithPath:(NSString *)pluginPath
{
    resourceRef = -1;
    
    OSType type = 0;

    if (!cfBundle)
        return NO;

    CFBundleGetPackageInfo(cfBundle.get(), &type, NULL);
    
    if (type != FOUR_CHAR_CODE('BRPL'))
        return NO;

#if USE(PLUGIN_HOST_PROCESS)
    RetainPtr<CFArrayRef> archs = adoptCF(CFBundleCopyExecutableArchitectures(cfBundle.get()));

    if ([(NSArray *)archs.get() containsObject:[NSNumber numberWithInteger:NSBundleExecutableArchitectureX86_64]])
        pluginHostArchitecture = CPU_TYPE_X86_64;
    else if ([(NSArray *)archs.get() containsObject:[NSNumber numberWithInteger:NSBundleExecutableArchitectureI386]])
        pluginHostArchitecture = CPU_TYPE_X86;
    else
        return NO;
#else
    RetainPtr<CFURLRef> executableURL = adoptCF(CFBundleCopyExecutableURL(cfBundle.get()));
    if (!executableURL)
        return NO;
    NSFileHandle *executableFile = [NSFileHandle fileHandleForReadingAtPath:[(NSURL *)executableURL.get() path]];
    NSData *data = [executableFile readDataOfLength:512];
    [executableFile closeFile];

     if (![self isNativeLibraryData:data])
         return NO;

#endif

    if (![self getPluginInfoFromPLists] && ![self getPluginInfoFromResources])
        return NO;
    
    return YES;
}

- (id)initWithPath:(NSString *)pluginPath
{
    if (!(self = [super initWithPath:pluginPath]))
        return nil;
    
    // Initializing a plugin package can cause it to be loaded.  If there was an error initializing the plugin package,
    // ensure that it is unloaded before deallocating it (WebBasePluginPackage requires & asserts this).
    if (![self _initWithPath:pluginPath]) {
        [self _unloadWithShutdown:YES];
        [self release];
        return nil;
    }
        
    return self;
}

#if USE(PLUGIN_HOST_PROCESS)
- (cpu_type_t)pluginHostArchitecture
{
    return pluginHostArchitecture;
}

- (void)createPropertyListFile
{
    NetscapePluginHostManager::shared().createPropertyListFile(path, pluginHostArchitecture, [self bundleIdentifier]);
}

#endif

- (void)unload
{
    [self _unloadWithShutdown:YES];
}

- (BOOL)_tryLoad
{
    NP_GetEntryPointsFuncPtr NP_GetEntryPoints = NULL;
    NP_InitializeFuncPtr NP_Initialize = NULL;
    NPError npErr;

#if !LOG_DISABLED
    CFAbsoluteTime start = CFAbsoluteTimeGetCurrent();
    CFAbsoluteTime currentTime;
    CFAbsoluteTime duration;
#endif
    LOG(Plugins, "%f Load timing started for: %@", start, (NSString *)[self pluginInfo].name);

    if (isLoaded)
        return YES;
    
    if (!CFBundleLoadExecutable(cfBundle.get()))
        return NO;
#if !LOG_DISABLED
    currentTime = CFAbsoluteTimeGetCurrent();
    duration = currentTime - start;
#endif
    LOG(Plugins, "%f CFBundleLoadExecutable took %f seconds", currentTime, duration);
    isLoaded = YES;

    NP_Initialize = (NP_InitializeFuncPtr)CFBundleGetFunctionPointerForName(cfBundle.get(), CFSTR("NP_Initialize"));
    NP_GetEntryPoints = (NP_GetEntryPointsFuncPtr)CFBundleGetFunctionPointerForName(cfBundle.get(), CFSTR("NP_GetEntryPoints"));
    NP_Shutdown = (NPP_ShutdownProcPtr)CFBundleGetFunctionPointerForName(cfBundle.get(), CFSTR("NP_Shutdown"));
    if (!NP_Initialize || !NP_GetEntryPoints || !NP_Shutdown)
        return NO;

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    // Plugins (at least QT) require that you call UseResFile on the resource file before loading it.
    resourceRef = [self openResourceFile];
    if (resourceRef != -1) {
        UseResFile(resourceRef);
    }
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

    browserFuncs.version = NP_VERSION_MINOR;
    browserFuncs.size = sizeof(NPNetscapeFuncs);
    browserFuncs.geturl = NPN_GetURL;
    browserFuncs.posturl = NPN_PostURL;
    browserFuncs.requestread = NPN_RequestRead;
    browserFuncs.newstream = NPN_NewStream;
    browserFuncs.write = NPN_Write;
    browserFuncs.destroystream = NPN_DestroyStream;
    browserFuncs.status = NPN_Status;
    browserFuncs.uagent = NPN_UserAgent;
    browserFuncs.memalloc = NPN_MemAlloc;
    browserFuncs.memfree = NPN_MemFree;
    browserFuncs.memflush = NPN_MemFlush;
    browserFuncs.reloadplugins = NPN_ReloadPlugins;
    browserFuncs.geturlnotify = NPN_GetURLNotify;
    browserFuncs.posturlnotify = NPN_PostURLNotify;
    browserFuncs.getvalue = NPN_GetValue;
    browserFuncs.setvalue = NPN_SetValue;
    browserFuncs.invalidaterect = NPN_InvalidateRect;
    browserFuncs.invalidateregion = NPN_InvalidateRegion;
    browserFuncs.forceredraw = NPN_ForceRedraw;
    browserFuncs.getJavaEnv = NPN_GetJavaEnv;
    browserFuncs.getJavaPeer = NPN_GetJavaPeer;
    browserFuncs.pushpopupsenabledstate = NPN_PushPopupsEnabledState;
    browserFuncs.poppopupsenabledstate = NPN_PopPopupsEnabledState;
    browserFuncs.pluginthreadasynccall = NPN_PluginThreadAsyncCall;
    browserFuncs.getvalueforurl = NPN_GetValueForURL;
    browserFuncs.setvalueforurl = NPN_SetValueForURL;
    browserFuncs.getauthenticationinfo = NPN_GetAuthenticationInfo;
    browserFuncs.scheduletimer = NPN_ScheduleTimer;
    browserFuncs.unscheduletimer = NPN_UnscheduleTimer;
    browserFuncs.popupcontextmenu = NPN_PopUpContextMenu;
    browserFuncs.convertpoint = NPN_ConvertPoint;

    browserFuncs.releasevariantvalue = _NPN_ReleaseVariantValue;
    browserFuncs.getstringidentifier = _NPN_GetStringIdentifier;
    browserFuncs.getstringidentifiers = _NPN_GetStringIdentifiers;
    browserFuncs.getintidentifier = _NPN_GetIntIdentifier;
    browserFuncs.identifierisstring = _NPN_IdentifierIsString;
    browserFuncs.utf8fromidentifier = _NPN_UTF8FromIdentifier;
    browserFuncs.intfromidentifier = _NPN_IntFromIdentifier;
    browserFuncs.createobject = _NPN_CreateObject;
    browserFuncs.retainobject = _NPN_RetainObject;
    browserFuncs.releaseobject = _NPN_ReleaseObject;
    browserFuncs.hasmethod = _NPN_HasMethod;
    browserFuncs.invoke = _NPN_Invoke;
    browserFuncs.invokeDefault = _NPN_InvokeDefault;
    browserFuncs.evaluate = _NPN_Evaluate;
    browserFuncs.hasproperty = _NPN_HasProperty;
    browserFuncs.getproperty = _NPN_GetProperty;
    browserFuncs.setproperty = _NPN_SetProperty;
    browserFuncs.removeproperty = _NPN_RemoveProperty;
    browserFuncs.setexception = _NPN_SetException;
    browserFuncs.enumerate = _NPN_Enumerate;
    browserFuncs.construct = _NPN_Construct;

#if !LOG_DISABLED
    CFAbsoluteTime initializeStart = CFAbsoluteTimeGetCurrent();
#endif
    LOG(Plugins, "%f NP_Initialize timing started", initializeStart);
    npErr = NP_Initialize(&browserFuncs);
    if (npErr != NPERR_NO_ERROR)
        return NO;
#if !LOG_DISABLED
    currentTime = CFAbsoluteTimeGetCurrent();
    duration = currentTime - initializeStart;
#endif
    LOG(Plugins, "%f NP_Initialize took %f seconds", currentTime, duration);

    pluginFuncs.size = sizeof(NPPluginFuncs);

    npErr = NP_GetEntryPoints(&pluginFuncs);
    if (npErr != NPERR_NO_ERROR)
        return NO;

    pluginSize = pluginFuncs.size;
    pluginVersion = pluginFuncs.version;

    if (pluginFuncs.javaClass)
        LOG(LiveConnect, "%@:  mach-o entry point for NPP_GetJavaClass = %p", (NSString *)[self pluginInfo].name, pluginFuncs.javaClass);
    else
        LOG(LiveConnect, "%@:  no entry point for NPP_GetJavaClass", (NSString *)[self pluginInfo].name);

#if !LOG_DISABLED
    currentTime = CFAbsoluteTimeGetCurrent();
    duration = currentTime - start;
#endif
    LOG(Plugins, "%f Total load time: %f seconds", currentTime, duration);

    return YES;
}

- (BOOL)load
{    
    if ([self _tryLoad])
        return [super load];

    [self _unloadWithShutdown:NO];
    return NO;
}

- (NPPluginFuncs *)pluginFuncs
{
    return &pluginFuncs;
}

- (NPNetscapeFuncs *)browserFuncs
{
    return &browserFuncs;
}

- (void)wasRemovedFromPluginDatabase:(WebPluginDatabase *)database
{
    [super wasRemovedFromPluginDatabase:database];
    
    // Unload when removed from final plug-in database
    if ([pluginDatabases count] == 0)
        [self _unloadWithShutdown:YES];
}

- (void)open
{
    instanceCount++;
    
    // Handle the case where all instances close a plug-in package, but another
    // instance opens the package before it is unloaded (which only happens when
    // the plug-in database is refreshed)
    needsUnload = NO;
    
    if (!isLoaded) {
        // Should load when the first instance opens the plug-in package
        ASSERT(instanceCount == 1);
        [self load];
    }
}

- (void)close
{
    ASSERT(instanceCount > 0);
    instanceCount--;
    if (instanceCount == 0 && needsUnload)
        [self _unloadWithShutdown:YES];
}


- (BOOL)supportsSnapshotting
{
    if ([self bundleIdentifier] != "com.macromedia.Flash Player.plugin")
        return YES;
    
    // Flash has a bogus Info.plist entry for CFBundleVersionString, so use CFBundleShortVersionString.
    NSString *versionString = (NSString *)CFDictionaryGetValue(CFBundleGetInfoDictionary(cfBundle.get()), CFSTR("CFBundleShortVersionString"));
    
    if (![versionString hasPrefix:@"10.1"])
        return YES;
    
    // Some prerelease versions of Flash 10.1 crash when sent a drawRect event using the CA drawing model: <rdar://problem/7739922>
    return CFStringCompare((CFStringRef)versionString, CFSTR("10.1.53.60"), kCFCompareNumerically) != kCFCompareLessThan;
}

@end

@implementation WebNetscapePluginPackage (Internal)

- (void)_unloadWithShutdown:(BOOL)shutdown
{
    if (!isLoaded)
        return;
    
    LOG(Plugins, "Unloading %@...", (NSString *)pluginInfo.name);

    // Cannot unload a plug-in package while an instance is still using it
    if (instanceCount > 0) {
        needsUnload = YES;
        return;
    }

    if (shutdown && NP_Shutdown)
        NP_Shutdown();

    if (resourceRef != -1)
        [self closeResourceFile:resourceRef];

    LOG(Plugins, "Plugin Unloaded");
    isLoaded = NO;
}

@end
#endif
