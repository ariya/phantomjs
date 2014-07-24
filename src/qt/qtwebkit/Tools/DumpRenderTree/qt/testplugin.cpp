/*
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
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
#include "testplugin.h"

TestPlugin::TestPlugin(QObject *parent)
    : QWebPluginFactory(parent)
{
}

TestPlugin::~TestPlugin()
{
}

QList<QWebPluginFactory::Plugin> TestPlugin::plugins() const
{
    QWebPluginFactory::Plugin plugin;

    plugin.name = "testplugin";
    plugin.description = "testdescription";
    MimeType mimeType;
    mimeType.name = "testtype";
    mimeType.fileExtensions.append("testsuffixes");
    plugin.mimeTypes.append(mimeType);

    plugin.name = "testplugin2";
    plugin.description = "testdescription2";
    mimeType.name = "testtype2";
    mimeType.fileExtensions.append("testsuffixes2");
    mimeType.fileExtensions.append("testsuffixes3");
    plugin.mimeTypes.append(mimeType);

    return QList<QWebPluginFactory::Plugin>() << plugin;
}

QObject *TestPlugin::create(const QString&,
                            const QUrl&,
                            const QStringList&,
                            const QStringList&) const
{
    return 0;
}

