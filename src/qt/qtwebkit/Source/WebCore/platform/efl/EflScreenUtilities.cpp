/*
 * Copyright (C) 2012 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "EflScreenUtilities.h"

#ifdef HAVE_ECORE_X
#include <Ecore_Evas.h>
#include <Ecore_X.h>
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>
#endif

namespace WebCore {

#ifdef HAVE_ECORE_X
class CursorMap {
private:
    HashMap<String, unsigned short> m_cursorStringMap;

public:
    CursorMap();
    int cursor(const String&);
};

int CursorMap::cursor(const String& cursorGroup)
{
    int ret = m_cursorStringMap.get(cursorGroup);

    if (ret < ECORE_X_CURSOR_X || ret > ECORE_X_CURSOR_XTERM)
        ret = ECORE_X_CURSOR_LEFT_PTR;

    return ret;
}

CursorMap::CursorMap()
{
    m_cursorStringMap.set(ASCIILiteral("cursor/pointer"), ECORE_X_CURSOR_LEFT_PTR);
    m_cursorStringMap.set(ASCIILiteral("cursor/move"), ECORE_X_CURSOR_FLEUR);
    m_cursorStringMap.set(ASCIILiteral("cursor/cross"), ECORE_X_CURSOR_CROSS);
    m_cursorStringMap.set(ASCIILiteral("cursor/hand"), ECORE_X_CURSOR_HAND2);
    m_cursorStringMap.set(ASCIILiteral("cursor/i_beam"), ECORE_X_CURSOR_XTERM);
    m_cursorStringMap.set(ASCIILiteral("cursor/wait"), ECORE_X_CURSOR_WATCH);
    m_cursorStringMap.set(ASCIILiteral("cursor/help"), ECORE_X_CURSOR_QUESTION_ARROW);
    m_cursorStringMap.set(ASCIILiteral("cursor/east_resize"), ECORE_X_CURSOR_RIGHT_SIDE);
    m_cursorStringMap.set(ASCIILiteral("cursor/north_resize"), ECORE_X_CURSOR_TOP_SIDE);
    m_cursorStringMap.set(ASCIILiteral("cursor/north_east_resize"), ECORE_X_CURSOR_TOP_RIGHT_CORNER);
    m_cursorStringMap.set(ASCIILiteral("cursor/north_west_resize"), ECORE_X_CURSOR_TOP_LEFT_CORNER);
    m_cursorStringMap.set(ASCIILiteral("cursor/south_resize"), ECORE_X_CURSOR_BOTTOM_SIDE);
    m_cursorStringMap.set(ASCIILiteral("cursor/south_east_resize"), ECORE_X_CURSOR_BOTTOM_RIGHT_CORNER);
    m_cursorStringMap.set(ASCIILiteral("cursor/south_west_resize"), ECORE_X_CURSOR_BOTTOM_LEFT_CORNER);
    m_cursorStringMap.set(ASCIILiteral("cursor/west_resize"), ECORE_X_CURSOR_LEFT_SIDE);
    m_cursorStringMap.set(ASCIILiteral("cursor/north_south_resize"), ECORE_X_CURSOR_SB_V_DOUBLE_ARROW);
    m_cursorStringMap.set(ASCIILiteral("cursor/east_west_resize"), ECORE_X_CURSOR_SB_H_DOUBLE_ARROW);
    m_cursorStringMap.set(ASCIILiteral("cursor/north_east_south_west_resize"), ECORE_X_CURSOR_SIZING);
    m_cursorStringMap.set(ASCIILiteral("cursor/north_west_south_east_resize"), ECORE_X_CURSOR_SIZING);
    m_cursorStringMap.set(ASCIILiteral("cursor/column_resize"), ECORE_X_CURSOR_SB_H_DOUBLE_ARROW);
    m_cursorStringMap.set(ASCIILiteral("cursor/row_resize"), ECORE_X_CURSOR_SB_V_DOUBLE_ARROW);
    m_cursorStringMap.set(ASCIILiteral("cursor/middle_panning"),  ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set(ASCIILiteral("cursor/east_panning"), ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set(ASCIILiteral("cursor/north_panning"), ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set(ASCIILiteral("cursor/north_east_panning"), ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set(ASCIILiteral("cursor/north_west_panning"), ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set(ASCIILiteral("cursor/south_panning"), ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set(ASCIILiteral("cursor/south_east_panning"), ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set(ASCIILiteral("cursor/south_west_panning"), ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set(ASCIILiteral("cursor/west_panning"), ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set(ASCIILiteral("cursor/vertical_text"), ECORE_X_CURSOR_SB_DOWN_ARROW);
    m_cursorStringMap.set(ASCIILiteral("cursor/cell"), ECORE_X_CURSOR_ICON);
    m_cursorStringMap.set(ASCIILiteral("cursor/context_menu"), ECORE_X_CURSOR_HAND2);
    m_cursorStringMap.set(ASCIILiteral("cursor/no_drop"), ECORE_X_CURSOR_DOT_BOX_MASK);
    m_cursorStringMap.set(ASCIILiteral("cursor/copy"), ECORE_X_CURSOR_ICON);
    m_cursorStringMap.set(ASCIILiteral("cursor/progress"), ECORE_X_CURSOR_WATCH);
    m_cursorStringMap.set(ASCIILiteral("cursor/alias"), ECORE_X_CURSOR_MAN);
    m_cursorStringMap.set(ASCIILiteral("cursor/none"), ECORE_X_CURSOR_X);
    m_cursorStringMap.set(ASCIILiteral("cursor/not_allowed"), ECORE_X_CURSOR_X);
    m_cursorStringMap.set(ASCIILiteral("cursor/zoom_in"), ECORE_X_CURSOR_DIAMOND_CROSS);
    m_cursorStringMap.set(ASCIILiteral("cursor/zoom_out"), ECORE_X_CURSOR_DIAMOND_CROSS);
    m_cursorStringMap.set(ASCIILiteral("cursor/grab"), ECORE_X_CURSOR_HAND2);
    m_cursorStringMap.set(ASCIILiteral("cursor/grabbing"), ECORE_X_CURSOR_HAND2);
}

int getEcoreCursor(const String& cursorString)
{
    DEFINE_STATIC_LOCAL(CursorMap, cursorStringMap, ());

    return cursorStringMap.cursor(cursorString);
}
#endif

void applyFallbackCursor(Ecore_Evas* ecoreEvas, const char* cursorString)
{
#ifdef HAVE_ECORE_X
    int shape = getEcoreCursor(cursorString);
    if (shape < ECORE_X_CURSOR_X || shape > ECORE_X_CURSOR_XTERM) {
        LOG_ERROR("cannot map an equivalent X cursor for"
                  " c ursor group %s", cursorString);
        shape = ECORE_X_CURSOR_LEFT_PTR;
    }

    Ecore_X_Window window;
#if USE(ACCELERATED_COMPOSITING)
    window = ecore_evas_gl_x11_window_get(ecoreEvas);
    // Fallback to software mode if necessary.
    if (!window)
#endif
    window = ecore_evas_software_x11_window_get(ecoreEvas);

    Ecore_X_Cursor cursor = ecore_x_cursor_shape_get(shape);
    ecore_x_window_cursor_set(window, cursor);
#endif
}

bool isUsingEcoreX(const Evas* evas)
{
#ifdef HAVE_ECORE_X
    Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(evas);
    const char* engine = ecore_evas_engine_name_get(ecoreEvas);
    return !strcmp(engine, "opengl_x11")
        || !strcmp(engine, "software_x11")
        || !strcmp(engine, "software_xcb")
        || !strcmp(engine, "software_16_x11")
        || !strncmp(engine, "xrender", sizeof("xrender") - 1);
#else
    UNUSED_PARAM(evas);
    return false;
#endif
}

} // namespace WebCore
