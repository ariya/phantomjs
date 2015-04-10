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

#import <WebKit/WebBasePluginPackage.h>

#import <algorithm>
#import <WebCore/RunLoop.h>
#import <WebCore/WebCoreObjCExtras.h>
#import <WebKit/WebKitNSStringExtras.h>
#import <WebKit/WebNSObjectExtras.h>
#import <WebKit/WebNetscapePluginPackage.h>
#import <WebKit/WebPluginPackage.h>
#import <runtime/InitializeThreading.h>
#import <wtf/Assertions.h>
#import <wtf/MainThread.h>
#import <wtf/Vector.h>
#import <wtf/text/CString.h>

#import <WebKitSystemInterface.h>

#import "WebKitLogging.h"
#import "WebTypesInternal.h"

#import <mach-o/arch.h>
#import <mach-o/fat.h>
#import <mach-o/loader.h>

#define JavaCocoaPluginIdentifier   "com.apple.JavaPluginCocoa"
#define JavaCarbonPluginIdentifier  "com.apple.JavaAppletPlugin"

#define QuickTimeCarbonPluginIdentifier       "com.apple.QuickTime Plugin.plugin"
#define QuickTimeCocoaPluginIdentifier        "com.apple.quicktime.webplugin"

@interface NSArray (WebPluginExtensions)
- (NSArray *)_web_lowercaseStrings;
@end;

using namespace WebCore;

@implementation WebBasePluginPackage

+ (void)initialize
{
    JSC::initializeThreading();
    WTF::initializeMainThreadToProcessMainThread();
    WebCore::RunLoop::initializeMainRunLoop();
    WebCoreObjCFinalizeOnMainThread(self);
}

+ (WebBasePluginPackage *)pluginWithPath:(NSString *)pluginPath
{
    
    WebBasePluginPackage *pluginPackage = [[WebPluginPackage alloc] initWithPath:pluginPath];

    if (!pluginPackage) {
#if ENABLE(NETSCAPE_PLUGIN_API)
        pluginPackage = [[WebNetscapePluginPackage alloc] initWithPath:pluginPath];
#else
        return nil;
#endif
    }

    return [pluginPackage autorelease];
}

+ (NSString *)preferredLocalizationName
{
    return WebCFAutorelease(WKCopyCFLocalizationPreferredName(NULL));
}

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
// FIXME: Rewrite this in terms of -[NSURL URLByResolvingBookmarkData:…].
static NSString *pathByResolvingSymlinksAndAliases(NSString *thePath)
{
    NSString *newPath = [thePath stringByResolvingSymlinksInPath];

    FSRef fref;
    OSStatus err;

    err = FSPathMakeRef((const UInt8 *)[thePath fileSystemRepresentation], &fref, NULL);
    if (err != noErr)
        return newPath;

    Boolean targetIsFolder;
    Boolean wasAliased;
    err = FSResolveAliasFileWithMountFlags(&fref, TRUE, &targetIsFolder, &wasAliased, kResolveAliasFileNoUI);
    if (err != noErr)
        return newPath;

    if (wasAliased) {
        CFURLRef URL = CFURLCreateFromFSRef(kCFAllocatorDefault, &fref);
        newPath = [(NSURL *)URL path];
        CFRelease(URL);
    }

    return newPath;
}
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

- (id)initWithPath:(NSString *)pluginPath
{
    if (!(self = [super init]))
        return nil;
        
    path = pathByResolvingSymlinksAndAliases(pluginPath);
    cfBundle = adoptCF(CFBundleCreate(kCFAllocatorDefault, (CFURLRef)[NSURL fileURLWithPath:path]));

    if (!cfBundle) {
        [self release];
        return nil;
    }

    return self;
}

- (void)unload
{
}

- (void)createPropertyListFile
{
    if ([self load] && BP_CreatePluginMIMETypesPreferences) {
        BP_CreatePluginMIMETypesPreferences();
        [self unload];
    }
}

- (NSDictionary *)pListForPath:(NSString *)pListPath createFile:(BOOL)createFile
{
    if (createFile)
        [self createPropertyListFile];
    
    NSDictionary *pList = nil;
    NSData *data = [NSData dataWithContentsOfFile:pListPath];
    if (data) {
        pList = [NSPropertyListSerialization propertyListFromData:data
                                                 mutabilityOption:NSPropertyListImmutable
                                                           format:nil
                                                 errorDescription:nil];
    }
    
    return pList;
}

- (id)_objectForInfoDictionaryKey:(NSString *)key
{
    CFDictionaryRef bundleInfoDictionary = CFBundleGetInfoDictionary(cfBundle.get());
    if (!bundleInfoDictionary)
        return nil;

    return (id)CFDictionaryGetValue(bundleInfoDictionary, key);
}

- (BOOL)getPluginInfoFromPLists
{
    if (!cfBundle)
        return NO;
    
    NSDictionary *MIMETypes = nil;
    NSString *pListFilename = [self _objectForInfoDictionaryKey:WebPluginMIMETypesFilenameKey];
    
    // Check if the MIME types are claimed in a plist in the user's preferences directory.
    if (pListFilename) {
        NSString *pListPath = [NSString stringWithFormat:@"%@/Library/Preferences/%@", NSHomeDirectory(), pListFilename];
        NSDictionary *pList = [self pListForPath:pListPath createFile:NO];
        if (pList) {
            // If the plist isn't localized, have the plug-in recreate it in the preferred language.
            NSString *localizationName = [pList objectForKey:WebPluginLocalizationNameKey];
            if (![localizationName isEqualToString:[[self class] preferredLocalizationName]])
                pList = [self pListForPath:pListPath createFile:YES];
            MIMETypes = [pList objectForKey:WebPluginMIMETypesKey];
        } else
            // Plist doesn't exist, ask the plug-in to create it.
            MIMETypes = [[self pListForPath:pListPath createFile:YES] objectForKey:WebPluginMIMETypesKey];
    }

    if (!MIMETypes) {
        MIMETypes = [self _objectForInfoDictionaryKey:WebPluginMIMETypesKey];
        if (!MIMETypes)
            return NO;
    }

    NSEnumerator *keyEnumerator = [MIMETypes keyEnumerator];
    NSDictionary *MIMEDictionary;
    NSString *MIME;

    while ((MIME = [keyEnumerator nextObject]) != nil) {
        MIMEDictionary = [MIMETypes objectForKey:MIME];
        
        // FIXME: Consider storing disabled MIME types.
        NSNumber *isEnabled = [MIMEDictionary objectForKey:WebPluginTypeEnabledKey];
        if (isEnabled && [isEnabled boolValue] == NO)
            continue;

        MimeClassInfo mimeClassInfo;
        
        NSArray *extensions = [[MIMEDictionary objectForKey:WebPluginExtensionsKey] _web_lowercaseStrings];
        for (NSUInteger i = 0; i < [extensions count]; ++i) {
            // The DivX plug-in lists multiple extensions in a comma separated string instead of using
            // multiple array elements in the property list. Work around this here by splitting the
            // extension string into components.
            NSArray *extensionComponents = [[extensions objectAtIndex:i] componentsSeparatedByString:@","];

            for (NSString *extension in extensionComponents)
                mimeClassInfo.extensions.append(extension);
        }

        mimeClassInfo.type = String(MIME).lower();

        mimeClassInfo.desc = [MIMEDictionary objectForKey:WebPluginTypeDescriptionKey];

        pluginInfo.mimes.append(mimeClassInfo);
    }

    NSString *filename = [(NSString *)path lastPathComponent];
    pluginInfo.file = filename;

    NSString *theName = [self _objectForInfoDictionaryKey:WebPluginNameKey];
    if (!theName)
        theName = filename;
    pluginInfo.name = theName;

    NSString *description = [self _objectForInfoDictionaryKey:WebPluginDescriptionKey];
    if (!description)
        description = filename;
    pluginInfo.desc = description;

    pluginInfo.isApplicationPlugin = false;

    return YES;
}

- (BOOL)load
{
    if (cfBundle && !BP_CreatePluginMIMETypesPreferences)
        BP_CreatePluginMIMETypesPreferences = (BP_CreatePluginMIMETypesPreferencesFuncPtr)CFBundleGetFunctionPointerForName(cfBundle.get(), CFSTR("BP_CreatePluginMIMETypesPreferences"));
    
    return YES;
}

- (void)dealloc
{
    ASSERT(!pluginDatabases || [pluginDatabases count] == 0);
    [pluginDatabases release];
    
    [super dealloc];
}

- (void)finalize
{
    ASSERT_MAIN_THREAD();
    ASSERT(!pluginDatabases || [pluginDatabases count] == 0);
    [pluginDatabases release];

    [super finalize];
}

- (const String&)path
{
    return path;
}

- (const PluginInfo&)pluginInfo
{
    return pluginInfo;
}

- (BOOL)supportsExtension:(const String&)extension
{
    ASSERT(extension.lower() == extension);
    
    for (size_t i = 0; i < pluginInfo.mimes.size(); ++i) {
        const Vector<String>& extensions = pluginInfo.mimes[i].extensions;

        if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end())
            return YES;
    }

    return NO;
}

- (BOOL)supportsMIMEType:(const WTF::String&)mimeType
{
    ASSERT(mimeType.lower() == mimeType);
    
    for (size_t i = 0; i < pluginInfo.mimes.size(); ++i) {
        if (pluginInfo.mimes[i].type == mimeType)
            return YES;
    }
    
    return NO;
}

- (NSString *)MIMETypeForExtension:(const String&)extension
{
    ASSERT(extension.lower() == extension);
    
    for (size_t i = 0; i < pluginInfo.mimes.size(); ++i) {
        const MimeClassInfo& mimeClassInfo = pluginInfo.mimes[i];
        const Vector<String>& extensions = mimeClassInfo.extensions;

        if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end())
            return mimeClassInfo.type;
    }

    return nil;
}

- (BOOL)isQuickTimePlugIn
{
    const String& bundleIdentifier = [self bundleIdentifier];
    return bundleIdentifier == QuickTimeCocoaPluginIdentifier || bundleIdentifier == QuickTimeCocoaPluginIdentifier;
}

- (BOOL)isJavaPlugIn
{
    const String& bundleIdentifier = [self bundleIdentifier];
    return bundleIdentifier == JavaCocoaPluginIdentifier || bundleIdentifier == JavaCarbonPluginIdentifier;
}

static inline void swapIntsInHeader(uint32_t* rawData, size_t length)
{
    for (size_t i = 0; i < length; ++i) 
        rawData[i] = OSSwapInt32(rawData[i]);
}

- (BOOL)isNativeLibraryData:(NSData *)data
{
    NSUInteger sizeInBytes = [data length];
    Vector<uint32_t, 128> rawData((sizeInBytes + 3) / 4);
    memcpy(rawData.data(), [data bytes], sizeInBytes);
    
    unsigned numArchs = 0;
    struct fat_arch singleArch = { 0, 0, 0, 0, 0 };
    struct fat_arch* archs = 0;
       
    if (sizeInBytes >= sizeof(struct mach_header_64)) {
        uint32_t magic = *rawData.data();
        
        if (magic == MH_MAGIC || magic == MH_CIGAM) {
            // We have a 32-bit thin binary
            struct mach_header* header = (struct mach_header*)rawData.data();

            // Check if we need to swap the bytes
            if (magic == MH_CIGAM)
                swapIntsInHeader(rawData.data(), rawData.size());
    
            singleArch.cputype = header->cputype;
            singleArch.cpusubtype = header->cpusubtype;

            archs = &singleArch;
            numArchs = 1;
        } else if (magic == MH_MAGIC_64 || magic == MH_CIGAM_64) {
            // We have a 64-bit thin binary
            struct mach_header_64* header = (struct mach_header_64*)rawData.data();

            // Check if we need to swap the bytes
            if (magic == MH_CIGAM_64)
                swapIntsInHeader(rawData.data(), rawData.size());
            
            singleArch.cputype = header->cputype;
            singleArch.cpusubtype = header->cpusubtype;
            
            archs = &singleArch;
            numArchs = 1;
        } else if (magic == FAT_MAGIC || magic == FAT_CIGAM) {
            // We have a fat (universal) binary

            // Check if we need to swap the bytes
            if (magic == FAT_CIGAM)
                swapIntsInHeader(rawData.data(), rawData.size());
            
            COMPILE_ASSERT(sizeof(struct fat_header) % sizeof(uint32_t) == 0, struct_fat_header_must_be_integral_size_of_uint32_t);
            archs = reinterpret_cast<struct fat_arch*>(rawData.data() + sizeof(struct fat_header) / sizeof(uint32_t));
            numArchs = reinterpret_cast<struct fat_header*>(rawData.data())->nfat_arch;
            
            unsigned maxArchs = (sizeInBytes - sizeof(struct fat_header)) / sizeof(struct fat_arch);
            if (numArchs > maxArchs)
                numArchs = maxArchs;
        }            
    }
    
    if (!archs || !numArchs)
        return NO;
    
    const NXArchInfo* localArch = NXGetLocalArchInfo();
    if (!localArch)
        return NO;
    
    cpu_type_t cputype = localArch->cputype;
    cpu_subtype_t cpusubtype = localArch->cpusubtype;
    
#ifdef __x86_64__
    // NXGetLocalArchInfo returns CPU_TYPE_X86 even when running in 64-bit. 
    // See <rdar://problem/4996965> for more information.
    cputype = CPU_TYPE_X86_64;
#endif
    
    return NXFindBestFatArch(cputype, cpusubtype, archs, numArchs) != 0;
}

- (UInt32)versionNumber
{
    // CFBundleGetVersionNumber doesn't work with all possible versioning schemes, but we think for now it's good enough for us.
    return CFBundleGetVersionNumber(cfBundle.get());
}

- (void)wasAddedToPluginDatabase:(WebPluginDatabase *)database
{    
    if (!pluginDatabases)
        pluginDatabases = [[NSMutableSet alloc] init];
        
    ASSERT(![pluginDatabases containsObject:database]);
    [pluginDatabases addObject:database];
}

- (void)wasRemovedFromPluginDatabase:(WebPluginDatabase *)database
{
    ASSERT(pluginDatabases);
    ASSERT([pluginDatabases containsObject:database]);

    [pluginDatabases removeObject:database];
}

- (String)bundleIdentifier
{
    return CFBundleGetIdentifier(cfBundle.get());
}

- (String)bundleVersion
{
    CFDictionaryRef infoDictionary = CFBundleGetInfoDictionary(cfBundle.get());
    if (!infoDictionary)
        return String();

    CFTypeRef bundleVersionString = CFDictionaryGetValue(infoDictionary, kCFBundleVersionKey);
    if (!bundleVersionString || CFGetTypeID(bundleVersionString) != CFStringGetTypeID())
        return String();

    return reinterpret_cast<CFStringRef>(bundleVersionString);
}

@end

@implementation NSArray (WebPluginExtensions)

- (NSArray *)_web_lowercaseStrings
{
    NSMutableArray *lowercaseStrings = [NSMutableArray arrayWithCapacity:[self count]];
    NSEnumerator *strings = [self objectEnumerator];
    NSString *string;

    while ((string = [strings nextObject]) != nil) {
        if ([string isKindOfClass:[NSString class]])
            [lowercaseStrings addObject:[string lowercaseString]];
    }

    return lowercaseStrings;
}

@end
