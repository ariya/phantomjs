/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#import "WebRenderLayer.h"

#import "WebFrameInternal.h"
#import <WebCore/Frame.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/FrameLoaderClient.h>
#import <WebCore/RenderLayer.h>
#import <WebCore/RenderLayerBacking.h>
#import <WebCore/RenderView.h>
#import <WebCore/StyledElement.h>
#import <wtf/text/StringBuilder.h>
#import <wtf/text/WTFString.h>

using namespace WebCore;

@interface WebRenderLayer(Private)

- (id)initWithRenderLayer:(RenderLayer *)layer;
- (void)buildDescendantLayers:(RenderLayer*)rootLayer;

@end

@implementation WebRenderLayer

+ (NSString *)nameForLayer:(RenderLayer*)layer
{
    RenderObject* renderer = layer->renderer();
    NSString *name = [NSString stringWithUTF8String:renderer->renderName()];

    if (Element* element = renderer->node() && renderer->node()->isElementNode() ? toElement(renderer->node()) : 0) {
        name = [name stringByAppendingFormat:@" %@", (NSString *)element->tagName()];
        if (element->hasID())
            name = [name stringByAppendingFormat:@" id=\"%@\"", (NSString *)element->getIdAttribute()];

        if (element->hasClass()) {
            StringBuilder classes;
            for (size_t i = 0; i < element->classNames().size(); ++i) {
                if (i > 0)
                    classes.append(' ');
                classes.append(element->classNames()[i]);
            }
            name = [name stringByAppendingFormat:@" class=\"%@\"", (NSString *)classes.toString()];
        }
    }

    if (layer->isReflection())
        name = [name stringByAppendingString:@" (reflection)"];

    return name;
}

+ (NSString *)compositingInfoForLayer:(RenderLayer*)layer
{
    if (!layer->isComposited())
        return @"";

    NSString *layerType = @"";
#if USE(ACCELERATED_COMPOSITING)
    RenderLayerBacking* backing = layer->backing();
    switch (backing->compositingLayerType()) {
        case NormalCompositingLayer:
            layerType = @"composited";
            break;
        case TiledCompositingLayer:
            layerType = @"composited: tiled layer";
            break;
        case MediaCompositingLayer:
            layerType = @"composited for plug-in, video or WebGL";
            break;
        case ContainerCompositingLayer:
            layerType = @"composited: container layer";
            break;
    }
    
    if (backing->hasClippingLayer())
        layerType = [layerType stringByAppendingString:@" (clipping)"];

    if (backing->hasAncestorClippingLayer())
        layerType = [layerType stringByAppendingString:@" (clipped)"];
#endif

    return layerType;
}

- (id)initWithRenderLayer:(RenderLayer*)layer
{
    if ((self = [super init])) {
        name = [[WebRenderLayer nameForLayer:layer] retain];
        bounds = layer->absoluteBoundingBox();
        composited = layer->isComposited();
        compositingInfo = [[WebRenderLayer compositingInfoForLayer:layer] retain];
    }

    return self;
}

- (id)initWithName:(NSString*)layerName
{
    if ((self = [super init])) {
        name = [layerName copy];
        separator = YES;
    }

    return self;
}

// Only called on the root.
- (id)initWithWebFrame:(WebFrame *)webFrame
{
    self = [super init];
    
    Frame* frame = core(webFrame);
    if (!frame->loader()->client()->hasHTMLView()) {
        [self release];
        return nil;
    }
    
    RenderObject* renderer = frame->contentRenderer();
    if (!renderer) {
        [self release];
        return nil;
    }

    if (renderer->hasLayer()) {
        RenderLayer* layer = toRenderBoxModelObject(renderer)->layer();

        name = [[WebRenderLayer nameForLayer:layer] retain];
        bounds = layer->absoluteBoundingBox();
        composited = layer->isComposited();
        compositingInfo = [[WebRenderLayer compositingInfoForLayer:layer] retain];
    
        [self buildDescendantLayers:layer];
    }
    
    return self;
}

- (void)dealloc
{
    [children release];
    [name release];
    [compositingInfo release];
    [super dealloc];
}

- (void)buildDescendantLayers:(RenderLayer*)layer
{
    NSMutableArray *childWebLayers = [[NSMutableArray alloc] init];

    // Build children in back to front order.
    
    if (Vector<RenderLayer*>* negZOrderList = layer->negZOrderList()) {
        size_t listSize = negZOrderList->size();

        if (listSize) {
            WebRenderLayer* newLayer = [[WebRenderLayer alloc] initWithName:@"-ve z-order list"];
            [childWebLayers addObject:newLayer];
            [newLayer release];
        }

        for (size_t i = 0; i < listSize; ++i) {
            RenderLayer* curLayer = negZOrderList->at(i);

            WebRenderLayer* newLayer = [[WebRenderLayer alloc] initWithRenderLayer:curLayer];
            [newLayer buildDescendantLayers:curLayer];

            [childWebLayers addObject:newLayer];
            [newLayer release];
        }
    }

    if (Vector<RenderLayer*>* normalFlowList = layer->normalFlowList()) {
        size_t listSize = normalFlowList->size();

        if (listSize) {
            WebRenderLayer* newLayer = [[WebRenderLayer alloc] initWithName:@"normal flow list"];
            [childWebLayers addObject:newLayer];
            [newLayer release];
        }
        
        for (size_t i = 0; i < listSize; ++i) {
            RenderLayer* curLayer = normalFlowList->at(i);

            WebRenderLayer* newLayer = [[WebRenderLayer alloc] initWithRenderLayer:curLayer];
            [newLayer buildDescendantLayers:curLayer];

            [childWebLayers addObject:newLayer];
            [newLayer release];
        }
    }

    if (Vector<RenderLayer*>* posZOrderList = layer->posZOrderList()) {
        size_t listSize = posZOrderList->size();

        if (listSize) {
            WebRenderLayer* newLayer = [[WebRenderLayer alloc] initWithName:@"+ve z-order list"];
            [childWebLayers addObject:newLayer];
            [newLayer release];
        }

        for (size_t i = 0; i < listSize; ++i) {
            RenderLayer* curLayer = posZOrderList->at(i);

            WebRenderLayer* newLayer = [[WebRenderLayer alloc] initWithRenderLayer:curLayer];
            [newLayer buildDescendantLayers:curLayer];

            [childWebLayers addObject:newLayer];
            [newLayer release];
        }
    }

    children = childWebLayers;
}

- (NSArray *)children
{
    return children;
}

- (NSString *)name
{
    return name;
}

- (NSString *)positionString
{
    return [NSString stringWithFormat:@"(%.0f, %.0f)", bounds.origin.x, bounds.origin.y];
}

- (NSString *)widthString
{
    return [NSString stringWithFormat:@"%.0f", bounds.size.width];
}

- (NSString *)heightString
{
    return [NSString stringWithFormat:@"%.0f", bounds.size.height];
}

- (NSString *)compositingInfo
{
    return compositingInfo;
}

- (BOOL)isComposited
{
    return composited;
}

- (BOOL)isSeparator
{
    return separator;
}

@end
