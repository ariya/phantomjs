/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOREAPPLICATION_H
#define QCOREAPPLICATION_H

#include <QtCore/qobject.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qeventloop.h>

#ifdef QT_INCLUDE_COMPAT
#include <QtCore/qstringlist.h>
#endif

#if defined(Q_WS_WIN) && !defined(tagMSG)
typedef struct tagMSG MSG;
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class QCoreApplicationPrivate;
class QTextCodec;
class QTranslator;
class QPostEventList;
class QStringList;

#define qApp QCoreApplication::instance()

class Q_CORE_EXPORT QCoreApplication : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString applicationName READ applicationName WRITE setApplicationName)
    Q_PROPERTY(QString applicationVersion READ applicationVersion WRITE setApplicationVersion)
    Q_PROPERTY(QString organizationName READ organizationName WRITE setOrganizationName)
    Q_PROPERTY(QString organizationDomain READ organizationDomain WRITE setOrganizationDomain)

    Q_DECLARE_PRIVATE(QCoreApplication)
public:
    enum { ApplicationFlags = QT_VERSION
#if !defined(QT3_SUPPORT)
        | 0x01000000
#endif
    };

#if defined(QT_BUILD_CORE_LIB) || defined(qdoc)
    QCoreApplication(int &argc, char **argv); // ### Qt5 remove
#endif
#if !defined(qdoc)
    QCoreApplication(int &argc, char **argv, int
#if !defined(QT_BUILD_CORE_LIB)
        = ApplicationFlags
#endif
        );
#endif

    ~QCoreApplication();

#ifdef QT_DEPRECATED
    QT_DEPRECATED static int argc();
    QT_DEPRECATED static char **argv();
#endif
    static QStringList arguments();

    static void setAttribute(Qt::ApplicationAttribute attribute, bool on = true);
    static bool testAttribute(Qt::ApplicationAttribute attribute);

    static void setOrganizationDomain(const QString &orgDomain);
    static QString organizationDomain();
    static void setOrganizationName(const QString &orgName);
    static QString organizationName();
    static void setApplicationName(const QString &application);
    static QString applicationName();
    static void setApplicationVersion(const QString &version);
    static QString applicationVersion();

    static QCoreApplication *instance() { return self; }

    static int exec();
    static void processEvents(QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents);
    static void processEvents(QEventLoop::ProcessEventsFlags flags, int maxtime);
    static void exit(int retcode=0);

    static bool sendEvent(QObject *receiver, QEvent *event);
    static void postEvent(QObject *receiver, QEvent *event);
    static void postEvent(QObject *receiver, QEvent *event, int priority);
    static void sendPostedEvents(QObject *receiver, int event_type);
    static void sendPostedEvents();
    static void removePostedEvents(QObject *receiver);
    static void removePostedEvents(QObject *receiver, int eventType);
    static bool hasPendingEvents();

    virtual bool notify(QObject *, QEvent *);

    static bool startingUp();
    static bool closingDown();

    static QString applicationDirPath();
    static QString applicationFilePath();
    static qint64 applicationPid();

#ifndef QT_NO_LIBRARY
    static void setLibraryPaths(const QStringList &);
    static QStringList libraryPaths();
    static void addLibraryPath(const QString &);
    static void removeLibraryPath(const QString &);
#endif // QT_NO_LIBRARY

#ifndef QT_NO_TRANSLATION
    static void installTranslator(QTranslator * messageFile);
    static void removeTranslator(QTranslator * messageFile);
#endif
    enum Encoding { CodecForTr, UnicodeUTF8, DefaultCodec = CodecForTr };
    // ### Qt 5: merge
    static QString translate(const char * context,
                             const char * key,
                             const char * disambiguation = 0,
                             Encoding encoding = CodecForTr);
    static QString translate(const char * context,
                             const char * key,
                             const char * disambiguation,
                             Encoding encoding, int n);

    static void flush();

#if defined(QT3_SUPPORT)
    inline QT3_SUPPORT void lock() {}
    inline QT3_SUPPORT void unlock(bool = true) {}
    inline QT3_SUPPORT bool locked() { return false; }
    inline QT3_SUPPORT bool tryLock() { return false; }

    static inline QT3_SUPPORT void processOneEvent()
    { processEvents(QEventLoop::WaitForMoreEvents); }
    static QT3_SUPPORT int enter_loop();
    static QT3_SUPPORT void exit_loop();
    static QT3_SUPPORT int loopLevel();
#endif

#if defined(Q_WS_WIN)
    virtual bool winEventFilter(MSG *message, long *result);
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_SYMBIAN)
    static void watchUnixSignal(int signal, bool watch);
#endif

    typedef bool (*EventFilter)(void *message, long *result);
    EventFilter setEventFilter(EventFilter filter);
    bool filterEvent(void *message, long *result);

public Q_SLOTS:
    static void quit();

Q_SIGNALS:
    void aboutToQuit();
    void unixSignal(int);

protected:
    bool event(QEvent *);

    virtual bool compressEvent(QEvent *, QObject *receiver, QPostEventList *);

protected:
    QCoreApplication(QCoreApplicationPrivate &p);

private:
    static bool sendSpontaneousEvent(QObject *receiver, QEvent *event);
    bool notifyInternal(QObject *receiver, QEvent *event);

    void init();

    static QCoreApplication *self;
    
    Q_DISABLE_COPY(QCoreApplication)

    friend class QEventDispatcherUNIXPrivate;
    friend class QApplication;
    friend class QApplicationPrivate;
    friend class QETWidget;
    friend class Q3AccelManager;
    friend class QShortcutMap;
    friend class QWidget;
    friend class QWidgetPrivate;
    friend bool qt_sendSpontaneousEvent(QObject*, QEvent*);
    friend Q_CORE_EXPORT QString qAppName();
    friend class QClassFactory;
};

inline bool QCoreApplication::sendEvent(QObject *receiver, QEvent *event)
{  if (event) event->spont = false; return self ? self->notifyInternal(receiver, event) : false; }

inline bool QCoreApplication::sendSpontaneousEvent(QObject *receiver, QEvent *event)
{ if (event) event->spont = true; return self ? self->notifyInternal(receiver, event) : false; }

inline void QCoreApplication::sendPostedEvents() { sendPostedEvents(0, 0); }

#ifdef QT_NO_TRANSLATION
// Simple versions
inline QString QCoreApplication::translate(const char *, const char *sourceText,
                                           const char *, Encoding encoding)
{
#ifndef QT_NO_TEXTCODEC
    if (encoding == UnicodeUTF8)
        return QString::fromUtf8(sourceText);
#else
    Q_UNUSED(encoding)
#endif
    return QString::fromLatin1(sourceText);
}

// Simple versions
inline QString QCoreApplication::translate(const char *, const char *sourceText,
                                           const char *, Encoding encoding, int)
{
#ifndef QT_NO_TEXTCODEC
    if (encoding == UnicodeUTF8)
        return QString::fromUtf8(sourceText);
#else
    Q_UNUSED(encoding)
#endif
    return QString::fromLatin1(sourceText);
}
#endif

// ### merge the four functions into two (using "int n = -1")
#define Q_DECLARE_TR_FUNCTIONS(context) \
public: \
    static inline QString tr(const char *sourceText, const char *disambiguation = 0) \
        { return QCoreApplication::translate(#context, sourceText, disambiguation); } \
    static inline QString trUtf8(const char *sourceText, const char *disambiguation = 0) \
        { return QCoreApplication::translate(#context, sourceText, disambiguation, \
                                             QCoreApplication::UnicodeUTF8); } \
    static inline QString tr(const char *sourceText, const char *disambiguation, int n) \
        { return QCoreApplication::translate(#context, sourceText, disambiguation, \
                                             QCoreApplication::CodecForTr, n); } \
    static inline QString trUtf8(const char *sourceText, const char *disambiguation, int n) \
        { return QCoreApplication::translate(#context, sourceText, disambiguation, \
                                             QCoreApplication::UnicodeUTF8, n); } \
private:

typedef void (*QtCleanUpFunction)();

Q_CORE_EXPORT void qAddPostRoutine(QtCleanUpFunction);
Q_CORE_EXPORT void qRemovePostRoutine(QtCleanUpFunction);
Q_CORE_EXPORT QString qAppName();                // get application name

#if defined(Q_WS_WIN) && !defined(QT_NO_DEBUG_STREAM)
Q_CORE_EXPORT QString decodeMSG(const MSG &);
Q_CORE_EXPORT QDebug operator<<(QDebug, const MSG &);
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QCOREAPPLICATION_H
