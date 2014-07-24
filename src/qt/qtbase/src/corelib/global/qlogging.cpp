/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qlogging.h"
#include "qlist.h"
#include "qbytearray.h"
#include "qstring.h"
#include "qvarlengtharray.h"
#include "qdebug.h"
#include "qmutex.h"
#include "qloggingcategory.h"
#ifndef QT_BOOTSTRAPPED
#include "qcoreapplication.h"
#include "qthread.h"
#include "private/qloggingregistry_p.h"
#endif
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif
#ifdef QT_USE_SLOG2
#include <slog2.h>
#endif

#ifdef Q_OS_ANDROID
#include <android/log.h>
#endif

#if defined(QT_USE_JOURNALD) && !defined(QT_BOOTSTRAPPED)
# define SD_JOURNAL_SUPPRESS_LOCATION
# include <systemd/sd-journal.h>
# include <syslog.h>
# include <unistd.h>
#endif

#include <stdio.h>

QT_BEGIN_NAMESPACE

#if !defined(Q_CC_MSVC)
Q_NORETURN
#endif
static void qt_message_fatal(QtMsgType, const QMessageLogContext &context, const QString &message);
static void qt_message_print(QtMsgType, const QMessageLogContext &context, const QString &message);

static bool isFatal(QtMsgType msgType)
{
    if (msgType == QtFatalMsg)
        return true;

    if (msgType == QtCriticalMsg) {
        static bool fatalCriticals = !qEnvironmentVariableIsEmpty("QT_FATAL_CRITICALS");
        return fatalCriticals;
    }

    if (msgType == QtWarningMsg || msgType == QtCriticalMsg) {
        static bool fatalWarnings = !qEnvironmentVariableIsEmpty("QT_FATAL_WARNINGS");
        return fatalWarnings;
    }

    return false;
}

#ifdef Q_OS_WIN

// Do we have stderr for QDebug? - Either there is a console or we are running
// with redirected stderr.
#  if !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
static inline bool hasStdErr()
{
    if (GetConsoleWindow())
        return true;
    STARTUPINFO info;
    GetStartupInfo(&info);
    return (info.dwFlags & STARTF_USESTDHANDLES) && info.hStdError
        && info.hStdError != INVALID_HANDLE_VALUE;
}
#  endif // !Q_OS_WINCE && !Q_OS_WINRT

Q_CORE_EXPORT bool qWinLogToStderr()
{
#  if !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    static const bool result = hasStdErr();
    return result;
#  else
    return false;
#  endif
}
#endif // Q_OS_WIN

/*!
    \class QMessageLogContext
    \inmodule QtCore
    \brief The QMessageLogContext class provides additional information about a log message.
    \since 5.0

    The class provides information about the source code location a qDebug(), qWarning(),
    qCritical() or qFatal() message was generated.

    \sa QMessageLogger, QtMessageHandler, qInstallMessageHandler()
*/

/*!
    \class QMessageLogger
    \inmodule QtCore
    \brief The QMessageLogger class generates log messages.
    \since 5.0

    QMessageLogger is used to generate messages for the Qt logging framework. Usually one uses
    it through qDebug(), qWarning(), qCritical, or qFatal() functions,
    which are actually macros that expand to QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).debug()
    et al.

    One example of direct use is to forward errors that stem from a scripting language, e.g. QML:

    \snippet code/qlogging/qlogging.cpp 1

    \sa QMessageLogContext, qDebug(), qWarning(), qCritical(), qFatal()
*/

#ifdef Q_OS_WIN
static inline void convert_to_wchar_t_elided(wchar_t *d, size_t space, const char *s) Q_DECL_NOEXCEPT
{
    size_t len = qstrlen(s);
    if (len + 1 > space) {
        const size_t skip = len - space + 4; // 4 for "..." + '\0'
        s += skip;
        for (int i = 0; i < 3; ++i)
          *d++ = L'.';
    }
    while (*s)
        *d++ = *s++;
    *d++ = 0;
}
#endif

#if !defined(QT_NO_EXCEPTIONS)
/*!
    \internal
    Uses a local buffer to output the message. Not locale safe + cuts off
    everything after character 255, but will work in out of memory situations.
    Stop the execution afterwards.
*/
static void qEmergencyOut(QtMsgType msgType, const char *msg, va_list ap) Q_DECL_NOEXCEPT
{
    char emergency_buf[256] = { '\0' };
    emergency_buf[sizeof emergency_buf - 1] = '\0';
#if defined(Q_OS_WIN) && defined(QT_BUILD_CORE_LIB) && (defined(Q_OS_WINCE) || defined(Q_OS_WINRT)) \
    || defined(Q_CC_MSVC) && defined(QT_DEBUG) && defined(_DEBUG) && defined(_CRT_ERROR)
    wchar_t emergency_bufL[sizeof emergency_buf];
#endif

    if (msg)
        qvsnprintf(emergency_buf, sizeof emergency_buf - 1, msg, ap);

#if defined(Q_OS_WIN) && defined(QT_BUILD_CORE_LIB)
# if defined(Q_OS_WINCE) || defined(Q_OS_WINRT)
    convert_to_wchar_t_elided(emergency_bufL, sizeof emergency_buf, emergency_buf);
    OutputDebugStringW(emergency_bufL);
# else
    if (qWinLogToStderr()) {
        fprintf(stderr, "%s\n", emergency_buf);
        fflush(stderr);
    } else {
        OutputDebugStringA(emergency_buf);
    }
# endif
#else
    fprintf(stderr, "%s\n", emergency_buf);
    fflush(stderr);
#endif

    if (isFatal(msgType)) {
#if defined(Q_CC_MSVC) && defined(QT_DEBUG) && defined(_DEBUG) && defined(_CRT_ERROR)
        // get the current report mode
        int reportMode = _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW);
        _CrtSetReportMode(_CRT_ERROR, reportMode);
# ifndef Q_OS_WINCE // otherwise already converted to wchar_t above
        convert_to_wchar_t_elided(emergency_bufL, sizeof emergency_buf, emergency_buf);
# endif
        int ret = _CrtDbgReportW(_CRT_ERROR, _CRT_WIDE(__FILE__), __LINE__,
                                 _CRT_WIDE(QT_VERSION_STR),
                                 emergency_bufL);
        if (ret == 1)
            _CrtDbgBreak();
#endif

#if (defined(Q_OS_UNIX) || defined(Q_CC_MINGW))
        abort(); // trap; generates core dump
#else
        exit(1); // goodbye cruel world
#endif
    }
}
#endif

/*!
    \internal
*/
static void qt_message(QtMsgType msgType, const QMessageLogContext &context, const char *msg,
                       va_list ap, QString &buf)
{
#if !defined(QT_NO_EXCEPTIONS)
    if (std::uncaught_exception()) {
        qEmergencyOut(msgType, msg, ap);
        return;
    }
#endif
    if (msg) {
        QT_TRY {
            buf = QString().vsprintf(msg, ap);
        } QT_CATCH(const std::bad_alloc &) {
#if !defined(QT_NO_EXCEPTIONS)
            qEmergencyOut(msgType, msg, ap);
            // don't rethrow - we use qWarning and friends in destructors.
            return;
#endif
        }
    }
    qt_message_print(msgType, context, buf);
}

#undef qDebug
/*!
    Logs a debug message specified with format \a msg. Additional
    parameters, specified by \a msg, may be used.

    \sa qDebug()
*/
void QMessageLogger::debug(const char *msg, ...) const
{
    QString message;

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtDebugMsg, context, msg, ap, message);
    va_end(ap);

    if (isFatal(QtDebugMsg))
        qt_message_fatal(QtDebugMsg, context, message);
}

/*!
    \typedef QMessageLogger::CategoryFunction

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet code/qlogging/qlogging.cpp 2

    A function which this signature is generated by Q_DECLARE_LOGGING_CATEGORY,
    Q_LOGGING_CATEGORY.

    \since 5.3
*/

/*!
    Logs a debug message specified with format \a msg for the context \a cat.
    Additional parameters, specified by \a msg, may be used.

    \since 5.3
    \sa qCDebug()
*/
void QMessageLogger::debug(const QLoggingCategory &cat, const char *msg, ...) const
{
    if (!cat.isDebugEnabled())
        return;

    QMessageLogContext ctxt;
    ctxt.copy(context);
    ctxt.category = cat.categoryName();

    QString message;

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtDebugMsg, ctxt, msg, ap, message);
    va_end(ap);

    if (isFatal(QtDebugMsg))
        qt_message_fatal(QtDebugMsg, ctxt, message);
}

/*!
    Logs a debug message specified with format \a msg for the context returned
    by \a catFunc. Additional parameters, specified by \a msg, may be used.

    \since 5.3
    \sa qCDebug()
*/
void QMessageLogger::debug(QMessageLogger::CategoryFunction catFunc,
                           const char *msg, ...) const
{
    const QLoggingCategory &cat = (*catFunc)();
    if (!cat.isDebugEnabled())
        return;

    QMessageLogContext ctxt;
    ctxt.copy(context);
    ctxt.category = cat.categoryName();

    QString message;

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtDebugMsg, ctxt, msg, ap, message);
    va_end(ap);

    if (isFatal(QtDebugMsg))
        qt_message_fatal(QtDebugMsg, ctxt, message);
}

#ifndef QT_NO_DEBUG_STREAM

/*!
    Logs a debug message using a QDebug stream

    \sa qDebug(), QDebug
*/
QDebug QMessageLogger::debug() const
{
    QDebug dbg = QDebug(QtDebugMsg);
    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copy(context);
    return dbg;
}

/*!
    Logs a debug message into category \a cat using a QDebug stream.

    \since 5.3
    \sa qCDebug(), QDebug
*/
QDebug QMessageLogger::debug(const QLoggingCategory &cat) const
{
    QDebug dbg = QDebug(QtDebugMsg);
    if (!cat.isDebugEnabled())
        dbg.stream->message_output = false;

    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copy(context);
    ctxt.category = cat.categoryName();

    return dbg;
}

/*!
    Logs a debug message into category returned by \a catFunc using a QDebug stream.

    \since 5.3
    \sa qCDebug(), QDebug
*/
QDebug QMessageLogger::debug(QMessageLogger::CategoryFunction catFunc) const
{
    return debug((*catFunc)());
}

/*!
    \internal

    Returns a QNoDebug object, which is used to ignore debugging output.

    \sa QNoDebug, qDebug()
*/
QNoDebug QMessageLogger::noDebug() const Q_DECL_NOTHROW
{
    return QNoDebug();
}

#endif

#undef qWarning
/*!
    Logs a warning message specified with format \a msg. Additional
    parameters, specified by \a msg, may be used.

    \sa qWarning()
*/
void QMessageLogger::warning(const char *msg, ...) const
{
    QString message;

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtWarningMsg, context, msg, ap, message);
    va_end(ap);

    if (isFatal(QtWarningMsg))
        qt_message_fatal(QtWarningMsg, context, message);
}

/*!
    Logs a warning message specified with format \a msg for the context \a cat.
    Additional parameters, specified by \a msg, may be used.

    \since 5.3
    \sa qCWarning()
*/
void QMessageLogger::warning(const QLoggingCategory &cat, const char *msg, ...) const
{
    if (!cat.isWarningEnabled())
        return;

    QMessageLogContext ctxt;
    ctxt.copy(context);
    ctxt.category = cat.categoryName();

    QString message;

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtWarningMsg, ctxt, msg, ap, message);
    va_end(ap);

    if (isFatal(QtWarningMsg))
        qt_message_fatal(QtWarningMsg, ctxt, message);
}

/*!
    Logs a warning message specified with format \a msg for the context returned
    by \a catFunc. Additional parameters, specified by \a msg, may be used.

    \since 5.3
    \sa qCWarning()
*/
void QMessageLogger::warning(QMessageLogger::CategoryFunction catFunc,
                             const char *msg, ...) const
{
    const QLoggingCategory &cat = (*catFunc)();
    if (!cat.isWarningEnabled())
        return;

    QMessageLogContext ctxt;
    ctxt.copy(context);
    ctxt.category = cat.categoryName();

    QString message;

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtWarningMsg, ctxt, msg, ap, message);
    va_end(ap);

    if (isFatal(QtWarningMsg))
        qt_message_fatal(QtWarningMsg, ctxt, message);
}

#ifndef QT_NO_DEBUG_STREAM
/*!
    Logs a warning message using a QDebug stream

    \sa qWarning(), QDebug
*/
QDebug QMessageLogger::warning() const
{
    QDebug dbg = QDebug(QtWarningMsg);
    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copy(context);
    return dbg;
}

/*!
    Logs a warning message into category \a cat using a QDebug stream.

    \sa qCWarning(), QDebug
*/
QDebug QMessageLogger::warning(const QLoggingCategory &cat) const
{
    QDebug dbg = QDebug(QtWarningMsg);
    if (!cat.isWarningEnabled())
        dbg.stream->message_output = false;

    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copy(context);
    ctxt.category = cat.categoryName();

    return dbg;
}

/*!
    Logs a warning message into category returned by \a catFunc using a QDebug stream.

    \since 5.3
    \sa qCWarning(), QDebug
*/
QDebug QMessageLogger::warning(QMessageLogger::CategoryFunction catFunc) const
{
    return warning((*catFunc)());
}

#endif

#undef qCritical

/*!
    Logs a critical message specified with format \a msg. Additional
    parameters, specified by \a msg, may be used.

    \sa qCritical()
*/
void QMessageLogger::critical(const char *msg, ...) const
{
    QString message;

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtCriticalMsg, context, msg, ap, message);
    va_end(ap);

    if (isFatal(QtCriticalMsg))
        qt_message_fatal(QtCriticalMsg, context, message);
}

/*!
    Logs a critical message specified with format \a msg for the context \a cat.
    Additional parameters, specified by \a msg, may be used.

    \since 5.3
    \sa qCCritical()
*/
void QMessageLogger::critical(const QLoggingCategory &cat, const char *msg, ...) const
{
    if (!cat.isCriticalEnabled())
        return;

    QMessageLogContext ctxt;
    ctxt.copy(context);
    ctxt.category = cat.categoryName();

    QString message;

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtCriticalMsg, ctxt, msg, ap, message);
    va_end(ap);

    if (isFatal(QtCriticalMsg))
        qt_message_fatal(QtCriticalMsg, ctxt, message);
}

/*!
    Logs a critical message specified with format \a msg for the context returned
    by \a catFunc. Additional parameters, specified by \a msg, may be used.

    \since 5.3
    \sa qCCritical()
*/
void QMessageLogger::critical(QMessageLogger::CategoryFunction catFunc,
                              const char *msg, ...) const
{
    const QLoggingCategory &cat = (*catFunc)();
    if (!cat.isCriticalEnabled())
        return;

    QMessageLogContext ctxt;
    ctxt.copy(context);
    ctxt.category = cat.categoryName();

    QString message;

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtCriticalMsg, ctxt, msg, ap, message);
    va_end(ap);

    if (isFatal(QtCriticalMsg))
        qt_message_fatal(QtCriticalMsg, ctxt, message);
}

#ifndef QT_NO_DEBUG_STREAM
/*!
    Logs a critical message using a QDebug stream

    \sa qCritical(), QDebug
*/
QDebug QMessageLogger::critical() const
{
    QDebug dbg = QDebug(QtCriticalMsg);
    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copy(context);
    return dbg;
}

/*!
    Logs a critical message into category \a cat using a QDebug stream.

    \since 5.3
    \sa qCCritical(), QDebug
*/
QDebug QMessageLogger::critical(const QLoggingCategory &cat) const
{
    QDebug dbg = QDebug(QtCriticalMsg);
    if (!cat.isCriticalEnabled())
        dbg.stream->message_output = false;

    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copy(context);
    ctxt.category = cat.categoryName();

    return dbg;
}

/*!
    Logs a critical message into category returned by \a catFunc using a QDebug stream.

    \since 5.3
    \sa qCCritical(), QDebug
*/
QDebug QMessageLogger::critical(QMessageLogger::CategoryFunction catFunc) const
{
    return critical((*catFunc)());
}

#endif

#undef qFatal
/*!
    Logs a fatal message specified with format \a msg. Additional
    parameters, specified by \a msg, may be used.

    \sa qFatal()
*/
void QMessageLogger::fatal(const char *msg, ...) const Q_DECL_NOTHROW
{
    QString message;

    va_list ap;
    va_start(ap, msg); // use variable arg list
    QT_TERMINATE_ON_EXCEPTION(qt_message(QtFatalMsg, context, msg, ap, message));
    va_end(ap);

    qt_message_fatal(QtFatalMsg, context, message);
}

/*!
    \internal
*/
Q_AUTOTEST_EXPORT QByteArray qCleanupFuncinfo(QByteArray info)
{
    // Strip the function info down to the base function name
    // note that this throws away the template definitions,
    // the parameter types (overloads) and any const/volatile qualifiers.

    if (info.isEmpty())
        return info;

    int pos;

    // skip trailing [with XXX] for templates (gcc)
    pos = info.size() - 1;
    if (info.endsWith(']')) {
        while (--pos) {
            if (info.at(pos) == '[')
                info.truncate(pos);
        }
    }

    // operator names with '(', ')', '<', '>' in it
    static const char operator_call[] = "operator()";
    static const char operator_lessThan[] = "operator<";
    static const char operator_greaterThan[] = "operator>";
    static const char operator_lessThanEqual[] = "operator<=";
    static const char operator_greaterThanEqual[] = "operator>=";

    // canonize operator names
    info.replace("operator ", "operator");

    // remove argument list
    forever {
        int parencount = 0;
        pos = info.lastIndexOf(')');
        if (pos == -1) {
            // Don't know how to parse this function name
            return info;
        }

        // find the beginning of the argument list
        --pos;
        ++parencount;
        while (pos && parencount) {
            if (info.at(pos) == ')')
                ++parencount;
            else if (info.at(pos) == '(')
                --parencount;
            --pos;
        }
        if (parencount != 0)
            return info;

        info.truncate(++pos);

        if (info.at(pos - 1) == ')') {
            if (info.indexOf(operator_call) == pos - (int)strlen(operator_call))
                break;

            // this function returns a pointer to a function
            // and we matched the arguments of the return type's parameter list
            // try again
            info.remove(0, info.indexOf('('));
            info.chop(1);
            continue;
        } else {
            break;
        }
    }

    // find the beginning of the function name
    int parencount = 0;
    int templatecount = 0;
    --pos;

    // make sure special characters in operator names are kept
    if (pos > -1) {
        switch (info.at(pos)) {
        case ')':
            if (info.indexOf(operator_call) == pos - (int)strlen(operator_call) + 1)
                pos -= 2;
            break;
        case '<':
            if (info.indexOf(operator_lessThan) == pos - (int)strlen(operator_lessThan) + 1)
                --pos;
            break;
        case '>':
            if (info.indexOf(operator_greaterThan) == pos - (int)strlen(operator_greaterThan) + 1)
                --pos;
            break;
        case '=': {
            int operatorLength = (int)strlen(operator_lessThanEqual);
            if (info.indexOf(operator_lessThanEqual) == pos - operatorLength + 1)
                pos -= 2;
            else if (info.indexOf(operator_greaterThanEqual) == pos - operatorLength + 1)
                pos -= 2;
            break;
        }
        default:
            break;
        }
    }

    while (pos > -1) {
        if (parencount < 0 || templatecount < 0)
            return info;

        char c = info.at(pos);
        if (c == ')')
            ++parencount;
        else if (c == '(')
            --parencount;
        else if (c == '>')
            ++templatecount;
        else if (c == '<')
            --templatecount;
        else if (c == ' ' && templatecount == 0 && parencount == 0)
            break;

        --pos;
    }
    info = info.mid(pos + 1);

    // remove trailing '*', '&' that are part of the return argument
    while ((info.at(0) == '*')
           || (info.at(0) == '&'))
        info = info.mid(1);

    // we have the full function name now.
    // clean up the templates
    while ((pos = info.lastIndexOf('>')) != -1) {
        if (!info.contains('<'))
            break;

        // find the matching close
        int end = pos;
        templatecount = 1;
        --pos;
        while (pos && templatecount) {
            char c = info.at(pos);
            if (c == '>')
                ++templatecount;
            else if (c == '<')
                --templatecount;
            --pos;
        }
        ++pos;
        info.remove(pos, end - pos + 1);
    }

    return info;
}

// tokens as recognized in QT_MESSAGE_PATTERN
static const char categoryTokenC[] = "%{category}";
static const char typeTokenC[] = "%{type}";
static const char messageTokenC[] = "%{message}";
static const char fileTokenC[] = "%{file}";
static const char lineTokenC[] = "%{line}";
static const char functionTokenC[] = "%{function}";
static const char pidTokenC[] = "%{pid}";
static const char appnameTokenC[] = "%{appname}";
static const char threadidTokenC[] = "%{threadid}";
static const char ifCategoryTokenC[] = "%{if-category}";
static const char ifDebugTokenC[] = "%{if-debug}";
static const char ifWarningTokenC[] = "%{if-warning}";
static const char ifCriticalTokenC[] = "%{if-critical}";
static const char ifFatalTokenC[] = "%{if-fatal}";
static const char endifTokenC[] = "%{endif}";
static const char emptyTokenC[] = "";

static const char defaultPattern[] = "%{if-category}%{category}: %{endif}%{message}";


struct QMessagePattern {
    QMessagePattern();
    ~QMessagePattern();

    void setPattern(const QString &pattern);

    // 0 terminated arrays of literal tokens / literal or placeholder tokens
    const char **literals;
    const char **tokens;

    bool fromEnvironment;
    static QBasicMutex mutex;
};

QBasicMutex QMessagePattern::mutex;

QMessagePattern::QMessagePattern()
    : literals(0)
    , tokens(0)
    , fromEnvironment(false)
{
    const QString envPattern = QString::fromLocal8Bit(qgetenv("QT_MESSAGE_PATTERN"));
    if (envPattern.isEmpty()) {
        setPattern(QLatin1String(defaultPattern));
    } else {
        setPattern(envPattern);
        fromEnvironment = true;
    }
}

QMessagePattern::~QMessagePattern()
{
    for (int i = 0; literals[i] != 0; ++i)
        delete [] literals[i];
    delete [] literals;
    literals = 0;
    delete [] tokens;
    tokens = 0;
}

void QMessagePattern::setPattern(const QString &pattern)
{
    delete [] tokens;
    delete [] literals;

    // scanner
    QList<QString> lexemes;
    QString lexeme;
    bool inPlaceholder = false;
    for (int i = 0; i < pattern.size(); ++i) {
        const QChar c = pattern.at(i);
        if ((c == QLatin1Char('%'))
                && !inPlaceholder) {
            if ((i + 1 < pattern.size())
                    && pattern.at(i + 1) == QLatin1Char('{')) {
                // beginning of placeholder
                if (!lexeme.isEmpty()) {
                    lexemes.append(lexeme);
                    lexeme.clear();
                }
                inPlaceholder = true;
            }
        }

        lexeme.append(c);

        if ((c == QLatin1Char('}') && inPlaceholder)) {
            // end of placeholder
            lexemes.append(lexeme);
            lexeme.clear();
            inPlaceholder = false;
        }
    }
    if (!lexeme.isEmpty())
        lexemes.append(lexeme);

    // tokenizer
    QVarLengthArray<const char*> literalsVar;
    tokens = new const char*[lexemes.size() + 1];
    tokens[lexemes.size()] = 0;

    bool nestedIfError = false;
    bool inIf = false;
    QString error;

    for (int i = 0; i < lexemes.size(); ++i) {
        const QString lexeme = lexemes.at(i);
        if (lexeme.startsWith(QLatin1String("%{"))
                && lexeme.endsWith(QLatin1Char('}'))) {
            // placeholder
            if (lexeme == QLatin1String(typeTokenC)) {
                tokens[i] = typeTokenC;
            } else if (lexeme == QLatin1String(categoryTokenC))
                tokens[i] = categoryTokenC;
            else if (lexeme == QLatin1String(messageTokenC))
                tokens[i] = messageTokenC;
            else if (lexeme == QLatin1String(fileTokenC))
                tokens[i] = fileTokenC;
            else if (lexeme == QLatin1String(lineTokenC))
                tokens[i] = lineTokenC;
            else if (lexeme == QLatin1String(functionTokenC))
                tokens[i] = functionTokenC;
            else if (lexeme == QLatin1String(pidTokenC))
                tokens[i] = pidTokenC;
            else if (lexeme == QLatin1String(appnameTokenC))
                tokens[i] = appnameTokenC;
            else if (lexeme == QLatin1String(threadidTokenC))
                tokens[i] = threadidTokenC;

#define IF_TOKEN(LEVEL) \
            else if (lexeme == QLatin1String(LEVEL)) { \
                if (inIf) \
                    nestedIfError = true; \
                tokens[i] = LEVEL; \
                inIf = true; \
            }
            IF_TOKEN(ifCategoryTokenC)
            IF_TOKEN(ifDebugTokenC)
            IF_TOKEN(ifWarningTokenC)
            IF_TOKEN(ifCriticalTokenC)
            IF_TOKEN(ifFatalTokenC)
#undef IF_TOKEN
            else if (lexeme == QLatin1String(endifTokenC)) {
                tokens[i] = endifTokenC;
                if (!inIf && !nestedIfError)
                    error += QStringLiteral("QT_MESSAGE_PATTERN: %{endif} without an %{if-*}\n");
                inIf = false;
            } else {
                tokens[i] = emptyTokenC;
                error += QStringLiteral("QT_MESSAGE_PATTERN: Unknown placeholder %1\n")
                        .arg(lexeme);
            }
        } else {
            char *literal = new char[lexeme.size() + 1];
            strncpy(literal, lexeme.toLatin1().constData(), lexeme.size());
            literal[lexeme.size()] = '\0';
            literalsVar.append(literal);
            tokens[i] = literal;
        }
    }
    if (nestedIfError)
        error += QStringLiteral("QT_MESSAGE_PATTERN: %{if-*} cannot be nested\n");
    else if (inIf)
        error += QStringLiteral("QT_MESSAGE_PATTERN: missing %{endif}\n");
    if (!error.isEmpty()) {
#if defined(Q_OS_WINCE) || defined(Q_OS_WINRT)
        OutputDebugString(reinterpret_cast<const wchar_t*>(error.utf16()));
        if (0)
#elif defined(Q_OS_WIN) && defined(QT_BUILD_CORE_LIB)
        if (!qWinLogToStderr()) {
            OutputDebugString(reinterpret_cast<const wchar_t*>(error.utf16()));
        } else
#endif
        {
            fprintf(stderr, "%s", error.toLocal8Bit().constData());
            fflush(stderr);
        }
    }
    literals = new const char*[literalsVar.size() + 1];
    literals[literalsVar.size()] = 0;
    memcpy(literals, literalsVar.constData(), literalsVar.size() * sizeof(const char*));
}

#if defined(QT_USE_SLOG2)
#ifndef QT_LOG_CODE
#define QT_LOG_CODE 9000
#endif

extern char *__progname;

static void slog2_default_handler(QtMsgType msgType, const char *message)
{
    if (slog2_set_default_buffer((slog2_buffer_t)-1) == 0) {
        slog2_buffer_set_config_t buffer_config;
        slog2_buffer_t buffer_handle;

        buffer_config.buffer_set_name = __progname;
        buffer_config.num_buffers = 1;
        buffer_config.verbosity_level = SLOG2_INFO;
        buffer_config.buffer_config[0].buffer_name = "default";
        buffer_config.buffer_config[0].num_pages = 8;

        if (slog2_register(&buffer_config, &buffer_handle, 0) == -1) {
            fprintf(stderr, "Error registering slogger2 buffer!\n");
            fprintf(stderr, "%s", message);
            fflush(stderr);
            return;
        }

        // Set as the default buffer
        slog2_set_default_buffer(buffer_handle);
    }
    int severity;
    //Determines the severity level
    switch (msgType) {
    case QtDebugMsg:
        severity = SLOG2_INFO;
        break;
    case QtWarningMsg:
        severity = SLOG2_NOTICE;
        break;
    case QtCriticalMsg:
        severity = SLOG2_WARNING;
        break;
    case QtFatalMsg:
        severity = SLOG2_ERROR;
        break;
    }
    //writes to the slog2 buffer
    slog2c(NULL, QT_LOG_CODE, severity, message);
}
#endif // QT_USE_SLOG2

Q_GLOBAL_STATIC(QMessagePattern, qMessagePattern)

/*!
    \internal
*/
Q_CORE_EXPORT QString qMessageFormatString(QtMsgType type, const QMessageLogContext &context,
                                              const QString &str)
{
    QString message;

    QMutexLocker lock(&QMessagePattern::mutex);

    QMessagePattern *pattern = qMessagePattern();
    if (!pattern) {
        // after destruction of static QMessagePattern instance
        message.append(str);
        message.append(QLatin1Char('\n'));
        return message;
    }

    // don't print anything if pattern was empty
    if (pattern->tokens[0] == 0)
        return message;

    bool skip = false;

    // we do not convert file, function, line literals to local encoding due to overhead
    for (int i = 0; pattern->tokens[i] != 0; ++i) {
        const char *token = pattern->tokens[i];
        if (token == endifTokenC) {
            skip = false;
        } else if (skip) {
            // do nothing
        } else if (token == messageTokenC) {
            message.append(str);
        } else if (token == categoryTokenC) {
            message.append(QLatin1String(context.category));
        } else if (token == typeTokenC) {
            switch (type) {
            case QtDebugMsg:   message.append(QLatin1String("debug")); break;
            case QtWarningMsg: message.append(QLatin1String("warning")); break;
            case QtCriticalMsg:message.append(QLatin1String("critical")); break;
            case QtFatalMsg:   message.append(QLatin1String("fatal")); break;
            }
        } else if (token == fileTokenC) {
            if (context.file)
                message.append(QLatin1String(context.file));
            else
                message.append(QLatin1String("unknown"));
        } else if (token == lineTokenC) {
            message.append(QString::number(context.line));
        } else if (token == functionTokenC) {
            if (context.function)
                message.append(QString::fromLatin1(qCleanupFuncinfo(context.function)));
            else
                message.append(QLatin1String("unknown"));
#ifndef QT_BOOTSTRAPPED
        } else if (token == pidTokenC) {
            message.append(QString::number(QCoreApplication::applicationPid()));
        } else if (token == appnameTokenC) {
            message.append(QCoreApplication::applicationName());
        } else if (token == threadidTokenC) {
            message.append(QLatin1String("0x"));
            message.append(QString::number(qlonglong(QThread::currentThread()->currentThread()), 16));
#endif
        } else if (token == ifCategoryTokenC) {
            if (!context.category || (strcmp(context.category, "default") == 0))
                skip = true;
#define HANDLE_IF_TOKEN(LEVEL)  \
        } else if (token == if##LEVEL##TokenC) { \
            skip = type != Qt##LEVEL##Msg;
        HANDLE_IF_TOKEN(Debug)
        HANDLE_IF_TOKEN(Warning)
        HANDLE_IF_TOKEN(Critical)
        HANDLE_IF_TOKEN(Fatal)
#undef HANDLE_IF_TOKEN
        } else {
            message.append(QLatin1String(token));
        }
    }
    message.append(QLatin1Char('\n'));
    return message;
}

#if !QT_DEPRECATED_SINCE(5, 0)
// make sure they're defined to be exported
typedef void (*QtMsgHandler)(QtMsgType, const char *);
Q_CORE_EXPORT QtMsgHandler qInstallMsgHandler(QtMsgHandler);
#endif

static QtMsgHandler msgHandler = 0;                // pointer to debug handler (without context)
static QtMessageHandler messageHandler = 0;         // pointer to debug handler (with context)

#if defined(QT_USE_JOURNALD) && !defined(QT_BOOTSTRAPPED)
static void systemd_default_message_handler(QtMsgType type,
                                            const QMessageLogContext &context,
                                            const QString &message)
{
    int priority = LOG_INFO; // Informational
    switch (type) {
    case QtDebugMsg:
        priority = LOG_DEBUG; // Debug-level messages
        break;
    case QtWarningMsg:
        priority = LOG_WARNING; // Warning conditions
        break;
    case QtCriticalMsg:
        priority = LOG_CRIT; // Critical conditions
        break;
    case QtFatalMsg:
        priority = LOG_ALERT; // Action must be taken immediately
        break;
    }

    sd_journal_send("MESSAGE=%s",     message.toUtf8().constData(),
                    "PRIORITY=%i",    priority,
                    "CODE_FUNC=%s",   context.function ? context.function : "unknown",
                    "CODE_LINE=%d",   context.line,
                    "CODE_FILE=%s",   context.file ? context.file : "unknown",
                    "QT_CATEGORY=%s", context.category ? context.category : "unknown",
                    NULL);
}
#endif

#ifdef Q_OS_ANDROID
static void android_default_message_handler(QtMsgType type,
                                  const QMessageLogContext &context,
                                  const QString &message)
{
    android_LogPriority priority = ANDROID_LOG_DEBUG;
    switch (type) {
    case QtDebugMsg: priority = ANDROID_LOG_DEBUG; break;
    case QtWarningMsg: priority = ANDROID_LOG_WARN; break;
    case QtCriticalMsg: priority = ANDROID_LOG_ERROR; break;
    case QtFatalMsg: priority = ANDROID_LOG_FATAL; break;
    };

    __android_log_print(priority, "Qt", "%s:%d (%s): %s",
                        context.file, context.line,
                        context.function, qPrintable(message));
}
#endif //Q_OS_ANDROID

/*!
    \internal
*/
static void qDefaultMessageHandler(QtMsgType type, const QMessageLogContext &context,
                                   const QString &buf)
{
    QString logMessage = qMessageFormatString(type, context, buf);

#if defined(Q_OS_WIN) && defined(QT_BUILD_CORE_LIB)
    if (!qWinLogToStderr()) {
        OutputDebugString(reinterpret_cast<const wchar_t *>(logMessage.utf16()));
        return;
    }
#endif // Q_OS_WIN

#if defined(QT_USE_SLOG2)
    slog2_default_handler(type, logMessage.toLocal8Bit().constData());
#elif defined(QT_USE_JOURNALD) && !defined(QT_BOOTSTRAPPED)
    // We use isatty to catch the obvious case of someone running something interactively.
    // We also support an environment variable for Qt Creator use, or more complicated cases like subprocesses.
    static bool logToConsole = isatty(fileno(stdin)) || !qEnvironmentVariableIsEmpty("QT_NO_JOURNALD_LOG");
    if (Q_LIKELY(!logToConsole)) {
        // remove trailing \n, systemd appears to want them newline-less
        logMessage.chop(1);
        systemd_default_message_handler(type, context, logMessage);
    } else {
        fprintf(stderr, "%s", logMessage.toLocal8Bit().constData());
        fflush(stderr);
    }
#elif defined(Q_OS_ANDROID)
    static bool logToAndroid = qEnvironmentVariableIsEmpty("QT_ANDROID_PLAIN_LOG");
    if (logToAndroid) {
        android_default_message_handler(type, context, logMessage);
    } else {
        fprintf(stderr, "%s", logMessage.toLocal8Bit().constData());
        fflush(stderr);
    }
#else
    fprintf(stderr, "%s", logMessage.toLocal8Bit().constData());
    fflush(stderr);
#endif
}

/*!
    \internal
*/
static void qDefaultMsgHandler(QtMsgType type, const char *buf)
{
    QMessageLogContext emptyContext;
    qDefaultMessageHandler(type, emptyContext, QString::fromLocal8Bit(buf));
}

#if defined(Q_COMPILER_THREAD_LOCAL) || (defined(Q_CC_MSVC) && !defined(Q_OS_WINCE))
#if defined(Q_CC_MSVC)
static __declspec(thread) bool msgHandlerGrabbed = false;
#else
static thread_local bool msgHandlerGrabbed = false;
#endif

static bool grabMessageHandler()
{
    if (msgHandlerGrabbed)
        return false;

    msgHandlerGrabbed = true;
    return true;
}

static void ungrabMessageHandler()
{
    msgHandlerGrabbed = false;
}

#else
static bool grabMessageHandler() { return true; }
static void ungrabMessageHandler() { }
#endif // (Q_COMPILER_THREAD_LOCAL) || ((Q_CC_MSVC) && !(Q_OS_WINCE))

static void qt_message_print(QtMsgType msgType, const QMessageLogContext &context, const QString &message)
{
#ifndef QT_BOOTSTRAPPED
    // qDebug, qWarning, ... macros do not check whether category is enabled
    if (!context.category || (strcmp(context.category, "default") == 0)) {
        if (QLoggingCategory *defaultCategory = QLoggingCategory::defaultCategory()) {
            if (!defaultCategory->isEnabled(msgType))
                return;
        }
    }
#endif

    if (!msgHandler)
        msgHandler = qDefaultMsgHandler;
    if (!messageHandler)
        messageHandler = qDefaultMessageHandler;

    // prevent recursion in case the message handler generates messages
    // itself, e.g. by using Qt API
    if (grabMessageHandler()) {
        // prefer new message handler over the old one
        if (msgHandler == qDefaultMsgHandler
                || messageHandler != qDefaultMessageHandler) {
            (*messageHandler)(msgType, context, message);
        } else {
            (*msgHandler)(msgType, message.toLocal8Bit().constData());
        }
        ungrabMessageHandler();
    } else {
        fprintf(stderr, "%s", message.toLocal8Bit().constData());
    }
}

static void qt_message_fatal(QtMsgType, const QMessageLogContext &context, const QString &message)
{
#if defined(Q_CC_MSVC) && defined(QT_DEBUG) && defined(_DEBUG) && defined(_CRT_ERROR)
    wchar_t contextFileL[256];
    // we probably should let the compiler do this for us, by declaring QMessageLogContext::file to
    // be const wchar_t * in the first place, but the #ifdefery above is very complex  and we
    // wouldn't be able to change it later on...
    convert_to_wchar_t_elided(contextFileL, sizeof contextFileL / sizeof *contextFileL,
                              context.file);
    // get the current report mode
    int reportMode = _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW);
    _CrtSetReportMode(_CRT_ERROR, reportMode);

    int ret = _CrtDbgReportW(_CRT_ERROR, contextFileL, context.line, _CRT_WIDE(QT_VERSION_STR),
                             reinterpret_cast<const wchar_t *>(message.utf16()));
    if ((ret == 0) && (reportMode & _CRTDBG_MODE_WNDW))
        return; // ignore
    else if (ret == 1)
        _CrtDbgBreak();
#else
    Q_UNUSED(context);
    Q_UNUSED(message);
#endif

#if (defined(Q_OS_UNIX) || defined(Q_CC_MINGW))
    abort(); // trap; generates core dump
#else
    exit(1); // goodbye cruel world
#endif
}


/*!
    \internal
*/
void qt_message_output(QtMsgType msgType, const QMessageLogContext &context, const QString &message)
{
    qt_message_print(msgType, context, message);
    if (isFatal(msgType))
        qt_message_fatal(msgType, context, message);
}

void qErrnoWarning(const char *msg, ...)
{
    // qt_error_string() will allocate anyway, so we don't have
    // to be careful here (like we do in plain qWarning())
    QString buf;
    va_list ap;
    va_start(ap, msg);
    if (msg)
        buf.vsprintf(msg, ap);
    va_end(ap);

    buf += QLatin1String(" (") + qt_error_string(-1) + QLatin1Char(')');
    QMessageLogContext context;
    qt_message_output(QtCriticalMsg, context, buf);
}

void qErrnoWarning(int code, const char *msg, ...)
{
    // qt_error_string() will allocate anyway, so we don't have
    // to be careful here (like we do in plain qWarning())
    QString buf;
    va_list ap;
    va_start(ap, msg);
    if (msg)
        buf.vsprintf(msg, ap);
    va_end(ap);

    buf += QLatin1String(" (") + qt_error_string(code) + QLatin1Char(')');
    QMessageLogContext context;
    qt_message_output(QtCriticalMsg, context, buf);
}

/*!
    \typedef QtMsgHandler
    \relates <QtGlobal>
    \deprecated

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet code/src_corelib_global_qglobal.cpp 7

    This typedef is deprecated, you should use QtMessageHandler instead.
    \sa QtMsgType, QtMessageHandler, qInstallMsgHandler(), qInstallMessageHandler()
*/

/*!
    \typedef QtMessageHandler
    \relates <QtGlobal>
    \since 5.0

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet code/src_corelib_global_qglobal.cpp 49

    \sa QtMsgType, qInstallMessageHandler()
*/

/*!
    \fn QtMessageHandler qInstallMessageHandler(QtMessageHandler handler)
    \relates <QtGlobal>
    \since 5.0

    Installs a Qt message \a handler which has been defined
    previously. Returns a pointer to the previous message handler
    (which may be 0).

    The message handler is a function that prints out debug messages,
    warnings, critical and fatal error messages. The Qt library (debug
    mode) contains hundreds of warning messages that are printed
    when internal errors (usually invalid function arguments)
    occur. Qt built in release mode also contains such warnings unless
    QT_NO_WARNING_OUTPUT and/or QT_NO_DEBUG_OUTPUT have been set during
    compilation. If you implement your own message handler, you get total
    control of these messages.

    The default message handler prints the message to the standard
    output under X11 or to the debugger under Windows. If it is a
    fatal message, the application aborts immediately.

    Only one message handler can be defined, since this is usually
    done on an application-wide basis to control debug output.

    To restore the message handler, call \c qInstallMessageHandler(0).

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 23

    \sa QtMessageHandler, QtMsgType, qDebug(), qWarning(), qCritical(), qFatal(),
    {Debugging Techniques}
*/

/*!
    \fn QtMsgHandler qInstallMsgHandler(QtMsgHandler handler)
    \relates <QtGlobal>
    \deprecated

    Installs a Qt message \a handler which has been defined
    previously. This method is deprecated, use qInstallMessageHandler
    instead.
    \sa QtMsgHandler, qInstallMessageHandler()
*/
/*!
    \fn void qSetMessagePattern(const QString &pattern)
    \relates <QtGlobal>
    \since 5.0

    \brief Changes the output of the default message handler.

    Allows to tweak the output of qDebug(), qWarning(), qCritical() and qFatal().

    Following placeholders are supported:

    \table
    \header \li Placeholder \li Description
    \row \li \c %{appname} \li QCoreApplication::applicationName()
    \row \li \c %{category} \li Logging category
    \row \li \c %{file} \li Path to source file
    \row \li \c %{function} \li Function
    \row \li \c %{line} \li Line in source file
    \row \li \c %{message} \li The actual message
    \row \li \c %{pid} \li QCoreApplication::applicationPid()
    \row \li \c %{threadid} \li ID of current thread
    \row \li \c %{type} \li "debug", "warning", "critical" or "fatal"
    \endtable

    You can also use conditionals on the type of the message using \c %{if-debug},
    \c %{if-warning}, \c %{if-critical} or \c %{if-fatal} followed by an \c %{endif}.
    What is inside the \c %{if-*} and \c %{endif} will only be printed if the type matches.

    Finally, text inside \c %{if-category} ... \c %{endif} is only printed if the category
    is not the default one.

    Example:
    \code
    QT_MESSAGE_PATTERN="[%{if-debug}D%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] %{file}:%{line} - %{message}"
    \endcode

    The default \a pattern is "%{if-category}%{category}: %{endif}%{message}".

    The \a pattern can also be changed at runtime by setting the QT_MESSAGE_PATTERN
    environment variable; if both qSetMessagePattern() is called and QT_MESSAGE_PATTERN is
    set, the environment variable takes precedence.

    qSetMessagePattern() has no effect if a custom message handler is installed.

    \sa qInstallMessageHandler(), {Debugging Techniques}
 */

QtMessageHandler qInstallMessageHandler(QtMessageHandler h)
{
    if (!messageHandler)
        messageHandler = qDefaultMessageHandler;
    QtMessageHandler old = messageHandler;
    messageHandler = h;
    return old;
}

QtMsgHandler qInstallMsgHandler(QtMsgHandler h)
{
    //if handler is 0, set it to the
    //default message handler
    if (!msgHandler)
        msgHandler = qDefaultMsgHandler;
    QtMsgHandler old = msgHandler;
    msgHandler = h;
    return old;
}

void qSetMessagePattern(const QString &pattern)
{
    QMutexLocker lock(&QMessagePattern::mutex);

    if (!qMessagePattern()->fromEnvironment)
        qMessagePattern()->setPattern(pattern);
}


/*!
    Copies context information from \a logContext into this QMessageLogContext
    \internal
*/
void QMessageLogContext::copy(const QMessageLogContext &logContext)
{
    this->category = logContext.category;
    this->file = logContext.file;
    this->line = logContext.line;
    this->function = logContext.function;
}

/*!
    \fn QMessageLogger::QMessageLogger()

    Constructs a default QMessageLogger. See the other constructors to specify
    context information.
*/

/*!
    \fn QMessageLogger::QMessageLogger(const char *file, int line, const char *function)

    Constructs a QMessageLogger to record log messages for \a file at \a line
    in \a function. The is equivalent to QMessageLogger(file, line, function, "default")
*/
/*!
    \fn QMessageLogger::QMessageLogger(const char *file, int line, const char *function, const char *category)

    Constructs a QMessageLogger to record \a category messages for \a file at \a line
    in \a function.
*/

/*!
    \fn void QMessageLogger::noDebug(const char *, ...) const
    \internal

    Ignores logging output

    \sa QNoDebug, qDebug()
*/

/*!
    \fn QMessageLogContext::QMessageLogContext()
    \internal

    Constructs a QMessageLogContext
*/

/*!
    \fn QMessageLogContext::QMessageLogContext(const char *fileName, int lineNumber, const char *functionName, const char *categoryName)
    \internal

    Constructs a QMessageLogContext with for file \a fileName at line
    \a lineNumber, in function \a functionName, and category \a categoryName.
*/

QT_END_NAMESPACE
