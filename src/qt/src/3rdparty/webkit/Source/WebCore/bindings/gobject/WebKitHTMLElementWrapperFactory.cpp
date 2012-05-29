/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2008 Luke Kenneth Casson Leighton <lkcl@lkcl.net>
 * Copyright (C) 2008 Martin Soto <soto@freedesktop.org>
 * Copyright (C) 2010 Igalia S.L.
 *
 * This file is derived by hand from an automatically generated file.
 * Keeping it up-to-date could potentially be done by adding
 * a make_names.pl generator, or by writing a separate
 * generater which takes JSHTMLElementWrapperFactory.h as input.
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

#include "config.h"
#include "WebKitHTMLElementWrapperFactory.h"

#include "HTMLAnchorElement.h"
#include "HTMLAppletElement.h"
#include "HTMLAreaElement.h"
#include "HTMLAudioElement.h"
#include "HTMLBRElement.h"
#include "HTMLBaseElement.h"
#include "HTMLBaseFontElement.h"
#include "HTMLBlockquoteElement.h"
#include "HTMLBodyElement.h"
#include "HTMLButtonElement.h"
#include "HTMLCanvasElement.h"
#include "HTMLDListElement.h"
#include "HTMLDirectoryElement.h"
#include "HTMLDivElement.h"
#include "HTMLEmbedElement.h"
#include "HTMLFieldSetElement.h"
#include "HTMLFontElement.h"
#include "HTMLFormElement.h"
#include "HTMLFrameElement.h"
#include "HTMLFrameSetElement.h"
#include "HTMLHRElement.h"
#include "HTMLHeadElement.h"
#include "HTMLHeadingElement.h"
#include "HTMLHtmlElement.h"
#include "HTMLIFrameElement.h"
#include "HTMLImageElement.h"
#include "HTMLInputElement.h"
#include "HTMLIsIndexElement.h"
#include "HTMLKeygenElement.h"
#include "HTMLLIElement.h"
#include "HTMLLabelElement.h"
#include "HTMLLegendElement.h"
#include "HTMLLinkElement.h"
#include "HTMLMapElement.h"
#include "HTMLMarqueeElement.h"
#include "HTMLMenuElement.h"
#include "HTMLMetaElement.h"
#include "HTMLModElement.h"
#include "HTMLNames.h"
#include "HTMLOListElement.h"
#include "HTMLObjectElement.h"
#include "HTMLOptGroupElement.h"
#include "HTMLOptionElement.h"
#include "HTMLParagraphElement.h"
#include "HTMLParamElement.h"
#include "HTMLPreElement.h"
#include "HTMLQuoteElement.h"
#include "HTMLScriptElement.h"
#include "HTMLSelectElement.h"
#include "HTMLStyleElement.h"
#include "HTMLTableCaptionElement.h"
#include "HTMLTableCellElement.h"
#include "HTMLTableColElement.h"
#include "HTMLTableElement.h"
#include "HTMLTableRowElement.h"
#include "HTMLTableSectionElement.h"
#include "HTMLTextAreaElement.h"
#include "HTMLTitleElement.h"
#include "HTMLUListElement.h"
#include "HTMLVideoElement.h"

#include "webkit/WebKitDOMHTMLAnchorElementPrivate.h"
#include "webkit/WebKitDOMHTMLAppletElementPrivate.h"
#include "webkit/WebKitDOMHTMLAreaElementPrivate.h"
#include "webkit/WebKitDOMHTMLBRElementPrivate.h"
#include "webkit/WebKitDOMHTMLBaseElementPrivate.h"
#include "webkit/WebKitDOMHTMLBaseFontElementPrivate.h"
#include "webkit/WebKitDOMHTMLBlockquoteElementPrivate.h"
#include "webkit/WebKitDOMHTMLBodyElementPrivate.h"
#include "webkit/WebKitDOMHTMLButtonElementPrivate.h"
#include "webkit/WebKitDOMHTMLCanvasElementPrivate.h"
#include "webkit/WebKitDOMHTMLDListElementPrivate.h"
#include "webkit/WebKitDOMHTMLDirectoryElementPrivate.h"
#include "webkit/WebKitDOMHTMLDivElementPrivate.h"
#include "webkit/WebKitDOMHTMLElementPrivate.h"
#include "webkit/WebKitDOMHTMLEmbedElementPrivate.h"
#include "webkit/WebKitDOMHTMLFieldSetElementPrivate.h"
#include "webkit/WebKitDOMHTMLFontElementPrivate.h"
#include "webkit/WebKitDOMHTMLFormElementPrivate.h"
#include "webkit/WebKitDOMHTMLFrameElementPrivate.h"
#include "webkit/WebKitDOMHTMLFrameSetElementPrivate.h"
#include "webkit/WebKitDOMHTMLHRElementPrivate.h"
#include "webkit/WebKitDOMHTMLHeadElementPrivate.h"
#include "webkit/WebKitDOMHTMLHeadingElementPrivate.h"
#include "webkit/WebKitDOMHTMLHtmlElementPrivate.h"
#include "webkit/WebKitDOMHTMLIFrameElementPrivate.h"
#include "webkit/WebKitDOMHTMLImageElementPrivate.h"
#include "webkit/WebKitDOMHTMLInputElementPrivate.h"
#include "webkit/WebKitDOMHTMLIsIndexElementPrivate.h"
#include "webkit/WebKitDOMHTMLKeygenElementPrivate.h"
#include "webkit/WebKitDOMHTMLLIElementPrivate.h"
#include "webkit/WebKitDOMHTMLLabelElementPrivate.h"
#include "webkit/WebKitDOMHTMLLegendElementPrivate.h"
#include "webkit/WebKitDOMHTMLLinkElementPrivate.h"
#include "webkit/WebKitDOMHTMLMapElementPrivate.h"
#include "webkit/WebKitDOMHTMLMarqueeElementPrivate.h"
#include "webkit/WebKitDOMHTMLMenuElementPrivate.h"
#include "webkit/WebKitDOMHTMLMetaElementPrivate.h"
#include "webkit/WebKitDOMHTMLModElementPrivate.h"
#include "webkit/WebKitDOMHTMLOListElementPrivate.h"
#include "webkit/WebKitDOMHTMLObjectElementPrivate.h"
#include "webkit/WebKitDOMHTMLOptGroupElementPrivate.h"
#include "webkit/WebKitDOMHTMLOptionElementPrivate.h"
#include "webkit/WebKitDOMHTMLParagraphElementPrivate.h"
#include "webkit/WebKitDOMHTMLParamElementPrivate.h"
#include "webkit/WebKitDOMHTMLPreElementPrivate.h"
#include "webkit/WebKitDOMHTMLQuoteElementPrivate.h"
#include "webkit/WebKitDOMHTMLScriptElementPrivate.h"
#include "webkit/WebKitDOMHTMLSelectElementPrivate.h"
#include "webkit/WebKitDOMHTMLStyleElementPrivate.h"
#include "webkit/WebKitDOMHTMLTableCaptionElementPrivate.h"
#include "webkit/WebKitDOMHTMLTableCellElementPrivate.h"
#include "webkit/WebKitDOMHTMLTableColElementPrivate.h"
#include "webkit/WebKitDOMHTMLTableElementPrivate.h"
#include "webkit/WebKitDOMHTMLTableRowElementPrivate.h"
#include "webkit/WebKitDOMHTMLTableSectionElementPrivate.h"
#include "webkit/WebKitDOMHTMLTextAreaElementPrivate.h"
#include "webkit/WebKitDOMHTMLTitleElementPrivate.h"
#include "webkit/WebKitDOMHTMLUListElementPrivate.h"
#include "webkit/webkitdom.h"

#if ENABLE(VIDEO)
#include "webkit/WebKitDOMHTMLAudioElementPrivate.h"
#include "webkit/WebKitDOMHTMLVideoElementPrivate.h"
#endif

#include <wtf/text/CString.h>

namespace WebKit {

using namespace WebCore;
using namespace WebCore::HTMLNames;

typedef gpointer (*CreateHTMLElementWrapperFunction)(PassRefPtr<HTMLElement>);

static gpointer createAnchorWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLAnchorElement(static_cast<HTMLAnchorElement*>(element.get()));
}

static gpointer createAppletWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLAppletElement(static_cast<HTMLAppletElement*>(element.get()));
}

static gpointer createAreaWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLAreaElement(static_cast<HTMLAreaElement*>(element.get()));
}

#if ENABLE(VIDEO)
static gpointer createAudioWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLAudioElement(static_cast<HTMLAudioElement*>(element.get()));
}

static gpointer createVideoWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLVideoElement(static_cast<HTMLVideoElement*>(element.get()));
}
#endif

static gpointer createBaseWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLBaseElement(static_cast<HTMLBaseElement*>(element.get()));
}

static gpointer createBaseFontWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLBaseFontElement(static_cast<HTMLBaseFontElement*>(element.get()));
}

static gpointer createBlockquoteWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLBlockquoteElement(static_cast<HTMLBlockquoteElement*>(element.get()));
}

static gpointer createBodyWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLBodyElement(static_cast<HTMLBodyElement*>(element.get()));
}

static gpointer createBRWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLBRElement(static_cast<HTMLBRElement*>(element.get()));
}

static gpointer createButtonWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLButtonElement(static_cast<HTMLButtonElement*>(element.get()));
}

static gpointer createCanvasWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLCanvasElement(static_cast<HTMLCanvasElement*>(element.get()));
}

static gpointer createTableCaptionWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLTableCaptionElement(static_cast<HTMLTableCaptionElement*>(element.get()));
}

static gpointer createTableColWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLTableColElement(static_cast<HTMLTableColElement*>(element.get()));
}

static gpointer createModWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLModElement(static_cast<HTMLModElement*>(element.get()));
}

static gpointer createDirectoryWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLDirectoryElement(static_cast<HTMLDirectoryElement*>(element.get()));
}

static gpointer createDivWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLDivElement(static_cast<HTMLDivElement*>(element.get()));
}

static gpointer createDListWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLDListElement(static_cast<HTMLDListElement*>(element.get()));
}

static gpointer createEmbedWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLEmbedElement(static_cast<HTMLEmbedElement*>(element.get()));
}

static gpointer createFieldSetWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLFieldSetElement(static_cast<HTMLFieldSetElement*>(element.get()));
}

static gpointer createFontWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLFontElement(static_cast<HTMLFontElement*>(element.get()));
}

static gpointer createFormWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLFormElement(static_cast<HTMLFormElement*>(element.get()));
}

static gpointer createFrameWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLFrameElement(static_cast<HTMLFrameElement*>(element.get()));
}

static gpointer createFrameSetWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLFrameSetElement(static_cast<HTMLFrameSetElement*>(element.get()));
}

static gpointer createHeadingWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLHeadingElement(static_cast<HTMLHeadingElement*>(element.get()));
}

static gpointer createHeadWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLHeadElement(static_cast<HTMLHeadElement*>(element.get()));
}

static gpointer createHRWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLHRElement(static_cast<HTMLHRElement*>(element.get()));
}

static gpointer createHtmlWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLHtmlElement(static_cast<HTMLHtmlElement*>(element.get()));
}

static gpointer createIFrameWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLIFrameElement(static_cast<HTMLIFrameElement*>(element.get()));
}

static gpointer createImageWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLImageElement(static_cast<HTMLImageElement*>(element.get()));
}

static gpointer createInputWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLInputElement(static_cast<HTMLInputElement*>(element.get()));
}

static gpointer createIsIndexWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLIsIndexElement(static_cast<HTMLIsIndexElement*>(element.get()));
}

static gpointer createKeygenWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLKeygenElement(static_cast<HTMLKeygenElement*>(element.get()));
}

static gpointer createLabelWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLLabelElement(static_cast<HTMLLabelElement*>(element.get()));
}

static gpointer createLegendWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLLegendElement(static_cast<HTMLLegendElement*>(element.get()));
}

static gpointer createLIWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLLIElement(static_cast<HTMLLIElement*>(element.get()));
}

static gpointer createLinkWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLLinkElement(static_cast<HTMLLinkElement*>(element.get()));
}

static gpointer createMapWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLMapElement(static_cast<HTMLMapElement*>(element.get()));
}

static gpointer createMarqueeWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLMarqueeElement(static_cast<HTMLMarqueeElement*>(element.get()));
}

static gpointer createMenuWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLMenuElement(static_cast<HTMLMenuElement*>(element.get()));
}

static gpointer createMetaWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLMetaElement(static_cast<HTMLMetaElement*>(element.get()));
}

static gpointer createObjectWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLObjectElement(static_cast<HTMLObjectElement*>(element.get()));
}

static gpointer createOListWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLOListElement(static_cast<HTMLOListElement*>(element.get()));
}

static gpointer createOptGroupWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLOptGroupElement(static_cast<HTMLOptGroupElement*>(element.get()));
}

static gpointer createOptionWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLOptionElement(static_cast<HTMLOptionElement*>(element.get()));
}

static gpointer createParagraphWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLParagraphElement(static_cast<HTMLParagraphElement*>(element.get()));
}

static gpointer createParamWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLParamElement(static_cast<HTMLParamElement*>(element.get()));
}

static gpointer createPreWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLPreElement(static_cast<HTMLPreElement*>(element.get()));
}

static gpointer createQuoteWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLQuoteElement(static_cast<HTMLQuoteElement*>(element.get()));
}

static gpointer createScriptWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLScriptElement(static_cast<HTMLScriptElement*>(element.get()));
}

static gpointer createSelectWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLSelectElement(static_cast<HTMLSelectElement*>(element.get()));
}

static gpointer createStyleWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLStyleElement(static_cast<HTMLStyleElement*>(element.get()));
}

static gpointer createTableWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLTableElement(static_cast<HTMLTableElement*>(element.get()));
}

static gpointer createTableSectionWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLTableSectionElement(static_cast<HTMLTableSectionElement*>(element.get()));
}

static gpointer createTableCellWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLTableCellElement(static_cast<HTMLTableCellElement*>(element.get()));
}

static gpointer createTextAreaWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLTextAreaElement(static_cast<HTMLTextAreaElement*>(element.get()));
}

static gpointer createTitleWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLTitleElement(static_cast<HTMLTitleElement*>(element.get()));
}

static gpointer createTableRowWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLTableRowElement(static_cast<HTMLTableRowElement*>(element.get()));
}

static gpointer createUListWrapper(PassRefPtr<HTMLElement> element)
{
    return wrapHTMLUListElement(static_cast<HTMLUListElement*>(element.get()));
}

gpointer createHTMLElementWrapper(PassRefPtr<WebCore::HTMLElement> element)
{
    static HashMap<WTF::AtomicStringImpl*, CreateHTMLElementWrapperFunction> map;
    if (map.isEmpty()) {
       map.set(aTag.localName().impl(), createAnchorWrapper);
       map.set(appletTag.localName().impl(), createAppletWrapper);
#if ENABLE(VIDEO)
       map.set(audioTag.localName().impl(), createAudioWrapper);
       map.set(videoTag.localName().impl(), createVideoWrapper);
#endif
       map.set(areaTag.localName().impl(), createAreaWrapper);
       map.set(baseTag.localName().impl(), createBaseWrapper);
       map.set(basefontTag.localName().impl(), createBaseFontWrapper);
       map.set(blockquoteTag.localName().impl(), createBlockquoteWrapper);
       map.set(bodyTag.localName().impl(), createBodyWrapper);
       map.set(brTag.localName().impl(), createBRWrapper);
       map.set(buttonTag.localName().impl(), createButtonWrapper);
       map.set(canvasTag.localName().impl(), createCanvasWrapper);
       map.set(captionTag.localName().impl(), createTableCaptionWrapper);
       map.set(colTag.localName().impl(), createTableColWrapper);
       map.set(delTag.localName().impl(), createModWrapper);
       map.set(dirTag.localName().impl(), createDirectoryWrapper);
       map.set(divTag.localName().impl(), createDivWrapper);
       map.set(dlTag.localName().impl(), createDListWrapper);
       map.set(embedTag.localName().impl(), createEmbedWrapper);
       map.set(fieldsetTag.localName().impl(), createFieldSetWrapper);
       map.set(fontTag.localName().impl(), createFontWrapper);
       map.set(formTag.localName().impl(), createFormWrapper);
       map.set(frameTag.localName().impl(), createFrameWrapper);
       map.set(framesetTag.localName().impl(), createFrameSetWrapper);
       map.set(h1Tag.localName().impl(), createHeadingWrapper);
       map.set(headTag.localName().impl(), createHeadWrapper);
       map.set(hrTag.localName().impl(), createHRWrapper);
       map.set(htmlTag.localName().impl(), createHtmlWrapper);
       map.set(iframeTag.localName().impl(), createIFrameWrapper);
       map.set(imgTag.localName().impl(), createImageWrapper);
       map.set(inputTag.localName().impl(), createInputWrapper);
       map.set(isindexTag.localName().impl(), createIsIndexWrapper);
       map.set(labelTag.localName().impl(), createLabelWrapper);
       map.set(legendTag.localName().impl(), createLegendWrapper);
       map.set(liTag.localName().impl(), createLIWrapper);
       map.set(linkTag.localName().impl(), createLinkWrapper);
       map.set(mapTag.localName().impl(), createMapWrapper);
       map.set(marqueeTag.localName().impl(), createMarqueeWrapper);
       map.set(menuTag.localName().impl(), createMenuWrapper);
       map.set(metaTag.localName().impl(), createMetaWrapper);
       map.set(objectTag.localName().impl(), createObjectWrapper);
       map.set(olTag.localName().impl(), createOListWrapper);
       map.set(optgroupTag.localName().impl(), createOptGroupWrapper);
       map.set(optionTag.localName().impl(), createOptionWrapper);
       map.set(pTag.localName().impl(), createParagraphWrapper);
       map.set(paramTag.localName().impl(), createParamWrapper);
       map.set(preTag.localName().impl(), createPreWrapper);
       map.set(qTag.localName().impl(), createQuoteWrapper);
       map.set(scriptTag.localName().impl(), createScriptWrapper);
       map.set(selectTag.localName().impl(), createSelectWrapper);
       map.set(styleTag.localName().impl(), createStyleWrapper);
       map.set(tableTag.localName().impl(), createTableWrapper);
       map.set(tbodyTag.localName().impl(), createTableSectionWrapper);
       map.set(tdTag.localName().impl(), createTableCellWrapper);
       map.set(textareaTag.localName().impl(), createTextAreaWrapper);
       map.set(titleTag.localName().impl(), createTitleWrapper);
       map.set(trTag.localName().impl(), createTableRowWrapper);
       map.set(ulTag.localName().impl(), createUListWrapper);
       map.set(colgroupTag.localName().impl(), createTableColWrapper);
       map.set(h2Tag.localName().impl(), createHeadingWrapper);
       map.set(h3Tag.localName().impl(), createHeadingWrapper);
       map.set(h4Tag.localName().impl(), createHeadingWrapper);
       map.set(h5Tag.localName().impl(), createHeadingWrapper);
       map.set(h6Tag.localName().impl(), createHeadingWrapper);
       map.set(imageTag.localName().impl(), createImageWrapper);
       map.set(insTag.localName().impl(), createModWrapper);
       map.set(keygenTag.localName().impl(), createKeygenWrapper);
       map.set(listingTag.localName().impl(), createPreWrapper);
       map.set(tfootTag.localName().impl(), createTableSectionWrapper);
       map.set(thTag.localName().impl(), createTableCellWrapper);
       map.set(theadTag.localName().impl(), createTableSectionWrapper);
       map.set(xmpTag.localName().impl(), createPreWrapper);
    }

    CreateHTMLElementWrapperFunction createWrapperFunction =
        map.get(element->localName().impl());
    if (createWrapperFunction)
        return createWrapperFunction(element);
    return wrapHTMLElement(element.get());
}

}
