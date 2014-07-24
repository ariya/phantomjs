/*
 * Copyright (C) 2011 ProFUSION Embedded Systems
 * Copyright (C) 2011 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FontManagement.h"

#include <Ecore_File.h>
#include <cstdio>
#include <fontconfig/fontconfig.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

static CString buildPath(const char* base, const char* first, ...)
{
    va_list ap;
    StringBuilder result;
    result.append(base);

    if (const char* current = first) {
        va_start(ap, first);
        do {
            result.append('/');
            result.append(current);
        } while ((current = va_arg(ap, const char*)));
        va_end(ap);
    }

    return result.toString().utf8();
}

static Vector<CString> getCoreFontFiles()
{
    Vector<CString> fontFilePaths;

    // Ahem is used by many layout tests.
    fontFilePaths.append(CString(FONTS_CONF_DIR "/AHEM____.TTF"));
    // A font with no valid Fontconfig encoding to test https://bugs.webkit.org/show_bug.cgi?id=47452
    fontFilePaths.append(CString(FONTS_CONF_DIR "/FontWithNoValidEncoding.fon"));

    for (int i = 1; i <= 9; i++) {
        char fontPath[EINA_PATH_MAX];
        snprintf(fontPath, EINA_PATH_MAX - 1, FONTS_CONF_DIR "/../../fonts/WebKitWeightWatcher%i00.ttf", i);
        fontFilePaths.append(CString(fontPath));
    }

    return fontFilePaths;
}

static void addFontDirectory(const CString& fontDirectory, FcConfig* config)
{
    const char* fontPath = fontDirectory.data();
    if (!fontPath || !FcConfigAppFontAddDir(config, reinterpret_cast<const FcChar8*>(fontPath)))
        fprintf(stderr, "Could not add font directory %s!\n", fontPath);
}

static void addFontFiles(const Vector<CString>& fontFiles, FcConfig* config)
{
    Vector<CString>::const_iterator it, end = fontFiles.end();
    for (it = fontFiles.begin(); it != end; ++it) {
        const char* filePath = (*it).data();
        if (!FcConfigAppFontAddFile(config, reinterpret_cast<const FcChar8*>(filePath)))
            fprintf(stderr, "Could not load font at %s!\n", filePath);
    }
}

static CString getCustomBuildDir()
{
    if (const char* userChosenBuildDir = getenv("WEBKIT_OUTPUTDIR")) {
        if (ecore_file_is_dir(userChosenBuildDir))
            return userChosenBuildDir;
        fprintf(stderr, "WEBKIT_OUTPUTDIR set to '%s', but path doesn't exist.\n", userChosenBuildDir);
    }

    return CString();
}

static CString getPlatformFontsPath()
{
    CString customBuildDir = getCustomBuildDir();
    if (!customBuildDir.isNull()) {
        CString fontsPath = buildPath(customBuildDir.data(), "Dependencies", "Root", "webkitgtk-test-fonts", 0);
        if (!ecore_file_exists(fontsPath.data()))
            fprintf(stderr, "WEBKIT_OUTPUTDIR set to '%s', but could not local test fonts.\n", customBuildDir.data());
        return fontsPath;
    }

    CString fontsPath = CString(DOWNLOADED_FONTS_DIR);
    if (ecore_file_exists(fontsPath.data()))
        return fontsPath;

    fprintf(stderr, "Could not locate tests fonts, try setting WEBKIT_OUTPUTDIR.\n");
    return CString();
}

void addFontsToEnvironment()
{
    FcInit();

    // Load our configuration file, which sets up proper aliases for family
    // names like sans, serif and monospace.
    FcConfig* config = FcConfigCreate();
    const char* fontConfigFilename = FONTS_CONF_DIR "/fonts.conf";
    if (!FcConfigParseAndLoad(config, reinterpret_cast<const FcChar8*>(fontConfigFilename), true)) {
        fprintf(stderr, "Couldn't load font configuration file from: %s\n", fontConfigFilename);
        exit(1);
    }

    addFontFiles(getCoreFontFiles(), config);
    addFontDirectory(getPlatformFontsPath(), config);

    if (!FcConfigSetCurrent(config)) {
        fprintf(stderr, "Could not set the current font configuration!\n");
        exit(1);
    }
}

