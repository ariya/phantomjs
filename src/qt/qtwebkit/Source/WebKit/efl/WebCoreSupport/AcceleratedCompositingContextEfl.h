/*
    Copyright (C) 2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef AcceleratedCompositingContextEfl_h
#define AcceleratedCompositingContextEfl_h

#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER_GL)

#include "TextureMapperFPSCounter.h"
#include "ewk_private.h"

namespace WebCore {

class HostWindow;
class TextureMapper;
class TextureMapperLayer;

class AcceleratedCompositingContext {
    WTF_MAKE_NONCOPYABLE(AcceleratedCompositingContext);
public:
    static PassOwnPtr<AcceleratedCompositingContext> create(HostWindow*);
    virtual ~AcceleratedCompositingContext();

    virtual void syncLayersNow();
    virtual void renderLayers();
    virtual void attachRootGraphicsLayer(GraphicsLayer* rootLayer);
    virtual GraphicsContext3D* context();

private:
    AcceleratedCompositingContext();

    virtual bool initialize(HostWindow*);

    Evas_Object* m_view;

    OwnPtr<TextureMapper> m_textureMapper;
    OwnPtr<GraphicsLayer> m_rootGraphicsLayer;
    TextureMapperLayer* m_rootTextureMapperLayer;

    RefPtr<GraphicsContext3D> m_context3D;
    TextureMapperFPSCounter m_fpsCounter;
};

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER_GL)
#endif // AcceleratedCompositingContextEfl_h
