/*
    Copyright (C) 2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "config.h"
#include "EWKTestView.h"

#include <EWebKit.h>
#include <wtf/NullPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>

namespace EWKUnitTests {

EWKTestView::EWKTestView()
{
}

bool EWKTestView::init(EwkViewType testViewType, int width, int height)
{
    m_webView = nullptr;

    m_ecoreEvas = adoptPtr(ecore_evas_new(0, 0, 0, width, height, 0));
    if (!m_ecoreEvas)
        return false;

    ecore_evas_show(m_ecoreEvas.get());
    Evas* evas = ecore_evas_get(m_ecoreEvas.get());
    if (!evas)
        return false;

    switch (testViewType) {
    case SingleView:
        m_webView = adoptRef(ewk_view_single_add(evas));
        break;

    case TiledView:
        m_webView = adoptRef(ewk_view_tiled_add(evas));
        break;
    }

    if (!m_webView)
        return false;

    ewk_view_theme_set(m_webView.get(), Config::defaultThemePath);

    evas_object_resize(m_webView.get(), width, height);
    evas_object_show(m_webView.get());
    evas_object_focus_set(m_webView.get(), EINA_TRUE);
    return true;
}

}
