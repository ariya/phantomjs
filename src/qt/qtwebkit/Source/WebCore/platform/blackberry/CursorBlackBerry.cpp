/*
 * Copyright (C) 2009, 2010, 2011 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "Cursor.h"

#include "Image.h"

#include <string>

typedef BlackBerry::Platform::CursorType CursorType;

namespace WebCore {

struct AllCursors {
    AllCursors()
    {
        for (int i = 0; i < BlackBerry::Platform::NumCursorTypes; ++i)
            m_cursors[i] = Cursor(BlackBerry::Platform::BlackBerryCursor((CursorType)i));
    }
    Cursor m_cursors[BlackBerry::Platform::NumCursorTypes];
};

static const Cursor& getCursor(CursorType type)
{
    static AllCursors allCursors;
    return allCursors.m_cursors[type];
}

Cursor::Cursor(const Cursor& other)
    : m_platformCursor(other.m_platformCursor)
{
}

Cursor::Cursor(Image*, const IntPoint& hotspot)
: m_platformCursor(BlackBerry::Platform::CursorCustomized, BlackBerry::Platform::String::emptyString(), hotspot)
{
}

Cursor::~Cursor()
{
}

Cursor& Cursor::operator=(const Cursor& other)
{
    m_platformCursor = other.m_platformCursor;
    return *this;
}

Cursor::Cursor(PlatformCursor c)
: m_platformCursor(c)
{
}

const Cursor& aliasCursor()
{
    return getCursor(BlackBerry::Platform::CursorAlias);
}

const Cursor& cellCursor()
{
    return getCursor(BlackBerry::Platform::CursorCell);
}

const Cursor& columnResizeCursor()
{
    return getCursor(BlackBerry::Platform::CursorColumnResize);
}

const Cursor& contextMenuCursor()
{
    return getCursor(BlackBerry::Platform::CursorContextMenu);
}

const Cursor& copyCursor()
{
    return getCursor(BlackBerry::Platform::CursorCopy);
}

const Cursor& crossCursor()
{
    return getCursor(BlackBerry::Platform::CursorCross);
}

const Cursor& eastResizeCursor()
{
    return getCursor(BlackBerry::Platform::CursorEastResize);
}

const Cursor& eastWestResizeCursor()
{
    return getCursor(BlackBerry::Platform::CursorEastWestResize);
}

const Cursor& grabbingCursor()
{
    return getCursor(BlackBerry::Platform::CursorHand);
}

const Cursor& grabCursor()
{
    return getCursor(BlackBerry::Platform::CursorHand);
}

const Cursor& handCursor()
{
    return getCursor(BlackBerry::Platform::CursorHand);
}

const Cursor& helpCursor()
{
    return getCursor(BlackBerry::Platform::CursorHelp);
}

const Cursor& iBeamCursor()
{
    return getCursor(BlackBerry::Platform::CursorBeam);
}

const Cursor& moveCursor()
{
    return getCursor(BlackBerry::Platform::CursorMove);
}

const Cursor& noDropCursor()
{
    return getCursor(BlackBerry::Platform::CursorNoDrop);
}

const Cursor& noneCursor()
{
    return getCursor(BlackBerry::Platform::CursorNone);
}

const Cursor& northEastResizeCursor()
{
    return getCursor(BlackBerry::Platform::CursorNorthEastResize);
}

const Cursor& northEastSouthWestResizeCursor()
{
    return getCursor(BlackBerry::Platform::CursorNorthEastSouthWestResize);
}

const Cursor& northResizeCursor()
{
    return getCursor(BlackBerry::Platform::CursorNorthResize);
}

const Cursor& northSouthResizeCursor()
{
    return getCursor(BlackBerry::Platform::CursorNorthSouthResize);
}

const Cursor& northWestResizeCursor()
{
    return getCursor(BlackBerry::Platform::CursorNorthWestResize);
}

const Cursor& northWestSouthEastResizeCursor()
{
    return getCursor(BlackBerry::Platform::CursorNorthWestSouthEastResize);
}

const Cursor& notAllowedCursor()
{
    return getCursor(BlackBerry::Platform::CursorNotAllowed);
}

const Cursor& pointerCursor()
{
    return getCursor(BlackBerry::Platform::CursorPointer);
}

const Cursor& progressCursor()
{
    return getCursor(BlackBerry::Platform::CursorProgress);
}

const Cursor& rowResizeCursor()
{
    return getCursor(BlackBerry::Platform::CursorRowResize);
}

const Cursor& southEastResizeCursor()
{
    return getCursor(BlackBerry::Platform::CursorSouthEastResize);
}

const Cursor& southResizeCursor()
{
    return getCursor(BlackBerry::Platform::CursorSouthResize);
}

const Cursor& southWestResizeCursor()
{
    return getCursor(BlackBerry::Platform::CursorSouthWestResize);
}

const Cursor& verticalTextCursor()
{
    return getCursor(BlackBerry::Platform::CursorVerticalText);
}

const Cursor& waitCursor()
{
    return getCursor(BlackBerry::Platform::CursorWait);
}

const Cursor& westResizeCursor()
{
    return getCursor(BlackBerry::Platform::CursorWestResize);
}

const Cursor& zoomInCursor()
{
    return getCursor(BlackBerry::Platform::CursorZoomIn);
}

const Cursor& zoomOutCursor()
{
    return getCursor(BlackBerry::Platform::CursorZoomOut);
}

const Cursor& middlePanningCursor()
{
    return moveCursor();
}

const Cursor& eastPanningCursor()
{
    return eastResizeCursor();
}

const Cursor& northPanningCursor()
{
    return northResizeCursor();
}

const Cursor& northEastPanningCursor()
{
    return northEastResizeCursor();
}

const Cursor& northWestPanningCursor()
{
    return northWestResizeCursor();
}

const Cursor& southPanningCursor()
{
    return southResizeCursor();
}

const Cursor& southEastPanningCursor()
{
    return southEastResizeCursor();
}

const Cursor& southWestPanningCursor()
{
    return southWestResizeCursor();
}

const Cursor& westPanningCursor()
{
    return westResizeCursor();
}

} // namespace WebCore
