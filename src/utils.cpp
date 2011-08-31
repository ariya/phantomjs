/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <QDir>

#include "consts.h"
#include "terminal.h"
#include "utils.h"

// public:
void Utils::showUsage()
{
    QFile file;
    file.setFileName(":/usage.txt");
    if ( !file.open(QFile::ReadOnly) ) {
        Terminal::instance()->cerr("Unable to print the usage message");
        exit(1);
    }
    Terminal::instance()->cout(QString::fromUtf8(file.readAll()));
    file.close();
}

void Utils::messageHandler(QtMsgType type, const char *msg)
{
    QDateTime now = QDateTime::currentDateTime();

    switch (type) {
    case QtDebugMsg:
        fprintf(stdout, "%s [DEBUG] %s\n", qPrintable(now.toString(Qt::ISODate)), msg);
        break;
    case QtWarningMsg:
        fprintf(stderr, "%s [WARNING] %s\n", qPrintable(now.toString(Qt::ISODate)), msg);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "%s [CRITICAL] %s\n", qPrintable(now.toString(Qt::ISODate)), msg);
        break;
    case QtFatalMsg:
        fprintf(stderr, "%s [FATAL] %s\n", qPrintable(now.toString(Qt::ISODate)), msg);
        abort();
    }
}

QVariant Utils::coffee2js(const QString &script)
{
    return CSConverter::instance()->convert(script);
}

bool Utils::injectJsInFrame(const QString &jsFilePath, const QString &libraryPath, QWebFrame *targetFrame, const bool startingScript)
{
    return injectJsInFrame(jsFilePath, Encoding::UTF8, libraryPath, targetFrame, startingScript);
}

bool Utils::injectJsInFrame(const QString &jsFilePath, const Encoding &jsFileEnc, const QString &libraryPath, QWebFrame *targetFrame, const bool startingScript)
{
    // Don't do anything if an empty string is passed
    if (!jsFilePath.isEmpty()) {
        QFile jsFile;

        // Is file in the PWD?
        jsFile.setFileName(QDir::fromNativeSeparators(jsFilePath)); //< Normalise User-provided path
        if (!jsFile.exists()) {
            // File is not in the PWD. Is it in the lookup directory?
            jsFile.setFileName( libraryPath + '/' + QDir::fromNativeSeparators(jsFilePath) );
        }

        if ( jsFile.open(QFile::ReadOnly) ) {
            QString scriptBody = jsFileEnc.decode(jsFile.readAll());
            // Remove CLI script heading
            if (scriptBody.startsWith("#!") && !jsFile.fileName().endsWith(COFFEE_SCRIPT_EXTENSION)) {
                scriptBody.prepend("//");
            }

            if (jsFile.fileName().endsWith(COFFEE_SCRIPT_EXTENSION)) {
                QVariant result = Utils::coffee2js(scriptBody);
                if (result.toStringList().at(0) == "false") {
                    if (startingScript) {
                        Terminal::instance()->cerr(result.toStringList().at(1));
                        exit(1);
                    } else {
                        qWarning() << qPrintable(result.toStringList().at(1));
                        scriptBody = QString();
                    }
                } else {
                    scriptBody = result.toStringList().at(1);
                }
            }

            // Execute JS code in the context of the document
            targetFrame->evaluateJavaScript(scriptBody);
            jsFile.close();
            return true;
        } else {
            if (startingScript) {
                Terminal::instance()->cerr(QString("Can't open '%1'").arg(jsFilePath));
            } else {
                qWarning("Can't open '%s'", qPrintable(jsFilePath));
            }
        }
    }
    return false;
}

// private:
Utils::Utils()
{
    // Nothing to do here
}
