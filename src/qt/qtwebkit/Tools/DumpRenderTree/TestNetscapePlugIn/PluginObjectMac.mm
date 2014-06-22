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

#include "PluginObject.h"


#include <QuartzCore/QuartzCore.h>

@interface TestPluginLayer : CALayer
@end

@implementation TestPluginLayer

- (void)drawInContext:(CGContextRef)context
{
    CGRect bounds = [self bounds];
    const char* text = "Test Plug-in";
    CGContextSelectFont(context, "Helvetica", 24, kCGEncodingMacRoman);
    CGContextShowTextAtPoint(context, bounds.origin.x + 3.0f, bounds.origin.y + bounds.size.height - 30.0f, text, strlen(text));
}

@end

void* createCoreAnimationLayer()
{
    CALayer *caLayer = [[TestPluginLayer alloc] init];

    NSNull *nullValue = [NSNull null];
    NSDictionary *actions = [NSDictionary dictionaryWithObjectsAndKeys:
                             nullValue, @"anchorPoint",
                             nullValue, @"bounds",
                             nullValue, @"contents",
                             nullValue, @"contentsRect",
                             nullValue, @"opacity",
                             nullValue, @"position",
                             nullValue, @"shadowColor",
                             nullValue, @"sublayerTransform",
                             nullValue, @"sublayers",
                             nullValue, @"transform",
                             nullValue, @"zPosition",
                             nil];
    // Turn off default animations.
    [caLayer setStyle:[NSDictionary dictionaryWithObject:actions forKey:@"actions"]];
    [caLayer setNeedsDisplayOnBoundsChange:YES];

    [caLayer setBounds:CGRectMake(0, 0, 200, 100)];
    [caLayer setAnchorPoint:CGPointZero];
    
    CGColorRef color = CGColorCreateGenericRGB(0.5, 0.5, 1, 1);
    [caLayer setBackgroundColor:color];
    CGColorRelease(color);
    
    [caLayer setLayoutManager:[CAConstraintLayoutManager layoutManager]];

    CALayer *sublayer = [CALayer layer];
    // Turn off default animations.
    [sublayer setStyle:[NSDictionary dictionaryWithObject:actions forKey:@"actions"]];

    color = CGColorCreateGenericRGB(0, 0, 0, 0.75);
    [sublayer setBackgroundColor:color];
    CGColorRelease(color);
    [sublayer setBounds:CGRectMake(0, 0, 180, 20)];

    [sublayer addConstraint:[CAConstraint constraintWithAttribute:kCAConstraintMinY
                                                     relativeTo:@"superlayer"
                                                      attribute:kCAConstraintMinY]];
    [sublayer addConstraint:[CAConstraint constraintWithAttribute:kCAConstraintMinX
                                                     relativeTo:@"superlayer"
                                                      attribute:kCAConstraintMinX]];
    [sublayer addConstraint:[CAConstraint constraintWithAttribute:kCAConstraintMaxX
                                                     relativeTo:@"superlayer"
                                                      attribute:kCAConstraintMaxX]];

    [caLayer addSublayer:sublayer];
    return caLayer;
}


