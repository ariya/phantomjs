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

#include "crashdump.h"

#include <QtGlobal>
#include <QString>
#include <QByteArray>

#include <exception>
#include <cstdlib>

#ifdef Q_OS_LINUX
#include "client/linux/handler/exception_handler.h"
#define HAVE_BREAKPAD
#define EHC_EXTRA_ARGS true
#define MDC_PATH_ARG   const char*
#define MDC_EXTRA_ARGS void*
#endif
#ifdef Q_OS_MAC
#include "client/mac/handler/exception_handler.h"
#define HAVE_BREAKPAD
#define EHC_EXTRA_ARGS true, NULL
#define MDC_PATH_ARG   const char*
#define MDC_EXTRA_ARGS void*
#endif
#ifdef Q_OS_WIN32
#include "client/windows/handler/exception_handler.h"
#define HAVE_BREAKPAD
#define EHC_EXTRA_ARGS BreakpadEH::HANDLER_ALL
#define MDC_PATH_ARG   const wchar_t*
#define MDC_EXTRA_ARGS void*, EXCEPTION_POINTERS*, MDRawAssertionInfo*
#endif

#ifdef HAVE_BREAKPAD

// This is not just 'using google_breakpad::ExceptionHandler' because
// one of the headers included by exception_handler.h on MacOS defines
// a typedef name 'ExceptionHandler' in the global namespace, and
// (apparently) a using-directive doesn't completely mask that.
typedef google_breakpad::ExceptionHandler BreakpadEH;

#ifdef Q_OS_WIN32
// qgetenv doesn't handle environment variables containing Unicode
// characters very well.  Breakpad-for-Windows works exclusively with
// Unicode paths anyway, so we use the native API to retrieve %TEMP%
// as Unicode.  This code based upon qglobal.cpp::qgetenv.
static QString
q_wgetenv(const wchar_t *varName)
{
    size_t requiredSize;
    _wgetenv_s(&requiredSize, 0, 0, varName);
    if (requiredSize == 0 || requiredSize > size_t(INT_MAX / sizeof(wchar_t)))
        return QString();

    // Unfortunately it does not appear to be safe to pass QString::data()
    // to a Windows API that expects wchar_t*.  QChar is too different.
    // We have to employ a scratch buffer.  Limiting the length to
    // INT_MAX / sizeof(wchar_t), above, ensures that the multiplication
    // here cannot overflow.
    wchar_t *buffer = (wchar_t *)malloc(requiredSize * sizeof(wchar_t));
    if (!buffer)
        return QString();

    // requiredSize includes the terminating null, which we don't want.
    // The range-check above also ensures that the conversion to int here
    // does not overflow.
    _wgetenv_s(&requiredSize, buffer, requiredSize, varName);
    Q_ASSERT(buffer[requiredSize-1] == L'\0');
    QString ret = QString::fromWCharArray(buffer, int(requiredSize - 1));

    free(buffer);
    return ret;
}
#endif

//
// Crash messages.
//

#ifdef Q_OS_WIN32
#define CRASH_DUMP_FMT "%ls\\%ls.dmp"
#define CRASH_DIR_FMT  "%%TEMP%% (%ls)"
#else
#define CRASH_DUMP_FMT "%s/%s.dmp"
#define CRASH_DIR_FMT  "$TMPDIR (%s)"
#endif

#define CRASH_MESSAGE_BASE \
    "PhantomJS has crashed. Please read the crash reporting guide at\n" \
    "<http://phantomjs.org/crash-reporting.html> and file a bug report at\n" \
    "<https://github.com/ariya/phantomjs/issues/new>.\n"

#define CRASH_MESSAGE_HAVE_DUMP \
    CRASH_MESSAGE_BASE \
    "Please attach the crash dump file:\n  " CRASH_DUMP_FMT "\n"

#define CRASH_MESSAGE_NO_DUMP \
    CRASH_MESSAGE_BASE \
    "Unfortunately, no crash dump is available.\n" \
    "(Is " CRASH_DIR_FMT " a directory you cannot write?)\n"

static bool minidumpCallback(MDC_PATH_ARG dump_path,
                             MDC_PATH_ARG minidump_id,
                             MDC_EXTRA_ARGS,
                             bool succeeded)
{
    if (succeeded)
        fprintf(stderr, CRASH_MESSAGE_HAVE_DUMP, dump_path, minidump_id);
    else
        fprintf(stderr, CRASH_MESSAGE_NO_DUMP, dump_path);
    return succeeded;
}

static BreakpadEH *initBreakpad()
{
    // On all platforms, Breakpad can be disabled by setting the
    // environment variable PHANTOMJS_DISABLE_CRASH_DUMPS to any
    // non-empty value.  This is not a command line argument because
    // we want to initialize Breakpad before parsing the command line.
    if (!qEnvironmentVariableIsEmpty("PHANTOMJS_DISABLE_CRASH_DUMPS"))
        return 0;

    // Windows and Unix have different conventions for the environment
    // variable naming the directory that should hold scratch files.
#ifdef Q_OS_WIN32
    std::wstring dumpPath(L".");
    QString varbuf = q_wgetenv(L"TEMP");
    if (!varbuf.isEmpty())
        dumpPath = varbuf.toStdWString();
#else
    std::string dumpPath("/tmp");
    QByteArray varbuf = qgetenv("TMPDIR");
    if (!varbuf.isEmpty())
        dumpPath = varbuf.constData();
#endif

    return new BreakpadEH(dumpPath, NULL, minidumpCallback, NULL,
                          EHC_EXTRA_ARGS);
}
#else // no HAVE_BREAKPAD
#define initBreakpad() NULL
#endif

// Qt, QtWebkit, and PhantomJS mostly don't make use of C++ exceptions,
// so in the rare cases where an exception does get thrown, it tends
// to pass all the way up the stack and cause the C++ runtime to call
// std::terminate().  The default std::terminate() handler in some
// C++ runtimes tries to print details of the exception or maybe even
// a stack trace.  Breakpad does a better job of this.
//
// Worse, if the exception is bad_alloc, thrown because we've run into
// a system-imposed hard upper limit on memory allocation, a clever
// terminate handler like that may itself perform more memory allocation,
// which will throw another bad_alloc, and cause a recursive call to
// terminate.  In some cases this may happen several times before the
// process finally dies.
//
// Short-circuit all this mess by forcing the terminate handler to
// be plain old std::abort, which will invoke Breakpad if it's active
// and crash promptly if not.

CrashHandler::CrashHandler()
  : old_terminate_handler(std::set_terminate(std::abort)),
    eh(initBreakpad())
{}

CrashHandler::~CrashHandler()
{
  delete eh;
  std::set_terminate(old_terminate_handler);
}
