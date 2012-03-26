/*
 * Copyright (C) 2011 Andreas Kling <kling@webkit.org>
 *
 * All rights reserved.
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
#include "IconDatabaseClientQt.h"

#include "FrameLoaderClientQt.h"
#include "IconDatabaseBase.h"
#include <wtf/text/CString.h>

namespace WebCore {

IconDatabaseClientQt* IconDatabaseClientQt::instance()
{
    static IconDatabaseClientQt* client = 0;
    if (!client) {
        client = new IconDatabaseClientQt;
        iconDatabase().setClient(client);
    }
    return client;
}

IconDatabaseClientQt::IconDatabaseClientQt()
{
}

IconDatabaseClientQt::~IconDatabaseClientQt()
{
}

bool IconDatabaseClientQt::performImport()
{
    return true;
}

void IconDatabaseClientQt::didRemoveAllIcons()
{
}

void IconDatabaseClientQt::didImportIconURLForPageURL(const String& url)
{
}

void IconDatabaseClientQt::didImportIconDataForPageURL(const String& url)
{
    emit iconLoadedForPageURL(url);
}

void IconDatabaseClientQt::didChangeIconForPageURL(const String& url)
{
}

void IconDatabaseClientQt::didFinishURLImport()
{
}

}

#include "moc_IconDatabaseClientQt.cpp"
