/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MIMETypeRegistry.h"

#include <QMimeDatabase>
#include <wtf/Assertions.h>
#include <wtf/MainThread.h>

namespace WebCore {

struct ExtensionMap {
    const char* extension;
    const char* dotExtension;
    const char* mimeType;
};

// This is a short list of extensions that are either not recognized by the freedesktop shared mimetype database 1.0,
// or too essential for QtWebKit that we can not allow a user configuration to potentially override it.
// Any extension that has to be recognized for the layout-tests to run should be added here.
static const ExtensionMap extensionMap[] = {
    { "htm", ".htm", "text/html" },
    { "html", ".html", "text/html" },
    { "js", ".js", "application/javascript" },
    { "mht", ".mht", "application/x-mimearchive" }, // Not in shared mimetype database
    { "mhtml", ".mhtml", "application/x-mimearchive" }, // Not in shared mimetype database
    { "svg", ".svg", "image/svg+xml" },
    { "text", ".text", "text/plain" }, // Not in shared mimetype database
    { "txt", ".txt", "text/plain"},
    { "wmlc", ".wmlc", "application/vnd.wap.wmlc" }, // Not in shared mimetype database
    { "xht", ".xht", "application/xhtml+xml" },
    { "xhtml", ".xhtml", "application/xhtml+xml" },
    { "xsl", ".xsl", "text/xsl" },
    { 0, 0, 0 }
};

String MIMETypeRegistry::getMIMETypeForExtension(const String &ext)
{
    String suffix = ext.lower();
    const ExtensionMap *e = extensionMap;
    while (e->extension) {
        if (suffix == e->extension)
            return e->mimeType;
        ++e;
    }

    // QMimeDatabase lacks the ability to query by extension alone, so we create a fake filename to lookup.
    const QString filename = QStringLiteral("filename.") + QString(suffix);

    // FIXME: We should get all the matched mimetypes with mimeTypesForFileName, and prefer one we support.
    // But initializeSupportedImageMIMETypes will first have to stop using getMIMETypeForExtension, or we
    // would be checking against an uninitialized set of supported mimetypes.
    QMimeType mimeType = QMimeDatabase().mimeTypeForFile(filename, QMimeDatabase::MatchExtension);
    if (mimeType.isValid() && !mimeType.isDefault()) {
        // getMIMETypeForExtension is used for preload mimetype check, so image looking files can not be loaded as anything but images.
        // Script looking files (.php) are loaded normally and will have their mimetype determined later.
        if (mimeType.inherits(QStringLiteral("application/x-executable")))
            return String();
        return mimeType.name();
    }

    return String();
}

String MIMETypeRegistry::getMIMETypeForPath(const String& path)
{
    const ExtensionMap *e = extensionMap;
    while (e->extension) {
        if (path.endsWith(e->dotExtension, /* caseSensitive */ false))
            return e->mimeType;
        ++e;
    }

    // FIXME: See comment in getMIMETypeForExtension.
    QMimeType type = QMimeDatabase().mimeTypeForFile(path, QMimeDatabase::MatchExtension);
    if (type.isValid() && !type.isDefault())
        return type.name();

    return defaultMIMEType();
}

Vector<String> MIMETypeRegistry::getExtensionsForMIMEType(const String& mimeTypeName)
{
    Vector<String> extensions;
    QMimeType mimeType = QMimeDatabase().mimeTypeForName(mimeTypeName);
    if (mimeType.isValid() && !mimeType.isDefault()) {
        Q_FOREACH(const QString& suffix, mimeType.suffixes()) {
            extensions.append(suffix);
        }
    }

    return extensions;
}

String MIMETypeRegistry::getPreferredExtensionForMIMEType(const String& mimeTypeName)
{
    QMimeType mimeType = QMimeDatabase().mimeTypeForName(mimeTypeName);
    if (mimeType.isValid() && !mimeType.isDefault())
        return mimeType.preferredSuffix();

    return String();
}

String MIMETypeRegistry::getNormalizedMIMEType(const String& mimeTypeName)
{
    // This looks up the mime type object by preferred name or alias, and returns the preferred name.
    QMimeType mimeType = QMimeDatabase().mimeTypeForName(mimeTypeName);
    if (mimeType.isValid() && !mimeType.isDefault())
        return mimeType.name();

    return mimeTypeName;
}

bool MIMETypeRegistry::isApplicationPluginMIMEType(const String& mimeType)
{
    return mimeType.startsWith("application/x-qt-plugin", false)
        || mimeType.startsWith("application/x-qt-styled-widget", false);
}

}
