/*
 * Copyright (C) 2005, 2005 Alexander Kellett <lypanov@kde.org>
 * Copyright (C) 2008 Rob Buis <buis@kde.org>
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
 */

#include "config.h"

#if ENABLE(SVG)
#include "SVGImageLoader.h"

#include "Event.h"
#include "EventNames.h"
#include "HTMLParserIdioms.h"
#include "RenderImage.h"
#include "SVGImageElement.h"

namespace WebCore {

SVGImageLoader::SVGImageLoader(SVGImageElement* node)
    : ImageLoader(node)
{
}

void SVGImageLoader::dispatchLoadEvent()
{
    if (image()->errorOccurred())
        element()->dispatchEvent(Event::create(eventNames().errorEvent, false, false));
    else {
        SVGImageElement* imageElement = static_cast<SVGImageElement*>(element());
        if (imageElement->externalResourcesRequiredBaseValue())
            imageElement->sendSVGLoadEventIfPossible(true);
    }
}

String SVGImageLoader::sourceURI(const AtomicString& attr) const
{
    KURL base = element()->baseURI();
    if (base.isValid())
        return KURL(base, stripLeadingAndTrailingHTMLSpaces(attr)).string();
    return element()->document()->completeURL(stripLeadingAndTrailingHTMLSpaces(attr));
}

}

#endif // ENABLE(SVG)
