/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
#import "Pasteboard.h"

#import "CachedResource.h"
#import "DOMRangeInternal.h"
#import "Document.h"
#import "DocumentFragment.h"
#import "DocumentLoader.h"
#import "Editor.h"
#import "EditorClient.h"
#import "Frame.h"
#import "FrameView.h"
#import "FrameLoaderClient.h"
#import "HitTestResult.h"
#import "HTMLAnchorElement.h"
#import "HTMLConverter.h"
#import "htmlediting.h"
#import "HTMLNames.h"
#import "Image.h"
#import "KURL.h"
#import "LegacyWebArchive.h"
#import "LoaderNSURLExtras.h"
#import "MIMETypeRegistry.h"
#import "Page.h"
#import "RenderImage.h"
#import "Text.h"
#import "WebCoreNSStringExtras.h"
#import "WebNSAttributedStringExtras.h"
#import "markup.h"
#import <wtf/StdLibExtras.h>
#import <wtf/RetainPtr.h>
#import <wtf/UnusedParam.h>
#import <wtf/unicode/CharacterNames.h>

@interface NSAttributedString (AppKitSecretsIKnowAbout)
- (id)_initWithDOMRange:(DOMRange *)domRange;
@end
namespace WebCore {

// FIXME: It's not great to have these both here and in WebKit.
NSString *WebArchivePboardType = @"Apple Web Archive pasteboard type";
NSString *WebSmartPastePboardType = @"NeXT smart paste pasteboard type";
NSString *WebURLNamePboardType = @"public.url-name";
NSString *WebURLPboardType = @"public.url";
NSString *WebURLsWithTitlesPboardType = @"WebURLsWithTitlesPboardType";

static NSArray* selectionPasteboardTypes(bool canSmartCopyOrDelete, bool selectionContainsAttachments)
{
    if (selectionContainsAttachments) {
        if (canSmartCopyOrDelete)
            return [NSArray arrayWithObjects:WebSmartPastePboardType, WebArchivePboardType, NSRTFDPboardType, NSRTFPboardType, NSStringPboardType, nil];
        else
            return [NSArray arrayWithObjects:WebArchivePboardType, NSRTFDPboardType, NSRTFPboardType, NSStringPboardType, nil];
    } else { // Don't write RTFD to the pasteboard when the copied attributed string has no attachments.
        if (canSmartCopyOrDelete)
            return [NSArray arrayWithObjects:WebSmartPastePboardType, WebArchivePboardType, NSRTFPboardType, NSStringPboardType, nil];
        else
            return [NSArray arrayWithObjects:WebArchivePboardType, NSRTFPboardType, NSStringPboardType, nil];
    }
}

static NSArray* writableTypesForURL()
{
    DEFINE_STATIC_LOCAL(RetainPtr<NSArray>, types, ([[NSArray alloc] initWithObjects:
            WebURLsWithTitlesPboardType,
            NSURLPboardType,
            WebURLPboardType,
            WebURLNamePboardType,
            NSStringPboardType,
            nil]));
    return types.get();
}

static inline NSArray* createWritableTypesForImage()
{
    NSMutableArray *types = [[NSMutableArray alloc] initWithObjects:NSTIFFPboardType, nil];
    [types addObjectsFromArray:writableTypesForURL()];
    [types addObject:NSRTFDPboardType];
    return types;
}

static NSArray* writableTypesForImage()
{
    DEFINE_STATIC_LOCAL(RetainPtr<NSArray>, types, (createWritableTypesForImage()));
    return types.get();
}

Pasteboard* Pasteboard::generalPasteboard() 
{
    static Pasteboard* pasteboard = new Pasteboard([NSPasteboard generalPasteboard]);
    return pasteboard;
}

Pasteboard::Pasteboard(NSPasteboard* pboard)
    : m_pasteboard(pboard)
{
}

void Pasteboard::clear()
{
    [m_pasteboard.get() declareTypes:[NSArray array] owner:nil];
}

void Pasteboard::writeSelection(NSPasteboard* pasteboard, NSArray* pasteboardTypes, Range* selectedRange, bool canSmartCopyOrDelete, Frame* frame)
{
    if (!WebArchivePboardType)
        Pasteboard::generalPasteboard(); // Initializes pasteboard types.
    ASSERT(selectedRange);
    
    // If the selection is at the beginning of content inside an anchor tag
    // we move the selection start to include the anchor.
    // This way the attributed string will contain the url attribute as well.
    // See <rdar://problem/9084267>.
    ExceptionCode ec;
    Node* commonAncestor = selectedRange->commonAncestorContainer(ec);
    ASSERT(commonAncestor);
    Node* enclosingAnchor = enclosingNodeWithTag(firstPositionInNode(commonAncestor), HTMLNames::aTag);
    if (enclosingAnchor && comparePositions(firstPositionInOrBeforeNode(selectedRange->startPosition().anchorNode()), selectedRange->startPosition()) >= 0)
        selectedRange->setStart(enclosingAnchor, 0, ec);

    // Using different API for WebKit and WebKit2.
    NSAttributedString *attributedString = nil;
    if (frame->view()->platformWidget())
        attributedString = [[[NSAttributedString alloc] _initWithDOMRange:kit(selectedRange)] autorelease];
#ifndef BUILDING_ON_LEOPARD
    else {
        // In WebKit2 we are using a different way to create the NSAttributedString from the DOMrange that doesn't require access to the WebView.
        RetainPtr<WebHTMLConverter> converter = [[WebHTMLConverter alloc] initWithDOMRange:kit(selectedRange)];
        if (converter)
            attributedString = [converter.get() attributedString];
    }
#endif

    NSArray *types = pasteboardTypes ? pasteboardTypes : selectionPasteboardTypes(canSmartCopyOrDelete, [attributedString containsAttachments]);
    [pasteboard declareTypes:types owner:nil];
    frame->editor()->client()->didSetSelectionTypesForPasteboard();
    
    // Put HTML on the pasteboard.
    if ([types containsObject:WebArchivePboardType]) {
        RefPtr<LegacyWebArchive> archive = LegacyWebArchive::createFromSelection(frame);
        RetainPtr<CFDataRef> data = archive ? archive->rawDataRepresentation() : 0;
        [pasteboard setData:(NSData *)data.get() forType:WebArchivePboardType];
    }
    
    // Put the attributed string on the pasteboard (RTF/RTFD format).
    if ([types containsObject:NSRTFDPboardType]) {
        NSData *RTFDData = [attributedString RTFDFromRange:NSMakeRange(0, [attributedString length]) documentAttributes:nil];
        [pasteboard setData:RTFDData forType:NSRTFDPboardType];
    }
    if ([types containsObject:NSRTFPboardType]) {
        if ([attributedString containsAttachments])
            attributedString = attributedStringByStrippingAttachmentCharacters(attributedString);
        NSData *RTFData = [attributedString RTFFromRange:NSMakeRange(0, [attributedString length]) documentAttributes:nil];
        [pasteboard setData:RTFData forType:NSRTFPboardType];
    }
    
    // Put plain string on the pasteboard.
    if ([types containsObject:NSStringPboardType]) {
        // Map &nbsp; to a plain old space because this is better for source code, other browsers do it,
        // and because HTML forces you to do this any time you want two spaces in a row.
        String text = frame->editor()->selectedText();
        NSMutableString *s = [[[(NSString*)text copy] autorelease] mutableCopy];
        
        NSString *NonBreakingSpaceString = [NSString stringWithCharacters:&noBreakSpace length:1];
        [s replaceOccurrencesOfString:NonBreakingSpaceString withString:@" " options:0 range:NSMakeRange(0, [s length])];
        [pasteboard setString:s forType:NSStringPboardType];
        [s release];
    }
    
    if ([types containsObject:WebSmartPastePboardType]) {
        [pasteboard setData:nil forType:WebSmartPastePboardType];
    }
}

void Pasteboard::writePlainText(NSPasteboard* pasteboard, const String& text)
{
    NSArray *types = [NSArray arrayWithObject:NSStringPboardType];
    [pasteboard declareTypes:types owner:nil];
    
    [pasteboard setString:text forType:NSStringPboardType];
}
    
void Pasteboard::writeSelection(Range* selectedRange, bool canSmartCopyOrDelete, Frame* frame)
{
    Pasteboard::writeSelection(m_pasteboard.get(), 0, selectedRange, canSmartCopyOrDelete, frame);
}

void Pasteboard::writePlainText(const String& text)
{
    if (!WebArchivePboardType)
        Pasteboard::generalPasteboard(); // Initializes pasteboard types.

    NSArray *types = [NSArray arrayWithObject:NSStringPboardType];
    NSPasteboard *pasteboard = m_pasteboard.get();
    [pasteboard declareTypes:types owner:nil];

    [pasteboard setString:text forType:NSStringPboardType];
}

void Pasteboard::writeURL(NSPasteboard* pasteboard, NSArray* types, const KURL& url, const String& titleStr, Frame* frame)
{
    if (!WebArchivePboardType)
        Pasteboard::generalPasteboard(); // Initializes pasteboard types.
   
    if (!types) {
        types = writableTypesForURL();
        [pasteboard declareTypes:types owner:nil];
    }
    
    ASSERT(!url.isEmpty());
    
    NSURL *cocoaURL = url;
    NSString *userVisibleString = frame->editor()->client()->userVisibleString(cocoaURL);
    
    NSString *title = (NSString*)titleStr;
    if ([title length] == 0) {
        title = [[cocoaURL path] lastPathComponent];
        if ([title length] == 0)
            title = userVisibleString;
    }
        
    if ([types containsObject:WebURLsWithTitlesPboardType])
        [pasteboard setPropertyList:[NSArray arrayWithObjects:[NSArray arrayWithObject:userVisibleString], 
                                     [NSArray arrayWithObject:(NSString*)titleStr.stripWhiteSpace()], 
                                     nil]
                            forType:WebURLsWithTitlesPboardType];
    if ([types containsObject:NSURLPboardType])
        [cocoaURL writeToPasteboard:pasteboard];
    if ([types containsObject:WebURLPboardType])
        [pasteboard setString:userVisibleString forType:WebURLPboardType];
    if ([types containsObject:WebURLNamePboardType])
        [pasteboard setString:title forType:WebURLNamePboardType];
    if ([types containsObject:NSStringPboardType])
        [pasteboard setString:userVisibleString forType:NSStringPboardType];
}
    
void Pasteboard::writeURL(const KURL& url, const String& titleStr, Frame* frame)
{
    Pasteboard::writeURL(m_pasteboard.get(), nil, url, titleStr, frame);
}

static NSFileWrapper* fileWrapperForImage(CachedResource* resource, NSURL *url)
{
    SharedBuffer* coreData = resource->data();
    NSData *data = [[[NSData alloc] initWithBytes:coreData->data() length:coreData->size()] autorelease];
    NSFileWrapper *wrapper = [[[NSFileWrapper alloc] initRegularFileWithContents:data] autorelease];
    String coreMIMEType = resource->response().mimeType();
    NSString *MIMEType = nil;
    if (!coreMIMEType.isNull())
        MIMEType = coreMIMEType;
    [wrapper setPreferredFilename:suggestedFilenameWithMIMEType(url, MIMEType)];
    return wrapper;
}

void Pasteboard::writeFileWrapperAsRTFDAttachment(NSFileWrapper* wrapper)
{
    NSTextAttachment *attachment = [[NSTextAttachment alloc] initWithFileWrapper:wrapper];
    
    NSAttributedString *string = [NSAttributedString attributedStringWithAttachment:attachment];
    [attachment release];
    
    NSData *RTFDData = [string RTFDFromRange:NSMakeRange(0, [string length]) documentAttributes:nil];
    [m_pasteboard.get() setData:RTFDData forType:NSRTFDPboardType];
}

void Pasteboard::writeImage(Node* node, const KURL& url, const String& title)
{
    ASSERT(node);
    Frame* frame = node->document()->frame();

    NSURL *cocoaURL = url;
    ASSERT(cocoaURL);

    ASSERT(node->renderer() && node->renderer()->isImage());
    RenderImage* renderer = toRenderImage(node->renderer());
    CachedImage* cachedImage = renderer->cachedImage();
    if (!cachedImage || cachedImage->errorOccurred())
        return;

    NSArray* types = writableTypesForImage();
    [m_pasteboard.get() declareTypes:types owner:nil];
    writeURL(m_pasteboard.get(), types, cocoaURL, nsStringNilIfEmpty(title), frame);
    
    Image* image = cachedImage->image();
    ASSERT(image);
    
    [m_pasteboard.get() setData:[image->getNSImage() TIFFRepresentation] forType:NSTIFFPboardType];

    String MIMEType = cachedImage->response().mimeType();
    ASSERT(MIMETypeRegistry::isSupportedImageResourceMIMEType(MIMEType));

    writeFileWrapperAsRTFDAttachment(fileWrapperForImage(cachedImage, cocoaURL));
}

bool Pasteboard::canSmartReplace()
{
    return [[m_pasteboard.get() types] containsObject:WebSmartPastePboardType];
}

String Pasteboard::plainText(Frame* frame)
{
    NSArray *types = [m_pasteboard.get() types];
    
    if ([types containsObject:NSStringPboardType])
        return [[m_pasteboard.get() stringForType:NSStringPboardType] precomposedStringWithCanonicalMapping];
    
    NSAttributedString *attributedString = nil;
    NSString *string;

    if ([types containsObject:NSRTFDPboardType])
        attributedString = [[NSAttributedString alloc] initWithRTFD:[m_pasteboard.get() dataForType:NSRTFDPboardType] documentAttributes:NULL];
    if (attributedString == nil && [types containsObject:NSRTFPboardType])
        attributedString = [[NSAttributedString alloc] initWithRTF:[m_pasteboard.get() dataForType:NSRTFPboardType] documentAttributes:NULL];
    if (attributedString != nil) {
        string = [[attributedString string] precomposedStringWithCanonicalMapping];
        [attributedString release];
        return string;
    }
    
    if ([types containsObject:NSFilenamesPboardType]) {
        string = [[[m_pasteboard.get() propertyListForType:NSFilenamesPboardType] componentsJoinedByString:@"\n"] precomposedStringWithCanonicalMapping];
        if (string != nil)
            return string;
    }
    
    
    if (NSURL *url = [NSURL URLFromPasteboard:m_pasteboard.get()]) {
        // FIXME: using the editorClient to call into webkit, for now, since 
        // calling _web_userVisibleString from WebCore involves migrating a sizable web of 
        // helper code that should either be done in a separate patch or figured out in another way.
        string = frame->editor()->client()->userVisibleString(url);
        if ([string length] > 0)
            return [string precomposedStringWithCanonicalMapping];
    }

    
    return String(); 
}
    
PassRefPtr<DocumentFragment> Pasteboard::documentFragmentWithImageResource(Frame* frame, PassRefPtr<ArchiveResource> resource)
{
    if (DocumentLoader* loader = frame->loader()->documentLoader())
        loader->addArchiveResource(resource.get());

    RefPtr<Element> imageElement = frame->document()->createElement(HTMLNames::imgTag, false);
    if (!imageElement)
        return 0;

    NSURL *URL = resource->url();
    imageElement->setAttribute(HTMLNames::srcAttr, [URL isFileURL] ? [URL absoluteString] : resource->url());
    RefPtr<DocumentFragment> fragment = frame->document()->createDocumentFragment();
    if (fragment) {
        ExceptionCode ec;
        fragment->appendChild(imageElement, ec);
        return fragment.release();       
    }
    return 0;
}

PassRefPtr<DocumentFragment> Pasteboard::documentFragmentWithRtf(Frame* frame, NSString* pboardType)
{
    if (!frame || !frame->document() || !frame->document()->isHTMLDocument())
        return 0;

    NSAttributedString *string = nil;
    if (pboardType == NSRTFDPboardType)
        string = [[NSAttributedString alloc] initWithRTFD:[m_pasteboard.get() dataForType:NSRTFDPboardType] documentAttributes:NULL];
    if (string == nil)
        string = [[NSAttributedString alloc] initWithRTF:[m_pasteboard.get() dataForType:NSRTFPboardType] documentAttributes:NULL];
    if (string == nil)
        return nil;

    bool wasDeferringCallbacks = frame->page()->defersLoading();
    if (!wasDeferringCallbacks)
        frame->page()->setDefersLoading(true);

    Vector<RefPtr<ArchiveResource> > resources;
    RefPtr<DocumentFragment> fragment = frame->editor()->client()->documentFragmentFromAttributedString(string, resources);

    size_t size = resources.size();
    if (size) {
        DocumentLoader* loader = frame->loader()->documentLoader();
        for (size_t i = 0; i < size; ++i)
            loader->addArchiveResource(resources[i]);    
    }

    if (!wasDeferringCallbacks)
        frame->page()->setDefersLoading(false);

    [string release];
    return fragment.release();
}

#define WebDataProtocolScheme @"webkit-fake-url"

static NSURL* uniqueURLWithRelativePart(NSString *relativePart)
{
    CFUUIDRef UUIDRef = CFUUIDCreate(kCFAllocatorDefault);
    NSString *UUIDString = (NSString *)CFUUIDCreateString(kCFAllocatorDefault, UUIDRef);
    CFRelease(UUIDRef);
    NSURL *URL = [NSURL URLWithString:[NSString stringWithFormat:@"%@://%@/%@", WebDataProtocolScheme, UUIDString, relativePart]];
    CFRelease(UUIDString);

    return URL;
}

NSURL *Pasteboard::getBestURL(Frame* frame)
{
    NSArray *types = [m_pasteboard.get() types];

    // FIXME: using the editorClient to call into webkit, for now, since 
    // calling webkit_canonicalize from WebCore involves migrating a sizable amount of 
    // helper code that should either be done in a separate patch or figured out in another way.
    
    if ([types containsObject:NSURLPboardType]) {
        NSURL *URLFromPasteboard = [NSURL URLFromPasteboard:m_pasteboard.get()];
        NSString *scheme = [URLFromPasteboard scheme];
        if ([scheme isEqualToString:@"http"] || [scheme isEqualToString:@"https"]) {
            return frame->editor()->client()->canonicalizeURL(URLFromPasteboard);
        }
    }
    
    if ([types containsObject:NSStringPboardType]) {
        NSString *URLString = [m_pasteboard.get() stringForType:NSStringPboardType];
        NSURL *URL = frame->editor()->client()->canonicalizeURLString(URLString);
        if (URL)
            return URL;
    }
    
    if ([types containsObject:NSFilenamesPboardType]) {
        NSArray *files = [m_pasteboard.get() propertyListForType:NSFilenamesPboardType];
        // FIXME: Maybe it makes more sense to allow multiple files and only use the first one?
        if ([files count] == 1) {
            NSString *file = [files objectAtIndex:0];
            BOOL isDirectory;
            if ([[NSFileManager defaultManager] fileExistsAtPath:file isDirectory:&isDirectory] && isDirectory)
                return nil;
            return frame->editor()->client()->canonicalizeURL([NSURL fileURLWithPath:file]);
        }
    }
    
    return nil;    
}

String Pasteboard::asURL(Frame* frame)
{
    return [getBestURL(frame) absoluteString];
}

PassRefPtr<DocumentFragment> Pasteboard::documentFragment(Frame* frame, PassRefPtr<Range> context, bool allowPlainText, bool& chosePlainText)
{
    NSArray *types = [m_pasteboard.get() types];
    RefPtr<DocumentFragment> fragment;
    chosePlainText = false;

    if ([types containsObject:WebArchivePboardType]) {
        RefPtr<LegacyWebArchive> coreArchive = LegacyWebArchive::create(SharedBuffer::wrapNSData([m_pasteboard.get() dataForType:WebArchivePboardType]).get());
        if (coreArchive) {
            RefPtr<ArchiveResource> mainResource = coreArchive->mainResource();
            if (mainResource) {
                NSString *MIMEType = mainResource->mimeType();
                if (!frame || !frame->document())
                    return 0;
                if (frame->loader()->client()->canShowMIMETypeAsHTML(MIMEType)) {
                    NSString *markupString = [[NSString alloc] initWithData:[mainResource->data()->createNSData() autorelease] encoding:NSUTF8StringEncoding];
                    // FIXME: seems poor form to do this as a side effect of getting a document fragment
                    if (DocumentLoader* loader = frame->loader()->documentLoader())
                        loader->addAllArchiveResources(coreArchive.get());
                    
                    fragment = createFragmentFromMarkup(frame->document(), markupString, mainResource->url(), FragmentScriptingNotAllowed);
                    [markupString release];
                } else if (MIMETypeRegistry::isSupportedImageMIMEType(MIMEType))
                   fragment = documentFragmentWithImageResource(frame, mainResource);                    
            }
        }
        if (fragment)
            return fragment.release();
    } 

    if ([types containsObject:NSFilenamesPboardType]) {
        NSArray* paths = [m_pasteboard.get() propertyListForType:NSFilenamesPboardType];
        NSEnumerator* enumerator = [paths objectEnumerator];
        NSString* path;
        Vector< RefPtr<Node> > refNodesVector;
        Vector<Node*> nodesVector;

        while ((path = [enumerator nextObject]) != nil) {
            // Non-image file types; _web_userVisibleString is appropriate here because this will
            // be pasted as visible text.
            NSString *url = frame->editor()->client()->userVisibleString([NSURL fileURLWithPath:path]);
            RefPtr<Node> textNode = frame->document()->createTextNode(url);
            refNodesVector.append(textNode.get());
            nodesVector.append(textNode.get());
        }
        fragment = createFragmentFromNodes(frame->document(), nodesVector);
        if (fragment && fragment->firstChild())
            return fragment.release();
    }

    if ([types containsObject:NSHTMLPboardType]) {
        NSString *HTMLString = [m_pasteboard.get() stringForType:NSHTMLPboardType];
        // This is a hack to make Microsoft's HTML pasteboard data work. See 3778785.
        if ([HTMLString hasPrefix:@"Version:"]) {
            NSRange range = [HTMLString rangeOfString:@"<html" options:NSCaseInsensitiveSearch];
            if (range.location != NSNotFound) {
                HTMLString = [HTMLString substringFromIndex:range.location];
            }
        }
        if ([HTMLString length] != 0 &&
            (fragment = createFragmentFromMarkup(frame->document(), HTMLString, "", FragmentScriptingNotAllowed)))
            return fragment.release();
    }

    if ([types containsObject:NSRTFDPboardType] &&
        (fragment = documentFragmentWithRtf(frame, NSRTFDPboardType)))
       return fragment.release();

    if ([types containsObject:NSRTFPboardType] &&
        (fragment = documentFragmentWithRtf(frame, NSRTFPboardType)))
        return fragment.release();

    if ([types containsObject:NSTIFFPboardType] &&
        (fragment = documentFragmentWithImageResource(frame, ArchiveResource::create(SharedBuffer::wrapNSData([[[m_pasteboard.get() dataForType:NSTIFFPboardType] copy] autorelease]), uniqueURLWithRelativePart(@"image.tiff"), "image/tiff", "", ""))))
        return fragment.release();

    if ([types containsObject:NSPDFPboardType] &&
        (fragment = documentFragmentWithImageResource(frame, ArchiveResource::create(SharedBuffer::wrapNSData([[[m_pasteboard.get() dataForType:NSPDFPboardType] copy] autorelease]), uniqueURLWithRelativePart(@"application.pdf"), "application/pdf", "", ""))))
        return fragment.release();

#ifdef BUILDING_ON_LEOPARD
    if ([types containsObject:NSPICTPboardType] &&
        (fragment = documentFragmentWithImageResource(frame, ArchiveResource::create(SharedBuffer::wrapNSData([[[m_pasteboard.get() dataForType:NSPICTPboardType] copy] autorelease]), uniqueURLWithRelativePart(@"image.pict"), "image/pict", "", ""))))
        return fragment.release();
#endif

    // Only 10.5 and higher support setting and retrieving pasteboard types with UTIs, but we don't believe
    // that any applications on Tiger put types for which we only have a UTI, like PNG, on the pasteboard.
    if ([types containsObject:(NSString*)kUTTypePNG] &&
        (fragment = documentFragmentWithImageResource(frame, ArchiveResource::create(SharedBuffer::wrapNSData([[[m_pasteboard.get() dataForType:(NSString*)kUTTypePNG] copy] autorelease]), uniqueURLWithRelativePart(@"image.png"), "image/png", "", ""))))
        return fragment.release();

    if ([types containsObject:NSURLPboardType]) {
        NSURL *URL = [NSURL URLFromPasteboard:m_pasteboard.get()];
        Document* document = frame->document();
        ASSERT(document);
        if (!document)
            return 0;
        RefPtr<Element> anchor = document->createElement(HTMLNames::aTag, false);
        NSString *URLString = [URL absoluteString]; // Original data is ASCII-only, so there is no need to precompose.
        if ([URLString length] == 0)
            return nil;
        NSString *URLTitleString = [[m_pasteboard.get() stringForType:WebURLNamePboardType] precomposedStringWithCanonicalMapping];
        ExceptionCode ec;
        anchor->setAttribute(HTMLNames::hrefAttr, URLString);
        anchor->appendChild(document->createTextNode(URLTitleString), ec);
        fragment = document->createDocumentFragment();
        if (fragment) {
            fragment->appendChild(anchor, ec);
            return fragment.release();
        }
    }

    if (allowPlainText && [types containsObject:NSStringPboardType]) {
        chosePlainText = true;
        fragment = createFragmentFromText(context.get(), [[m_pasteboard.get() stringForType:NSStringPboardType] precomposedStringWithCanonicalMapping]);
        return fragment.release();
    }

    return 0;
}

}
