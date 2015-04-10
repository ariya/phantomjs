/*
 * Copyright (C) 2007, 2011 Apple Inc. All rights reserved.
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

#import "config.h"
#import "FileSystem.h"

#import "WebCoreNSURLExtras.h"
#import "WebCoreSystemInterface.h"
#import <wtf/RetainPtr.h>
#import <wtf/text/CString.h>
#import <wtf/text/WTFString.h>

namespace WebCore {

String homeDirectoryPath()
{
    return NSHomeDirectory();
}

String openTemporaryFile(const String& prefix, PlatformFileHandle& platformFileHandle)
{
    platformFileHandle = invalidPlatformFileHandle;
    
    Vector<char> temporaryFilePath(PATH_MAX);
    if (!confstr(_CS_DARWIN_USER_TEMP_DIR, temporaryFilePath.data(), temporaryFilePath.size()))
        return String();

    // Shrink the vector.   
    temporaryFilePath.shrink(strlen(temporaryFilePath.data()));
    ASSERT(temporaryFilePath.last() == '/');

    // Append the file name.
    CString prefixUtf8 = prefix.utf8();
    temporaryFilePath.append(prefixUtf8.data(), prefixUtf8.length());
    temporaryFilePath.append("XXXXXX", 6);
    temporaryFilePath.append('\0');

    platformFileHandle = mkstemp(temporaryFilePath.data());
    if (platformFileHandle == invalidPlatformFileHandle)
        return String();

    return String::fromUTF8(temporaryFilePath.data());
}

typedef struct MetaDataInfo
{
    String URLString;
    String referrer;
    String path;
} MetaDataInfo;

static void* setMetaData(void* context)
{
    MetaDataInfo *info = (MetaDataInfo *)context;
    wkSetMetadataURL((NSString *)info->URLString, (NSString *)info->referrer, (NSString *)String::fromUTF8(fileSystemRepresentation(info->path).data()));
    
    delete info;
    
    return 0;
}

void setMetadataURL(String& URLString, const String& referrer, const String& path)
{
    NSURL *URL = URLWithUserTypedString(URLString, nil);
    if (URL)
        URLString = userVisibleString(URLByRemovingUserInfo(URL));
    
    // Spawn a background thread for WKSetMetadataURL because this function will not return until mds has
    // journaled the data we're're trying to set. Depending on what other I/O is going on, it can take some
    // time. 
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    MetaDataInfo *info = new MetaDataInfo;
    
    info->URLString = URLString;
    info->referrer = referrer;
    info->path = path;
    
    pthread_create(&tid, &attr, setMetaData, info);
    pthread_attr_destroy(&attr);
}

#if !PLATFORM(IOS)
bool canExcludeFromBackup()
{
    return true;
}

bool excludeFromBackup(const String& path)
{
    // It is critical to pass FALSE for excludeByPath because excluding by path requires root privileges.
    CSBackupSetItemExcluded(pathAsURL(path).get(), TRUE, FALSE); 
    return true;
}
#endif

} // namespace WebCore
