/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 Rob Buis <buis@kde.org>
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
#include "SVGFitToViewBox.h"

#include "AffineTransform.h"
#include "Attribute.h"
#include "Document.h"
#include "FloatRect.h"
#include "SVGDocumentExtensions.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"
#include "SVGPreserveAspectRatio.h"
#include <wtf/text/StringImpl.h>

namespace WebCore {

bool SVGFitToViewBox::parseViewBox(Document* doc, const String& s, FloatRect& viewBox)
{
    const UChar* c = s.characters();
    const UChar* end = c + s.length();
    return parseViewBox(doc, c, end, viewBox, true);
}

bool SVGFitToViewBox::parseViewBox(Document* doc, const UChar*& c, const UChar* end, FloatRect& viewBox, bool validate)
{
    String str(c, end - c);

    skipOptionalSVGSpaces(c, end);

    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    bool valid = parseNumber(c, end, x) && parseNumber(c, end, y) && parseNumber(c, end, width) && parseNumber(c, end, height, false);
    if (!validate) {
        viewBox = FloatRect(x, y, width, height);
        return true;
    }
    if (!valid) {
        doc->accessSVGExtensions()->reportWarning("Problem parsing viewBox=\"" + str + "\"");
        return false;
    }

    if (width < 0.0) { // check that width is positive
        doc->accessSVGExtensions()->reportError("A negative value for ViewBox width is not allowed");
        return false;
    }
    if (height < 0.0) { // check that height is positive
        doc->accessSVGExtensions()->reportError("A negative value for ViewBox height is not allowed");
        return false;
    }
    skipOptionalSVGSpaces(c, end);
    if (c < end) { // nothing should come after the last, fourth number
        doc->accessSVGExtensions()->reportWarning("Problem parsing viewBox=\"" + str + "\"");
        return false;
    }

    viewBox = FloatRect(x, y, width, height);
    return true;
}

AffineTransform SVGFitToViewBox::viewBoxToViewTransform(const FloatRect& viewBoxRect, const SVGPreserveAspectRatio& preserveAspectRatio, float viewWidth, float viewHeight)
{
    if (!viewBoxRect.width() || !viewBoxRect.height())
        return AffineTransform();

    return preserveAspectRatio.getCTM(viewBoxRect.x(), viewBoxRect.y(), viewBoxRect.width(), viewBoxRect.height(), viewWidth, viewHeight);
}

bool SVGFitToViewBox::isKnownAttribute(const QualifiedName& attrName)
{
    return attrName == SVGNames::viewBoxAttr || attrName == SVGNames::preserveAspectRatioAttr;
}

void SVGFitToViewBox::addSupportedAttributes(HashSet<QualifiedName>& supportedAttributes)
{
    supportedAttributes.add(SVGNames::viewBoxAttr);
    supportedAttributes.add(SVGNames::preserveAspectRatioAttr);
}

}

#endif // ENABLE(SVG)
