/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies)
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
#ifndef TextureMapperLayerClientQt_h
#define TextureMapperLayerClientQt_h

class QWebFrameAdapter;

#include "GraphicsLayer.h"
#include "TextureMapper.h"
#include "TextureMapperFPSCounter.h"
#include "Timer.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

class TextureMapperLayer;

#if USE(ACCELERATED_COMPOSITING)
class TextureMapperLayerClientQt {
public:
    TextureMapperLayerClientQt(QWebFrameAdapter*);
    ~TextureMapperLayerClientQt();
    void syncRootLayer();
    TextureMapperLayer* rootLayer();

    void markForSync(bool scheduleSync);

    void setRootGraphicsLayer(GraphicsLayer*);

    void syncLayers(Timer<TextureMapperLayerClientQt>*);

    void renderCompositedLayers(GraphicsContext*, const IntRect& clip);
private:
    QWebFrameAdapter* m_frame;
    OwnPtr<GraphicsLayer> m_rootGraphicsLayer;
    Timer<TextureMapperLayerClientQt> m_syncTimer;
    WebCore::TextureMapperLayer* m_rootTextureMapperLayer;
    OwnPtr<WebCore::TextureMapper> m_textureMapper;
    WebCore::TextureMapperFPSCounter m_fpsCounter;
};
#endif

}

#endif
