/*
 * Copyright (C) 2013 Research In Motion Limited. All rights reserved.
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

#ifndef LayerRendererClient_h
#define LayerRendererClient_h

#if USE(ACCELERATED_COMPOSITING)

namespace BlackBerry {
namespace Platform {
namespace Graphics {
class GLES2Context;
}
}
}

namespace WebCore {

class LayerRendererClient {
public:
    virtual ~LayerRendererClient() { }

    virtual void layerRendererDestroyed() { }

    virtual BlackBerry::Platform::Graphics::GLES2Context* context() const = 0;

    virtual bool shouldChildWindowsUseDocumentCoordinates() = 0;
};

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif
