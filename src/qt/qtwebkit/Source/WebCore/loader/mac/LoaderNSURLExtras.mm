/*
 * Copyright (C) 2005, 2008, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
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
#import "LoaderNSURLExtras.h"

#import <wtf/Assertions.h>
#import <wtf/Vector.h>
#import "KURL.h"
#import "LocalizedStrings.h"
#import "MIMETypeRegistry.h"
#import "WebCoreNSStringExtras.h"
#import <wtf/text/WTFString.h>

using namespace WebCore;

static bool vectorContainsString(const Vector<String>& vector, const String& string)
{
    int size = vector.size();
    for (int i = 0; i < size; i++)
        if (vector[i] == string)
            return true;
    return false;
}

NSString *suggestedFilenameWithMIMEType(NSURL *url, const String& mimeType)
{
    // Get the filename from the URL. Try the lastPathComponent first.
    NSString *lastPathComponent = [[url path] lastPathComponent];
    NSString *filename = filenameByFixingIllegalCharacters(lastPathComponent);
    NSString *extension = nil;

    if ([filename length] == 0 || [lastPathComponent isEqualToString:@"/"]) {
        // lastPathComponent is no good, try the host.
        NSString *host = KURL(url).host();
        filename = filenameByFixingIllegalCharacters(host);
        if ([filename length] == 0) {
            // Can't make a filename using this URL, use "unknown".
            filename = copyImageUnknownFileLabel();
        }
    } else {
        // Save the extension for later correction. Only correct the extension of the lastPathComponent.
        // For example, if the filename ends up being the host, we wouldn't want to correct ".com" in "www.apple.com".
        extension = [filename pathExtension];
    }

    // Do not correct filenames that are reported with a mime type of tar, and 
    // have a filename which has .tar in it or ends in .tgz
    if ((mimeType == "application/tar" || mimeType == "application/x-tar")
        && (hasCaseInsensitiveSubstring(filename, @".tar")
        || hasCaseInsensitiveSuffix(filename, @".tgz"))) {
        return filename;
    }

    // I don't think we need to worry about this for the image case
    // If the type is known, check the extension and correct it if necessary.
    if (mimeType != "application/octet-stream" && mimeType != "text/plain") {
        Vector<String> extensions = MIMETypeRegistry::getExtensionsForMIMEType(mimeType);

        if (extensions.isEmpty() || !vectorContainsString(extensions, extension)) {
            // The extension doesn't match the MIME type. Correct this.
            NSString *correctExtension = MIMETypeRegistry::getPreferredExtensionForMIMEType(mimeType);
            if ([correctExtension length] != 0) {
                // Append the correct extension.
                filename = [filename stringByAppendingPathExtension:correctExtension];
            }
        }
    }

    return filename;
}
