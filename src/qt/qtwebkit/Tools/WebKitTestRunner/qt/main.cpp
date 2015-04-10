/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 University of Szeged.
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

#include "QtTestSupport.h"
#include "TestController.h"
#include "qquickwebview_p.h"

#include <stdio.h>
#if !defined(NDEBUG) && defined(Q_OS_UNIX)
#include <signal.h>
#include <unistd.h>
#endif

#include <QApplication>
#include <QObject>
#include <QTimer>

class Launcher : public QObject {
    Q_OBJECT

public:
    Launcher(int argc, char** argv)
        : m_argc(argc)
        , m_argv(argv)
    {
    }

    ~Launcher()
    {
        delete m_controller;
    }

public Q_SLOTS:
    void launch()
    {
        m_controller = new WTR::TestController(m_argc, const_cast<const char**>(m_argv));
        QApplication::exit();
    }

private:
    WTR::TestController* m_controller;
    int m_argc;
    char** m_argv;
};

#if !defined(NDEBUG) && defined(Q_OS_UNIX)
static void sigcontHandler(int)
{
}
#endif

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
#if !defined(NDEBUG) && defined(Q_OS_UNIX)
    if (qgetenv("QT_WEBKIT_PAUSE_UI_PROCESS") == "1") {
        struct sigaction newAction, oldAction;
        newAction.sa_handler = sigcontHandler;
        sigemptyset(&newAction.sa_mask);
        newAction.sa_flags = 0;
        sigaction(SIGCONT, &newAction, &oldAction);
        fprintf(stderr, "Pausing UI process, please attach to PID %d and send signal SIGCONT... ", getpid());
        pause();
        sigaction(SIGCONT, &oldAction, 0);
        fprintf(stderr, " OK\n");
    }
#endif

    // Suppress debug output from Qt if not started with --verbose
    bool suppressQtDebugOutput = true;
    for (int i = 1; i < argc; ++i) {
        if (!qstrcmp(argv[i], "--verbose")) {
            suppressQtDebugOutput = false;
            break;
        }
    }

    // Has to be done before QApplication is constructed in case
    // QApplication itself produces debug output.
    if (suppressQtDebugOutput) {
        qInstallMessageHandler(messageHandler);
        if (qgetenv("QT_WEBKIT_SUPPRESS_WEB_PROCESS_OUTPUT").isEmpty())
            qputenv("QT_WEBKIT_SUPPRESS_WEB_PROCESS_OUTPUT", "1");
    }

    qputenv("QT_WEBKIT_THEME_NAME", "qstyle");

    WebKit::QtTestSupport::initializeTestFonts();
    QCoreApplication::setAttribute(Qt::AA_Use96Dpi, true);

    QApplication app(argc, argv);
    Launcher launcher(argc, argv);
    QTimer::singleShot(0, &launcher, SLOT(launch()));
    return app.exec();;
}

#include "main.moc"
