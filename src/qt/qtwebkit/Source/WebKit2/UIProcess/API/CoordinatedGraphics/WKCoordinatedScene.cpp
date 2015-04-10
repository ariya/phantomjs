/*
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#if USE(COORDINATED_GRAPHICS)

#include "WKCoordinatedScene.h"

#include "WKCoordinatedSceneAPICast.h"

#include <WebCore/CoordinatedGraphicsScene.h>
#include <WebCore/TextureMapperLayer.h>

WK_EXPORT WKCoordinatedSceneLayer WKCoordinatedSceneFindScrollableContentsLayerAt(WKCoordinatedScene scene, WKPoint point)
{
    return toAPI(toImpl(scene)->findScrollableContentsLayerAt(WebCore::FloatPoint(point.x, point.y)));
}

WK_EXPORT uint32_t WKCoordinatedSceneGetLayerID(WKCoordinatedSceneLayer layer)
{
    return toImpl(layer)->id();
}

WK_EXPORT void WKCoordinatedSceneScrollBy(WKCoordinatedSceneLayer layer, WKSize offset)
{
    toImpl(layer)->scrollBy(WebCore::FloatSize(offset.width, offset.height));
}
#endif
