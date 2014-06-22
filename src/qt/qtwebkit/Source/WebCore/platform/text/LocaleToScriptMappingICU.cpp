/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "LocaleToScriptMapping.h"

#include <unicode/uloc.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

// Treat certain families of script codes as a single script for assigning a per-script font in Settings.
static UScriptCode scriptCodeForFontSelection(UScriptCode scriptCode)
{
    switch (scriptCode) {
    case USCRIPT_HIRAGANA:
    case USCRIPT_KATAKANA:
    case USCRIPT_JAPANESE:
        return USCRIPT_KATAKANA_OR_HIRAGANA;
    case USCRIPT_KOREAN:
        return USCRIPT_HANGUL;
    default:
        return scriptCode;
    }
}

UScriptCode localeToScriptCodeForFontSelection(const String& locale)
{
    if (locale.isEmpty())
        return USCRIPT_COMMON;

    char maximizedLocale[ULOC_FULLNAME_CAPACITY];
    UErrorCode status = U_ZERO_ERROR;
    uloc_addLikelySubtags(locale.utf8().data(), maximizedLocale, sizeof(maximizedLocale), &status);
    if (U_FAILURE(status))
        return USCRIPT_COMMON;

    char script[ULOC_SCRIPT_CAPACITY];
    uloc_getScript(maximizedLocale, script, sizeof(script), &status);
    if (U_FAILURE(status))
        return USCRIPT_COMMON;

    UScriptCode scriptCode = USCRIPT_COMMON;
    uscript_getCode(script, &scriptCode, 1, &status);
    // Ignore error that multiple scripts could be returned, since we only want one script.
    if (U_FAILURE(status) && status != U_BUFFER_OVERFLOW_ERROR)
        return USCRIPT_COMMON;

    return scriptCodeForFontSelection(scriptCode);
}

UScriptCode scriptNameToCode(const String& name)
{
    int32_t code = u_getPropertyValueEnum(UCHAR_SCRIPT, name.utf8().data());
    if (code >= 0 && code < USCRIPT_CODE_LIMIT)
        return static_cast<UScriptCode>(code);
    return USCRIPT_INVALID_CODE;
}

} // namespace WebCore
