/*
 * Copyright (C) 2010, 2011 Nokia Inc. All rights reserved.
 * Copyright (C) 2011 University of Szeged. All rights reserved.
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

#if ENABLE(PLUGIN_PROCESS)

#include "NetscapePluginModule.h"
#include "PluginProcess.h"
#include "WebKit2Initialize.h"
#include <QDebug>
#include <QGuiApplication>
#include <QStringList>
#include <QtGlobal>
#include <WebCore/RunLoop.h>

using namespace WebCore;

namespace WebKit {

static void messageHandler(QtMsgType type, const QMessageLogContext&, const QString& message)
{
    if (type == QtCriticalMsg) {
        fprintf(stderr, "%s\n", qPrintable(message));
        return;
    }

    // Do nothing
}

static bool initializeGtk()
{
    QLibrary gtkLibrary(QLatin1String("libgtk-x11-2.0"), 0);
    if (!gtkLibrary.load())
        return false;
    typedef void* (*gtk_init_ptr)(void*, void*);
    gtk_init_ptr gtkInit = reinterpret_cast<gtk_init_ptr>(gtkLibrary.resolve("gtk_init"));
    if (!gtkInit)
        return false;
    gtkInit(0, 0);
    return true;
}

Q_DECL_EXPORT int PluginProcessMain(int argc, char** argv)
{
    QByteArray suppressOutput = qgetenv("QT_WEBKIT_SUPPRESS_WEB_PROCESS_OUTPUT");
    if (!suppressOutput.isEmpty() && suppressOutput != "0")
        qInstallMessageHandler(messageHandler);

    QGuiApplication app(argc, argv);

    // Workaround the issue that some versions of flash does not initialize Gtk properly.
    if (!initializeGtk())
        return EXIT_FAILURE;

    InitializeWebKit2();

    if (argc <= 1)
        return EXIT_FAILURE;

    if (app.arguments().at(1) == QLatin1String("-scanPlugin")) {
        if (argc != 3)
            return EXIT_FAILURE;
        String pluginPath(app.arguments().at(2));
        if (!NetscapePluginModule::scanPlugin(pluginPath))
            return EXIT_FAILURE;
        return EXIT_SUCCESS;
    }

    // Create the connection.
    bool isNumber = false;
    int identifier = app.arguments().at(1).toInt(&isNumber, 10);
    if (!isNumber)
        return EXIT_FAILURE;

    WebKit::ChildProcessInitializationParameters parameters;
    parameters.connectionIdentifier = identifier;
    parameters.extraInitializationData.add("plugin-path", app.arguments().at(2));

    WebKit::PluginProcess::shared().initialize(parameters);

    RunLoop::run();

    return 0;
}

}

#endif // ENABLE(PLUGIN_PROCESS)
