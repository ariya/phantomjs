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
#import "PasteboardTypes.h"

#import <wtf/RetainPtr.h>

namespace WebKit {

NSString * const PasteboardTypes::WebArchivePboardType = @"Apple Web Archive pasteboard type";
NSString * const PasteboardTypes::WebURLsWithTitlesPboardType = @"WebURLsWithTitlesPboardType";
NSString * const PasteboardTypes::WebURLPboardType = @"public.url";
NSString * const PasteboardTypes::WebURLNamePboardType = @"public.url-name";

static inline NSArray *retain(NSArray *array)
{
    CFRetain(array);
    return array;
}
    
NSArray* PasteboardTypes::forEditing()
{
    static NSArray *types = retain([NSArray arrayWithObjects:WebArchivePboardType, NSHTMLPboardType, NSFilenamesPboardType, NSTIFFPboardType, NSPDFPboardType,
        NSURLPboardType, NSRTFDPboardType, NSRTFPboardType, NSStringPboardType, NSColorPboardType, kUTTypePNG, nil]);
    return types;
}

NSArray* PasteboardTypes::forURL()
{
    static NSArray *types = retain([NSArray arrayWithObjects:WebURLsWithTitlesPboardType, NSURLPboardType, WebURLPboardType,  WebURLNamePboardType, NSStringPboardType, NSFilenamesPboardType, nil]);
    return types;
}

NSArray* PasteboardTypes::forImages()
{
    static NSArray *types = retain([NSArray arrayWithObjects:NSTIFFPboardType, WebURLsWithTitlesPboardType, NSURLPboardType, WebURLPboardType, WebURLNamePboardType, NSStringPboardType, nil]);
    return types;
}

NSArray* PasteboardTypes::forImagesWithArchive()
{
    static NSArray *types = retain([NSArray arrayWithObjects:NSTIFFPboardType, WebURLsWithTitlesPboardType, NSURLPboardType, WebURLPboardType, WebURLNamePboardType, NSStringPboardType, NSRTFDPboardType, WebArchivePboardType, nil]);
    return types;
}

NSArray* PasteboardTypes::forSelection()
{
    static NSArray *types = retain([NSArray arrayWithObjects:WebArchivePboardType, NSRTFDPboardType, NSRTFPboardType, NSStringPboardType, nil]);
    return types;
}
    
} // namespace WebKit
