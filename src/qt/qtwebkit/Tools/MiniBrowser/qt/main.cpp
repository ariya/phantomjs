/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2010 University of Szeged
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

#include "BrowserWindow.h"

#include "MiniBrowserApplication.h"
#include "UrlLoader.h"

#include <qqml.h>

#include <QDir>
#include <QLatin1String>
#include <QRegExp>

int main(int argc, char** argv)
{
    MiniBrowserApplication app(argc, argv);

    if (app.isRobotized()) {
        BrowserWindow* window = new BrowserWindow(app.windowOptions());
        UrlLoader loader(window, app.urls().at(0), app.robotTimeout(), app.robotExtraTime());
        loader.loadNext();
        window->show();
        return app.exec();
    }

    QStringList urls = app.urls();

    if (urls.isEmpty()) {
        QString defaultIndexFile = QString("%1/%2").arg(QDir::homePath()).arg(QLatin1String("index.html"));
        if (QFile(defaultIndexFile).exists())
            urls.append(QString("file://") + defaultIndexFile);
        else
            urls.append("http://www.google.com");
    }

    BrowserWindow* window = new BrowserWindow(app.windowOptions());
    window->load(urls.at(0));

    for (int i = 1; i < urls.size(); ++i)
        window->newWindow(urls.at(i));

    app.exec();

    return 0;
}
