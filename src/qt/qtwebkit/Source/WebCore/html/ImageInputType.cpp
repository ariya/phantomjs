/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "ImageInputType.h"

#include "CachedImage.h"
#include "FormDataList.h"
#include "HTMLFormElement.h"
#include "HTMLImageLoader.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "InputTypeNames.h"
#include "MouseEvent.h"
#include "RenderImage.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

using namespace HTMLNames;

inline ImageInputType::ImageInputType(HTMLInputElement* element)
    : BaseButtonInputType(element)
{
}

PassOwnPtr<InputType> ImageInputType::create(HTMLInputElement* element)
{
    return adoptPtr(new ImageInputType(element));
}

const AtomicString& ImageInputType::formControlType() const
{
    return InputTypeNames::image();
}

bool ImageInputType::isFormDataAppendable() const
{
    return true;
}

bool ImageInputType::appendFormData(FormDataList& encoding, bool) const
{
    if (!element()->isActivatedSubmit())
        return false;
    const AtomicString& name = element()->name();
    if (name.isEmpty()) {
        encoding.appendData("x", m_clickLocation.x());
        encoding.appendData("y", m_clickLocation.y());
        return true;
    }

    DEFINE_STATIC_LOCAL(String, dotXString, (ASCIILiteral(".x")));
    DEFINE_STATIC_LOCAL(String, dotYString, (ASCIILiteral(".y")));
    encoding.appendData(name + dotXString, m_clickLocation.x());
    encoding.appendData(name + dotYString, m_clickLocation.y());

    if (!element()->value().isEmpty())
        encoding.appendData(name, element()->value());
    return true;
}

bool ImageInputType::supportsValidation() const
{
    return false;
}

void ImageInputType::handleDOMActivateEvent(Event* event)
{
    RefPtr<HTMLInputElement> element = this->element();
    if (element->isDisabledFormControl() || !element->form())
        return;
    element->setActivatedSubmit(true);
    if (event->underlyingEvent() && event->underlyingEvent()->isMouseEvent()) {
        MouseEvent* mouseEvent = static_cast<MouseEvent*>(event->underlyingEvent());
        m_clickLocation = IntPoint(mouseEvent->offsetX(), mouseEvent->offsetY());
    } else
        m_clickLocation = IntPoint();
    element->form()->prepareForSubmission(event); // Event handlers can run.
    element->setActivatedSubmit(false);
    event->setDefaultHandled();
}

RenderObject* ImageInputType::createRenderer(RenderArena* arena, RenderStyle*) const
{
    RenderImage* image = new (arena) RenderImage(element());
    image->setImageResource(RenderImageResource::create());
    return image;
}

void ImageInputType::altAttributeChanged()
{
    RenderImage* image = toRenderImage(element()->renderer());
    if (!image)
        return;
    image->updateAltText();
}

void ImageInputType::srcAttributeChanged()
{
    if (!element()->renderer())
        return;
    element()->imageLoader()->updateFromElementIgnoringPreviousError();
}

void ImageInputType::attach()
{
    BaseButtonInputType::attach();

    HTMLImageLoader* imageLoader = element()->imageLoader();
    imageLoader->updateFromElement();

    RenderImage* renderer = toRenderImage(element()->renderer());
    if (!renderer)
        return;

    if (imageLoader->hasPendingBeforeLoadEvent())
        return;

    RenderImageResource* imageResource = renderer->imageResource();
    imageResource->setCachedImage(imageLoader->image()); 

    // If we have no image at all because we have no src attribute, set
    // image height and width for the alt text instead.
    if (!imageLoader->image() && !imageResource->cachedImage())
        renderer->setImageSizeForAltText();
}

bool ImageInputType::shouldRespectAlignAttribute()
{
    return true;
}

bool ImageInputType::canBeSuccessfulSubmitButton()
{
    return true;
}

bool ImageInputType::isImageButton() const
{
    return true;
}

bool ImageInputType::isEnumeratable()
{
    return false;
}

bool ImageInputType::shouldRespectHeightAndWidthAttributes()
{
    return true;
}

unsigned ImageInputType::height() const
{
    RefPtr<HTMLInputElement> element = this->element();

    if (!element->renderer()) {
        // Check the attribute first for an explicit pixel value.
        unsigned height;
        if (parseHTMLNonNegativeInteger(element->fastGetAttribute(heightAttr), height))
            return height;

        // If the image is available, use its height.
        if (element->hasImageLoader()) {
            HTMLImageLoader* imageLoader = element->imageLoader();
            if (imageLoader->image())
                return imageLoader->image()->imageSizeForRenderer(element->renderer(), 1).height();
        }
    }

    element->document()->updateLayout();

    RenderBox* box = element->renderBox();
    return box ? adjustForAbsoluteZoom(box->contentHeight(), box) : 0;
}

unsigned ImageInputType::width() const
{
    RefPtr<HTMLInputElement> element = this->element();

    if (!element->renderer()) {
        // Check the attribute first for an explicit pixel value.
        unsigned width;
        if (parseHTMLNonNegativeInteger(element->fastGetAttribute(widthAttr), width))
            return width;

        // If the image is available, use its width.
        if (element->hasImageLoader()) {
            HTMLImageLoader* imageLoader = element->imageLoader();
            if (imageLoader->image())
                return imageLoader->image()->imageSizeForRenderer(element->renderer(), 1).width();
        }
    }

    element->document()->updateLayout();

    RenderBox* box = element->renderBox();
    return box ? adjustForAbsoluteZoom(box->contentWidth(), box) : 0;
}

} // namespace WebCore
