/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#import "UTIUtilities.h"

#if PLATFORM(IOS)
#import "SoftLinking.h"
#import <MobileCoreServices/MobileCoreServices.h>

SOFT_LINK_FRAMEWORK(MobileCoreServices)

SOFT_LINK(MobileCoreServices, UTTypeCopyDeclaration, CFDictionaryRef, (CFStringRef inUTI), (inUTI))
SOFT_LINK(MobileCoreServices, UTTypeCopyPreferredTagWithClass, CFStringRef, (CFStringRef inUTI, CFStringRef inTagClass), (inUTI, inTagClass))

SOFT_LINK_CONSTANT(MobileCoreServices, kUTTypeConformsToKey, CFStringRef)
SOFT_LINK_CONSTANT(MobileCoreServices, kUTTagClassMIMEType, CFStringRef)

#define kUTTagClassMIMEType getkUTTagClassMIMEType()
#define kUTTypeConformsToKey getkUTTypeConformsToKey()
#endif

namespace WebCore {

RetainPtr<CFStringRef> mimeTypeFromUTITree(CFStringRef uti)
{
    // Check if this UTI has a MIME type.
    RetainPtr<CFStringRef> mimeType = adoptCF(UTTypeCopyPreferredTagWithClass(uti, kUTTagClassMIMEType));
    if (mimeType)
        return mimeType.get();

    // If not, walk the ancestory of this UTI via its "ConformsTo" tags and return the first MIME type we find.
    RetainPtr<CFDictionaryRef> decl = adoptCF(UTTypeCopyDeclaration(uti));
    if (!decl)
        return nil;
    CFTypeRef value = CFDictionaryGetValue(decl.get(), kUTTypeConformsToKey);
    if (!value)
        return nil;
    CFTypeID typeID = CFGetTypeID(value);

    if (typeID == CFStringGetTypeID())
        return mimeTypeFromUTITree((CFStringRef)value);

    if (typeID == CFArrayGetTypeID()) {
        CFArrayRef newTypes = (CFArrayRef)value;
        CFIndex count = CFArrayGetCount(newTypes);
        for (CFIndex i = 0; i < count; ++i) {
        CFTypeRef object = CFArrayGetValueAtIndex(newTypes, i);
        if (CFGetTypeID(object) != CFStringGetTypeID())
            continue;

        if (RetainPtr<CFStringRef> mimeType = mimeTypeFromUTITree((CFStringRef)object))
            return mimeType;
        }
    }

    return nil;
}

RetainPtr<CFStringRef> UTIFromMIMEType(CFStringRef mime)
{
    RetainPtr<CFStringRef> uti = adoptCF(UTTypeCreatePreferredIdentifierForTag(kUTTagClassMIMEType, mime, 0));
    return uti;
}

}
