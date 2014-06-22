/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
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

#include "DumpRenderTreeQt.h"

#include "QtTestSupport.h"
#include <qapplication.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfont.h>
#include <qstringlist.h>
#include <qstylefactory.h>
#include <qtimer.h>
#include <qurl.h>
#include <qwebdatabase.h>

#ifdef Q_WS_X11
#include <qx11info_x11.h>
#endif

#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#endif

#include <limits.h>

#include <wtf/Assertions.h>

void messageHandler(QtMsgType type, const QMessageLogContext&, const QString &message)
{
    if (type == QtCriticalMsg || type == QtFatalMsg) {
        fprintf(stderr, "%s\n", qPrintable(message));
        return;
    }
    // do nothing
}

// We only support -v, -p, --pixel-tests, --stdout, --stderr and -, all the others will be
// pass as test case name (even -abc.html is a valid test case name)
bool isOption(const QString& str)
{
    return str == QString("-v") || str == QString("-p") || str == QString("--pixel-tests")
           || str == QString("--stdout") || str == QString("--stderr")
           || str == QString("--timeout") || str == QString("--no-timeout")
           || str == QString("-");
}

QString takeOptionValue(QStringList& arguments, int index)
{
    QString result;

    if (index + 1 < arguments.count() && !isOption(arguments.at(index + 1)))
        result = arguments.takeAt(index + 1);
    arguments.removeAt(index);

    return result;
}

void printUsage()
{
    fprintf(stderr, "Usage: DumpRenderTree [-v|-p|--pixel-tests] [--stdout output_filename] [-stderr error_filename] [--no-timeout] [--timeout timeout_MS] filename [filename2..n]\n");
    fprintf(stderr, "Or folder containing test files: DumpRenderTree [-v|--pixel-tests] dirpath\n");
    fflush(stderr);
}

int main(int argc, char* argv[])
{
#ifdef Q_OS_WIN
    _setmode(1, _O_BINARY);
    _setmode(2, _O_BINARY);
#endif

    // Suppress debug output from Qt if not started with -v
    bool suppressQtDebugOutput = true;
    for (int i = 1; i < argc; ++i) {
        if (!qstrcmp(argv[i], "-v")) {
            suppressQtDebugOutput = false;
            break;
        }
    }

    // Has to be done before QApplication is constructed in case
    // QApplication itself produces debug output.
    if (suppressQtDebugOutput)
        qInstallMessageHandler(messageHandler);

    WebKit::QtTestSupport::initializeTestFonts();

    QApplication::setStyle(QStyleFactory::create(QLatin1String("windows")));
    QApplication::setDesktopSettingsAware(false);

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    QCoreApplication::setAttribute(Qt::AA_Use96Dpi, true);

    WTFInstallReportBacktraceOnCrashHook();

    QStringList args = app.arguments();
    if (args.count() < (!suppressQtDebugOutput ? 3 : 2)) {
        printUsage();
        exit(1);
    }

    // Remove the first arguments, it is application name itself
    args.removeAt(0);

    DumpRenderTree dumper;

    int index = args.indexOf(QLatin1String("--stdout"));
    if (index != -1) {
        QString fileName = takeOptionValue(args, index);
        dumper.setRedirectOutputFileName(fileName);
        if (fileName.isEmpty() || !freopen(qPrintable(fileName), "w", stdout)) {
            fprintf(stderr, "STDOUT redirection failed.");
            exit(1);
        }
    }
    index = args.indexOf(QLatin1String("--stderr"));
    if (index != -1) {
        QString fileName = takeOptionValue(args, index);
        dumper.setRedirectErrorFileName(fileName);
        if (!freopen(qPrintable(fileName), "w", stderr)) {
            fprintf(stderr, "STDERR redirection failed.");
            exit(1);
        }
    }
    index = args.indexOf("--pixel-tests");
    if (index == -1)
        index = args.indexOf("-p");
    if (index != -1) {
        dumper.setShouldDumpPixelsForAllTests();
        args.removeAt(index);
    }

    QWebDatabase::removeAllDatabases();

    index = args.indexOf(QLatin1String("--timeout"));
    if (index != -1) {
        int timeout = takeOptionValue(args, index).toInt();
        dumper.setTimeout(timeout);
    }

    index = args.indexOf(QLatin1String("--no-timeout"));
    if (index != -1) {
        dumper.setShouldTimeout(false);
        args.removeAt(index);
    }

    index = args.indexOf(QLatin1String("-"));
    if (index != -1) {
        args.removeAt(index);

        // Continue waiting in STDIN for more test case after process one test case
        QObject::connect(&dumper, SIGNAL(ready()), &dumper, SLOT(readLine()), Qt::QueuedConnection);   

        // Read and only read the first test case, ignore the others 
        if (args.size() > 0) { 
            // Process the argument first
            dumper.processLine(args[0]);
        } else
           QTimer::singleShot(0, &dumper, SLOT(readLine()));
    } else {
        // Go into standalone mode
        // Standalone mode need at least one test case
        if (args.count() < 1) {
            printUsage();
            exit(1);
        }
        dumper.processArgsLine(args);
    }
    return app.exec();
}
