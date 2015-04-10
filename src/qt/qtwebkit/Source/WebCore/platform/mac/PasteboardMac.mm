/*
 * Copyright (C) 2006, 2013 Apple Inc. All rights reserved.
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

#import "CachedImage.h"
#import "Clipboard.h"
#import "DOMRangeInternal.h"
#import "Document.h"
#import "DocumentFragment.h"
#import "DocumentLoader.h"
#import "DragData.h"
#import "Editor.h"
#import "EditorClient.h"
#import "ExceptionCodePlaceholder.h"
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
#import "PasteboardStrategy.h"
#import "PlatformStrategies.h"
#import "RenderImage.h"
#import "ResourceBuffer.h"
#import "Text.h"
#import "WebCoreNSStringExtras.h"
#import "WebNSAttributedStringExtras.h"
#import "markup.h"
#import <wtf/StdLibExtras.h>
#import <wtf/RetainPtr.h>
#import <wtf/text/StringBuilder.h>
#import <wtf/unicode/CharacterNames.h>

namespace WebCore {

// FIXME: It's not great to have these both here and in WebKit.
const char* WebArchivePboardType = "Apple Web Archive pasteboard type";
const char* WebSmartPastePboardType = "NeXT smart paste pasteboard type";
const char* WebURLNamePboardType = "public.url-name";
const char* WebURLPboardType = "public.url";
const char* WebURLsWithTitlesPboardType = "WebURLsWithTitlesPboardType";

static Vector<String> selectionPasteboardTypes(bool canSmartCopyOrDelete, bool selectionContainsAttachments)
{
    Vector<String> types;
    if (canSmartCopyOrDelete)
        types.append(WebSmartPastePboardType);
    types.append(WebArchivePboardType);
    if (selectionContainsAttachments)
        types.append(String(NSRTFDPboardType));
    types.append(String(NSRTFPboardType));
    types.append(String(NSStringPboardType));

    return types;
}

static const Vector<String> writableTypesForURL()
{
    Vector<String> types;
    
    types.append(WebURLsWithTitlesPboardType);
    types.append(String(NSURLPboardType));
    types.append(WebURLPboardType);
    types.append(WebURLNamePboardType);
    types.append(String(NSStringPboardType));        
    return types;
}

static inline Vector<String> createWritableTypesForImage()
{
    Vector<String> types;
    
    types.append(String(NSTIFFPboardType));
    types.appendVector(writableTypesForURL());
    types.append(String(NSRTFDPboardType));
    return types;
}

static Vector<String> writableTypesForImage()
{
    Vector<String> types;
    types.appendVector(createWritableTypesForImage());
    return types;
}

Pasteboard* Pasteboard::generalPasteboard() 
{
    static Pasteboard* pasteboard = new Pasteboard(NSGeneralPboard);
    return pasteboard;
}

Pasteboard::Pasteboard(const String& pasteboardName)
    : m_pasteboardName(pasteboardName)
    , m_changeCount(platformStrategies()->pasteboardStrategy()->changeCount(m_pasteboardName))
{
    ASSERT(pasteboardName);
}

PassOwnPtr<Pasteboard> Pasteboard::create(const String& pasteboardName)
{
    return adoptPtr(new Pasteboard(pasteboardName));
}

PassOwnPtr<Pasteboard> Pasteboard::createForCopyAndPaste()
{
    return create(NSGeneralPboard);
}

PassOwnPtr<Pasteboard> Pasteboard::createPrivate()
{
    return create(platformStrategies()->pasteboardStrategy()->uniqueName());
}

PassOwnPtr<Pasteboard> Pasteboard::createForDragAndDrop()
{
    return create(NSDragPboard);
}

PassOwnPtr<Pasteboard> Pasteboard::createForDragAndDrop(const DragData& dragData)
{
    return create(dragData.pasteboardName());
}

void Pasteboard::clear()
{
    platformStrategies()->pasteboardStrategy()->setTypes(Vector<String>(), m_pasteboardName);
}

PassRefPtr<SharedBuffer> Pasteboard::getDataSelection(Frame* frame, const String& pasteboardType)
{
    if (pasteboardType == WebArchivePboardType) {
        RefPtr<LegacyWebArchive> archive = LegacyWebArchive::createFromSelection(frame);
        RetainPtr<CFDataRef> data = archive ? archive->rawDataRepresentation() : 0;
        return SharedBuffer::wrapNSData((NSData *)data.get());
    }

    RefPtr<Range> range = frame->editor().selectedRange();
    Node* commonAncestor = range->commonAncestorContainer(IGNORE_EXCEPTION);
    ASSERT(commonAncestor);
    Node* enclosingAnchor = enclosingNodeWithTag(firstPositionInNode(commonAncestor), HTMLNames::aTag);
    if (enclosingAnchor && comparePositions(firstPositionInOrBeforeNode(range->startPosition().anchorNode()), range->startPosition()) >= 0)
        range->setStart(enclosingAnchor, 0, IGNORE_EXCEPTION);
    
    NSAttributedString* attributedString = nil;
    RetainPtr<WebHTMLConverter> converter = adoptNS([[WebHTMLConverter alloc] initWithDOMRange:kit(range.get())]);
    if (converter)
        attributedString = [converter.get() attributedString];
    
    if (pasteboardType == String(NSRTFDPboardType)) {
        NSData *RTFDData = [attributedString RTFDFromRange:NSMakeRange(0, [attributedString length]) documentAttributes:nil];
        return SharedBuffer::wrapNSData((NSData *)RTFDData);
    }
    if (pasteboardType == String(NSRTFPboardType)) {
        if ([attributedString containsAttachments])
            attributedString = attributedStringByStrippingAttachmentCharacters(attributedString);
        NSData *RTFData = [attributedString RTFFromRange:NSMakeRange(0, [attributedString length]) documentAttributes:nil];
        return SharedBuffer::wrapNSData((NSData *)RTFData);
    }
    return 0;
}

void Pasteboard::writeSelectionForTypes(const Vector<String>& pasteboardTypes, bool canSmartCopyOrDelete, Frame* frame, ShouldSerializeSelectedTextForClipboard shouldSerializeSelectedTextForClipboard)
{
    NSAttributedString* attributedString = nil;
    RetainPtr<WebHTMLConverter> converter = adoptNS([[WebHTMLConverter alloc] initWithDOMRange:kit(frame->editor().selectedRange().get())]);
    if (converter)
        attributedString = [converter.get() attributedString];
    
    Vector<String> types = !pasteboardTypes.isEmpty() ? pasteboardTypes : selectionPasteboardTypes(canSmartCopyOrDelete, [attributedString containsAttachments]);

    Vector<String> clientTypes;
    Vector<RefPtr<SharedBuffer> > clientData;
    frame->editor().client()->getClientPasteboardDataForRange(frame->editor().selectedRange().get(), clientTypes, clientData);
    types.appendVector(clientTypes);

    platformStrategies()->pasteboardStrategy()->setTypes(types, m_pasteboardName);
    frame->editor().client()->didSetSelectionTypesForPasteboard();

    for (size_t i = 0; i < clientTypes.size(); ++i)
        platformStrategies()->pasteboardStrategy()->setBufferForType(clientData[i], clientTypes[i], m_pasteboardName);

    // Put HTML on the pasteboard.
    if (types.contains(WebArchivePboardType))
        platformStrategies()->pasteboardStrategy()->setBufferForType(getDataSelection(frame, WebArchivePboardType), WebArchivePboardType, m_pasteboardName);
    
    // Put the attributed string on the pasteboard (RTF/RTFD format).
    if (types.contains(String(NSRTFDPboardType)))
        platformStrategies()->pasteboardStrategy()->setBufferForType(getDataSelection(frame, NSRTFDPboardType), NSRTFDPboardType, m_pasteboardName);

    if (types.contains(String(NSRTFPboardType)))
        platformStrategies()->pasteboardStrategy()->setBufferForType(getDataSelection(frame, NSRTFPboardType), NSRTFPboardType, m_pasteboardName);
    
    // Put plain string on the pasteboard.
    if (types.contains(String(NSStringPboardType))) {
        String text = shouldSerializeSelectedTextForClipboard == IncludeImageAltTextForClipboard
            ? frame->editor().stringSelectionForPasteboardWithImageAltText()
            : frame->editor().stringSelectionForPasteboard();
        platformStrategies()->pasteboardStrategy()->setStringForType(text, NSStringPboardType, m_pasteboardName);
    }
    
    if (types.contains(WebSmartPastePboardType))
        platformStrategies()->pasteboardStrategy()->setBufferForType(0, WebSmartPastePboardType, m_pasteboardName);
}

void Pasteboard::writePlainText(const String& text, SmartReplaceOption smartReplaceOption)
{
    Vector<String> types;
    types.append(NSStringPboardType);
    if (smartReplaceOption == CanSmartReplace)
        types.append(WebSmartPastePboardType);

    platformStrategies()->pasteboardStrategy()->setTypes(types, m_pasteboardName);
    platformStrategies()->pasteboardStrategy()->setStringForType(text, NSStringPboardType, m_pasteboardName);
    if (smartReplaceOption == CanSmartReplace)
        platformStrategies()->pasteboardStrategy()->setBufferForType(0, WebSmartPastePboardType, m_pasteboardName);
}
    
void Pasteboard::writeSelection(Range*, bool canSmartCopyOrDelete, Frame* frame, ShouldSerializeSelectedTextForClipboard shouldSerializeSelectedTextForClipboard)
{
    writeSelectionForTypes(Vector<String>(), canSmartCopyOrDelete, frame, shouldSerializeSelectedTextForClipboard);
}

static void writeURLForTypes(const Vector<String>& types, const String& pasteboardName, const KURL& url, const String& titleStr, Frame* frame)
{
    platformStrategies()->pasteboardStrategy()->setTypes(types, pasteboardName);
    
    ASSERT(!url.isEmpty());
    
    NSURL *cocoaURL = url;
    NSString *userVisibleString = frame->editor().client()->userVisibleString(cocoaURL);
    
    NSString *title = (NSString*)titleStr;
    if ([title length] == 0) {
        title = [[cocoaURL path] lastPathComponent];
        if ([title length] == 0)
            title = userVisibleString;
    }
    if (types.contains(WebURLsWithTitlesPboardType)) {
        Vector<String> paths;
        paths.append([cocoaURL absoluteString]);
        paths.append(titleStr.stripWhiteSpace());
        platformStrategies()->pasteboardStrategy()->setPathnamesForType(paths, WebURLsWithTitlesPboardType, pasteboardName);
    }
    if (types.contains(String(NSURLPboardType)))
        platformStrategies()->pasteboardStrategy()->setStringForType([cocoaURL absoluteString], NSURLPboardType, pasteboardName);
    if (types.contains(WebURLPboardType))
        platformStrategies()->pasteboardStrategy()->setStringForType(userVisibleString, WebURLPboardType, pasteboardName);
    if (types.contains(WebURLNamePboardType))
        platformStrategies()->pasteboardStrategy()->setStringForType(title, WebURLNamePboardType, pasteboardName);
    if (types.contains(String(NSStringPboardType)))
        platformStrategies()->pasteboardStrategy()->setStringForType(userVisibleString, NSStringPboardType, pasteboardName);
}
    
void Pasteboard::writeURL(const KURL& url, const String& titleStr, Frame* frame)
{
    writeURLForTypes(writableTypesForURL(), m_pasteboardName, url, titleStr, frame);
}

static NSFileWrapper* fileWrapperForImage(CachedResource* resource, NSURL *url)
{
    ResourceBuffer* coreData = resource->resourceBuffer();
    NSData *data = [[[NSData alloc] initWithBytes:coreData->data() length:coreData->size()] autorelease];
    NSFileWrapper *wrapper = [[[NSFileWrapper alloc] initRegularFileWithContents:data] autorelease];
    String coreMIMEType = resource->response().mimeType();
    NSString *MIMEType = nil;
    if (!coreMIMEType.isNull())
        MIMEType = coreMIMEType;
    [wrapper setPreferredFilename:suggestedFilenameWithMIMEType(url, MIMEType)];
    return wrapper;
}

static void writeFileWrapperAsRTFDAttachment(NSFileWrapper* wrapper, const String& pasteboardName)
{
    NSTextAttachment *attachment = [[NSTextAttachment alloc] initWithFileWrapper:wrapper];
    
    NSAttributedString *string = [NSAttributedString attributedStringWithAttachment:attachment];
    [attachment release];
    
    NSData *RTFDData = [string RTFDFromRange:NSMakeRange(0, [string length]) documentAttributes:nil];
    if (RTFDData)
        platformStrategies()->pasteboardStrategy()->setBufferForType(SharedBuffer::wrapNSData(RTFDData).get(), NSRTFDPboardType, pasteboardName);
}

void Pasteboard::writeImage(Node* node, const KURL& url, const String& title)
{
    ASSERT(node);

    if (!(node->renderer() && node->renderer()->isImage()))
        return;

    NSURL *cocoaURL = url;
    ASSERT(cocoaURL);

    RenderImage* renderer = toRenderImage(node->renderer());
    CachedImage* cachedImage = renderer->cachedImage();
    if (!cachedImage || cachedImage->errorOccurred())
        return;

    writeURLForTypes(writableTypesForImage(), m_pasteboardName, cocoaURL, nsStringNilIfEmpty(title), node->document()->frame());
    
    Image* image = cachedImage->imageForRenderer(renderer);
    if (!image)
        return;
    NSData *imageData = (NSData *)[image->getNSImage() TIFFRepresentation];
    if (!imageData)
        return;
    platformStrategies()->pasteboardStrategy()->setBufferForType(SharedBuffer::wrapNSData(imageData), NSTIFFPboardType, m_pasteboardName);

    String MIMEType = cachedImage->response().mimeType();
    ASSERT(MIMETypeRegistry::isSupportedImageResourceMIMEType(MIMEType));

    writeFileWrapperAsRTFDAttachment(fileWrapperForImage(cachedImage, cocoaURL), m_pasteboardName);
}

void Pasteboard::writePasteboard(const Pasteboard& pasteboard)
{
    platformStrategies()->pasteboardStrategy()->copy(pasteboard.m_pasteboardName, m_pasteboardName);
}

bool Pasteboard::canSmartReplace()
{
    Vector<String> types;
    platformStrategies()->pasteboardStrategy()->getTypes(types, m_pasteboardName);
    return types.contains(WebSmartPastePboardType);
}

String Pasteboard::plainText(Frame* frame)
{
    Vector<String> types;
    platformStrategies()->pasteboardStrategy()->getTypes(types, m_pasteboardName);
    
    if (types.contains(String(NSStringPboardType)))
        return [(NSString *)platformStrategies()->pasteboardStrategy()->stringForType(NSStringPboardType, m_pasteboardName) precomposedStringWithCanonicalMapping];
    
    NSAttributedString *attributedString = nil;
    NSString *string;

    if (types.contains(String(NSRTFDPboardType))) {
        RefPtr<SharedBuffer> data = platformStrategies()->pasteboardStrategy()->bufferForType(NSRTFDPboardType, m_pasteboardName);
        if (data)
            attributedString = [[NSAttributedString alloc] initWithRTFD:[data->createNSData() autorelease] documentAttributes:NULL];
    }
    if (attributedString == nil && types.contains(String(NSRTFPboardType))) {
        RefPtr<SharedBuffer> data = platformStrategies()->pasteboardStrategy()->bufferForType(NSRTFPboardType, m_pasteboardName);
        if (data)
            attributedString = [[NSAttributedString alloc] initWithRTF:[data->createNSData() autorelease] documentAttributes:NULL];
    }
    if (attributedString != nil) {
        string = [[attributedString string] precomposedStringWithCanonicalMapping];
        [attributedString release];
        return string;
    }
    
    if (types.contains(String(NSFilenamesPboardType))) {
        Vector<String> pathnames;
        platformStrategies()->pasteboardStrategy()->getPathnamesForType(pathnames, NSFilenamesPboardType, m_pasteboardName);
        StringBuilder builder;
        for (size_t i = 0; i < pathnames.size(); i++) {
            if (i)
                builder.append('\n');
            builder.append(pathnames[i]);
        }
        string = builder.toString();
        return [string precomposedStringWithCanonicalMapping];
    }
    
    string = platformStrategies()->pasteboardStrategy()->stringForType(NSURLPboardType, m_pasteboardName);
    if ([string length]) {
        // FIXME: using the editorClient to call into webkit, for now, since 
        // calling _web_userVisibleString from WebCore involves migrating a sizable web of 
        // helper code that should either be done in a separate patch or figured out in another way.
        string = frame->editor().client()->userVisibleString([NSURL URLWithString:string]);
        if ([string length] > 0)
            return [string precomposedStringWithCanonicalMapping];
    }

    
    return String(); 
}
    
static PassRefPtr<DocumentFragment> documentFragmentWithImageResource(Frame* frame, PassRefPtr<ArchiveResource> resource)
{
    if (!resource)
        return 0;
    
    if (DocumentLoader* loader = frame->loader()->documentLoader())
        loader->addArchiveResource(resource.get());

    RefPtr<Element> imageElement = frame->document()->createElement(HTMLNames::imgTag, false);
    if (!imageElement)
        return 0;

    NSURL *URL = resource->url();
    imageElement->setAttribute(HTMLNames::srcAttr, [URL isFileURL] ? [URL absoluteString] : resource->url());
    RefPtr<DocumentFragment> fragment = frame->document()->createDocumentFragment();
    if (fragment) {
        fragment->appendChild(imageElement, IGNORE_EXCEPTION);
        return fragment.release();
    }
    return 0;
}

static PassRefPtr<DocumentFragment> documentFragmentWithRTF(Frame* frame, NSString *pasteboardType,const String& pastebordName)
{
    if (!frame || !frame->document() || !frame->document()->isHTMLDocument())
        return 0;

    NSAttributedString *string = nil;
    if (pasteboardType == NSRTFDPboardType) {
        RefPtr<SharedBuffer> data = platformStrategies()->pasteboardStrategy()->bufferForType(NSRTFDPboardType, pastebordName);
        if (data)
            string = [[NSAttributedString alloc] initWithRTFD:[data->createNSData() autorelease] documentAttributes:NULL];
    }
    if (string == nil) {
        RefPtr<SharedBuffer> data = platformStrategies()->pasteboardStrategy()->bufferForType(NSRTFPboardType, pastebordName);
        if (data)
            string = [[NSAttributedString alloc] initWithRTF:[data->createNSData() autorelease] documentAttributes:NULL];
    }
    if (string == nil)
        return nil;

    bool wasDeferringCallbacks = frame->page()->defersLoading();
    if (!wasDeferringCallbacks)
        frame->page()->setDefersLoading(true);

    Vector<RefPtr<ArchiveResource> > resources;
    RefPtr<DocumentFragment> fragment = frame->editor().client()->documentFragmentFromAttributedString(string, resources);

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

static PassRefPtr<DocumentFragment> fragmentFromWebArchive(Frame* frame, PassRefPtr<LegacyWebArchive> coreArchive)
{
    RefPtr<ArchiveResource> mainResource = coreArchive->mainResource();
    if (!mainResource)
        return 0;

    const String& MIMEType = mainResource->mimeType();
    if (!frame || !frame->document())
        return 0;

    if (frame->loader()->client()->canShowMIMETypeAsHTML(MIMEType)) {
        RetainPtr<NSString> markupString = adoptNS([[NSString alloc] initWithData:[mainResource->data()->createNSData() autorelease] encoding:NSUTF8StringEncoding]);
        // FIXME: seems poor form to do this as a side effect of getting a document fragment
        if (DocumentLoader* loader = frame->loader()->documentLoader())
            loader->addAllArchiveResources(coreArchive.get());
        return createFragmentFromMarkup(frame->document(), markupString.get(), mainResource->url(), DisallowScriptingAndPluginContent);
    }

    if (MIMETypeRegistry::isSupportedImageMIMEType(MIMEType))
        return documentFragmentWithImageResource(frame, mainResource);

    return 0;
}

PassRefPtr<DocumentFragment> Pasteboard::documentFragment(Frame* frame, PassRefPtr<Range> context, bool allowPlainText, bool& chosePlainText)
{
    Vector<String> types;
    platformStrategies()->pasteboardStrategy()->getTypes(types, m_pasteboardName);
    RefPtr<DocumentFragment> fragment;
    chosePlainText = false;

    if (types.contains(WebArchivePboardType)) {
        if (RefPtr<SharedBuffer> webArchiveBuffer = platformStrategies()->pasteboardStrategy()->bufferForType(WebArchivePboardType, m_pasteboardName)) {
            if (RefPtr<LegacyWebArchive> coreArchive = LegacyWebArchive::create(KURL(), webArchiveBuffer.get())) {
                if ((fragment = fragmentFromWebArchive(frame, coreArchive)))
                    return fragment.release();
            }
        }
    }

    if (types.contains(String(NSFilenamesPboardType))) {
        Vector<String> paths;
        platformStrategies()->pasteboardStrategy()->getPathnamesForType(paths, NSFilenamesPboardType, m_pasteboardName);
        Vector< RefPtr<Node> > refNodesVector;
        Vector<Node*> nodesVector;

        for (size_t i = 0; i < paths.size(); i++) {
            // Non-image file types; _web_userVisibleString is appropriate here because this will
            // be pasted as visible text.
            NSString *url = frame->editor().client()->userVisibleString([NSURL fileURLWithPath:paths[i]]);
            RefPtr<Node> textNode = frame->document()->createTextNode(url);
            refNodesVector.append(textNode.get());
            nodesVector.append(textNode.get());
        }
        fragment = createFragmentFromNodes(frame->document(), nodesVector);
        if (fragment && fragment->firstChild())
            return fragment.release();
    }

    if (types.contains(String(NSHTMLPboardType))) {
        NSString *HTMLString = platformStrategies()->pasteboardStrategy()->stringForType(NSHTMLPboardType, m_pasteboardName);
        // This is a hack to make Microsoft's HTML pasteboard data work. See 3778785.
        if ([HTMLString hasPrefix:@"Version:"]) {
            NSRange range = [HTMLString rangeOfString:@"<html" options:NSCaseInsensitiveSearch];
            if (range.location != NSNotFound) {
                HTMLString = [HTMLString substringFromIndex:range.location];
            }
        }
        if ([HTMLString length] != 0 &&
            (fragment = createFragmentFromMarkup(frame->document(), HTMLString, emptyString(), DisallowScriptingAndPluginContent)))
            return fragment.release();
    }

    if (types.contains(String(NSRTFDPboardType)) &&
        (fragment = documentFragmentWithRTF(frame, NSRTFDPboardType, m_pasteboardName)))
       return fragment.release();

    if (types.contains(String(NSRTFPboardType)) &&
        (fragment = documentFragmentWithRTF(frame, NSRTFPboardType, m_pasteboardName)))
        return fragment.release();

    if (types.contains(String(NSTIFFPboardType)) &&
        (fragment = documentFragmentWithImageResource(frame, ArchiveResource::create(platformStrategies()->pasteboardStrategy()->bufferForType(NSTIFFPboardType, m_pasteboardName), uniqueURLWithRelativePart(@"image.tiff"), ASCIILiteral("image/tiff"), emptyString(), emptyString()))))
        return fragment.release();

    if (types.contains(String(NSPDFPboardType)) &&
        (fragment = documentFragmentWithImageResource(frame, ArchiveResource::create(platformStrategies()->pasteboardStrategy()->bufferForType(NSPDFPboardType, m_pasteboardName).get(), uniqueURLWithRelativePart(@"application.pdf"), ASCIILiteral("application/pdf"), emptyString(), emptyString()))))
        return fragment.release();

    if (types.contains(String(kUTTypePNG)) &&
        (fragment = documentFragmentWithImageResource(frame, ArchiveResource::create(platformStrategies()->pasteboardStrategy()->bufferForType(String(kUTTypePNG), m_pasteboardName), uniqueURLWithRelativePart(@"image.png"), ASCIILiteral("image/png"), emptyString(), emptyString()))))
        return fragment.release();

    if (types.contains(String(NSURLPboardType))) {
        NSURL *URL = platformStrategies()->pasteboardStrategy()->url(m_pasteboardName);
        Document* document = frame->document();
        ASSERT(document);
        if (!document)
            return 0;
        RefPtr<Element> anchor = document->createElement(HTMLNames::aTag, false);
        NSString *URLString = [URL absoluteString]; // Original data is ASCII-only, so there is no need to precompose.
        if ([URLString length] == 0)
            return nil;
        NSString *URLTitleString = [platformStrategies()->pasteboardStrategy()->stringForType(WebURLNamePboardType, m_pasteboardName) precomposedStringWithCanonicalMapping];
        anchor->setAttribute(HTMLNames::hrefAttr, URLString);
        anchor->appendChild(document->createTextNode(URLTitleString), IGNORE_EXCEPTION);
        fragment = document->createDocumentFragment();
        if (fragment) {
            fragment->appendChild(anchor, IGNORE_EXCEPTION);
            return fragment.release();
        }
    }

    if (allowPlainText && types.contains(String(NSStringPboardType))) {
        chosePlainText = true;
        fragment = createFragmentFromText(context.get(), [platformStrategies()->pasteboardStrategy()->stringForType(NSStringPboardType, m_pasteboardName) precomposedStringWithCanonicalMapping]);
        return fragment.release();
    }

    return 0;
}

bool Pasteboard::hasData()
{
    Vector<String> types;
    platformStrategies()->pasteboardStrategy()->getTypes(types, m_pasteboardName);
    return !types.isEmpty();
}

static String cocoaTypeFromHTMLClipboardType(const String& type)
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/dnd.html#dom-datatransfer-setdata
    String qType = type.lower();

    if (qType == "text")
        qType = ASCIILiteral("text/plain");
    if (qType == "url")
        qType = ASCIILiteral("text/uri-list");

    // Ignore any trailing charset - JS strings are Unicode, which encapsulates the charset issue
    if (qType == "text/plain" || qType.startsWith("text/plain;"))
        return String(NSStringPboardType);
    if (qType == "text/uri-list")
        // special case because UTI doesn't work with Cocoa's URL type
        return String(NSURLPboardType); // note special case in getData to read NSFilenamesType

    // Blacklist types that might contain subframe information
    if (qType == "text/rtf" || qType == "public.rtf" || qType == "com.apple.traditional-mac-plain-text")
        return String();

    // Try UTI now
    String mimeType = qType;
    if (RetainPtr<CFStringRef> utiType = adoptCF(UTTypeCreatePreferredIdentifierForTag(kUTTagClassMIMEType, mimeType.createCFString().get(), NULL))) {
        RetainPtr<CFStringRef> pbType = adoptCF(UTTypeCopyPreferredTagWithClass(utiType.get(), kUTTagClassNSPboardType));
        if (pbType)
            return pbType.get();
    }

    // No mapping, just pass the whole string though
    return qType;
}

void Pasteboard::clear(const String& type)
{
    String cocoaType = cocoaTypeFromHTMLClipboardType(type);
    if (cocoaType.isEmpty())
        return;
    platformStrategies()->pasteboardStrategy()->setStringForType(emptyString(), cocoaType, m_pasteboardName);
}

static Vector<String> absoluteURLsFromPasteboardFilenames(const String& pasteboardName, bool onlyFirstURL = false)
{
    Vector<String> fileList;
    platformStrategies()->pasteboardStrategy()->getPathnamesForType(fileList, String(NSFilenamesPboardType), pasteboardName);

    if (fileList.isEmpty())
        return fileList;

    size_t count = onlyFirstURL ? 1 : fileList.size();
    Vector<String> urls;
    for (size_t i = 0; i < count; i++) {
        NSURL *url = [NSURL fileURLWithPath:fileList[i]];
        urls.append(String([url absoluteString]));
    }
    return urls;
}

static Vector<String> absoluteURLsFromPasteboard(const String& pasteboardName, bool onlyFirstURL = false)
{
    // NOTE: We must always check [availableTypes containsObject:] before accessing pasteboard data
    // or CoreFoundation will printf when there is not data of the corresponding type.
    Vector<String> availableTypes;
    Vector<String> absoluteURLs;
    platformStrategies()->pasteboardStrategy()->getTypes(availableTypes, pasteboardName);

    // Try NSFilenamesPboardType because it contains a list
    if (availableTypes.contains(String(NSFilenamesPboardType))) {
        absoluteURLs = absoluteURLsFromPasteboardFilenames(pasteboardName, onlyFirstURL);
        if (!absoluteURLs.isEmpty())
            return absoluteURLs;
    }

    // Fallback to NSURLPboardType (which is a single URL)
    if (availableTypes.contains(String(NSURLPboardType))) {
        absoluteURLs.append(platformStrategies()->pasteboardStrategy()->stringForType(String(NSURLPboardType), pasteboardName));
        return absoluteURLs;
    }

    // No file paths on the pasteboard, return nil
    return Vector<String>();
}

String Pasteboard::readString(const String& type)
{
    const String& cocoaType = cocoaTypeFromHTMLClipboardType(type);
    String cocoaValue;

    // Grab the value off the pasteboard corresponding to the cocoaType
    if (cocoaType == String(NSURLPboardType)) {
        // "url" and "text/url-list" both map to NSURLPboardType in cocoaTypeFromHTMLClipboardType(), "url" only wants the first URL
        bool onlyFirstURL = (equalIgnoringCase(type, "url"));
        Vector<String> absoluteURLs = absoluteURLsFromPasteboard(m_pasteboardName, onlyFirstURL);
        for (size_t i = 0; i < absoluteURLs.size(); i++)
            cocoaValue = i ? "\n" + absoluteURLs[i]: absoluteURLs[i];
    } else if (cocoaType == String(NSStringPboardType))
        cocoaValue = [platformStrategies()->pasteboardStrategy()->stringForType(cocoaType, m_pasteboardName) precomposedStringWithCanonicalMapping];
    else if (!cocoaType.isEmpty())
        cocoaValue = platformStrategies()->pasteboardStrategy()->stringForType(cocoaType, m_pasteboardName);

    // Enforce changeCount ourselves for security.  We check after reading instead of before to be
    // sure it doesn't change between our testing the change count and accessing the data.
    if (!cocoaValue.isEmpty() && m_changeCount == platformStrategies()->pasteboardStrategy()->changeCount(m_pasteboardName))
        return cocoaValue;

    return String();
}

static String utiTypeFromCocoaType(const String& type)
{
    if (RetainPtr<CFStringRef> utiType = adoptCF(UTTypeCreatePreferredIdentifierForTag(kUTTagClassNSPboardType, type.createCFString().get(), 0))) {
        if (RetainPtr<CFStringRef> mimeType = adoptCF(UTTypeCopyPreferredTagWithClass(utiType.get(), kUTTagClassMIMEType)))
            return String(mimeType.get());
    }
    return String();
}

static void addHTMLClipboardTypesForCocoaType(ListHashSet<String>& resultTypes, const String& cocoaType, const String& pasteboardName)
{
    // UTI may not do these right, so make sure we get the right, predictable result
    if (cocoaType == String(NSStringPboardType)) {
        resultTypes.add(ASCIILiteral("text/plain"));
        return;
    }
    if (cocoaType == String(NSURLPboardType)) {
        resultTypes.add(ASCIILiteral("text/uri-list"));
        return;
    }
    if (cocoaType == String(NSFilenamesPboardType)) {
        // If file list is empty, add nothing.
        // Note that there is a chance that the file list count could have changed since we grabbed the types array.
        // However, this is not really an issue for us doing a sanity check here.
        Vector<String> fileList;
        platformStrategies()->pasteboardStrategy()->getPathnamesForType(fileList, String(NSFilenamesPboardType), pasteboardName);
        if (!fileList.isEmpty()) {
            // It is unknown if NSFilenamesPboardType always implies NSURLPboardType in Cocoa,
            // but NSFilenamesPboardType should imply both 'text/uri-list' and 'Files'
            resultTypes.add(ASCIILiteral("text/uri-list"));
            resultTypes.add(ASCIILiteral("Files"));
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

bool Pasteboard::writeString(const String& type, const String& data)
{
    const String& cocoaType = cocoaTypeFromHTMLClipboardType(type);
    String cocoaData = data;

    if (cocoaType == String(NSURLPboardType) || cocoaType == String(kUTTypeFileURL)) {
        NSURL *url = [NSURL URLWithString:cocoaData];
        if ([url isFileURL])
            return false;

        Vector<String> types;
        types.append(cocoaType);
        platformStrategies()->pasteboardStrategy()->setTypes(types, m_pasteboardName);
        platformStrategies()->pasteboardStrategy()->setStringForType(cocoaData, cocoaType, m_pasteboardName);

        return true;
    }

    if (!cocoaType.isEmpty()) {
        // everything else we know of goes on the pboard as a string
        Vector<String> types;
        types.append(cocoaType);
        platformStrategies()->pasteboardStrategy()->addTypes(types, m_pasteboardName);
        platformStrategies()->pasteboardStrategy()->setStringForType(cocoaData, cocoaType, m_pasteboardName);
        return true;
    }

    return false;
}

ListHashSet<String> Pasteboard::types()
{
    Vector<String> types;
    platformStrategies()->pasteboardStrategy()->getTypes(types, m_pasteboardName);

    // Enforce changeCount ourselves for security. We check after reading instead of before to be
    // sure it doesn't change between our testing the change count and accessing the data.
    if (m_changeCount != platformStrategies()->pasteboardStrategy()->changeCount(m_pasteboardName))
        return ListHashSet<String>();

    ListHashSet<String> result;
    // FIXME: This loop could be split into two stages. One which adds all the HTML5 specified types
    // and a second which adds all the extra types from the cocoa clipboard (which is Mac-only behavior).
    for (size_t i = 0; i < types.size(); i++) {
        if (types[i] == "NeXT plain ascii pasteboard type")
            continue;   // skip this ancient type that gets auto-supplied by some system conversion

        addHTMLClipboardTypesForCocoaType(result, types[i], m_pasteboardName);
    }

    return result;
}

Vector<String> Pasteboard::readFilenames()
{
    // FIXME: Seems silly to convert paths to URLs and then back to paths. Does that do anything helpful?
    Vector<String> absoluteURLs = absoluteURLsFromPasteboardFilenames(m_pasteboardName);
    Vector<String> paths;
    paths.reserveCapacity(absoluteURLs.size());
    for (size_t i = 0; i < absoluteURLs.size(); i++) {
        NSURL *absoluteURL = [NSURL URLWithString:absoluteURLs[i]];
        ASSERT([absoluteURL isFileURL]);
        paths.uncheckedAppend([absoluteURL path]);
    }
    return paths;
}

void Pasteboard::setDragImage(DragImageRef image, const IntPoint& location)
{
    // Don't allow setting the drag image if someone kept a pasteboard and is trying to set the image too late.
    if (m_changeCount != platformStrategies()->pasteboardStrategy()->changeCount(m_pasteboardName))
        return;

    // Dashboard wants to be able to set the drag image during dragging, but Cocoa does not allow this.
    // Instead we must drop down to the CoreGraphics API.
    wkSetDragImage(image.get(), location);

    // Hack: We must post an event to wake up the NSDragManager, which is sitting in a nextEvent call
    // up the stack from us because the CoreFoundation drag manager does not use the run loop by itself.
    // This is the most innocuous event to use, per Kristen Forster.
    NSEvent* event = [NSEvent mouseEventWithType:NSMouseMoved location:NSZeroPoint
        modifierFlags:0 timestamp:0 windowNumber:0 context:nil eventNumber:0 clickCount:0 pressure:0];
    [NSApp postEvent:event atStart:YES];
}

}
