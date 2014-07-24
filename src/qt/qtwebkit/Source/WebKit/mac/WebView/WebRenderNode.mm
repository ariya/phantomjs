/*
 * Copyright (C) 2005, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "WebRenderNode.h"

#import "WebFrameInternal.h"
#import <WebCore/Frame.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/FrameLoaderClient.h>
#import <WebCore/RenderText.h>
#import <WebCore/RenderWidget.h>
#import <WebCore/RenderView.h>
#import <WebCore/Widget.h>

using namespace WebCore;

static WebRenderNode *copyRenderNode(RenderObject*);

@implementation WebRenderNode

- (id)_initWithCoreFrame:(Frame *)frame
{
    [self release];
    
    if (!frame->loader()->client()->hasHTMLView())
        return nil;
    
    RenderObject* renderer = frame->contentRenderer();
    if (!renderer)
        return nil;
    
    return copyRenderNode(renderer);
}

- (id)_initWithName:(NSString *)n position:(NSPoint)p rect:(NSRect)r coreFrame:(Frame*)coreFrame children:(NSArray *)c
{
    NSMutableArray *collectChildren;
    
    self = [super init];
    if (!self)
        return nil;

    collectChildren = [c mutableCopy];

    name = [n retain];
    rect = r;
    absolutePosition = p;

    if (coreFrame) {
        WebRenderNode *node = [[WebRenderNode alloc] _initWithCoreFrame:coreFrame];
        [collectChildren addObject:node];
        [node release];
    }
    
    children = [collectChildren copy];
    [collectChildren release];
    
    return self;
}

static WebRenderNode *copyRenderNode(RenderObject* node)
{
    NSMutableArray *children = [[NSMutableArray alloc] init];
    for (RenderObject* child = node->firstChild(); child; child = child->nextSibling()) {
        WebRenderNode *childCopy = copyRenderNode(child);
        [children addObject:childCopy];
        [childCopy release];
    }

    NSString *name = [[NSString alloc] initWithUTF8String:node->renderName()];
    
    RenderWidget* renderWidget = node->isWidget() ? toRenderWidget(node) : 0;
    Widget* widget = renderWidget ? renderWidget->widget() : 0;
    FrameView* frameView = widget && widget->isFrameView() ? toFrameView(widget) : 0;
    Frame* frame = frameView ? frameView->frame() : 0;

    // FIXME: broken with transforms
    FloatPoint absPos = node->localToAbsolute();
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    if (node->isBox()) {
        RenderBox* box = toRenderBox(node);
        x = box->x();
        y = box->y();
        width = box->width();
        height = box->height();
    } else if (node->isText()) {
        // FIXME: Preserve old behavior even though it's strange.
        RenderText* text = toRenderText(node);
        x = text->firstRunX();
        y = text->firstRunY();
        IntRect box = text->linesBoundingBox();
        width = box.width();
        height = box.height();
    } else if (node->isRenderInline()) {
        RenderBoxModelObject* inlineFlow = toRenderBoxModelObject(node);
        IntRect boundingBox = inlineFlow->borderBoundingBox();
        x = boundingBox.x();
        y = boundingBox.y();
        width = boundingBox.width();
        height = boundingBox.height();
    }

    WebRenderNode *result = [[WebRenderNode alloc] _initWithName:name
                                                        position:absPos rect:NSMakeRect(x, y, width, height)
                                                       coreFrame:frame children:children];

    [name release];
    [children release];

    return result;
}

- (id)initWithWebFrame:(WebFrame *)frame
{
    return [self _initWithCoreFrame:core(frame)];
}

- (void)dealloc
{
    [children release];
    [name release];
    [super dealloc];
}

- (NSArray *)children
{
    return children;
}

- (NSString *)name
{
    return name;
}

- (NSString *)absolutePositionString
{
    return [NSString stringWithFormat:@"(%.0f, %.0f)", absolutePosition.x, absolutePosition.y];
}

- (NSString *)positionString
{
    return [NSString stringWithFormat:@"(%.0f, %.0f)", rect.origin.x, rect.origin.y];
}

- (NSString *)widthString
{
    return [NSString stringWithFormat:@"%.0f", rect.size.width];
}

- (NSString *)heightString
{
    return [NSString stringWithFormat:@"%.0f", rect.size.height];
}

@end
