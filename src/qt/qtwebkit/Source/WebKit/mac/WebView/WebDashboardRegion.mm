/*
 * Copyright (C) 2004 Apple Inc. All rights reserved.
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

#import "WebDashboardRegion.h"

#if ENABLE(DASHBOARD_SUPPORT)

@implementation WebDashboardRegion

- (id)initWithRect:(NSRect)r clip:(NSRect)c type:(WebDashboardRegionType)t
{
    self = [super init];
    rect = r;
    clip = c;
    type = t;
    return self;
}

- (id)copyWithZone:(NSZone *)unusedZone
{
    UNUSED_PARAM(unusedZone);

    return [self retain];
}

- (NSRect)dashboardRegionClip
{
    return clip;
}

- (NSRect)dashboardRegionRect
{
    return rect;
}

- (WebDashboardRegionType)dashboardRegionType
{
    return type;
}

static const char* typeName(WebDashboardRegionType type)
{
    switch (type) {
    case WebDashboardRegionTypeNone:
        return "None";
    case WebDashboardRegionTypeCircle:
        return "Circle";
    case WebDashboardRegionTypeRectangle:
        return "Rectangle";
    case WebDashboardRegionTypeScrollerRectangle:
        return "ScrollerRectangle";
    }
    return "Unknown";
}

- (NSString *)description
{
    return [NSString stringWithFormat:@"rect:%@ clip:%@ type:%s", NSStringFromRect(rect), NSStringFromRect(clip), typeName(type)];
}

// FIXME: Overriding isEqual: without overriding hash will cause trouble if this ever goes into a NSSet or is the key in an NSDictionary.
- (BOOL)isEqual:(id)other
{
    return NSEqualRects(rect, [other dashboardRegionRect]) && NSEqualRects(clip, [other dashboardRegionClip]) && type == [other dashboardRegionType];
}

@end

#endif
