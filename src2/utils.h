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

#ifndef UTILS_H
#define UTILS_H

#include <QtGlobal>
#include <QWebFrame>
#include <QFile>

#include "csconverter.h"
#include "encoding.h"

#ifdef Q_OS_WIN32
#include "client/windows/handler/exception_handler.h"
#endif

class QTemporaryFile;
/**
 * Aggregate common utility functions.
 * Functions are static methods.
 * It's important to notice that, at the moment, this class can't be instantiated by design.
 */
class Utils
{
public:
    static void messageHandler(QtMsgType type, const char *msg);
#ifdef Q_OS_WIN32
    static bool exceptionHandler(const wchar_t* dump_path, const wchar_t* minidump_id, void* context, EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo *assertion, bool succeeded);
#else
    static bool exceptionHandler(const char* dump_path, const char* minidump_id, void* context, bool succeeded);
#endif    
    static QVariant coffee2js(const QString &script);
    static bool injectJsInFrame(const QString &jsFilePath, const QString &libraryPath, QWebFrame *targetFrame, const bool startingScript = false);
    static bool injectJsInFrame(const QString &jsFilePath, const Encoding &jsFileEnc, const QString &libraryPath, QWebFrame *targetFrame, const bool startingScript = false);
    static QString readResourceFileUtf8(const QString &resourceFilePath);

    static bool loadJSForDebug(const QString &jsFilePath, const Encoding &jsFileEnc, const QString &libraryPath, QWebFrame *targetFrame, const bool autorun = false);
    static bool loadJSForDebug(const QString &jsFilePath, const QString &libraryPath, QWebFrame *targetFrame, const bool autorun = false);
    static void cleanupFromDebug();

    static bool printDebugMessages;

private:
    static QString findScript(const QString &jsFilePath, const QString& libraryPath);
    static QString jsFromScriptFile(const QString& scriptPath, const Encoding& enc);
    Utils(); //< This class shouldn't be instantiated

    static QTemporaryFile* m_tempHarness; //< We want to make sure to clean up after ourselves
    static QTemporaryFile* m_tempWrapper;
};

#endif // UTILS_H
