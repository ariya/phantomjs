/*
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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

#if ENABLE(FILTERS)
#include "SourceAlpha.h"

#include "Color.h"
#include "Filter.h"
#include "GraphicsContext.h"
#include "PlatformString.h"
#include "RenderTreeAsText.h"
#include "TextStream.h"

#include <wtf/StdLibExtras.h>

namespace WebCore {

PassRefPtr<SourceAlpha> SourceAlpha::create(Filter* filter)
{
    return adoptRef(new SourceAlpha(filter));
}

const AtomicString& SourceAlpha::effectName()
{
    DEFINE_STATIC_LOCAL(const AtomicString, s_effectName, ("SourceAlpha"));
    return s_effectName;
}

void SourceAlpha::determineAbsolutePaintRect()
{
    Filter* filter = this->filter();
    FloatRect paintRect = filter->sourceImageRect();
    paintRect.scale(filter->filterResolution().width(), filter->filterResolution().height());
    setAbsolutePaintRect(enclosingIntRect(paintRect));
}

void SourceAlpha::apply()
{
    if (hasResult())
        return;
    ImageBuffer* resultImage = createImageBufferResult();
    Filter* filter = this->filter();
    if (!resultImage || !filter->sourceImage())
        return;

    setIsAlphaImage(true);

    FloatRect imageRect(FloatPoint(), absolutePaintRect().size());
    GraphicsContext* filterContext = resultImage->context();
    GraphicsContextStateSaver stateSaver(*filterContext);
    filterContext->clipToImageBuffer(filter->sourceImage(), imageRect);
    filterContext->fillRect(imageRect, Color::black, ColorSpaceDeviceRGB);
}

void SourceAlpha::dump()
{
}

TextStream& SourceAlpha::externalRepresentation(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    ts << "[SourceAlpha]\n";
    return ts;
}

} // namespace WebCore

#endif // ENABLE(FILTERS)
