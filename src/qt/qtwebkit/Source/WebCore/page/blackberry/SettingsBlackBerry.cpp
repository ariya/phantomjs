/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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
#include "Settings.h"

#include "LocaleToScriptMapping.h"

#include <BlackBerryPlatformFontInfo.h>

namespace WebCore {

void Settings::initializeDefaultFontFamilies()
{
    static std::vector<BlackBerry::Platform::String> languages;
    static bool init = false;
    if (!init) {
        languages = BlackBerry::Platform::FontInfo::instance()->languagesWithFonts();
        init = true;
    }

    STATIC_LOCAL_STRING(s_webkitCursive, "-webkit-cursive");
    STATIC_LOCAL_STRING(s_webkitFantasy, "-webkit-fantasy");
    STATIC_LOCAL_STRING(s_webkitMonospace, "-webkit-monospace");
    STATIC_LOCAL_STRING(s_webkitSansSerif, "-webkit-sans-serif");
    STATIC_LOCAL_STRING(s_webkitSerif, "-webkit-serif");
    STATIC_LOCAL_STRING(s_webkitStandard, "-webkit-standard");

    setCursiveFontFamily(BlackBerry::Platform::FontInfo::instance()->fontFamily(s_webkitCursive, BlackBerry::Platform::String::emptyString()));
    setFantasyFontFamily(BlackBerry::Platform::FontInfo::instance()->fontFamily(s_webkitFantasy, BlackBerry::Platform::String::emptyString()));
    setFixedFontFamily(BlackBerry::Platform::FontInfo::instance()->fontFamily(s_webkitMonospace, BlackBerry::Platform::String::emptyString()));
    setSansSerifFontFamily(BlackBerry::Platform::FontInfo::instance()->fontFamily(s_webkitSansSerif, BlackBerry::Platform::String::emptyString()));
    setSerifFontFamily(BlackBerry::Platform::FontInfo::instance()->fontFamily(s_webkitSerif, BlackBerry::Platform::String::emptyString()));
    setStandardFontFamily(BlackBerry::Platform::FontInfo::instance()->fontFamily(s_webkitStandard, BlackBerry::Platform::String::emptyString()));

    STATIC_LOCAL_STRING(s_monospace, "monospace");
    STATIC_LOCAL_STRING(s_serif, "serif");
    STATIC_LOCAL_STRING(s_sansSerif, "sans-serif");
    for (size_t i = 0; i < languages.size(); ++i) {
        UScriptCode script = localeToScriptCodeForFontSelection(languages[i]);
        setFixedFontFamily(BlackBerry::Platform::FontInfo::instance()->fontFamily(s_monospace, languages[i]), script);
        setSansSerifFontFamily(BlackBerry::Platform::FontInfo::instance()->fontFamily(s_sansSerif, languages[i]), script);
        setSerifFontFamily(BlackBerry::Platform::FontInfo::instance()->fontFamily(s_serif, languages[i]), script);
        setStandardFontFamily(BlackBerry::Platform::FontInfo::instance()->fontFamily(BlackBerry::Platform::String::emptyString(), languages[i]), script);
    }
}

} // namespace WebCore
