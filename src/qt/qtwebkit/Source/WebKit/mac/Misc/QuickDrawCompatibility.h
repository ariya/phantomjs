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

#ifndef QuickDrawCompatibility_h
#define QuickDrawCompatibility_h

#ifndef __LP64__

#import <Carbon/Carbon.h>

#if defined(QD_HEADERS_ARE_PRIVATE) && QD_HEADERS_ARE_PRIVATE

#ifdef __cplusplus
extern "C" {
#endif

#define MacSetRect SetRect
#define MacSetRectRgn SetRectRgn
#define MacUnionRgn UnionRgn

extern Boolean EmptyRgn(RgnHandle);
extern OSStatus CreateCGContextForPort(CGrafPtr, CGContextRef*);
extern OSStatus SyncCGContextOriginWithPort(CGContextRef, CGrafPtr);
extern PixMapHandle GetPortPixMap(CGrafPtr);
extern QDErr NewGWorldFromPtr(GWorldPtr*, UInt32, const Rect*, CTabHandle, GDHandle, GWorldFlags, Ptr, SInt32);
extern Rect* GetPortBounds(CGrafPtr, Rect*);
extern Rect* GetRegionBounds(RgnHandle, Rect*);
extern RgnHandle GetPortClipRegion(CGrafPtr, RgnHandle);
extern RgnHandle GetPortVisibleRegion(CGrafPtr, RgnHandle);
extern RgnHandle NewRgn();
extern void BackColor(long);
extern void DisposeGWorld(GWorldPtr);
extern void DisposeRgn(RgnHandle);
extern void ForeColor(long);
extern void GetGWorld(CGrafPtr*, GDHandle*);
extern void GetPort(GrafPtr*);
extern void GlobalToLocal(Point*);
extern void MacSetRect(Rect*, short, short, short, short);
extern void MacSetRectRgn(RgnHandle, short, short, short, short);
extern void MacUnionRgn(RgnHandle, RgnHandle, RgnHandle);
extern void MovePortTo(short, short);
extern void OffsetRect(Rect*, short, short);
extern void OffsetRgn(RgnHandle, short, short);
extern void PaintRect(const Rect*);
extern void PenNormal();
extern void PortSize(short, short);
extern void RectRgn(RgnHandle, const Rect*);
extern void SectRgn(RgnHandle, RgnHandle, RgnHandle);
extern void SetGWorld(CGrafPtr, GDHandle);
extern void SetOrigin(short, short);
extern void SetPort(GrafPtr);
extern void SetPortClipRegion(CGrafPtr, RgnHandle);
extern void SetPortVisibleRegion(CGrafPtr, RgnHandle);

enum {
    blackColor = 33,
    whiteColor = 30,
    greenColor = 341,
};

#ifdef __cplusplus
}
#endif

#endif // defined(QD_HEADERS_ARE_PRIVATE) && QD_HEADERS_ARE_PRIVATE

#endif // __LP64__

#endif // QuickDrawCompatibility_h
