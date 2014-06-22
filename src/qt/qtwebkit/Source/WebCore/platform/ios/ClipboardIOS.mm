/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "ClipboardIOS.h"

#import "DragData.h"
#import "Editor.h"
#import "EditorClient.h"
#import "FileList.h"
#import "Frame.h"
#import "Pasteboard.h"

#include "SoftLinking.h"
#include <MobileCoreServices/MobileCoreServices.h>

SOFT_LINK_FRAMEWORK(MobileCoreServices)

SOFT_LINK(MobileCoreServices, UTTypeCreatePreferredIdentifierForTag, CFStringRef, (CFStringRef inTagClass, CFStringRef inTag, CFStringRef inConformingToUTI), (inTagClass, inTag, inConformingToUTI))
SOFT_LINK(MobileCoreServices, UTTypeCopyPreferredTagWithClass, CFStringRef, (CFStringRef inUTI, CFStringRef inTagClass), (inUTI, inTagClass))

SOFT_LINK_CONSTANT(MobileCoreServices, kUTTypeText, CFStringRef)
SOFT_LINK_CONSTANT(MobileCoreServices, kUTTypeURL, CFStringRef)
SOFT_LINK_CONSTANT(MobileCoreServices, kUTTagClassMIMEType, CFStringRef)

#define kUTTypeText getkUTTypeText()
#define kUTTypeURL  getkUTTypeURL()
#define kUTTypeTIFF getkUTTypeTIFF()
#define kUTTagClassMIMEType getkUTTagClassMIMEType()

namespace WebCore {

PassRefPtr<Clipboard> Clipboard::create(ClipboardAccessPolicy policy, DragData*, Frame* frame)
{
    return ClipboardIOS::create(DragAndDrop, policy, frame);
}

ClipboardIOS::ClipboardIOS(ClipboardType clipboardType, ClipboardAccessPolicy policy, Frame* frame)
    : Clipboard(policy, clipboardType)
    , m_frame(frame)
{
    m_changeCount = m_frame->editor().client()->pasteboardChangeCount();
}

ClipboardIOS::~ClipboardIOS()
{
}

bool ClipboardIOS::hasData()
{
    return m_frame->editor().client()->getPasteboardItemsCount() != 0;
}

static String utiTypeFromCocoaType(NSString* type)
{
    RetainPtr<CFStringRef> utiType = adoptCF(UTTypeCreatePreferredIdentifierForTag(kUTTagClassMIMEType, (CFStringRef)type, NULL));
    if (utiType) {
        RetainPtr<CFStringRef> mimeType = adoptCF(UTTypeCopyPreferredTagWithClass(utiType.get(), kUTTagClassMIMEType));
        if (mimeType)
            return String(mimeType.get());
    }
    return String();
}

static RetainPtr<NSString> cocoaTypeFromHTMLClipboardType(const String& type)
{
    String qType = type.stripWhiteSpace();

    if (qType == "Text")
        return (NSString*)kUTTypeText;
    if (qType == "URL")
        return (NSString*)kUTTypeURL;

    // Ignore any trailing charset - JS strings are Unicode, which encapsulates the charset issue.
    if (qType.startsWith(ASCIILiteral("text/plain")))
        return (NSString*)kUTTypeText;
    if (qType == "text/uri-list")
        // Special case because UTI doesn't work with Cocoa's URL type.
        return (NSString*)kUTTypeURL;

    // Try UTI now.
    NSString *pbType = utiTypeFromCocoaType(qType);
    if (pbType)
        return pbType;

    // No mapping, just pass the whole string though.
    return (NSString*)qType;
}

static void addHTMLClipboardTypesForCocoaType(ListHashSet<String>& resultTypes, NSString* cocoaType)
{
    // UTI may not do these right, so make sure we get the right, predictable result.
    if ([cocoaType isEqualToString:(NSString*)kUTTypeText]) {
        resultTypes.add(ASCIILiteral("text/plain"));
        return;
    }
    if ([cocoaType isEqualToString:(NSString*)kUTTypeURL]) {
        resultTypes.add(ASCIILiteral("text/uri-list"));
        return;
    }
    String utiType = utiTypeFromCocoaType(cocoaType);
    if (!utiType.isEmpty()) {
        resultTypes.add(utiType);
        return;
    }
    // No mapping, just pass the whole string though.
    resultTypes.add(cocoaType);
}

void ClipboardIOS::clearData(const String& type)
{
    if (!canWriteData())
        return;

    // Note UIPasteboard enforces changeCount itself on writing - can't write if not the owner.

    if (RetainPtr<NSString> cocoaType = cocoaTypeFromHTMLClipboardType(type)) {
        RetainPtr<NSDictionary> representations = adoptNS([[NSMutableDictionary alloc] init]);
        [representations.get() setValue:0 forKey:cocoaType.get()];
        m_frame->editor().client()->writeDataToPasteboard(representations.get());
    }
}

void ClipboardIOS::clearData()
{
    if (!canWriteData())
        return;

    RetainPtr<NSDictionary> representations = adoptNS([[NSMutableDictionary alloc] init]);
    m_frame->editor().client()->writeDataToPasteboard(representations.get());
}

String ClipboardIOS::getData(const String& type) const
{
    if (!canReadData())
        return String();

    RetainPtr<NSString> cocoaType = cocoaTypeFromHTMLClipboardType(type);
    NSString *cocoaValue = nil;

    // Grab the value off the pasteboard corresponding to the cocoaType.
    RetainPtr<NSArray> pasteboardItem = m_frame->editor().client()->readDataFromPasteboard(cocoaType.get(), 0);

    if ([pasteboardItem.get() count] == 0)
        return String();

    if ([cocoaType.get() isEqualToString:(NSString*)kUTTypeURL]) {
        id value = [pasteboardItem.get() objectAtIndex:0];
        if (![value isKindOfClass:[NSURL class]]) {
            ASSERT([value isKindOfClass:[NSURL class]]);
            return String();
        }
        NSURL* absoluteURL = (NSURL*)value;

        if (absoluteURL)
            cocoaValue = [absoluteURL absoluteString];
    } else if ([cocoaType.get() isEqualToString:(NSString*)kUTTypeText]) {
        id value = [pasteboardItem.get() objectAtIndex:0];
        if (![value isKindOfClass:[NSString class]]) {
            ASSERT([value isKindOfClass:[NSString class]]);
            return String();
        }

        cocoaValue = [(NSString*)value precomposedStringWithCanonicalMapping];
    } else if (cocoaType) {
        ASSERT([pasteboardItem.get() count] == 1);
        id value = [pasteboardItem.get() objectAtIndex:0];
        if (![value isKindOfClass:[NSData class]]) {
            ASSERT([value isKindOfClass:[NSData class]]);
            return String();
        }
        cocoaValue = [[[NSString alloc] initWithData:(NSData *)value encoding:NSUTF8StringEncoding] autorelease];
    }

    // Enforce changeCount ourselves for security.  We check after reading instead of before to be
    // sure it doesn't change between our testing the change count and accessing the data.
    if (cocoaValue && m_changeCount == m_frame->editor().client()->pasteboardChangeCount())
        return cocoaValue;

    return String();
}

bool ClipboardIOS::setData(const String& type, const String& data)
{
    if (!canWriteData())
        return false;

    RetainPtr<NSString> cocoaType = cocoaTypeFromHTMLClipboardType(type);
    NSString *cocoaData = data;

    RetainPtr<NSDictionary> representations = adoptNS([[NSMutableDictionary alloc] init]);

    if ([cocoaType.get() isEqualToString:(NSString*)kUTTypeURL]) {
        RetainPtr<NSURL> url = adoptNS([[NSURL alloc] initWithString:cocoaData]);
        [representations.get() setValue:url.get() forKey:(NSString*)kUTTypeURL];
        m_frame->editor().client()->writeDataToPasteboard(representations.get());
        return true;
    }

    if (cocoaType) {
        // Everything else we know of goes on the pboard as the original string
        // we received as parameter.
        [representations.get() setValue:cocoaData forKey:cocoaType.get()];
        m_frame->editor().client()->writeDataToPasteboard(representations.get());
        return true;
    }

    return false;
}

ListHashSet<String> ClipboardIOS::types() const
{
    if (!canReadTypes())
        return ListHashSet<String>();

    NSArray* types = Pasteboard::supportedPasteboardTypes();

    // Enforce changeCount ourselves for security.  We check after reading instead of before to be
    // sure it doesn't change between our testing the change count and accessing the data.
    if (m_changeCount != m_frame->editor().client()->pasteboardChangeCount())
        return ListHashSet<String>();

    ListHashSet<String> result;
    NSUInteger count = [types count];
    for (NSUInteger i = 0; i < count; i++) {
        NSString* pbType = [types objectAtIndex:i];
        addHTMLClipboardTypesForCocoaType(result, pbType);
    }

    return result;
}

PassRefPtr<FileList> ClipboardIOS::files() const
{
    return 0;
}

void ClipboardIOS::writeRange(Range* range, Frame* frame)
{
    ASSERT(range);
    ASSERT(frame);
    Pasteboard::generalPasteboard()->writeSelection(range, frame->editor().smartInsertDeleteEnabled() && frame->selection()->granularity() == WordGranularity, frame);
}

void ClipboardIOS::writePlainText(const String& text)
{
    Pasteboard::generalPasteboard()->writePlainText(text, m_frame);
}

void ClipboardIOS::writeURL(const KURL&, const String&, Frame*)
{
}

#if ENABLE(DRAG_SUPPORT)
void ClipboardIOS::declareAndWriteDragImage(Element* element, const KURL& url, const String& title, Frame* frame)
{
    ASSERT(frame);
    if (Page* page = frame->page())
        page->dragController()->client()->declareAndWriteDragImage(m_pasteboard.get(), kit(element), url, title, frame);
}
#endif // ENABLE(DRAG_SUPPORT)

DragImageRef ClipboardIOS::createDragImage(IntPoint&) const
{
    return 0;
}

void ClipboardIOS::setDragImage(CachedImage*, const IntPoint&)
{
}

void ClipboardIOS::setDragImageElement(Node *, const IntPoint&)
{
}

}
