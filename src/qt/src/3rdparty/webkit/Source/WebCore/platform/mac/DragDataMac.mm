/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"
#import "DragData.h"

#if ENABLE(DRAG_SUPPORT)
#import "Document.h"
#import "DocumentFragment.h"
#import "DOMDocumentFragment.h"
#import "DOMDocumentFragmentInternal.h"
#import "MIMETypeRegistry.h"
#import "Pasteboard.h"
#import "Range.h"

namespace WebCore {

DragData::DragData(DragDataRef data, const IntPoint& clientPosition, const IntPoint& globalPosition, 
    DragOperation sourceOperationMask, DragApplicationFlags flags)
    : m_clientPosition(clientPosition)
    , m_globalPosition(globalPosition)
    , m_platformDragData(data)
    , m_draggingSourceOperationMask(sourceOperationMask)
    , m_applicationFlags(flags)
    , m_pasteboard([m_platformDragData draggingPasteboard])
{
}

DragData::DragData(const String& dragStorageName, const IntPoint& clientPosition, const IntPoint& globalPosition,
    DragOperation sourceOperationMask, DragApplicationFlags flags)
    : m_clientPosition(clientPosition)
    , m_globalPosition(globalPosition)
    , m_platformDragData(0)
    , m_draggingSourceOperationMask(sourceOperationMask)
    , m_applicationFlags(flags)
    , m_pasteboard([NSPasteboard pasteboardWithName:dragStorageName])
{
}
    
bool DragData::canSmartReplace() const
{
    //Need to call this so that the various Pasteboard type strings are intialised
    Pasteboard::generalPasteboard();
    return [[m_pasteboard.get() types] containsObject:WebSmartPastePboardType];
}

bool DragData::containsColor() const
{
    return [[m_pasteboard.get() types] containsObject:NSColorPboardType];
}

bool DragData::containsFiles() const
{
    return [[m_pasteboard.get() types] containsObject:NSFilenamesPboardType];
}

void DragData::asFilenames(Vector<String>& result) const
{
    NSArray *filenames = [m_pasteboard.get() propertyListForType:NSFilenamesPboardType];
    NSEnumerator *fileEnumerator = [filenames objectEnumerator];
    
    while (NSString *filename = [fileEnumerator nextObject])
        result.append(filename);
}

bool DragData::containsPlainText() const
{
    NSArray *types = [m_pasteboard.get() types];
    
    return [types containsObject:NSStringPboardType] 
        || [types containsObject:NSRTFDPboardType]
        || [types containsObject:NSRTFPboardType]
        || [types containsObject:NSFilenamesPboardType]
        || [NSURL URLFromPasteboard:m_pasteboard.get()];
}

String DragData::asPlainText(Frame *frame) const
{
    Pasteboard pasteboard(m_pasteboard.get());
    return pasteboard.plainText(frame);
}

Color DragData::asColor() const
{
    NSColor *color = [NSColor colorFromPasteboard:m_pasteboard.get()];
    return makeRGBA((int)([color redComponent] * 255.0 + 0.5), (int)([color greenComponent] * 255.0 + 0.5), 
                    (int)([color blueComponent] * 255.0 + 0.5), (int)([color alphaComponent] * 255.0 + 0.5));
}

static NSArray *insertablePasteboardTypes()
{
    static NSArray *types = nil;
    if (!types) {
        types = [[NSArray alloc] initWithObjects:WebArchivePboardType, NSHTMLPboardType, NSFilenamesPboardType, NSTIFFPboardType, NSPDFPboardType,
#ifdef BUILDING_ON_LEOPARD
                 NSPICTPboardType,
#endif
                 NSURLPboardType, NSRTFDPboardType, NSRTFPboardType, NSStringPboardType, NSColorPboardType, kUTTypePNG, nil];
        CFRetain(types);
    }
    return types;
}
    
bool DragData::containsCompatibleContent() const
{
    NSMutableSet *types = [NSMutableSet setWithArray:[m_pasteboard.get() types]];
    [types intersectSet:[NSSet setWithArray:insertablePasteboardTypes()]];
    return [types count] != 0;
}
    
bool DragData::containsURL(Frame* frame, FilenameConversionPolicy filenamePolicy) const
{
    return !asURL(frame, filenamePolicy).isEmpty();
}
    
String DragData::asURL(Frame* frame, FilenameConversionPolicy filenamePolicy, String* title) const
{
    // FIXME: Use filenamePolicy.
    (void)filenamePolicy;

    if (title) {
        if (NSString *URLTitleString = [m_pasteboard.get() stringForType:WebURLNamePboardType])
            *title = URLTitleString;
    }
    Pasteboard pasteboard(m_pasteboard.get());
    return pasteboard.asURL(frame);
}

PassRefPtr<DocumentFragment> DragData::asFragment(Frame* frame, PassRefPtr<Range> range, bool allowPlainText, bool& chosePlainText) const
{
    Pasteboard pasteboard(m_pasteboard.get());
    
    return pasteboard.documentFragment(frame, range, allowPlainText, chosePlainText);
}
    
} // namespace WebCore

#endif // ENABLE(DRAG_SUPPORT)
