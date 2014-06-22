/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2006 James G. Speth (speth@end.com)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
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
#import "DOMInternal.h" // import first to make the private/public trick work
#import "DOM.h"

#import "CachedImage.h"
#import "DOMElementInternal.h"
#import "DOMHTMLCanvasElement.h"
#import "DOMHTMLTableCellElementInternal.h"
#import "DOMNodeInternal.h"
#import "DOMPrivate.h"
#import "DOMRangeInternal.h"
#import "Font.h"
#import "Frame.h"
#import "FrameSnapshottingMac.h"
#import "HTMLElement.h"
#import "HTMLNames.h"
#import "HTMLParserIdioms.h"
#import "HTMLTableCellElement.h"
#import "Image.h"
#import "JSNode.h"
#import "NodeFilter.h"
#import "Range.h"
#import "RenderImage.h"
#import "ScriptController.h"
#import "WebScriptObjectPrivate.h"
#import <JavaScriptCore/APICast.h>
#import <wtf/HashMap.h>

using namespace JSC;
using namespace WebCore;

// FIXME: Would be nice to break this up into separate files to match how other WebKit
// code is organized.

//------------------------------------------------------------------------------------------
// DOMNode

namespace WebCore {

typedef HashMap<const QualifiedName::QualifiedNameImpl*, Class> ObjCClassMap;
static ObjCClassMap* elementClassMap;

static void addElementClass(const QualifiedName& tag, Class objCClass)
{
    elementClassMap->set(tag.impl(), objCClass);
}

static void createElementClassMap()
{
    // Create the table.
    elementClassMap = new ObjCClassMap;

    // FIXME: Reflect marquee once the API has been determined.

    // Populate it with HTML and SVG element classes.
    addElementClass(HTMLNames::aTag, [DOMHTMLAnchorElement class]);
    addElementClass(HTMLNames::appletTag, [DOMHTMLAppletElement class]);
    addElementClass(HTMLNames::areaTag, [DOMHTMLAreaElement class]);
    addElementClass(HTMLNames::baseTag, [DOMHTMLBaseElement class]);
    addElementClass(HTMLNames::basefontTag, [DOMHTMLBaseFontElement class]);
    addElementClass(HTMLNames::bodyTag, [DOMHTMLBodyElement class]);
    addElementClass(HTMLNames::brTag, [DOMHTMLBRElement class]);
    addElementClass(HTMLNames::buttonTag, [DOMHTMLButtonElement class]);
    addElementClass(HTMLNames::canvasTag, [DOMHTMLCanvasElement class]);
    addElementClass(HTMLNames::captionTag, [DOMHTMLTableCaptionElement class]);
    addElementClass(HTMLNames::colTag, [DOMHTMLTableColElement class]);
    addElementClass(HTMLNames::colgroupTag, [DOMHTMLTableColElement class]);
    addElementClass(HTMLNames::delTag, [DOMHTMLModElement class]);
    addElementClass(HTMLNames::dirTag, [DOMHTMLDirectoryElement class]);
    addElementClass(HTMLNames::divTag, [DOMHTMLDivElement class]);
    addElementClass(HTMLNames::dlTag, [DOMHTMLDListElement class]);
    addElementClass(HTMLNames::embedTag, [DOMHTMLEmbedElement class]);
    addElementClass(HTMLNames::fieldsetTag, [DOMHTMLFieldSetElement class]);
    addElementClass(HTMLNames::fontTag, [DOMHTMLFontElement class]);
    addElementClass(HTMLNames::formTag, [DOMHTMLFormElement class]);
    addElementClass(HTMLNames::frameTag, [DOMHTMLFrameElement class]);
    addElementClass(HTMLNames::framesetTag, [DOMHTMLFrameSetElement class]);
    addElementClass(HTMLNames::h1Tag, [DOMHTMLHeadingElement class]);
    addElementClass(HTMLNames::h2Tag, [DOMHTMLHeadingElement class]);
    addElementClass(HTMLNames::h3Tag, [DOMHTMLHeadingElement class]);
    addElementClass(HTMLNames::h4Tag, [DOMHTMLHeadingElement class]);
    addElementClass(HTMLNames::h5Tag, [DOMHTMLHeadingElement class]);
    addElementClass(HTMLNames::h6Tag, [DOMHTMLHeadingElement class]);
    addElementClass(HTMLNames::headTag, [DOMHTMLHeadElement class]);
    addElementClass(HTMLNames::hrTag, [DOMHTMLHRElement class]);
    addElementClass(HTMLNames::htmlTag, [DOMHTMLHtmlElement class]);
    addElementClass(HTMLNames::iframeTag, [DOMHTMLIFrameElement class]);
    addElementClass(HTMLNames::imgTag, [DOMHTMLImageElement class]);
    addElementClass(HTMLNames::inputTag, [DOMHTMLInputElement class]);
    addElementClass(HTMLNames::insTag, [DOMHTMLModElement class]);
    addElementClass(HTMLNames::labelTag, [DOMHTMLLabelElement class]);
    addElementClass(HTMLNames::legendTag, [DOMHTMLLegendElement class]);
    addElementClass(HTMLNames::liTag, [DOMHTMLLIElement class]);
    addElementClass(HTMLNames::linkTag, [DOMHTMLLinkElement class]);
    addElementClass(HTMLNames::listingTag, [DOMHTMLPreElement class]);
    addElementClass(HTMLNames::mapTag, [DOMHTMLMapElement class]);
    addElementClass(HTMLNames::marqueeTag, [DOMHTMLMarqueeElement class]);
    addElementClass(HTMLNames::menuTag, [DOMHTMLMenuElement class]);
    addElementClass(HTMLNames::metaTag, [DOMHTMLMetaElement class]);
    addElementClass(HTMLNames::objectTag, [DOMHTMLObjectElement class]);
    addElementClass(HTMLNames::olTag, [DOMHTMLOListElement class]);
    addElementClass(HTMLNames::optgroupTag, [DOMHTMLOptGroupElement class]);
    addElementClass(HTMLNames::optionTag, [DOMHTMLOptionElement class]);
    addElementClass(HTMLNames::pTag, [DOMHTMLParagraphElement class]);
    addElementClass(HTMLNames::paramTag, [DOMHTMLParamElement class]);
    addElementClass(HTMLNames::preTag, [DOMHTMLPreElement class]);
    addElementClass(HTMLNames::qTag, [DOMHTMLQuoteElement class]);
    addElementClass(HTMLNames::scriptTag, [DOMHTMLScriptElement class]);
    addElementClass(HTMLNames::selectTag, [DOMHTMLSelectElement class]);
    addElementClass(HTMLNames::styleTag, [DOMHTMLStyleElement class]);
    addElementClass(HTMLNames::tableTag, [DOMHTMLTableElement class]);
    addElementClass(HTMLNames::tbodyTag, [DOMHTMLTableSectionElement class]);
    addElementClass(HTMLNames::tdTag, [DOMHTMLTableCellElement class]);
    addElementClass(HTMLNames::textareaTag, [DOMHTMLTextAreaElement class]);
    addElementClass(HTMLNames::tfootTag, [DOMHTMLTableSectionElement class]);
    addElementClass(HTMLNames::thTag, [DOMHTMLTableCellElement class]);
    addElementClass(HTMLNames::theadTag, [DOMHTMLTableSectionElement class]);
    addElementClass(HTMLNames::titleTag, [DOMHTMLTitleElement class]);
    addElementClass(HTMLNames::trTag, [DOMHTMLTableRowElement class]);
    addElementClass(HTMLNames::ulTag, [DOMHTMLUListElement class]);
    addElementClass(HTMLNames::xmpTag, [DOMHTMLPreElement class]);
}

static Class lookupElementClass(const QualifiedName& tag)
{
    // Do a special lookup to ignore element prefixes
    if (tag.hasPrefix())
        return elementClassMap->get(QualifiedName(nullAtom, tag.localName(), tag.namespaceURI()).impl());
    
    return elementClassMap->get(tag.impl());
}

static Class elementClass(const QualifiedName& tag, Class defaultClass)
{
    if (!elementClassMap)
        createElementClassMap();
    Class objcClass = lookupElementClass(tag);
    if (!objcClass)
        objcClass = defaultClass;
    return objcClass;
}

static NSArray *kit(const Vector<IntRect>& rects)
{
    size_t size = rects.size();
    NSMutableArray *array = [NSMutableArray arrayWithCapacity:size];
    for (size_t i = 0; i < size; ++i)
        [array addObject:[NSValue valueWithRect:rects[i]]];
    return array;
}

} // namespace WebCore

@implementation DOMNode (WebCoreInternal)

- (NSString *)description
{
    if (!_internal)
        return [NSString stringWithFormat:@"<%@: null>", [[self class] description]];

    NSString *value = [self nodeValue];
    if (value)
        return [NSString stringWithFormat:@"<%@ [%@]: %p '%@'>",
            [[self class] description], [self nodeName], _internal, value];

    return [NSString stringWithFormat:@"<%@ [%@]: %p>", [[self class] description], [self nodeName], _internal];
}

- (JSC::Bindings::RootObject*)_rootObject
{
    WebCore::Frame* frame = core(self)->document()->frame();
    if (!frame)
        return 0;
    return frame->script()->bindingRootObject();
}

@end

Class kitClass(WebCore::Node* impl)
{
    switch (impl->nodeType()) {
        case WebCore::Node::ELEMENT_NODE:
            if (impl->isHTMLElement())
                return WebCore::elementClass(toHTMLElement(impl)->tagQName(), [DOMHTMLElement class]);
            return [DOMElement class];
        case WebCore::Node::ATTRIBUTE_NODE:
            return [DOMAttr class];
        case WebCore::Node::TEXT_NODE:
            return [DOMText class];
        case WebCore::Node::CDATA_SECTION_NODE:
            return [DOMCDATASection class];
        case WebCore::Node::ENTITY_REFERENCE_NODE:
            return [DOMEntityReference class];
        case WebCore::Node::ENTITY_NODE:
            return [DOMEntity class];
        case WebCore::Node::PROCESSING_INSTRUCTION_NODE:
            return [DOMProcessingInstruction class];
        case WebCore::Node::COMMENT_NODE:
            return [DOMComment class];
        case WebCore::Node::DOCUMENT_NODE:
            if (static_cast<WebCore::Document*>(impl)->isHTMLDocument())
                return [DOMHTMLDocument class];
            return [DOMDocument class];
        case WebCore::Node::DOCUMENT_TYPE_NODE:
            return [DOMDocumentType class];
        case WebCore::Node::DOCUMENT_FRAGMENT_NODE:
            return [DOMDocumentFragment class];
        case WebCore::Node::NOTATION_NODE:
            return [DOMNotation class];
        case WebCore::Node::XPATH_NAMESPACE_NODE:
            // FIXME: Create an XPath objective C wrapper
            // See http://bugs.webkit.org/show_bug.cgi?id=8755
            return nil;
    }
    ASSERT_NOT_REACHED();
    return nil;
}

id <DOMEventTarget> kit(WebCore::EventTarget* eventTarget)
{
    if (!eventTarget)
        return nil;

    if (WebCore::Node* node = eventTarget->toNode())
        return kit(node);

    // We don't have an ObjC binding for XMLHttpRequest.

    return nil;
}

@implementation DOMNode (DOMNodeExtensions)

- (NSRect)boundingBox
{
    // FIXME: Could we move this function to WebCore::Node and autogenerate?
    core(self)->document()->updateLayoutIgnorePendingStylesheets();
    WebCore::RenderObject* renderer = core(self)->renderer();
    if (!renderer)
        return NSZeroRect;
    return renderer->absoluteBoundingBoxRect();
}

- (NSArray *)lineBoxRects
{
    return [self textRects];
}

@end

@implementation DOMNode (DOMNodeExtensionsPendingPublic)

- (NSImage *)renderedImage
{
    // FIXME: Could we move this function to WebCore::Node and autogenerate?
    WebCore::Node* node = core(self);
    WebCore::Frame* frame = node->document()->frame();
    if (!frame)
        return nil;
    return frame->nodeImage(node).get();
}

- (NSArray *)textRects
{
    core(self)->document()->updateLayoutIgnorePendingStylesheets();
    if (!core(self)->renderer())
        return nil;
    Vector<WebCore::IntRect> rects;
    core(self)->textRects(rects);
    return kit(rects);
}

@end

@implementation DOMNode (WebPrivate)

+ (id)_nodeFromJSWrapper:(JSObjectRef)jsWrapper
{
    JSObject* object = toJS(jsWrapper);

    if (!object->inherits(&JSNode::s_info))
        return nil;

    WebCore::Node* node = jsCast<JSNode*>(object)->impl();
    return kit(node);
}

@end

@implementation DOMRange (DOMRangeExtensions)

- (NSRect)boundingBox
{
    // FIXME: The call to updateLayoutIgnorePendingStylesheets should be moved into WebCore::Range.
    core(self)->ownerDocument()->updateLayoutIgnorePendingStylesheets();
    return core(self)->boundingBox();
}

- (NSImage *)renderedImageForcingBlackText:(BOOL)forceBlackText
{
    WebCore::Range* range = core(self);
    WebCore::Frame* frame = range->ownerDocument()->frame();
    if (!frame)
        return nil;

    return WebCore::rangeImage(frame, range, forceBlackText);
}

- (NSArray *)textRects
{
    // FIXME: The call to updateLayoutIgnorePendingStylesheets should be moved into WebCore::Range.
    Vector<WebCore::IntRect> rects;
    core(self)->ownerDocument()->updateLayoutIgnorePendingStylesheets();
    core(self)->textRects(rects);
    return kit(rects);
}

- (NSArray *)lineBoxRects
{
    // FIXME: Remove this once all clients stop using it and we drop Leopard support.
    return [self textRects];
}

@end

//------------------------------------------------------------------------------------------
// DOMElement

@implementation DOMElement (DOMElementAppKitExtensions)

- (NSImage*)image
{
    // FIXME: Could we move this function to WebCore::Node and autogenerate?
    WebCore::RenderObject* renderer = core(self)->renderer();
    if (!renderer || !renderer->isImage())
        return nil;
    WebCore::CachedImage* cachedImage = static_cast<WebCore::RenderImage*>(renderer)->cachedImage();
    if (!cachedImage || cachedImage->errorOccurred())
        return nil;
    return cachedImage->imageForRenderer(renderer)->getNSImage();
}

@end

@implementation DOMElement (WebPrivate)

- (NSFont *)_font
{
    // FIXME: Could we move this function to WebCore::Element and autogenerate?
    WebCore::RenderObject* renderer = core(self)->renderer();
    if (!renderer)
        return nil;
    return renderer->style()->font().primaryFont()->getNSFont();
}

- (NSData *)_imageTIFFRepresentation
{
    // FIXME: Could we move this function to WebCore::Element and autogenerate?
    WebCore::RenderObject* renderer = core(self)->renderer();
    if (!renderer || !renderer->isImage())
        return nil;
    WebCore::CachedImage* cachedImage = static_cast<WebCore::RenderImage*>(renderer)->cachedImage();
    if (!cachedImage || cachedImage->errorOccurred())
        return nil;
    return (NSData *)cachedImage->imageForRenderer(renderer)->getTIFFRepresentation();
}

- (NSURL *)_getURLAttribute:(NSString *)name
{
    // FIXME: Could we move this function to WebCore::Element and autogenerate?
    ASSERT(name);
    WebCore::Element* element = core(self);
    ASSERT(element);
    return element->document()->completeURL(stripLeadingAndTrailingHTMLSpaces(element->getAttribute(name)));
}

- (BOOL)isFocused
{
    // FIXME: Could we move this function to WebCore::Element and autogenerate?
    WebCore::Element* element = core(self);
    return element->document()->focusedElement() == element;
}

@end

//------------------------------------------------------------------------------------------
// DOMRange

@implementation DOMRange (WebPrivate)

- (NSString *)description
{
    if (!_internal)
        return @"<DOMRange: null>";
    return [NSString stringWithFormat:@"<DOMRange: %@ %d %@ %d>",
               [self startContainer], [self startOffset], [self endContainer], [self endOffset]];
}

// FIXME: This should be removed as soon as all internal Apple uses of it have been replaced with
// calls to the public method - (NSString *)text.
- (NSString *)_text
{
    return [self text];
}

@end

//------------------------------------------------------------------------------------------
// DOMRGBColor

@implementation DOMRGBColor (WebPrivate)

// FIXME: This should be removed as soon as all internal Apple uses of it have been replaced with
// calls to the public method - (NSColor *)color.
- (NSColor *)_color
{
    return [self color];
}

@end


@implementation DOMHTMLTableCellElement (WebPrivate)

- (DOMHTMLTableCellElement *)_cellAbove
{
    return kit(core(self)->cellAbove());
}

@end

//------------------------------------------------------------------------------------------
// DOMNodeFilter

DOMNodeFilter *kit(WebCore::NodeFilter* impl)
{
    if (!impl)
        return nil;
    
    if (DOMNodeFilter *wrapper = getDOMWrapper(impl))
        return [[wrapper retain] autorelease];
    
    DOMNodeFilter *wrapper = [[DOMNodeFilter alloc] _init];
    wrapper->_internal = reinterpret_cast<DOMObjectInternal*>(impl);
    impl->ref();
    addDOMWrapper(wrapper, impl);
    return [wrapper autorelease];
}

WebCore::NodeFilter* core(DOMNodeFilter *wrapper)
{
    return wrapper ? reinterpret_cast<WebCore::NodeFilter*>(wrapper->_internal) : 0;
}

@implementation DOMNodeFilter

- (void)dealloc
{
    if (_internal)
        reinterpret_cast<WebCore::NodeFilter*>(_internal)->deref();
    [super dealloc];
}

- (void)finalize
{
    if (_internal)
        reinterpret_cast<WebCore::NodeFilter*>(_internal)->deref();
    [super finalize];
}

- (short)acceptNode:(DOMNode *)node
{
    return core(self)->acceptNode(core(node));
}

@end
