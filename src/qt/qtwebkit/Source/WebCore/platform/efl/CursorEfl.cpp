/*
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2006 Charles Samuels <charles@kde.org>
 * Copyright (C) 2008 Holger Hans Peter Freyther
 * Copyright (C) 2008 Kenneth Rohde Christiansen
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2010 Samsung Electronics
 *
 * All rights reserved.
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

#include <Edje.h>
#include <Evas.h>
#include <wtf/Assertions.h>

namespace WebCore {

Cursor::Cursor(const Cursor& other)
    : m_type(other.m_type)
    , m_image(other.m_image)
    , m_hotSpot(other.m_hotSpot)
#if ENABLE(MOUSE_CURSOR_SCALE)
    , m_imageScaleFactor(other.m_imageScaleFactor)
#endif
    , m_platformCursor(other.m_platformCursor)
{
}

Cursor::~Cursor()
{
}

Cursor& Cursor::operator=(const Cursor& other)
{
    m_type = other.m_type;
    m_image = other.m_image;
    m_hotSpot = other.m_hotSpot;
#if ENABLE(MOUSE_CURSOR_SCALE)
    m_imageScaleFactor = other.m_imageScaleFactor;
#endif
    m_platformCursor = other.m_platformCursor;

    return *this;
}

static const char* cursorString(Cursor::Type type)
{
    static const char* cursorStrings[] = {
        "cursor/pointer",
        "cursor/cross",
        "cursor/hand",
        "cursor/i_beam",
        "cursor/wait",
        "cursor/help",
        "cursor/east_resize",
        "cursor/north_resize",
        "cursor/north_east_resize",
        "cursor/north_west_resize",
        "cursor/south_resize",
        "cursor/south_east_resize",
        "cursor/south_west_resize",
        "cursor/west_resize",
        "cursor/north_south_resize",
        "cursor/east_west_resize",
        "cursor/north_east_south_west_resize",
        "cursor/north_west_south_east_resize",
        "cursor/column_resize",
        "cursor/row_resize",
        "cursor/middle_panning",
        "cursor/east_panning",
        "cursor/north_panning",
        "cursor/north_east_panning",
        "cursor/north_west_panning",
        "cursor/south_panning",
        "cursor/south_east_panning",
        "cursor/south_west_panning",
        "cursor/west_panning",
        "cursor/move",
        "cursor/vertical_text",
        "cursor/cell",
        "cursor/context_menu",
        "cursor/alias",
        "cursor/progress",
        "cursor/no_drop",
        "cursor/copy",
        "cursor/none",
        "cursor/not_allowed",
        "cursor/zoom_in",
        "cursor/zoom_out",
        "cursor/grab",
        "cursor/grabbing",
        "" // Custom cursor.
    };
    return cursorStrings[type];
}

void Cursor::ensurePlatformCursor() const
{
    if (m_platformCursor)
        return;
    m_platformCursor = cursorString(m_type);
}

}
