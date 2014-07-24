/*
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#import <WebKit/WebNSFileManagerExtras.h>

#import "WebKitNSStringExtras.h"
#import "WebNSObjectExtras.h"
#import "WebNSURLExtras.h"
#import <wtf/Assertions.h>
#import <WebKitSystemInterface.h>
#import <sys/stat.h>
#import <wtf/RetainPtr.h>

#if __MAC_OS_X_VERSION_MIN_REQUIRED == 1060
extern "C" DADiskRef DADiskCreateFromVolumePath(CFAllocatorRef allocator, DASessionRef session, CFURLRef path);
#endif

@implementation NSFileManager (WebNSFileManagerExtras)


typedef struct MetaDataInfo
{
    CFStringRef URLString;
    CFStringRef referrer;
    CFStringRef path;
} MetaDataInfo;

static void *setMetaData(void* context)
{
    MetaDataInfo *info = (MetaDataInfo *)context;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    WKSetMetadataURL((NSString *)info->URLString, (NSString *)info->referrer, (NSString *)info->path);

    if (info->URLString)
        CFRelease(info->URLString);
    if (info->referrer)
        CFRelease(info->referrer);
    if (info->path)
        CFRelease(info->path);

    free(info);
    [pool drain];

    return 0;
}

- (void)_webkit_setMetadataURL:(NSString *)URLString referrer:(NSString *)referrer atPath:(NSString *)path
{
    ASSERT(URLString);
    ASSERT(path);

    NSURL *URL = [NSURL _web_URLWithUserTypedString:URLString];
    if (URL)
        URLString = [[URL _web_URLByRemovingUserInfo] _web_userVisibleString];
 
    // Spawn a background thread for WKSetMetadataURL because this function will not return until mds has
    // journaled the data we're're trying to set. Depending on what other I/O is going on, it can take some
    // time. 
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    MetaDataInfo *info = static_cast<MetaDataInfo *>(malloc(sizeof(MetaDataInfo)));
    
    info->URLString = URLString ? CFStringCreateCopy(0, (CFStringRef)URLString) : 0;
    info->referrer = referrer ? CFStringCreateCopy(0, (CFStringRef)referrer) : 0;
    info->path = path ? CFStringCreateCopy(0, (CFStringRef)path) : 0;

    pthread_create(&tid, &attr, setMetaData, info);
    pthread_attr_destroy(&attr);
}

- (NSString *)_webkit_startupVolumeName
{
    RetainPtr<DASessionRef> session = adoptCF(DASessionCreate(kCFAllocatorDefault));
    RetainPtr<DADiskRef> disk = adoptCF(DADiskCreateFromVolumePath(kCFAllocatorDefault, session.get(), (CFURLRef)[NSURL fileURLWithPath:@"/"]));
    RetainPtr<CFDictionaryRef> diskDescription = adoptCF(DADiskCopyDescription(disk.get()));
    RetainPtr<NSString> diskName = (NSString *)CFDictionaryGetValue(diskDescription.get(), kDADiskDescriptionVolumeNameKey);
    return WebCFAutorelease(diskName.leakRef());
}

// -[NSFileManager fileExistsAtPath:] returns NO if there is a broken symlink at the path.
// So we use this function instead, which returns YES if there is anything there, including
// a broken symlink.
static BOOL fileExists(NSString *path)
{
    struct stat statBuffer;
    return !lstat([path fileSystemRepresentation], &statBuffer);
}

- (NSString *)_webkit_pathWithUniqueFilenameForPath:(NSString *)path
{
    // "Fix" the filename of the path.
    NSString *filename = [[path lastPathComponent] _webkit_filenameByFixingIllegalCharacters];
    path = [[path stringByDeletingLastPathComponent] stringByAppendingPathComponent:filename];

    if (fileExists(path)) {
        // Don't overwrite existing file by appending "-n", "-n.ext" or "-n.ext.ext" to the filename.
        NSString *extensions = nil;
        NSString *pathWithoutExtensions;
        NSString *lastPathComponent = [path lastPathComponent];
        NSRange periodRange = [lastPathComponent rangeOfString:@"."];
        
        if (periodRange.location == NSNotFound) {
            pathWithoutExtensions = path;
        } else {
            extensions = [lastPathComponent substringFromIndex:periodRange.location + 1];
            lastPathComponent = [lastPathComponent substringToIndex:periodRange.location];
            pathWithoutExtensions = [[path stringByDeletingLastPathComponent] stringByAppendingPathComponent:lastPathComponent];
        }

        for (unsigned i = 1; ; i++) {
            NSString *pathWithAppendedNumber = [NSString stringWithFormat:@"%@-%d", pathWithoutExtensions, i];
            path = [extensions length] ? [pathWithAppendedNumber stringByAppendingPathExtension:extensions] : pathWithAppendedNumber;
            if (!fileExists(path))
                break;
        }
    }

    return path;
}

@end

