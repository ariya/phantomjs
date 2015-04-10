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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WKGeometry_h
#define WKGeometry_h

#include <WebKit2/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

struct WKPoint {
    double x;
    double y;
};
typedef struct WKPoint WKPoint;

WK_INLINE WKPoint WKPointMake(double x, double y)
{
    WKPoint point;
    point.x = x;
    point.y = y;
    return point;
}

struct WKSize {
    double width;
    double height;
};
typedef struct WKSize WKSize;

WK_INLINE WKSize WKSizeMake(double width, double height)
{
    WKSize size;
    size.width = width;
    size.height = height;
    return size;
}

struct WKRect {
    WKPoint origin;
    WKSize size;
};
typedef struct WKRect WKRect;

WK_INLINE WKRect WKRectMake(double x, double y, double width, double height)
{
    WKRect rect;
    rect.origin.x = x;
    rect.origin.y = y;
    rect.size.width = width;
    rect.size.height = height;
    return rect;
}

WK_EXPORT WKTypeID WKSizeGetTypeID();
WK_EXPORT WKTypeID WKPointGetTypeID();
WK_EXPORT WKTypeID WKRectGetTypeID();

WK_EXPORT WKPointRef WKPointCreate(WKPoint point);
WK_EXPORT WKSizeRef WKSizeCreate(WKSize size);
WK_EXPORT WKRectRef WKRectCreate(WKRect rect);

WK_EXPORT WKSize WKSizeGetValue(WKSizeRef size);
WK_EXPORT WKPoint WKPointGetValue(WKPointRef point);
WK_EXPORT WKRect WKRectGetValue(WKRectRef rect);


#ifdef __cplusplus
}

inline bool operator==(const WKPoint& a, const WKPoint& b)
{
    return a.x == b.x && a.y == b.y;
}
#endif

#endif /* WKGeometry_h */
