/*
 * Copyright (C) 2012, 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef SelectionOverlay_h
#define SelectionOverlay_h

#include "BlackBerryGlobal.h"

#if USE(ACCELERATED_COMPOSITING)

#include "Color.h"
#include "FloatQuad.h"
#include "GraphicsLayerClient.h"
#include "IntRect.h"

#include <BlackBerryPlatformIntRectRegion.h>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

namespace BlackBerry {
namespace WebKit {

class WebPagePrivate;

class SelectionOverlay : public WebCore::GraphicsLayerClient {
public:
    typedef HashMap<WebCore::GraphicsLayer*, Vector<WebCore::FloatQuad> > Selection;

    static PassOwnPtr<SelectionOverlay> create(WebPagePrivate* page)
    {
        return adoptPtr(new SelectionOverlay(page));
    }

    ~SelectionOverlay();

    void draw(const Selection&);
    void hide();

    // GraphicsLayerClient
    virtual void notifyAnimationStarted(const WebCore::GraphicsLayer*, double) { }
    virtual void notifyFlushRequired(const WebCore::GraphicsLayer*);
    virtual void paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext&, WebCore::GraphicsLayerPaintingPhase, const WebCore::IntRect&);

private:
    SelectionOverlay(WebPagePrivate*);

    WebPagePrivate* m_page;
    Selection m_selection;
    Vector<OwnPtr<WebCore::GraphicsLayer> > m_layers;
};

} // namespace WebKit
} // namespace BlackBerry

#endif // USE(ACCELERATED_COMPOSITING)

#endif // SelectionOverlay_h
