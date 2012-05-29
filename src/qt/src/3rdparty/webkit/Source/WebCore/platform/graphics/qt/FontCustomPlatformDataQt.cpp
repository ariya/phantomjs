/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/
#include "config.h"
#include "FontCustomPlatformData.h"

#include "FontPlatformData.h"
#include "SharedBuffer.h"
#include <QFontDatabase>
#include <QStringList>

namespace WebCore {

FontCustomPlatformData::~FontCustomPlatformData()
{
    QFontDatabase::removeApplicationFont(m_handle);
}

FontPlatformData FontCustomPlatformData::fontPlatformData(int size, bool bold, bool italic, FontOrientation, TextOrientation, FontWidthVariant, FontRenderingMode)
{
    QFont font;
    font.setFamily(QFontDatabase::applicationFontFamilies(m_handle)[0]);
    font.setPixelSize(size);
    if (bold)
        font.setWeight(QFont::Bold);
    font.setItalic(italic);

    return FontPlatformData(font);
}

FontCustomPlatformData* createFontCustomPlatformData(SharedBuffer* buffer)
{
    ASSERT_ARG(buffer, buffer);

    int id = QFontDatabase::addApplicationFontFromData(QByteArray(buffer->data(), buffer->size()));
    if (id == -1)
        return 0;

    Q_ASSERT(QFontDatabase::applicationFontFamilies(id).size() > 0);

    FontCustomPlatformData *data = new FontCustomPlatformData;
    data->m_handle = id;
    return data;
}

bool FontCustomPlatformData::supportsFormat(const String& format)
{
    return equalIgnoringCase(format, "truetype") || equalIgnoringCase(format, "opentype");
}

}
