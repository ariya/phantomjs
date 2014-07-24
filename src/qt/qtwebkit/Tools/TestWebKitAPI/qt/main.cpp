/*
 * Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "TestsController.h"

#include "qquickwebview_p.h"
#include <QGuiApplication>

void addQtWebProcessToPath()
{
    // Since tests won't find ./QtWebProcess, add it to PATH (at the end to prevent surprises).
    // ROOT_BUILD_DIR should be defined by qmake.
    qputenv("PATH", qgetenv("PATH") + QByteArray(":" ROOT_BUILD_DIR "/bin"));
}

void messageHandler(QtMsgType type, const QMessageLogContext&, const QString& message)
{
    if (type == QtCriticalMsg) {
        fprintf(stderr, "%s\n", qPrintable(message));
        return;
    }

    // Do nothing
}

int main(int argc, char** argv)
{
    bool suppressQtDebugOutput = true; // Suppress debug output from Qt if not started with --verbose.
    bool useDesktopBehavior = true; // Use traditional desktop behavior if not started with --flickable.

    for (int i = 1; i < argc; ++i) {
        if (!qstrcmp(argv[i], "--verbose"))
            suppressQtDebugOutput = false;
        else if (!qstrcmp(argv[i], "--flickable"))
            useDesktopBehavior = false;
    }

    QQuickWebViewExperimental::setFlickableViewportEnabled(!useDesktopBehavior);

    // Has to be done before QApplication is constructed in case
    // QApplication itself produces debug output.
    if (suppressQtDebugOutput) {
        qInstallMessageHandler(messageHandler);
        if (qgetenv("QT_WEBKIT_SUPPRESS_WEB_PROCESS_OUTPUT").isEmpty())
            qputenv("QT_WEBKIT_SUPPRESS_WEB_PROCESS_OUTPUT", "1");
    }

    QGuiApplication app(argc, argv);
    addQtWebProcessToPath();

    return TestWebKitAPI::TestsController::shared().run(argc, argv) ? EXIT_SUCCESS : EXIT_FAILURE;
}
