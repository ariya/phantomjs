/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
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

#ifndef SVGDocument_h
#define SVGDocument_h
#if ENABLE(SVG)

#include "Document.h"
#include "FloatPoint.h"

namespace WebCore {

class DOMImplementation;
class SVGElement;
class SVGSVGElement;

class SVGDocument : public Document {
public:
    static PassRefPtr<SVGDocument> create(Frame* frame, const KURL& url)
    {
        return adoptRef(new SVGDocument(frame, url));
    }

    SVGSVGElement* rootElement() const;

    void dispatchZoomEvent(float prevScale, float newScale);
    void dispatchScrollEvent();

    bool zoomAndPanEnabled() const;

    void startPan(const FloatPoint& start);
    void updatePan(const FloatPoint& pos) const;

private:
    SVGDocument(Frame*, const KURL&);

    virtual bool isSVGDocument() const { return true; }

    virtual bool childShouldCreateRenderer(Node*) const;

    FloatPoint m_translate;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGDocument_h

// vim:ts=4:noet
