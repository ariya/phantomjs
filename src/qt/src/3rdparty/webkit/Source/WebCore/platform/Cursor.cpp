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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "Cursor.h"

#include "Image.h"

namespace WebCore {

IntPoint determineHotSpot(Image* image, const IntPoint& specifiedHotSpot)
{
    // Hot spot must be inside cursor rectangle.
    IntRect imageRect = image->rect();
    if (imageRect.contains(specifiedHotSpot))
        return specifiedHotSpot;

    // If hot spot is not specified externally, it can be extracted from some image formats (e.g. .cur).
    IntPoint intrinsicHotSpot;
    bool imageHasIntrinsicHotSpot = image->getHotSpot(intrinsicHotSpot);
    if (imageHasIntrinsicHotSpot && imageRect.contains(intrinsicHotSpot))
        return intrinsicHotSpot;

    return IntPoint();
}

const Cursor& Cursor::fromType(Cursor::Type type)
{
    switch (type) {
    case Cursor::Pointer:
        return pointerCursor();
    case Cursor::Cross:
        return crossCursor();
    case Cursor::Hand:
        return handCursor();
    case Cursor::IBeam:
        return iBeamCursor();
    case Cursor::Wait:
        return waitCursor();
    case Cursor::Help:
        return helpCursor();
    case Cursor::EastResize:
        return eastResizeCursor();
    case Cursor::NorthResize:
        return northResizeCursor();
    case Cursor::NorthEastResize:
        return northEastResizeCursor();
    case Cursor::NorthWestResize:
        return northWestResizeCursor();
    case Cursor::SouthResize:
        return southResizeCursor();
    case Cursor::SouthEastResize:
        return southEastResizeCursor();
    case Cursor::SouthWestResize:
        return southWestResizeCursor();
    case Cursor::WestResize:
        return westResizeCursor();
    case Cursor::NorthSouthResize:
        return northSouthResizeCursor();
    case Cursor::EastWestResize:
        return eastWestResizeCursor();
    case Cursor::NorthEastSouthWestResize:
        return northEastSouthWestResizeCursor();
    case Cursor::NorthWestSouthEastResize:
        return northWestSouthEastResizeCursor();
    case Cursor::ColumnResize:
        return columnResizeCursor();
    case Cursor::RowResize:
        return rowResizeCursor();
    case Cursor::MiddlePanning:
        return middlePanningCursor();
    case Cursor::EastPanning:
        return eastPanningCursor();
    case Cursor::NorthPanning:
        return northPanningCursor();
    case Cursor::NorthEastPanning:
        return northEastPanningCursor();
    case Cursor::NorthWestPanning:
        return northWestPanningCursor();
    case Cursor::SouthPanning:
        return southPanningCursor();
    case Cursor::SouthEastPanning:
        return southEastPanningCursor();
    case Cursor::SouthWestPanning:
        return southWestPanningCursor();
    case Cursor::WestPanning:
        return westPanningCursor();
    case Cursor::Move:
        return moveCursor();
    case Cursor::VerticalText:
        return verticalTextCursor();
    case Cursor::Cell:
        return cellCursor();
    case Cursor::ContextMenu:
        return contextMenuCursor();
    case Cursor::Alias:
        return aliasCursor();
    case Cursor::Progress:
        return progressCursor();
    case Cursor::NoDrop:
        return noDropCursor();
    case Cursor::Copy:
        return copyCursor();
    case Cursor::None:
        return noneCursor();
    case Cursor::NotAllowed:
        return notAllowedCursor();
    case Cursor::ZoomIn:
        return zoomInCursor();
    case Cursor::ZoomOut:
        return zoomOutCursor();
    case Cursor::Grab:
        return grabCursor();
    case Cursor::Grabbing:
        return grabbingCursor();
    case Cursor::Custom:
        ASSERT_NOT_REACHED();
    }
    return pointerCursor();
}

const char* nameForCursorType(Cursor::Type type)
{
    switch (type) {
    case Cursor::Pointer:
        return "Pointer";
    case Cursor::Cross:
        return "Cross";
    case Cursor::Hand:
        return "Hand";
    case Cursor::IBeam:
        return "IBeam";
    case Cursor::Wait:
        return "Wait";
    case Cursor::Help:
        return "Help";
    case Cursor::EastResize:
        return "EastResize";
    case Cursor::NorthResize:
        return "NorthResize";
    case Cursor::NorthEastResize:
        return "NorthEastResize";
    case Cursor::NorthWestResize:
        return "NorthWestResize";
    case Cursor::SouthResize:
        return "SouthResize";
    case Cursor::SouthEastResize:
        return "SouthEastResize";
    case Cursor::SouthWestResize:
        return "SouthWestResize";
    case Cursor::WestResize:
        return "WestResize";
    case Cursor::NorthSouthResize:
        return "NorthSouthResize";
    case Cursor::EastWestResize:
        return "EastWestResize";
    case Cursor::NorthEastSouthWestResize:
        return "NorthEastSouthWestResize";
    case Cursor::NorthWestSouthEastResize:
        return "NorthWestSouthEastResize";
    case Cursor::ColumnResize:
        return "ColumnResize";
    case Cursor::RowResize:
        return "RowResize";
    case Cursor::MiddlePanning:
        return "MiddlePanning";
    case Cursor::EastPanning:
        return "EastPanning";
    case Cursor::NorthPanning:
        return "NorthPanning";
    case Cursor::NorthEastPanning:
        return "NorthEastPanning";
    case Cursor::NorthWestPanning:
        return "NorthWestPanning";
    case Cursor::SouthPanning:
        return "SouthPanning";
    case Cursor::SouthEastPanning:
        return "SouthEastPanning";
    case Cursor::SouthWestPanning:
        return "SouthWestPanning";
    case Cursor::WestPanning:
        return "WestPanning";
    case Cursor::Move:
        return "Move";
    case Cursor::VerticalText:
        return "VerticalText";
    case Cursor::Cell:
        return "Cell";
    case Cursor::ContextMenu:
        return "ContextMenu";
    case Cursor::Alias:
        return "Alias";
    case Cursor::Progress:
        return "Progress";
    case Cursor::NoDrop:
        return "NoDrop";
    case Cursor::Copy:
        return "Copy";
    case Cursor::None:
        return "None";
    case Cursor::NotAllowed:
        return "NotAllowed";
    case Cursor::ZoomIn:
        return "ZoomIn";
    case Cursor::ZoomOut:
        return "ZoomOut";
    case Cursor::Grab:
        return "Grab";
    case Cursor::Grabbing:
        return "Grabbing";
    case Cursor::Custom:
        return "Custom";
    }

    return "ERROR";
}

#if USE(LAZY_NATIVE_CURSOR)

Cursor::Cursor(Image* image, const IntPoint& hotSpot)
    : m_type(Custom)
    , m_image(image)
    , m_hotSpot(determineHotSpot(image, hotSpot))
    , m_platformCursor(0)
{
}

Cursor::Cursor(Type type)
    : m_type(type)
    , m_platformCursor(0)
{
}

#if !PLATFORM(MAC)

PlatformCursor Cursor::platformCursor() const
{
    ensurePlatformCursor();
    return m_platformCursor;
}

#endif

const Cursor& pointerCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::Pointer));
    return c;
}

const Cursor& crossCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::Cross));
    return c;
}

const Cursor& handCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::Hand));
    return c;
}

const Cursor& moveCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::Move));
    return c;
}

const Cursor& verticalTextCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::VerticalText));
    return c;
}

const Cursor& cellCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::Cell));
    return c;
}

const Cursor& contextMenuCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::ContextMenu));
    return c;
}

const Cursor& aliasCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::Alias));
    return c;
}

const Cursor& zoomInCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::ZoomIn));
    return c;
}

const Cursor& zoomOutCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::ZoomOut));
    return c;
}

const Cursor& copyCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::Copy));
    return c;
}

const Cursor& noneCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::None));
    return c;
}

const Cursor& progressCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::Progress));
    return c;
}

const Cursor& noDropCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::NoDrop));
    return c;
}

const Cursor& notAllowedCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::NotAllowed));
    return c;
}

const Cursor& iBeamCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::IBeam));
    return c;
}

const Cursor& waitCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::Wait));
    return c;
}

const Cursor& helpCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::Help));
    return c;
}

const Cursor& eastResizeCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::EastResize));
    return c;
}

const Cursor& northResizeCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::NorthResize));
    return c;
}

const Cursor& northEastResizeCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::NorthEastResize));
    return c;
}

const Cursor& northWestResizeCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::NorthWestResize));
    return c;
}

const Cursor& southResizeCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::SouthResize));
    return c;
}

const Cursor& southEastResizeCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::SouthEastResize));
    return c;
}

const Cursor& southWestResizeCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::SouthWestResize));
    return c;
}

const Cursor& westResizeCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::WestResize));
    return c;
}

const Cursor& northSouthResizeCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::NorthSouthResize));
    return c;
}

const Cursor& eastWestResizeCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::EastWestResize));
    return c;
}

const Cursor& northEastSouthWestResizeCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::NorthEastSouthWestResize));
    return c;
}

const Cursor& northWestSouthEastResizeCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::NorthWestSouthEastResize));
    return c;
}

const Cursor& columnResizeCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::ColumnResize));
    return c;
}

const Cursor& rowResizeCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::RowResize));
    return c;
}

const Cursor& middlePanningCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::MiddlePanning));
    return c;
}
    
const Cursor& eastPanningCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::EastPanning));
    return c;
}
    
const Cursor& northPanningCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::NorthPanning));
    return c;
}
    
const Cursor& northEastPanningCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::NorthEastPanning));
    return c;
}
    
const Cursor& northWestPanningCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::NorthWestPanning));
    return c;
}
    
const Cursor& southPanningCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::SouthPanning));
    return c;
}
    
const Cursor& southEastPanningCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::SouthEastPanning));
    return c;
}
    
const Cursor& southWestPanningCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::SouthWestPanning));
    return c;
}
    
const Cursor& westPanningCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::WestPanning));
    return c;
}

const Cursor& grabCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::Grab));
    return c;
}

const Cursor& grabbingCursor()
{
    DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::Grabbing));
    return c;
}

#endif

} // namespace WebCore
