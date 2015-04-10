/*
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2012 University of Szeged. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "QtTestSupport.h"

#include "CrossOriginPreflightResultCache.h"
#include "FontCache.h"
#include "MemoryCache.h"
#include "PageCache.h"
#include <QFontDatabase>

#if HAVE(FONTCONFIG)
#include <QByteArray>
#include <QDir>
#include <fontconfig/fontconfig.h>
#endif

using namespace WebCore;

namespace WebKit {

void QtTestSupport::clearMemoryCaches()
{
    if (!memoryCache()->disabled()) {
        memoryCache()->setDisabled(true);
        memoryCache()->setDisabled(false);
    }

    int pageCapacity = WebCore::pageCache()->capacity();
    WebCore::pageCache()->setCapacity(0);
    WebCore::pageCache()->setCapacity(pageCapacity);

    WebCore::fontCache()->invalidate();

    WebCore::CrossOriginPreflightResultCache::shared().empty();
}

void QtTestSupport::initializeTestFonts()
{
#if HAVE(FONTCONFIG)
    static int numFonts = -1;

    FcInit();

    // Some test cases may add or remove application fonts (via @font-face).
    // Make sure to re-initialize the font set if necessary.
    FcFontSet* appFontSet = FcConfigGetFonts(0, FcSetApplication);
    if (appFontSet && numFonts >= 0 && appFontSet->nfont == numFonts)
        return;

    QByteArray fontDir = getenv("WEBKIT_TESTFONTS");
    if (fontDir.isEmpty() || !QDir(QString::fromLatin1(fontDir)).exists()) {
        qFatal("\n\n"
            "----------------------------------------------------------------------\n"
            "WEBKIT_TESTFONTS environment variable is not set correctly.\n"
            "This variable has to point to the directory containing the fonts\n"
            "you can clone from git://gitorious.org/qtwebkit/testfonts.git\n"
            "----------------------------------------------------------------------\n"
            );
    }

    QByteArray configFile = fontDir + "/fonts.conf";
    FcConfig* config = FcConfigCreate();
    if (!FcConfigParseAndLoad(config, reinterpret_cast<const FcChar8*>(configFile.constData()), FcTrue))
        qFatal("Couldn't load font configuration file");
    if (!FcConfigAppFontAddDir(config, reinterpret_cast<const FcChar8*>(fontDir.data())))
        qFatal("Couldn't add font dir!");
    FcConfigSetCurrent(config);

    appFontSet = FcConfigGetFonts(config, FcSetApplication);
    numFonts = appFontSet->nfont;

    WebCore::fontCache()->invalidate();
    QFontDatabase::removeAllApplicationFonts();
#endif
}

}
