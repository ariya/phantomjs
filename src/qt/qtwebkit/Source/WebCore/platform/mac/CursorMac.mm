/*
 * Copyright (C) 2004, 2006, 2010 Apple Inc. All rights reserved.
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

#import "config.h"
#import "Cursor.h"

#import "BlockExceptions.h"
#import "WebCoreSystemInterface.h"
#import <wtf/StdLibExtras.h>

@interface WebCoreCursorBundle : NSObject { }
@end

@implementation WebCoreCursorBundle
@end

namespace WebCore {

// Simple NSCursor calls shouldn't need protection,
// but creating a cursor with a bad image might throw.

static RetainPtr<NSCursor> createCustomCursor(Image* image, const IntPoint& hotSpot)
{
    // FIXME: The cursor won't animate.  Not sure if that's a big deal.
    NSImage* nsImage = image->getNSImage();
    if (!nsImage)
        return 0;
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    return adoptNS([[NSCursor alloc] initWithImage:nsImage hotSpot:hotSpot]);
    END_BLOCK_OBJC_EXCEPTIONS;
    return 0;
}

static RetainPtr<NSCursor> createNamedCursor(const char* name, int x, int y)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    RetainPtr<NSString> resourceName = adoptNS([[NSString alloc] initWithUTF8String:name]);
    RetainPtr<NSImage> cursorImage = adoptNS([[NSImage alloc] initWithContentsOfFile:[[NSBundle bundleForClass:[WebCoreCursorBundle class]] pathForResource:resourceName.get() ofType:@"png"]]);
    
    RetainPtr<NSCursor> cursor;

    if (cursorImage)
        cursor = adoptNS([[NSCursor alloc] initWithImage:cursorImage.get() hotSpot:NSMakePoint(x, y)]);

    return cursor;
    END_BLOCK_OBJC_EXCEPTIONS;
    return nil;
}

void Cursor::ensurePlatformCursor() const
{
    if (m_platformCursor)
        return;

    switch (m_type) {
    case Cursor::Pointer:
        m_platformCursor = [NSCursor arrowCursor];
        break;

    case Cursor::Cross:
        m_platformCursor = [NSCursor crosshairCursor];
        break;

    case Cursor::Hand:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = [NSCursor pointingHandCursor];
#else
        // The pointingHandCursor from NSCursor does not have a shadow on
        // older versions of OS X, so use our own custom cursor.
        m_platformCursor = createNamedCursor("linkCursor", 6, 1);
#endif
        break;

    case Cursor::IBeam:
        m_platformCursor = [NSCursor IBeamCursor];
        break;

    case Cursor::Wait:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("BusyButClickable");
#else
        m_platformCursor = createNamedCursor("waitCursor", 7, 7);
#endif
        break;

    case Cursor::Help:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("Help");
        if (m_platformCursor)
            break;
#endif
        m_platformCursor = createNamedCursor("helpCursor", 8, 8);
        break;

    case Cursor::Move:
    case Cursor::MiddlePanning:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("Move");
#else
        m_platformCursor = createNamedCursor("moveCursor", 7, 7);
#endif
        break;

    case Cursor::EastResize:
    case Cursor::EastPanning:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("ResizeEast");
#else
        m_platformCursor = createNamedCursor("eastResizeCursor", 14, 7);
#endif
        break;

    case Cursor::NorthResize:
    case Cursor::NorthPanning:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("ResizeNorth");
#else
        m_platformCursor = createNamedCursor("northResizeCursor", 7, 1);
#endif
        break;

    case Cursor::NorthEastResize:
    case Cursor::NorthEastPanning:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("ResizeNortheast");
#else
        m_platformCursor = createNamedCursor("northEastResizeCursor", 14, 1);
#endif
        break;

    case Cursor::NorthWestResize:
    case Cursor::NorthWestPanning:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("ResizeNorthwest");
#else
        m_platformCursor = createNamedCursor("northWestResizeCursor", 0, 0);
#endif
        break;

    case Cursor::SouthResize:
    case Cursor::SouthPanning:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("ResizeSouth");
#else
        m_platformCursor = createNamedCursor("southResizeCursor", 7, 14);
#endif
        break;

    case Cursor::SouthEastResize:
    case Cursor::SouthEastPanning:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("ResizeSoutheast");
#else
        m_platformCursor = createNamedCursor("southEastResizeCursor", 14, 14);
#endif
        break;

    case Cursor::SouthWestResize:
    case Cursor::SouthWestPanning:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("ResizeSouthwest");
#else
        m_platformCursor = createNamedCursor("southWestResizeCursor", 1, 14);
#endif
        break;

    case Cursor::WestResize:
    case Cursor::WestPanning:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("ResizeWest");
#else
        m_platformCursor = createNamedCursor("westResizeCursor", 1, 7);
#endif
        break;

    case Cursor::NorthSouthResize:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("ResizeNorthSouth");
#else
        m_platformCursor = createNamedCursor("northSouthResizeCursor", 7, 7);
#endif
        break;

    case Cursor::EastWestResize:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("ResizeEastWest");
#else
        m_platformCursor = createNamedCursor("eastWestResizeCursor", 7, 7);
#endif
        break;

    case Cursor::NorthEastSouthWestResize:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("ResizeNortheastSouthwest");
#else
        m_platformCursor = createNamedCursor("northEastSouthWestResizeCursor", 7, 7);
#endif
        break;

    case Cursor::NorthWestSouthEastResize:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("ResizeNorthwestSoutheast");
#else
        m_platformCursor = createNamedCursor("northWestSouthEastResizeCursor", 7, 7);
#endif
        break;

    case Cursor::ColumnResize:
        m_platformCursor = [NSCursor resizeLeftRightCursor];
        break;

    case Cursor::RowResize:
        m_platformCursor = [NSCursor resizeUpDownCursor];
        break;

    case Cursor::VerticalText:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = [NSCursor IBeamCursorForVerticalLayout];
#else
        m_platformCursor = createNamedCursor("verticalTextCursor", 7, 7);
#endif
        break;

    case Cursor::Cell:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("Cell");
        if (m_platformCursor)
            break;
#endif
        m_platformCursor = createNamedCursor("cellCursor", 7, 7);
        break;

    case Cursor::ContextMenu:
        m_platformCursor = [NSCursor contextualMenuCursor];
        break;

    case Cursor::Alias:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("MakeAlias");
#else
        m_platformCursor = createNamedCursor("aliasCursor", 11, 3);
#endif
        break;

    case Cursor::Progress:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("BusyButClickable");
#else
        m_platformCursor = createNamedCursor("progressCursor", 3, 2);
#endif
        break;

    case Cursor::NoDrop:
        m_platformCursor = [NSCursor operationNotAllowedCursor];
        break;

    case Cursor::Copy:
        m_platformCursor = [NSCursor dragCopyCursor];
        break;

    case Cursor::None:
        m_platformCursor = createNamedCursor("noneCursor", 7, 7);
        break;

    case Cursor::NotAllowed:
        m_platformCursor = [NSCursor operationNotAllowedCursor];
        break;

    case Cursor::ZoomIn:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("ZoomIn");
        if (m_platformCursor)
            break;
#endif
        m_platformCursor = createNamedCursor("zoomInCursor", 7, 7);
        break;

    case Cursor::ZoomOut:
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        m_platformCursor = wkCursor("ZoomOut");
        if (m_platformCursor)
            break;
#endif
        m_platformCursor = createNamedCursor("zoomOutCursor", 7, 7);
        break;

    case Cursor::Grab:
        m_platformCursor = [NSCursor openHandCursor];
        break;

    case Cursor::Grabbing:
        m_platformCursor = [NSCursor closedHandCursor];
        break;

    case Cursor::Custom:
        m_platformCursor = createCustomCursor(m_image.get(), m_hotSpot);
        break;
    }
}

Cursor::Cursor(const Cursor& other)
    : m_type(other.m_type)
    , m_image(other.m_image)
    , m_hotSpot(other.m_hotSpot)
    , m_imageScaleFactor(other.m_imageScaleFactor)
    , m_platformCursor(other.m_platformCursor)
{
}

Cursor& Cursor::operator=(const Cursor& other)
{
    m_type = other.m_type;
    m_image = other.m_image;
    m_hotSpot = other.m_hotSpot;
    m_imageScaleFactor = other.m_imageScaleFactor;
    m_platformCursor = other.m_platformCursor;
    return *this;
}

Cursor::~Cursor()
{
}

NSCursor *Cursor::platformCursor() const
{
    ensurePlatformCursor();
    return m_platformCursor.get();
}

} // namespace WebCore
