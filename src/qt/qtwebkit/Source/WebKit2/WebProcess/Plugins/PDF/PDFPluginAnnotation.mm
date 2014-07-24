/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#import "PDFPluginAnnotation.h"

#if ENABLE(PDFKIT_PLUGIN)

#import "PDFKitImports.h"
#import "PDFLayerControllerDetails.h"
#import "PDFPlugin.h"
#import "PDFPluginChoiceAnnotation.h"
#import "PDFPluginTextAnnotation.h"
#import <PDFKit/PDFKit.h>
#import <WebCore/CSSPrimitiveValue.h>
#import <WebCore/CSSPropertyNames.h>
#import <WebCore/ColorMac.h>
#import <WebCore/HTMLElement.h>
#import <WebCore/HTMLInputElement.h>
#import <WebCore/HTMLNames.h>
#import <WebCore/HTMLOptionElement.h>
#import <WebCore/HTMLSelectElement.h>
#import <WebCore/HTMLTextAreaElement.h>
#import <WebCore/Page.h>

using namespace WebCore;

namespace WebKit {

using namespace HTMLNames;

PassRefPtr<PDFPluginAnnotation> PDFPluginAnnotation::create(PDFAnnotation *annotation, PDFLayerController *pdfLayerController, PDFPlugin* plugin)
{
    if ([annotation isKindOfClass:pdfAnnotationTextWidgetClass()])
        return PDFPluginTextAnnotation::create(annotation, pdfLayerController, plugin);
    if ([annotation isKindOfClass:pdfAnnotationChoiceWidgetClass()])
        return PDFPluginChoiceAnnotation::create(annotation, pdfLayerController, plugin);

    return 0;
}

void PDFPluginAnnotation::attach(Element* parent)
{
    ASSERT(!m_parent);

    m_parent = parent;
    m_element = createAnnotationElement();

    m_element->setAttribute(classAttr, "annotation");
    m_element->addEventListener(eventNames().changeEvent, m_eventListener, false);
    m_element->addEventListener(eventNames().blurEvent, m_eventListener, false);

    updateGeometry();

    m_parent->appendChild(m_element);

    // FIXME: The text cursor doesn't blink after this. Why?
    m_element->focus();
}

void PDFPluginAnnotation::commit()
{
    m_plugin->didMutatePDFDocument();
}

PDFPluginAnnotation::~PDFPluginAnnotation()
{
    m_element->removeEventListener(eventNames().changeEvent, m_eventListener.get(), false);
    m_element->removeEventListener(eventNames().blurEvent, m_eventListener.get(), false);

    m_eventListener->setAnnotation(0);

    m_parent->removeChild(element());
}

void PDFPluginAnnotation::updateGeometry()
{
    IntSize documentSize(m_pdfLayerController.contentSizeRespectingZoom);
    IntPoint scrollPosition(m_pdfLayerController.scrollPosition);
    NSRect annotationRect = NSRectFromCGRect([m_pdfLayerController boundsForAnnotation:m_annotation.get()]);

    StyledElement* styledElement = static_cast<StyledElement*>(element());
    styledElement->setInlineStyleProperty(CSSPropertyWidth, annotationRect.size.width, CSSPrimitiveValue::CSS_PX);
    styledElement->setInlineStyleProperty(CSSPropertyHeight, annotationRect.size.height, CSSPrimitiveValue::CSS_PX);
    styledElement->setInlineStyleProperty(CSSPropertyLeft, annotationRect.origin.x - scrollPosition.x(), CSSPrimitiveValue::CSS_PX);
    styledElement->setInlineStyleProperty(CSSPropertyTop, documentSize.height() - annotationRect.origin.y - annotationRect.size.height - scrollPosition.y(), CSSPrimitiveValue::CSS_PX);
}

bool PDFPluginAnnotation::handleEvent(Event* event)
{
    if (event->type() == eventNames().blurEvent || event->type() == eventNames().changeEvent) {
        m_plugin->setActiveAnnotation(0);
        return true;
    }

    return false;
}

void PDFPluginAnnotation::PDFPluginAnnotationEventListener::handleEvent(ScriptExecutionContext*, Event* event)
{
    if (m_annotation)
        m_annotation->handleEvent(event);
}

} // namespace WebKit

#endif // ENABLE(PDFKIT_PLUGIN)
