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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebPreferences.h"
#include <QFont>

namespace WebKit {

static void setStringValueIfInUserDefaults(const String& baseKey, WebPreferencesStore& store, const QHash<String, QFont::StyleHint> qFontHint)
{
    // Use same approach as DumpRenderTree does - get font info from QFont.
    static QFont defaultFont;
    defaultFont.setStyleHint(qFontHint[baseKey]);
    store.setStringValueForKey(baseKey, defaultFont.defaultFamily());
}

void WebPreferences::platformInitializeStore()
{
    if (!m_identifier)
        return;

    QHash <String, QFont::StyleHint> qFontHint;

    qFontHint["StandardFontFamily"] = QFont::Serif;
    qFontHint["CursiveFontFamily"] = QFont::Cursive;
    qFontHint["FantasyFontFamily"] = QFont::Fantasy;
    qFontHint["FixedFontFamily"] = QFont::Monospace;
    qFontHint["SansSerifFontFamily"] = QFont::SansSerif;
    qFontHint["SerifFontFamily"] = QFont::Serif;
    qFontHint["PictographFontFamily"] = QFont::Serif;

#define INITIALIZE_FONT_PREFERENCES(KeyUpper, KeyLower, TypeName, Type, DefaultValue) \
    set##TypeName##ValueIfInUserDefaults(WebPreferencesKey::KeyLower##Key(), m_store, qFontHint);

    FOR_EACH_WEBKIT_FONT_FAMILY_PREFERENCE(INITIALIZE_FONT_PREFERENCES)

#undef INITIALIZE_FONT_PREFERENCES
}

void WebPreferences::platformUpdateStringValueForKey(const String&, const String&)
{
}

void WebPreferences::platformUpdateBoolValueForKey(const String&, bool)
{
}

void WebPreferences::platformUpdateUInt32ValueForKey(const String&, uint32_t)
{
}

void WebPreferences::platformUpdateDoubleValueForKey(const String&, double)
{
}

void WebPreferences::platformUpdateFloatValueForKey(const String&, float)
{
}

} // namespace WebKit
