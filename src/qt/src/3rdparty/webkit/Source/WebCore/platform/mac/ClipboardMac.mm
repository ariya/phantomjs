/*
 * Copyright (C) 2004, 2005, 2006, 2008, 2010 Apple Inc. All rights reserved.
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
#import "ClipboardMac.h"

#import "DOMElementInternal.h"
#import "DragClient.h"
#import "DragController.h"
#import "DragData.h"
#import "Editor.h"
#import "FileList.h"
#import "Frame.h"
#import "Image.h"
#import "Page.h"
#import "Pasteboard.h"
#import "RenderImage.h"
#import "ScriptExecutionContext.h"
#import "SecurityOrigin.h"
#import "WebCoreSystemInterface.h"


namespace WebCore {

PassRefPtr<Clipboard> Clipboard::create(ClipboardAccessPolicy policy, DragData* dragData, Frame* frame)
{
    return ClipboardMac::create(DragAndDrop, dragData->pasteboard(), policy, frame);
}

ClipboardMac::ClipboardMac(ClipboardType clipboardType, NSPasteboard *pasteboard, ClipboardAccessPolicy policy, Frame *frame)
    : Clipboard(policy, clipboardType)
    , m_pasteboard(pasteboard)
    , m_frame(frame)
{
    m_changeCount = [m_pasteboard.get() changeCount];
}

ClipboardMac::~ClipboardMac()
{
}

bool ClipboardMac::hasData()
{
    return m_pasteboard && [m_pasteboard.get() types] && [[m_pasteboard.get() types] count] > 0;
}
    
static RetainPtr<NSString> cocoaTypeFromHTMLClipboardType(const String& type)
{
    String qType = type.stripWhiteSpace();

    // two special cases for IE compatibility
    if (qType == "Text")
        return NSStringPboardType;
    if (qType == "URL")
        return NSURLPboardType;

    // Ignore any trailing charset - JS strings are Unicode, which encapsulates the charset issue
    if (qType == "text/plain" || qType.startsWith("text/plain;"))
        return NSStringPboardType;
    if (qType == "text/uri-list")
        // special case because UTI doesn't work with Cocoa's URL type
        return NSURLPboardType; // note special case in getData to read NSFilenamesType
    
    // Try UTI now
    NSString *mimeType = qType;
    RetainPtr<CFStringRef> utiType(AdoptCF, UTTypeCreatePreferredIdentifierForTag(kUTTagClassMIMEType, (CFStringRef)mimeType, NULL));
    if (utiType) {
        CFStringRef pbType = UTTypeCopyPreferredTagWithClass(utiType.get(), kUTTagClassNSPboardType);
        if (pbType)
            return (NSString *)pbType;
    }

    // No mapping, just pass the whole string though
    return (NSString *)qType;
}

static String utiTypeFromCocoaType(NSString *type)
{
    RetainPtr<CFStringRef> utiType(AdoptCF, UTTypeCreatePreferredIdentifierForTag(kUTTagClassNSPboardType, (CFStringRef)type, NULL));
    if (utiType) {
        RetainPtr<CFStringRef> mimeType(AdoptCF, UTTypeCopyPreferredTagWithClass(utiType.get(), kUTTagClassMIMEType));
        if (mimeType)
            return String(mimeType.get());
    }
    return String();
}

static void addHTMLClipboardTypesForCocoaType(HashSet<String>& resultTypes, NSString *cocoaType, NSPasteboard *pasteboard)
{
    // UTI may not do these right, so make sure we get the right, predictable result
    if ([cocoaType isEqualToString:NSStringPboardType]) {
        resultTypes.add("text/plain");
        return;
    }
    if ([cocoaType isEqualToString:NSURLPboardType]) {
        resultTypes.add("text/uri-list");
        return;
    }
    if ([cocoaType isEqualToString:NSFilenamesPboardType]) {
        // If file list is empty, add nothing.
        // Note that there is a chance that the file list count could have changed since we grabbed the types array.
        // However, this is not really an issue for us doing a sanity check here.
        NSArray *fileList = [pasteboard propertyListForType:NSFilenamesPboardType];
        if ([fileList count]) {
            // It is unknown if NSFilenamesPboardType always implies NSURLPboardType in Cocoa,
            // but NSFilenamesPboardType should imply both 'text/uri-list' and 'Files'
            resultTypes.add("text/uri-list");
            resultTypes.add("Files");
        }
        return;
    }
    String utiType = utiTypeFromCocoaType(cocoaType);
    if (!utiType.isEmpty()) {
        resultTypes.add(utiType);
        return;
    }
    // No mapping, just pass the whole string though
    resultTypes.add(cocoaType);
}

void ClipboardMac::clearData(const String& type)
{
    if (policy() != ClipboardWritable)
        return;

    // note NSPasteboard enforces changeCount itself on writing - can't write if not the owner

    if (RetainPtr<NSString> cocoaType = cocoaTypeFromHTMLClipboardType(type))
        [m_pasteboard.get() setString:@"" forType:cocoaType.get()];
}

void ClipboardMac::clearAllData()
{
    if (policy() != ClipboardWritable)
        return;

    // note NSPasteboard enforces changeCount itself on writing - can't write if not the owner

    [m_pasteboard.get() declareTypes:[NSArray array] owner:nil];
}

static NSArray *absoluteURLsFromPasteboardFilenames(NSPasteboard* pasteboard, bool onlyFirstURL = false)
{
    NSArray *fileList = [pasteboard propertyListForType:NSFilenamesPboardType];

    // FIXME: Why does this code need to guard against bad values on the pasteboard?
    ASSERT(!fileList || [fileList isKindOfClass:[NSArray class]]);
    if (!fileList || ![fileList isKindOfClass:[NSArray class]] || ![fileList count])
        return nil;

    NSUInteger count = onlyFirstURL ? 1 : [fileList count];
    NSMutableArray *urls = [NSMutableArray array];
    for (NSUInteger i = 0; i < count; i++) {
        NSString *string = [fileList objectAtIndex:i];

        ASSERT([string isKindOfClass:[NSString class]]);  // Added to understand why this if code is here
        if (![string isKindOfClass:[NSString class]])
            return nil; // Non-string object in the list, bail out!  FIXME: When can this happen?

        NSURL *url = [NSURL fileURLWithPath:string];
        [urls addObject:[url absoluteString]];
    }
    return urls;
}

static NSArray *absoluteURLsFromPasteboard(NSPasteboard* pasteboard, bool onlyFirstURL = false)
{
    // NOTE: We must always check [availableTypes containsObject:] before accessing pasteboard data
    // or CoreFoundation will printf when there is not data of the corresponding type.
    NSArray *availableTypes = [pasteboard types];

    // Try NSFilenamesPboardType because it contains a list
    if ([availableTypes containsObject:NSFilenamesPboardType]) {
        if (NSArray* absoluteURLs = absoluteURLsFromPasteboardFilenames(pasteboard, onlyFirstURL))
            return absoluteURLs;
    }

    // Fallback to NSURLPboardType (which is a single URL)
    if ([availableTypes containsObject:NSURLPboardType]) {
        if (NSURL *url = [NSURL URLFromPasteboard:pasteboard])
            return [NSArray arrayWithObject:[url absoluteString]];
    }

    // No file paths on the pasteboard, return nil
    return nil;
}

String ClipboardMac::getData(const String& type, bool& success) const
{
    success = false;
    if (policy() != ClipboardReadable)
        return String();

    RetainPtr<NSString> cocoaType = cocoaTypeFromHTMLClipboardType(type);
    NSString *cocoaValue = nil;

    // Grab the value off the pasteboard corresponding to the cocoaType
    if ([cocoaType.get() isEqualToString:NSURLPboardType]) {
        // "URL" and "text/url-list" both map to NSURLPboardType in cocoaTypeFromHTMLClipboardType(), "URL" only wants the first URL
        bool onlyFirstURL = (type == "URL");
        NSArray *absoluteURLs = absoluteURLsFromPasteboard(m_pasteboard.get(), onlyFirstURL);
        cocoaValue = [absoluteURLs componentsJoinedByString:@"\n"];
    } else if ([cocoaType.get() isEqualToString:NSStringPboardType]) {
        cocoaValue = [[m_pasteboard.get() stringForType:cocoaType.get()] precomposedStringWithCanonicalMapping];
    } else if (cocoaType)
        cocoaValue = [m_pasteboard.get() stringForType:cocoaType.get()];

    // Enforce changeCount ourselves for security.  We check after reading instead of before to be
    // sure it doesn't change between our testing the change count and accessing the data.
    if (cocoaValue && m_changeCount == [m_pasteboard.get() changeCount]) {
        success = true;
        return cocoaValue;
    }

    return String();
}

bool ClipboardMac::setData(const String &type, const String &data)
{
    if (policy() != ClipboardWritable)
        return false;
    // note NSPasteboard enforces changeCount itself on writing - can't write if not the owner

    RetainPtr<NSString> cocoaType = cocoaTypeFromHTMLClipboardType(type);
    NSString *cocoaData = data;

    if ([cocoaType.get() isEqualToString:NSURLPboardType]) {
        [m_pasteboard.get() addTypes:[NSArray arrayWithObject:NSURLPboardType] owner:nil];
        NSURL *url = [[NSURL alloc] initWithString:cocoaData];
        [url writeToPasteboard:m_pasteboard.get()];

        if ([url isFileURL] && m_frame->document()->securityOrigin()->canLoadLocalResources()) {
            [m_pasteboard.get() addTypes:[NSArray arrayWithObject:NSFilenamesPboardType] owner:nil];
            NSArray *fileList = [NSArray arrayWithObject:[url path]];
            [m_pasteboard.get() setPropertyList:fileList forType:NSFilenamesPboardType];
        }

        [url release];
        return true;
    }

    if (cocoaType) {
        // everything else we know of goes on the pboard as a string
        [m_pasteboard.get() addTypes:[NSArray arrayWithObject:cocoaType.get()] owner:nil];
        return [m_pasteboard.get() setString:cocoaData forType:cocoaType.get()];
    }

    return false;
}

HashSet<String> ClipboardMac::types() const
{
    if (policy() != ClipboardReadable && policy() != ClipboardTypesReadable)
        return HashSet<String>();

    NSArray *types = [m_pasteboard.get() types];

    // Enforce changeCount ourselves for security.  We check after reading instead of before to be
    // sure it doesn't change between our testing the change count and accessing the data.
    if (m_changeCount != [m_pasteboard.get() changeCount])
        return HashSet<String>();

    HashSet<String> result;
    NSUInteger count = [types count];
    // FIXME: This loop could be split into two stages. One which adds all the HTML5 specified types
    // and a second which adds all the extra types from the cocoa clipboard (which is Mac-only behavior).
    for (NSUInteger i = 0; i < count; i++) {
        NSString *pbType = [types objectAtIndex:i];
        if ([pbType isEqualToString:@"NeXT plain ascii pasteboard type"])
            continue;   // skip this ancient type that gets auto-supplied by some system conversion

        addHTMLClipboardTypesForCocoaType(result, pbType, m_pasteboard.get());
    }

    return result;
}

// FIXME: We could cache the computed fileList if necessary
// Currently each access gets a new copy, setData() modifications to the
// clipboard are not reflected in any FileList objects the page has accessed and stored
PassRefPtr<FileList> ClipboardMac::files() const
{
    if (policy() != ClipboardReadable)
        return FileList::create();

    NSArray *absoluteURLs = absoluteURLsFromPasteboardFilenames(m_pasteboard.get());
    NSUInteger count = [absoluteURLs count];

    RefPtr<FileList> fileList = FileList::create();
    for (NSUInteger x = 0; x < count; x++) {
        NSURL *absoluteURL = [NSURL URLWithString:[absoluteURLs objectAtIndex:x]];
        ASSERT([absoluteURL isFileURL]);
        fileList->append(File::create([absoluteURL path]));
    }
    return fileList.release(); // We will always return a FileList, sometimes empty
}

// The rest of these getters don't really have any impact on security, so for now make no checks

void ClipboardMac::setDragImage(CachedImage* img, const IntPoint &loc)
{
    setDragImage(img, 0, loc);
}

void ClipboardMac::setDragImageElement(Node *node, const IntPoint &loc)
{
    setDragImage(0, node, loc);
}

void ClipboardMac::setDragImage(CachedImage* image, Node *node, const IntPoint &loc)
{
    if (policy() == ClipboardImageWritable || policy() == ClipboardWritable) {
        if (m_dragImage)
            m_dragImage->removeClient(this);
        m_dragImage = image;
        if (m_dragImage)
            m_dragImage->addClient(this);

        m_dragLoc = loc;
        m_dragImageElement = node;
        
        if (dragStarted() && m_changeCount == [m_pasteboard.get() changeCount]) {
            NSPoint cocoaLoc;
            NSImage* cocoaImage = dragNSImage(cocoaLoc);
            if (cocoaImage) {
                // Dashboard wants to be able to set the drag image during dragging, but Cocoa does not allow this.
                // Instead we must drop down to the CoreGraphics API.
                wkSetDragImage(cocoaImage, cocoaLoc);

                // Hack: We must post an event to wake up the NSDragManager, which is sitting in a nextEvent call
                // up the stack from us because the CoreFoundation drag manager does not use the run loop by itself.
                // This is the most innocuous event to use, per Kristen Forster.
                NSEvent* ev = [NSEvent mouseEventWithType:NSMouseMoved location:NSZeroPoint
                    modifierFlags:0 timestamp:0 windowNumber:0 context:nil eventNumber:0 clickCount:0 pressure:0];
                [NSApp postEvent:ev atStart:YES];
            }
        }
        // Else either 1) we haven't started dragging yet, so we rely on the part to install this drag image
        // as part of getting the drag kicked off, or 2) Someone kept a ref to the clipboard and is trying to
        // set the image way too late.
    }
}
    
void ClipboardMac::writeRange(Range* range, Frame* frame)
{
    ASSERT(range);
    ASSERT(frame);
    Pasteboard::writeSelection(m_pasteboard.get(), 0, range, frame->editor()->smartInsertDeleteEnabled() && frame->selection()->granularity() == WordGranularity, frame);
}

void ClipboardMac::writePlainText(const String& text)
{
    Pasteboard::writePlainText(m_pasteboard.get(), text);
}

void ClipboardMac::writeURL(const KURL& url, const String& title, Frame* frame)
{   
    ASSERT(frame);
    ASSERT(m_pasteboard);
    Pasteboard::writeURL(m_pasteboard.get(), nil, url, title, frame);
}
    
#if ENABLE(DRAG_SUPPORT)
void ClipboardMac::declareAndWriteDragImage(Element* element, const KURL& url, const String& title, Frame* frame)
{
    ASSERT(frame);
    if (Page* page = frame->page())
        page->dragController()->client()->declareAndWriteDragImage(m_pasteboard.get(), kit(element), url, title, frame);
}
#endif // ENABLE(DRAG_SUPPORT)
    
DragImageRef ClipboardMac::createDragImage(IntPoint& loc) const
{
    NSPoint nsloc = {loc.x(), loc.y()};
    DragImageRef result = dragNSImage(nsloc);
    loc = (IntPoint)nsloc;
    return result;
}
    
NSImage *ClipboardMac::dragNSImage(NSPoint& loc) const
{
    NSImage *result = nil;
    if (m_dragImageElement) {
        if (m_frame) {
            NSRect imageRect;
            NSRect elementRect;
            result = m_frame->snapshotDragImage(m_dragImageElement.get(), &imageRect, &elementRect);
            // Client specifies point relative to element, not the whole image, which may include child
            // layers spread out all over the place.
            loc.x = elementRect.origin.x - imageRect.origin.x + m_dragLoc.x();
            loc.y = elementRect.origin.y - imageRect.origin.y + m_dragLoc.y();
            loc.y = imageRect.size.height - loc.y;
        }
    } else if (m_dragImage) {
        result = m_dragImage->image()->getNSImage();
        
        loc = m_dragLoc;
        loc.y = [result size].height - loc.y;
    }
    return result;
}

}
