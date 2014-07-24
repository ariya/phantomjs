/*
 * Copyright (C) 2010 codeaurora.org All rights reserved.
 * Copyright (C) 2011 Collabora Ltd.
 * Copyright (C) 2011 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TileCairo.h"

#if USE(TILED_BACKING_STORE) && USE(CAIRO)
#include "CairoUtilities.h"
#include "GraphicsContext.h"
#include "PlatformContextCairo.h"
#include "TiledBackingStore.h"
#include "TiledBackingStoreClient.h"
#include <RefPtrCairo.h>

namespace WebCore {

TileCairo::TileCairo(TiledBackingStore* backingStore, const Coordinate& tileCoordinate)
    : m_backingStore(backingStore)
    , m_coordinate(tileCoordinate)
    , m_rect(m_backingStore->tileRectForCoordinate(tileCoordinate))
{
    cairo_rectangle_int_t rect = m_rect;
    m_dirtyRegion = adoptRef(cairo_region_create_rectangle(&rect));
}

TileCairo::~TileCairo()
{
}

bool TileCairo::isDirty() const
{
    return !cairo_region_is_empty(m_dirtyRegion.get());
}

bool TileCairo::isReadyToPaint() const
{
    return m_buffer;
}

void TileCairo::invalidate(const IntRect& dirtyRect)
{
    IntRect tileDirtyRect = intersection(dirtyRect, m_rect);
    if (tileDirtyRect.isEmpty())
        return;

    cairo_rectangle_int_t rect = tileDirtyRect;
    cairo_region_union_rectangle(m_dirtyRegion.get(), &rect);
}

Vector<IntRect> TileCairo::updateBackBuffer()
{
    if (m_buffer && !isDirty())
        return Vector<IntRect>();

    if (!m_buffer)
        m_buffer = adoptRef(cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                                       m_backingStore->tileSize().width(),
                                                       m_backingStore->tileSize().height()));

    RefPtr<cairo_t> cr = adoptRef(cairo_create(m_buffer.get()));
    GraphicsContext context(cr.get());
    context.translate(-m_rect.x(), -m_rect.y());

    Vector<IntRect> updateRects;
    cairo_rectangle_int_t rect;

    int rectCount = cairo_region_num_rectangles(m_dirtyRegion.get());
    for (int i = 0; i < rectCount; ++i) {
        cairo_region_get_rectangle(m_dirtyRegion.get(), i, &rect);
        updateRects.append(IntRect(rect));

        context.save();
        context.clip(FloatRect(rect));
        context.scale(FloatSize(m_backingStore->contentsScale(), m_backingStore->contentsScale()));
        m_backingStore->client()->tiledBackingStorePaint(&context, m_backingStore->mapToContents(rect));
        context.restore();
    }

    m_dirtyRegion.clear();
    m_dirtyRegion = adoptRef(cairo_region_create());

    return updateRects;
}

void TileCairo::swapBackBufferToFront()
{
}

void TileCairo::paint(GraphicsContext* context, const IntRect& rect)
{
    if (!m_buffer)
        return;

    IntRect target = intersection(rect, m_rect);

    cairo_t* cr = context->platformContext()->cr();
    cairo_set_source_surface(cr, m_buffer.get(), m_rect.x(), m_rect.y());
    cairo_rectangle(cr, target.x(), target.y(), target.width(), target.height());
    cairo_fill(cr);
}

void TileCairo::resize(const WebCore::IntSize& newSize)
{
    IntRect oldRect = m_rect;
    m_rect = IntRect(m_rect.location(), newSize);
    if (m_rect.maxX() > oldRect.maxX())
        invalidate(IntRect(oldRect.maxX(), oldRect.y(), m_rect.maxX() - oldRect.maxX(), m_rect.height()));
    if (m_rect.maxY() > oldRect.maxY())
        invalidate(IntRect(oldRect.x(), oldRect.maxY(), m_rect.width(), m_rect.maxY() - oldRect.maxY()));
}

}

#endif
