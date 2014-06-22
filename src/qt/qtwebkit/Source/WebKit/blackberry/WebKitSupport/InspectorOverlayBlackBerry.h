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

#ifndef InspectorOverlayBlackBerry_h
#define InspectorOverlayBlackBerry_h

#include "WebOverlay.h"
#include <GraphicsLayerClient.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {
class GraphicsContext;
class GraphicsLayer;
}

namespace BlackBerry {
namespace WebKit {
class WebPagePrivate;

class InspectorOverlay : public WebCore::GraphicsLayerClient {
public:
    class InspectorOverlayClient {
    public:
        virtual void paintInspectorOverlay(WebCore::GraphicsContext&) = 0;
    };

    static PassOwnPtr<InspectorOverlay> create(WebPagePrivate*, InspectorOverlayClient*);

    ~InspectorOverlay();

    void setClient(InspectorOverlayClient* client) { m_client = client; }

    void clear();
    void update();
    void paintWebFrame(WebCore::GraphicsContext&);

#if USE(ACCELERATED_COMPOSITING)
    virtual void notifyAnimationStarted(const WebCore::GraphicsLayer*, double) { }
    virtual void notifyFlushRequired(const WebCore::GraphicsLayer*);
    virtual void paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext&, WebCore::GraphicsLayerPaintingPhase, const WebCore::IntRect&);
#endif

private:
    InspectorOverlay(WebPagePrivate*, InspectorOverlayClient*);
    void invalidateWebFrame();

    WebPagePrivate* m_webPage;
    InspectorOverlayClient* m_client;
    OwnPtr<WebOverlay> m_overlay;
};

} // namespace WebKit
} // namespace BlackBerry

#endif /* InspectorOverlayBlackBerry_h */
