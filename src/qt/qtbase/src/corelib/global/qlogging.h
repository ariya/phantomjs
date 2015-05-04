/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qglobal.h>

#ifndef QLOGGING_H
#define QLOGGING_H

#if 0
// header is automatically included in qglobal.h
#pragma qt_no_master_include
#endif

QT_BEGIN_NAMESPACE

/*
  Forward declarations only.

  In order to use the qDebug() stream, you must #include<QDebug>
*/
class QDebug;
class QNoDebug;

enum QtMsgType { QtDebugMsg, QtWarningMsg, QtCriticalMsg, QtFatalMsg, QtSystemMsg = QtCriticalMsg };

class QMessageLogContext
{
    Q_DISABLE_COPY(QMessageLogContext)
public:
    Q_DECL_CONSTEXPR QMessageLogContext() : version(2), line(0), file(0), function(0), category(0) {}
    Q_DECL_CONSTEXPR QMessageLogContext(const char *fileName, int lineNumber, const char *functionName, const char *categoryName)
        : version(2), line(lineNumber), file(fileName), function(functionName), category(categoryName) {}

    void copy(const QMessageLogContext &logContext);

    int version;
    int line;
    const char *file;
    const char *function;
    const char *category;

private:
    friend class QMessageLogger;
    friend class QDebug;
};

class QLoggingCategory;

class Q_CORE_EXPORT QMessageLogger
{
    Q_DISABLE_COPY(QMessageLogger)
public:
    Q_DECL_CONSTEXPR QMessageLogger() : context() {}
    Q_DECL_CONSTEXPR QMessageLogger(const char *file, int line, const char *function)
        : context(file, line, function, "default") {}
    Q_DECL_CONSTEXPR QMessageLogger(const char *file, int line, const char *function, const char *category)
        : context(file, line, function, category) {}

    void debug(const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);
    void noDebug(const char *, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3)
    {}
    void warning(const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);
    void critical(const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);

    typedef const QLoggingCategory &(*CategoryFunction)();

    void debug(const QLoggingCategory &cat, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    void debug(CategoryFunction catFunc, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    void warning(const QLoggingCategory &cat, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    void warning(CategoryFunction catFunc, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    void critical(const QLoggingCategory &cat, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    void critical(CategoryFunction catFunc, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);

#ifndef Q_CC_MSVC
    Q_NORETURN
#endif
    void fatal(const char *msg, ...) const Q_DECL_NOTHROW Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);

#ifndef QT_NO_DEBUG_STREAM
    QDebug debug() const;
    QDebug debug(const QLoggingCategory &cat) const;
    QDebug debug(CategoryFunction catFunc) const;
    QDebug warning() const;
    QDebug warning(const QLoggingCategory &cat) const;
    QDebug warning(CategoryFunction catFunc) const;
    QDebug critical() const;
    QDebug critical(const QLoggingCategory &cat) const;
    QDebug critical(CategoryFunction catFunc) const;

    QNoDebug noDebug() const Q_DECL_NOTHROW;
#endif // QT_NO_DEBUG_STREAM

private:
    QMessageLogContext context;
};

#if !defined(QT_MESSAGELOGCONTEXT) && !defined(QT_NO_MESSAGELOGCONTEXT)
#  if defined(QT_NO_DEBUG)
#    define QT_NO_MESSAGELOGCONTEXT
#  else
#    define QT_MESSAGELOGCONTEXT
#  endif
#endif

#ifdef QT_MESSAGELOGCONTEXT
  #define QT_MESSAGELOG_FILE __FILE__
  #define QT_MESSAGELOG_LINE __LINE__
  #define QT_MESSAGELOG_FUNC Q_FUNC_INFO
#else
  #define QT_MESSAGELOG_FILE 0
  #define QT_MESSAGELOG_LINE 0
  #define QT_MESSAGELOG_FUNC 0
#endif

#define qDebug QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).debug
#define qWarning QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).warning
#define qCritical QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).critical
#define qFatal QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).fatal

#define QT_NO_QDEBUG_MACRO while (false) QMessageLogger().noDebug
#define QT_NO_QWARNING_MACRO while (false) QMessageLogger().noDebug

#if defined(QT_NO_DEBUG_OUTPUT)
#  undef qDebug
#  define qDebug QT_NO_QDEBUG_MACRO
#endif
#if defined(QT_NO_WARNING_OUTPUT)
#  undef qWarning
#  define qWarning QT_NO_QWARNING_MACRO
#endif

Q_CORE_EXPORT void qt_message_output(QtMsgType, const QMessageLogContext &context,
                                     const QString &message);

Q_CORE_EXPORT void qErrnoWarning(int code, const char *msg, ...);
Q_CORE_EXPORT void qErrnoWarning(const char *msg, ...);

#if QT_DEPRECATED_SINCE(5, 0)// deprecated. Use qInstallMessageHandler instead!
typedef void (*QtMsgHandler)(QtMsgType, const char *);
Q_CORE_EXPORT QT_DEPRECATED QtMsgHandler qInstallMsgHandler(QtMsgHandler);
#endif

typedef void (*QtMessageHandler)(QtMsgType, const QMessageLogContext &, const QString &);
Q_CORE_EXPORT QtMessageHandler qInstallMessageHandler(QtMessageHandler);

Q_CORE_EXPORT void qSetMessagePattern(const QString &messagePattern);
Q_CORE_EXPORT QString qFormatLogMessage(QtMsgType type, const QMessageLogContext &context,
                                        const QString &buf);

QT_END_NAMESPACE
#endif // QLOGGING_H
