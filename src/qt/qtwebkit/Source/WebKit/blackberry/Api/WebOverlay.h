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

#ifndef WebOverlay_h
#define WebOverlay_h

#include "BlackBerryGlobal.h"
#include "WebOverlayOverride.h"

#include <BlackBerryPlatformPrimitives.h>

namespace WebCore {
class GraphicsLayerClient;
}

namespace BlackBerry {
namespace Platform {
class String;
}

namespace WebKit {

class WebAnimation;
class WebOverlayClient;
class WebOverlayOverride;
class WebOverlayPrivate;
class WebPage;

/**
 * Represents an overlay that is rendered superimposed on a web page.
 *
 * The WebOverlay is not thread safe, but it is reentrant when used on either
 * the WebKit or the compositing thread. This means that overlays can be
 * on either of these threads, but each instance must only be used on the
 * thread where it was created. The only exception is the override mechanism.
 *
 * Note that WebKit thread usage of WebOverlay is obsolete and will be removed
 * soon.
 *
 * The WebOverlayOverride object returned by WebOverlay::override() can be used
 * to override the values of specific properties from the UI thread.
 *
 * They have the following semantics: If they are called for a specific overlay
 * on the UI thread, the value set will override any value set on the WK thread
 * until you call resetOverrides(). resetOverrides() is thread safe.
 */
class BLACKBERRY_EXPORT WebOverlay {
public:
    enum ImageDataAdoptionType {
        ReferenceImageData,
        CopyImageData
    };

    WebOverlay();
    WebOverlay(WebCore::GraphicsLayerClient*);
    virtual ~WebOverlay();

    // The position of the layer (the location of its top-left corner in its parent).
    Platform::FloatPoint position() const;
    void setPosition(const Platform::FloatPoint&);

    // Anchor point: (0, 0) is top left, (1, 1) is bottom right. The anchor point
    // affects the origin of the transforms.
    Platform::FloatPoint anchorPoint() const;
    void setAnchorPoint(const Platform::FloatPoint&);

    // The size of the layer.
    Platform::FloatSize size() const;
    void setSize(const Platform::FloatSize&);

    // Whether the layer is scaled together with the web page.
    bool sizeIsScaleInvariant() const;
    void setSizeIsScaleInvariant(bool);

    // Transform can be used to rotate the layer, among other things.
    Platform::TransformationMatrix transform() const;
    void setTransform(const Platform::TransformationMatrix&);

    // Opacity. Can also be used to temporarily hide a layer.
    float opacity() const;
    void setOpacity(float);

    // Adds/removes an animation
    // Note that WebAnimation and BlackBerry::Platform::String are not thread safe and have to be
    // created on the thread where they'll be used.
    void addAnimation(const WebAnimation&);
    void removeAnimation(const BlackBerry::Platform::String& name);

    // Returns the rectangle occupied by this overlay, in pixels, relative to the current viewport
    // Can be used for hit testing (noting though, that the overlay may have transparent pixels that
    // cause it not to occupy the whole rectangle, for example when the overlay draws an image with
    // alpha channel). Not implemented for WebKit-thread overlays.
    Platform::IntRect pixelViewportRect() const;

    WebOverlay* parent() const;
    bool addChild(WebOverlay*);
    void removeFromParent();

    void setContentsToImage(const unsigned char* data, const Platform::IntSize& imageSize, ImageDataAdoptionType = ReferenceImageData);
    void setContentsToColor(int r, int g, int b, int a);
    void setDrawsContent(bool);

    // Will result in a future call to WebOverlayClient::drawContents, if the layer draws custom contents.
    void invalidate();

    // The client can be used to draw layer contents.
    void setClient(WebOverlayClient*);

    // Must be called on UI thread.
    WebOverlayOverride* override();

    /**
     * Thread safe. Next time the attributes are changed on the WK thread, make
     * those values override any set on the UI thread.
     */
    void resetOverrides();

private:
    friend class WebPage;
    friend class WebOverlayPrivate;

    // Disable copy constructor and operator=.
    WebOverlay(const WebOverlay&);
    WebOverlay& operator=(const WebOverlay&);

    WebOverlayPrivate* d;
};

}
}

#endif // WebOverlay_h
