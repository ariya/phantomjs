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

#ifndef QLOGGINGCATEGORY_H
#define QLOGGINGCATEGORY_H

#include <QtCore/qglobal.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QLoggingCategory
{
    Q_DISABLE_COPY(QLoggingCategory)
public:
    // ### Qt 6: Merge constructors
    explicit QLoggingCategory(const char *category);
    QLoggingCategory(const char *category, QtMsgType severityLevel);
    ~QLoggingCategory();

    bool isEnabled(QtMsgType type) const;
    void setEnabled(QtMsgType type, bool enable);

#ifdef Q_ATOMIC_INT8_IS_SUPPORTED
    bool isDebugEnabled() const { return bools.enabledDebug.load(); }
    bool isWarningEnabled() const { return bools.enabledWarning.load(); }
    bool isCriticalEnabled() const { return bools.enabledCritical.load(); }
#else
    bool isDebugEnabled() const { return enabled.load() >> DebugShift & 1; }
    bool isWarningEnabled() const { return enabled.load() >> WarningShift & 1; }
    bool isCriticalEnabled() const { return enabled.load() >> CriticalShift & 1; }
#endif
    const char *categoryName() const { return name; }

    // allows usage of both factory method and variable in qCX macros
    QLoggingCategory &operator()() { return *this; }
    const QLoggingCategory &operator()() const { return *this; }

    static QLoggingCategory *defaultCategory();

    typedef void (*CategoryFilter)(QLoggingCategory*);
    static CategoryFilter installFilter(CategoryFilter);

    static void setFilterRules(const QString &rules);

private:
    void init(const char *category, QtMsgType severityLevel);

    Q_DECL_UNUSED_MEMBER void *d; // reserved for future use
    const char *name;

#ifdef Q_BIG_ENDIAN
    enum { DebugShift = 0, WarningShift = 8, CriticalShift = 16 };
#else
    enum { DebugShift = 24, WarningShift = 16, CriticalShift = 8 };
#endif

    struct AtomicBools {
#ifdef Q_ATOMIC_INT8_IS_SUPPORTED
        QBasicAtomicInteger<bool> enabledDebug;
        QBasicAtomicInteger<bool> enabledWarning;
        QBasicAtomicInteger<bool> enabledCritical;
#endif
    };
    union {
        AtomicBools bools;
        QBasicAtomicInt enabled;
    };
    Q_DECL_UNUSED_MEMBER bool placeholder[4]; // reserved for future use
};

#define Q_DECLARE_LOGGING_CATEGORY(name) \
    extern const QLoggingCategory &name();

#if defined(Q_COMPILER_VARIADIC_MACROS) || defined(Q_MOC_RUN)

#define Q_LOGGING_CATEGORY(name, ...) \
    const QLoggingCategory &name() \
    { \
        static const QLoggingCategory category(__VA_ARGS__); \
        return category; \
    }

#define qCDebug(category, ...) \
    for (bool qt_category_enabled = category().isDebugEnabled(); qt_category_enabled; qt_category_enabled = false) \
        QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, category().categoryName()).debug(__VA_ARGS__)
#define qCWarning(category, ...) \
    for (bool qt_category_enabled = category().isWarningEnabled(); qt_category_enabled; qt_category_enabled = false) \
        QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, category().categoryName()).warning(__VA_ARGS__)
#define qCCritical(category, ...) \
    for (bool qt_category_enabled = category().isCriticalEnabled(); qt_category_enabled; qt_category_enabled = false) \
        QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, category().categoryName()).critical(__VA_ARGS__)

#else // defined(Q_COMPILER_VARIADIC_MACROS) || defined(Q_MOC_RUN)

// Optional msgType argument not supported
#define Q_LOGGING_CATEGORY(name, string) \
    const QLoggingCategory &name() \
    { \
        static const QLoggingCategory category(string); \
        return category; \
    }

// check for enabled category inside QMessageLogger.
#define qCDebug qDebug
#define qCWarning qWarning
#define qCCritical qCritical

#endif // Q_COMPILER_VARIADIC_MACROS || defined(Q_MOC_RUN)

#if defined(QT_NO_DEBUG_OUTPUT)
#  undef qCDebug
#  define qCDebug(category) QT_NO_QDEBUG_MACRO()
#endif
#if defined(QT_NO_WARNING_OUTPUT)
#  undef qCWarning
#  define qCWarning(category) QT_NO_QWARNING_MACRO()
#endif

QT_END_NAMESPACE

#endif // QLOGGINGCATEGORY_H
