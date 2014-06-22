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

#ifndef QCOREAPPLICATION_P_H
#define QCOREAPPLICATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qcoreapplication.h"
#include "QtCore/qtranslator.h"
#include "QtCore/qsettings.h"
#ifndef QT_NO_QOBJECT
#include "private/qobject_p.h"
#endif

QT_BEGIN_NAMESPACE

typedef QList<QTranslator*> QTranslatorList;

class QAbstractEventDispatcher;

class Q_CORE_EXPORT QCoreApplicationPrivate
#ifndef QT_NO_QOBJECT
    : public QObjectPrivate
#endif
{
    Q_DECLARE_PUBLIC(QCoreApplication)

public:
    enum Type {
        Tty,
        Gui
    };

    QCoreApplicationPrivate(int &aargc,  char **aargv, uint flags);
    ~QCoreApplicationPrivate();

    QString appName() const;

#ifdef Q_OS_MAC
    static QString macMenuBarName();
#endif

    static void initLocale();

    static bool checkInstance(const char *method);

#ifndef QT_NO_QOBJECT
    bool sendThroughApplicationEventFilters(QObject *, QEvent *);
    bool sendThroughObjectEventFilters(QObject *, QEvent *);
    bool notify_helper(QObject *, QEvent *);

    virtual void createEventDispatcher();
    virtual void eventDispatcherReady();
    static void removePostedEvent(QEvent *);
#ifdef Q_OS_WIN
    static void removePostedTimerEvent(QObject *object, int timerId);
#endif

    QAtomicInt quitLockRef;
    void ref();
    void deref();
    virtual bool shouldQuit() {
      return true;
    }
    void maybeQuit();

    static QThread *theMainThread;
    static QThread *mainThread();
    static void sendPostedEvents(QObject *receiver, int event_type, QThreadData *data);

    void checkReceiverThread(QObject *receiver);
    void cleanupThreadData();
#endif // QT_NO_QOBJECT

    int &argc;
    char **argv;
#ifdef Q_OS_WIN
    int origArgc;
    char **origArgv; // store unmodified arguments for QCoreApplication::arguments()
#endif
    void appendApplicationPathToLibraryPaths(void);

#ifndef QT_NO_TRANSLATION
    QTranslatorList translators;

    static bool isTranslatorInstalled(QTranslator *translator);
#endif

    QCoreApplicationPrivate::Type application_type;

    QString cachedApplicationDirPath;
    static QString *cachedApplicationFilePath;
    static void setApplicationFilePath(const QString &path);
    static inline void clearApplicationFilePath() { delete cachedApplicationFilePath; cachedApplicationFilePath = 0; }

#ifndef QT_NO_QOBJECT
    bool in_exec;
    bool aboutToQuitEmitted;
    bool threadData_clean;

    static QAbstractEventDispatcher *eventDispatcher;
    static bool is_app_running;
    static bool is_app_closing;
#endif

    static bool setuidAllowed;
    static uint attribs;
    static inline bool testAttribute(uint flag) { return attribs & (1 << flag); }
    static int app_compile_version;

    void processCommandLineArguments();
    QString qmljs_debug_arguments; // a string containing arguments for js/qml debugging.
    inline QString qmljsDebugArgumentsString() { return qmljs_debug_arguments; }

#ifdef QT_NO_QOBJECT
    QCoreApplication *q_ptr;
#endif
};

QT_END_NAMESPACE

#endif // QCOREAPPLICATION_P_H
