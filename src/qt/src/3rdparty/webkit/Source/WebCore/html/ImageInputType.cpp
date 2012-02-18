/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#include "FormDataList.h"
#include "HTMLFormElement.h"
#include "HTMLImageLoader.h"
#include "HTMLInputElement.h"
#include "MouseEvent.h"
#include "RenderImage.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

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
    encoding.appendData(name.isEmpty() ? "x" : (name + ".x"), m_clickLocation.x());
    encoding.appendData(name.isEmpty() ? "y" : (name + ".y"), m_clickLocation.y());
    if (!name.isEmpty() && !element()->value().isEmpty())
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
    if (element->disabled() || !element->form())
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
    if (!m_imageLoader)
        m_imageLoader = adoptPtr(new HTMLImageLoader(element()));
    m_imageLoader->updateFromElementIgnoringPreviousError();
}

void ImageInputType::attach()
{
    BaseButtonInputType::attach();

    if (!m_imageLoader)
        m_imageLoader = adoptPtr(new HTMLImageLoader(element()));
    m_imageLoader->updateFromElement();

    RenderImage* renderer = toRenderImage(element()->renderer());
    if (!renderer)
        return;

    if (!m_imageLoader->haveFiredBeforeLoadEvent())
        return;

    RenderImageResource* imageResource = renderer->imageResource();
    imageResource->setCachedImage(m_imageLoader->image()); 

    // If we have no image at all because we have no src attribute, set
    // image height and width for the alt text instead.
    if (!m_imageLoader->image() && !imageResource->cachedImage())
        renderer->setImageSizeForAltText();
}

void ImageInputType::willMoveToNewOwnerDocument()
{
    BaseButtonInputType::willMoveToNewOwnerDocument();
    if (m_imageLoader)
        m_imageLoader->elementWillMoveToNewOwnerDocument();
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

} // namespace WebCore
