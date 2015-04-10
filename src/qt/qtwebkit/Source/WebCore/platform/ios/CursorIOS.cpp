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

#include "config.h"
#include "Cursor.h"

namespace WebCore {

static const Cursor& cursor()
{
    DEFINE_STATIC_LOCAL(const Cursor, cursor, ());
    return cursor;
}

const Cursor& pointerCursor()
{
    return cursor();
}

const Cursor& crossCursor()
{
    return cursor();
}

const Cursor& handCursor()
{
    return cursor();
}

const Cursor& moveCursor()
{
    return cursor();
}

const Cursor& iBeamCursor()
{
    return cursor();
}

const Cursor& waitCursor()
{
    return cursor();
}

const Cursor& helpCursor()
{
    return cursor();
}

const Cursor& eastResizeCursor()
{
    return cursor();
}

const Cursor& northResizeCursor()
{
    return cursor();
}

const Cursor& northEastResizeCursor()
{
    return cursor();
}

const Cursor& northWestResizeCursor()
{
    return cursor();
}

const Cursor& southResizeCursor()
{
    return cursor();
}

const Cursor& southEastResizeCursor()
{
    return cursor();
}

const Cursor& southWestResizeCursor()
{
    return cursor();
}

const Cursor& westResizeCursor()
{
    return cursor();
}

const Cursor& northSouthResizeCursor()
{
    return cursor();
}

const Cursor& eastWestResizeCursor()
{
    return cursor();
}

const Cursor& northEastSouthWestResizeCursor()
{
    return cursor();
}

const Cursor& northWestSouthEastResizeCursor()
{
    return cursor();
}

const Cursor& columnResizeCursor()
{
    return cursor();
}

const Cursor& rowResizeCursor()
{
    return cursor();
}

const Cursor& middlePanningCursor()
{
    return cursor();
}

const Cursor& eastPanningCursor()
{
    return cursor();
}

const Cursor& northPanningCursor()
{
    return cursor();
}

const Cursor& northEastPanningCursor()
{
    return cursor();
}

const Cursor& northWestPanningCursor()
{
    return cursor();
}

const Cursor& southPanningCursor()
{
    return cursor();
}

const Cursor& southEastPanningCursor()
{
    return cursor();
}

const Cursor& southWestPanningCursor()
{
    return cursor();
}

const Cursor& westPanningCursor()
{
    return cursor();
}

const Cursor& verticalTextCursor()
{
    return cursor();
}

const Cursor& cellCursor()
{
    return cursor();
}

const Cursor& contextMenuCursor()
{
    return cursor();
}

const Cursor& noDropCursor()
{
    return cursor();
}

const Cursor& notAllowedCursor()
{
    return cursor();
}

const Cursor& progressCursor()
{
    return cursor();
}

const Cursor& aliasCursor()
{
    return cursor();
}

const Cursor& zoomInCursor()
{
    return cursor();
}

const Cursor& zoomOutCursor()
{
    return cursor();
}

const Cursor& copyCursor()
{
    return cursor();
}

const Cursor& noneCursor()
{
    return cursor();
}

const Cursor& grabCursor()
{
    return cursor();
}

const Cursor& grabbingCursor()
{
    return cursor();
}

IntPoint determineHotSpot(Image*, const IntPoint&)
{
    return IntPoint();
}

}
