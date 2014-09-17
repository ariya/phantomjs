/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "Icon.h"

#include "GraphicsContext.h"
#include "PlatformString.h"
#include "IntRect.h"

#include <qpainter.h>
#include <qpixmap.h>
#include <qrect.h>
#include <qglobal.h>

namespace WebCore {

Icon::Icon()
{
}

Icon::~Icon()
{
}

// FIXME: Move the code to ChromeClient::iconForFiles().
PassRefPtr<Icon> Icon::createIconForFiles(const Vector<String>& filenames)
{
    if (filenames.isEmpty())
        return 0;

    if (filenames.size() == 1) {
        RefPtr<Icon> i = adoptRef(new Icon);
        i->m_icon = QIcon(filenames[0]);
        return i.release();
    }

    //FIXME: Implement this
    return 0;
}

void Icon::paint(GraphicsContext* ctx, const IntRect& rect)
{
    QPixmap px = m_icon.pixmap(rect.size());
    QPainter *p = static_cast<QPainter*>(ctx->platformContext());
    if (p && !px.isNull())
        p->drawPixmap(rect.x(), rect.y(), px);
}

}

// vim: ts=4 sw=4 et
