/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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

#import "config.h"
#import "WebTiledBackingLayer.h"

#import "IntRect.h"
#import "TileController.h"
#import <wtf/MainThread.h>

using namespace WebCore;

@implementation WebTiledBackingLayer

- (id)init
{
    self = [super init];
    if (!self)
        return nil;

    _tileController = TileController::create(self);
#ifndef NDEBUG
    [self setName:@"WebTiledBackingLayer"];
#endif
    return self;
}

- (void)dealloc
{
    ASSERT(!_tileController);

    [super dealloc];
}

- (id)initWithLayer:(id)layer
{
    UNUSED_PARAM(layer);

    ASSERT_NOT_REACHED();
    return nil;
}

- (id<CAAction>)actionForKey:(NSString *)key
{
    UNUSED_PARAM(key);
    
    // Disable all animations.
    return nil;
}

- (void)setBounds:(CGRect)bounds
{
    [super setBounds:bounds];

    _tileController->tileCacheLayerBoundsChanged();
}

- (void)setOpaque:(BOOL)opaque
{
    _tileController->setTilesOpaque(opaque);
}

- (BOOL)isOpaque
{
    return _tileController->tilesAreOpaque();
}

- (void)setNeedsDisplay
{
    _tileController->setNeedsDisplay();
}

- (void)setNeedsDisplayInRect:(CGRect)rect
{
    _tileController->setNeedsDisplayInRect(enclosingIntRect(rect));
}

- (void)setAcceleratesDrawing:(BOOL)acceleratesDrawing
{
    _tileController->setAcceleratesDrawing(acceleratesDrawing);
}

- (BOOL)acceleratesDrawing
{
    return _tileController->acceleratesDrawing();
}

- (void)setContentsScale:(CGFloat)contentsScale
{
    _tileController->setScale(contentsScale);
}

- (CALayer *)tileContainerLayer
{
    return _tileController->tileContainerLayer();
}

- (WebCore::TiledBacking*)tiledBacking
{
    return _tileController.get();
}

- (void)invalidate
{
    ASSERT(isMainThread());
    ASSERT(_tileController);
    _tileController = nullptr;
}

- (void)setBorderColor:(CGColorRef)borderColor
{
    _tileController->setTileDebugBorderColor(borderColor);
}

- (void)setBorderWidth:(CGFloat)borderWidth
{
    // Tiles adjoin, so halve the border width.
    _tileController->setTileDebugBorderWidth(borderWidth / 2);
}

@end
