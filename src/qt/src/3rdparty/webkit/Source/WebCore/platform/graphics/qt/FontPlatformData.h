/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2008 Holger Hans Peter Freyther
    Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/

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
#ifndef FontPlatformData_h
#define FontPlatformData_h

#include "FontDescription.h"
#include "FontOrientation.h"
#include <QFont>
#include <QHash>
#if HAVE(QRAWFONT)
#include <QRawFont>
#endif
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class FontPlatformDataPrivate : public RefCounted<FontPlatformDataPrivate> {
    WTF_MAKE_NONCOPYABLE(FontPlatformDataPrivate); WTF_MAKE_FAST_ALLOCATED;
public:
    FontPlatformDataPrivate()
        : size(font.pixelSize())
        , bold(font.bold())
        , oblique(false)
        , isDeletedValue(false)
    { }
    FontPlatformDataPrivate(const float size, const bool bold, const bool oblique)
        : size(size)
        , bold(bold)
        , oblique(oblique)
        , isDeletedValue(false)
    { }
    FontPlatformDataPrivate(const QFont& font)
        : font(font)
#if HAVE(QRAWFONT)
        , rawFont(QRawFont::fromFont(font, QFontDatabase::Any))
#endif
        , size(font.pixelSize())
        , bold(font.bold())
        , oblique(false)
        , isDeletedValue(false)
    { }
#if HAVE(QRAWFONT)
    FontPlatformDataPrivate(const QRawFont& rawFont)
        : font()
        , rawFont(rawFont)
        , size(rawFont.pixelSize())
        , bold(rawFont.weight() >= QFont::Bold)
        , oblique(false)
        , isDeletedValue(false)
    { }
#endif
    FontPlatformDataPrivate(WTF::HashTableDeletedValueType)
        : isDeletedValue(true)
    { }

    QFont font;
#if HAVE(QRAWFONT)
    QRawFont rawFont;
#endif
    float size;
    bool bold : 1;
    bool oblique : 1;
    bool isDeletedValue : 1;
};

class FontPlatformData {
    WTF_MAKE_FAST_ALLOCATED;
public:
    FontPlatformData(float size, bool bold, bool oblique);
    FontPlatformData(const FontDescription&, const AtomicString& familyName, int wordSpacing = 0, int letterSpacing = 0);
    FontPlatformData(const QFont& font)
        : m_data(adoptRef(new FontPlatformDataPrivate(font)))
    { }
#if HAVE(QRAWFONT)
    FontPlatformData(const FontPlatformData&, float size);
    FontPlatformData(const QRawFont& rawFont)
        : m_data(adoptRef(new FontPlatformDataPrivate(rawFont)))
    { }
#endif
    FontPlatformData(WTF::HashTableDeletedValueType)
        : m_data(adoptRef(new FontPlatformDataPrivate()))
    {
        m_data->isDeletedValue = true;
    }

    bool operator==(const FontPlatformData&) const;

    bool isHashTableDeletedValue() const
    {
        return m_data && m_data->isDeletedValue;
    }

    QFont font() const
    {
        Q_ASSERT(!isHashTableDeletedValue());
        if (!m_data)
            return QFont();
        return m_data->font;
    }
#if HAVE(QRAWFONT)
    QRawFont rawFont() const
    {
        Q_ASSERT(!isHashTableDeletedValue());
        if (!m_data)
            return QRawFont();
        return m_data->rawFont;
    }
#endif
    float size() const
    {
        Q_ASSERT(!isHashTableDeletedValue());
        if (!m_data)
            return 0;
        return m_data->size;
    }
    QString family() const
    {
        Q_ASSERT(!isHashTableDeletedValue());
        if (!m_data)
            return QString();
        return m_data->font.family();
    }
    bool bold() const
    {
        Q_ASSERT(!isHashTableDeletedValue());
        if (!m_data)
            return false;
        return m_data->bold;
    }
    bool italic() const
    {
        Q_ASSERT(!isHashTableDeletedValue());
        if (!m_data)
            return false;
        return m_data->font.italic();
    }
    bool smallCaps() const
    {
        Q_ASSERT(!isHashTableDeletedValue());
        if (!m_data)
            return false;
        return m_data->font.capitalization() == QFont::SmallCaps;
    }
    
    FontOrientation orientation() const { return Horizontal; } // FIXME: Implement.
    void setOrientation(FontOrientation) { } // FIXME: Implement.

    unsigned hash() const;

#ifndef NDEBUG
    String description() const;
#endif
private:
    RefPtr<FontPlatformDataPrivate> m_data;
};

} // namespace WebCore

#endif // FontPlatformData_h
