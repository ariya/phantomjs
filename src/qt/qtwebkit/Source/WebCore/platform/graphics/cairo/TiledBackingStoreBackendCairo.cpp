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
#include "TiledBackingStoreBackend.h"

#if USE(TILED_BACKING_STORE) && USE(CAIRO)
#include "CairoUtilities.h"
#include "PlatformContextCairo.h"
#include "TileCairo.h"
#include <cairo.h>

namespace WebCore {

PassRefPtr<Tile> TiledBackingStoreBackend::createTile(TiledBackingStore* backingStore, const Tile::Coordinate& tileCoordinate)
{
    return TileCairo::create(backingStore, tileCoordinate);
}

static const unsigned checkerSize = 16;
static const unsigned checkerColorDarkGrey = 0xff555555;
static const unsigned checkerColorLightGrey = 0xffaaaaaa;

static cairo_surface_t* checkeredSurface()
{
    static cairo_surface_t* surface = 0;
    if (surface)
        return surface;

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, checkerSize, checkerSize);
    RefPtr<cairo_t> cr = adoptRef(cairo_create(surface));

    unsigned halfCheckerSize = checkerSize / 2;
    setSourceRGBAFromColor(cr.get(), checkerColorLightGrey);
    cairo_rectangle(cr.get(), 0, 0, halfCheckerSize, halfCheckerSize);
    cairo_rectangle(cr.get(), halfCheckerSize, halfCheckerSize, halfCheckerSize, halfCheckerSize);
    cairo_fill(cr.get());

    setSourceRGBAFromColor(cr.get(), checkerColorDarkGrey);
    cairo_rectangle(cr.get(), halfCheckerSize, 0, halfCheckerSize, halfCheckerSize);
    cairo_rectangle(cr.get(), 0, halfCheckerSize, halfCheckerSize, halfCheckerSize);
    cairo_fill(cr.get());

    return surface;
}

void TiledBackingStoreBackend::paintCheckerPattern(GraphicsContext* context, const FloatRect& target)
{
    PlatformContextCairo* platformContext = context->platformContext();
    cairo_t* cr = platformContext->cr();
    cairo_set_source_surface(cr, checkeredSurface(), 0, 0);
    cairo_pattern_set_extend(cairo_get_source(cr), CAIRO_EXTEND_REPEAT);
    cairo_rectangle(cr, target.x(), target.y(), target.width(), target.height());
    cairo_fill(cr);
}

}

#endif

