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
#include "IntRect.h"
#include "NotImplemented.h"
#include <QMimeDatabase>
#include <wtf/text/WTFString.h>

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

    QMimeType mimeType = QMimeDatabase().mimeTypeForFile(filenames[0], QMimeDatabase::MatchExtension);

    QString iconName = mimeType.iconName();
    QString genericIconName = mimeType.genericIconName();

    // We try to match one of three cases:
    // 1. All the files have the same type.
    // 2. All the files are of the same generic type.
    // 3. The files are not even of the same generic type.
    const int count = filenames.size();
    for (int i = 1; i < count; ++i) {
        mimeType = QMimeDatabase().mimeTypeForFile(filenames[i], QMimeDatabase::MatchExtension);
        if (iconName != mimeType.iconName())
            iconName.clear();
        if (genericIconName != mimeType.genericIconName()) {
            genericIconName.clear();
            break;
        }
    }

    // FIXME: By default, only X11 will support themed icons.
    RefPtr<Icon> icon = adoptRef(new Icon);
    if (!iconName.isEmpty())
        icon->m_icon = QIcon::fromTheme(iconName, QIcon::fromTheme(genericIconName));
    else if (!genericIconName.isEmpty())
        icon->m_icon = QIcon::fromTheme(genericIconName);

    if (icon->m_icon.isNull())
        return 0;
    return icon.release();
}

void Icon::paint(GraphicsContext* context, const IntRect& rect)
{
    if (m_icon.isNull())
        return;

    m_icon.paint(context->platformContext(), rect);
}

}

// vim: ts=4 sw=4 et
