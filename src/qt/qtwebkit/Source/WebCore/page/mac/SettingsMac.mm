/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "Settings.h"

namespace WebCore {

void Settings::initializeDefaultFontFamilies()
{
#if !PLATFORM(IOS)
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    setStandardFontFamily("Songti TC", USCRIPT_TRADITIONAL_HAN);
    setSerifFontFamily("Songti TC", USCRIPT_TRADITIONAL_HAN);
#else
    setStandardFontFamily("Apple LiSung", USCRIPT_TRADITIONAL_HAN);
    setSerifFontFamily("Apple LiSung", USCRIPT_TRADITIONAL_HAN);
#endif
#else
    // There is no serif Chinese font in default iOS installation.
    setStandardFontFamily("Heiti TC", USCRIPT_TRADITIONAL_HAN);
    setSerifFontFamily("Heiti TC", USCRIPT_TRADITIONAL_HAN);
#endif
    setFixedFontFamily("Heiti TC", USCRIPT_TRADITIONAL_HAN);
    setSansSerifFontFamily("Heiti TC", USCRIPT_TRADITIONAL_HAN);

#if !PLATFORM(IOS)
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    setStandardFontFamily("Songti SC", USCRIPT_SIMPLIFIED_HAN);
    setSerifFontFamily("Songti SC", USCRIPT_SIMPLIFIED_HAN);
#else
    setStandardFontFamily("STSong", USCRIPT_SIMPLIFIED_HAN);
    setSerifFontFamily("STSong", USCRIPT_SIMPLIFIED_HAN);
#endif
#else
    // There is no serif Chinese font in default iOS installation.
    setStandardFontFamily("Heiti SC", USCRIPT_SIMPLIFIED_HAN);
    setSerifFontFamily("Heiti SC", USCRIPT_SIMPLIFIED_HAN);
#endif
    setFixedFontFamily("Heiti SC", USCRIPT_SIMPLIFIED_HAN);
    setSansSerifFontFamily("Heiti SC", USCRIPT_SIMPLIFIED_HAN);

    setStandardFontFamily("Hiragino Mincho ProN", USCRIPT_KATAKANA_OR_HIRAGANA);
#if !PLATFORM(IOS)
    setFixedFontFamily("Osaka-Mono", USCRIPT_KATAKANA_OR_HIRAGANA);
#else
    setFixedFontFamily("Hiragino Kaku Gothic ProN", USCRIPT_KATAKANA_OR_HIRAGANA);
#endif
    setSerifFontFamily("Hiragino Mincho ProN", USCRIPT_KATAKANA_OR_HIRAGANA);
    setSansSerifFontFamily("Hiragino Kaku Gothic ProN", USCRIPT_KATAKANA_OR_HIRAGANA);

#if !PLATFORM(IOS)
    setStandardFontFamily("AppleMyungjo", USCRIPT_HANGUL);
    setSerifFontFamily("AppleMyungjo", USCRIPT_HANGUL);
#else
    // There is no serif Korean font in default iOS installation.
    setStandardFontFamily("Apple SD Gothic Neo", USCRIPT_HANGUL);
    setSerifFontFamily("Apple SD Gothic Neo", USCRIPT_HANGUL);
#endif
#if PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
    setFixedFontFamily("Apple SD Gothic Neo", USCRIPT_HANGUL);
    setSansSerifFontFamily("Apple SD Gothic Neo", USCRIPT_HANGUL);
#else
    setFixedFontFamily("AppleGothic", USCRIPT_HANGUL);
    setSansSerifFontFamily("AppleGothic", USCRIPT_HANGUL);
#endif

    setStandardFontFamily("Times", USCRIPT_COMMON);
    setFixedFontFamily("Courier", USCRIPT_COMMON);
    setSerifFontFamily("Times", USCRIPT_COMMON);
    setSansSerifFontFamily("Helvetica", USCRIPT_COMMON);
}


} // namespace WebCore
