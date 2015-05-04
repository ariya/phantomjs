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

#ifndef QPROCESS_P_H
#define QPROCESS_P_H

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

#include "QtCore/qprocess.h"
#include "QtCore/qstringlist.h"
#include "QtCore/qhash.h"
#include "QtCore/qshareddata.h"
#include "private/qringbuffer_p.h"
#include "private/qiodevice_p.h"
#ifdef Q_OS_UNIX
#include <QtCore/private/qorderedmutexlocker_p.h>
#endif

#ifdef Q_OS_WIN
#include "QtCore/qt_windows.h"
typedef HANDLE Q_PIPE;
#define INVALID_Q_PIPE INVALID_HANDLE_VALUE
#else
typedef int Q_PIPE;
#define INVALID_Q_PIPE -1
#endif

#ifndef QT_NO_PROCESS

QT_BEGIN_NAMESPACE

class QSocketNotifier;
class QWindowsPipeReader;
class QWindowsPipeWriter;
class QWinEventNotifier;
class QTimer;

#ifdef Q_OS_WIN
class QProcEnvKey : public QString
{
public:
    QProcEnvKey() {}
    explicit QProcEnvKey(const QString &other) : QString(other) {}
    QProcEnvKey(const QProcEnvKey &other) : QString(other) {}
    bool operator==(const QProcEnvKey &other) const { return !compare(other, Qt::CaseInsensitive); }
};
inline uint qHash(const QProcEnvKey &key) { return qHash(key.toCaseFolded()); }

typedef QString QProcEnvValue;
#else
class QProcEnvKey
{
public:
    QProcEnvKey() : hash(0) {}
    explicit QProcEnvKey(const QByteArray &other) : key(other), hash(qHash(key)) {}
    QProcEnvKey(const QProcEnvKey &other) { *this = other; }
    bool operator==(const QProcEnvKey &other) const { return key == other.key; }

    QByteArray key;
    uint hash;
};
inline uint qHash(const QProcEnvKey &key) { return key.hash; }

class QProcEnvValue
{
public:
    QProcEnvValue() {}
    QProcEnvValue(const QProcEnvValue &other) { *this = other; }
    explicit QProcEnvValue(const QString &value) : stringValue(value) {}
    explicit QProcEnvValue(const QByteArray &value) : byteValue(value) {}
    bool operator==(const QProcEnvValue &other) const
    {
        return byteValue.isEmpty() && other.byteValue.isEmpty()
                ? stringValue == other.stringValue
                : bytes() == other.bytes();
    }
    QByteArray bytes() const
    {
        if (byteValue.isEmpty() && !stringValue.isEmpty())
            byteValue = stringValue.toLocal8Bit();
        return byteValue;
    }
    QString string() const
    {
        if (stringValue.isEmpty() && !byteValue.isEmpty())
            stringValue = QString::fromLocal8Bit(byteValue);
        return stringValue;
    }

    mutable QByteArray byteValue;
    mutable QString stringValue;
};
Q_DECLARE_TYPEINFO(QProcEnvValue, Q_MOVABLE_TYPE);
#endif
Q_DECLARE_TYPEINFO(QProcEnvKey, Q_MOVABLE_TYPE);

class QProcessEnvironmentPrivate: public QSharedData
{
public:
    typedef QProcEnvKey Key;
    typedef QProcEnvValue Value;
#ifdef Q_OS_WIN
    inline Key prepareName(const QString &name) const { return Key(name); }
    inline QString nameToString(const Key &name) const { return name; }
    inline Value prepareValue(const QString &value) const { return value; }
    inline QString valueToString(const Value &value) const { return value; }
    struct MutexLocker {
        MutexLocker(const QProcessEnvironmentPrivate *) {}
    };
    struct OrderedMutexLocker {
        OrderedMutexLocker(const QProcessEnvironmentPrivate *,
                           const QProcessEnvironmentPrivate *) {}
    };
#else
    inline Key prepareName(const QString &name) const
    {
        Key &ent = nameMap[name];
        if (ent.key.isEmpty())
            ent = Key(name.toLocal8Bit());
        return ent;
    }
    inline QString nameToString(const Key &name) const
    {
        const QString sname = QString::fromLocal8Bit(name.key);
        nameMap[sname] = name;
        return sname;
    }
    inline Value prepareValue(const QString &value) const { return Value(value); }
    inline QString valueToString(const Value &value) const { return value.string(); }

    struct MutexLocker : public QMutexLocker
    {
        MutexLocker(const QProcessEnvironmentPrivate *d) : QMutexLocker(&d->mutex) {}
    };
    struct OrderedMutexLocker : public QOrderedMutexLocker
    {
        OrderedMutexLocker(const QProcessEnvironmentPrivate *d1,
                           const QProcessEnvironmentPrivate *d2) :
            QOrderedMutexLocker(&d1->mutex, &d2->mutex)
        {}
    };

    QProcessEnvironmentPrivate() : QSharedData() {}
    QProcessEnvironmentPrivate(const QProcessEnvironmentPrivate &other) :
        QSharedData()
    {
        // This being locked ensures that the functions that only assign
        // d pointers don't need explicit locking.
        // We don't need to lock our own mutex, as this object is new and
        // consequently not shared. For the same reason, non-const methods
        // do not need a lock, as they detach objects (however, we need to
        // ensure that they really detach before using prepareName()).
        MutexLocker locker(&other);
        hash = other.hash;
        nameMap = other.nameMap;
        // We need to detach our members, so that our mutex can protect them.
        // As we are being detached, they likely would be detached a moment later anyway.
        hash.detach();
        nameMap.detach();
    }
#endif

    typedef QHash<Key, Value> Hash;
    Hash hash;

#ifdef Q_OS_UNIX
    typedef QHash<QString, Key> NameHash;
    mutable NameHash nameMap;

    mutable QMutex mutex;
#endif

    static QProcessEnvironment fromList(const QStringList &list);
    QStringList toList() const;
    QStringList keys() const;
    void insert(const QProcessEnvironmentPrivate &other);
};

template<> Q_INLINE_TEMPLATE void QSharedDataPointer<QProcessEnvironmentPrivate>::detach()
{
    if (d && d->ref.load() == 1)
        return;
    QProcessEnvironmentPrivate *x = (d ? new QProcessEnvironmentPrivate(*d)
                                     : new QProcessEnvironmentPrivate);
    x->ref.ref();
    if (d && !d->ref.deref())
        delete d;
    d = x;
}

class QProcessPrivate : public QIODevicePrivate
{
public:
    Q_DECLARE_PUBLIC(QProcess)

    struct Channel {
        enum ProcessChannelType {
            Normal = 0,
            PipeSource = 1,
            PipeSink = 2,
            Redirect = 3
            // if you add "= 4" here, increase the number of bits below
        };

        Channel() : process(0), notifier(0), type(Normal), closed(false), append(false)
        {
            pipe[0] = INVALID_Q_PIPE;
            pipe[1] = INVALID_Q_PIPE;
#ifdef Q_OS_WIN
            reader = 0;
#endif
        }

        void clear();

        Channel &operator=(const QString &fileName)
        {
            clear();
            file = fileName;
            type = fileName.isEmpty() ? Normal : Redirect;
            return *this;
        }

        void pipeTo(QProcessPrivate *other)
        {
            clear();
            process = other;
            type = PipeSource;
        }

        void pipeFrom(QProcessPrivate *other)
        {
            clear();
            process = other;
            type = PipeSink;
        }

        QString file;
        QProcessPrivate *process;
        QSocketNotifier *notifier;
#ifdef Q_OS_WIN
        union {
            QWindowsPipeReader *reader;
            QWindowsPipeWriter *writer;
        };
#endif
        QRingBuffer buffer;
        Q_PIPE pipe[2];

        unsigned type : 2;
        bool closed : 1;
        bool append : 1;
    };

    QProcessPrivate();
    virtual ~QProcessPrivate();

    // private slots
    bool _q_canReadStandardOutput();
    bool _q_canReadStandardError();
    bool _q_canWrite();
    bool _q_startupNotification();
    bool _q_processDied();
    void _q_notified();

    QProcess::ProcessChannel processChannel;
    QProcess::ProcessChannelMode processChannelMode;
    QProcess::InputChannelMode inputChannelMode;
    QProcess::ProcessError processError;
    QProcess::ProcessState processState;
    QString workingDirectory;
    Q_PID pid;
    int sequenceNumber;

    bool dying;
    bool emittedReadyRead;
    bool emittedBytesWritten;

    Channel stdinChannel;
    Channel stdoutChannel;
    Channel stderrChannel;
    bool openChannel(Channel &channel);
    void closeChannel(Channel *channel);
    void closeWriteChannel();
    bool tryReadFromChannel(Channel *channel); // obviously, only stdout and stderr

    QString program;
    QStringList arguments;
#if defined(Q_OS_WIN)
    QString nativeArguments;
#endif
    QProcessEnvironment environment;

    Q_PIPE childStartedPipe[2];
    Q_PIPE deathPipe[2];
    void destroyPipe(Q_PIPE pipe[2]);

    QSocketNotifier *startupSocketNotifier;
    QSocketNotifier *deathNotifier;

#ifdef Q_OS_WIN
    // the wonderful windows notifier
    QTimer *notifier;
    QWinEventNotifier *processFinishedNotifier;
#endif

    void start(QIODevice::OpenMode mode);
    void startProcess();
#if defined(Q_OS_UNIX) && !defined(Q_OS_QNX)
    void execChild(const char *workingDirectory, char **path, char **argv, char **envp);
#elif defined(Q_OS_QNX)
    pid_t spawnChild(const char *workingDirectory, char **argv, char **envp);
#endif
    bool processStarted();
    void terminateProcess();
    void killProcess();
    void findExitCode();
#ifdef Q_OS_UNIX
    bool waitForDeadChild();
#endif
#ifdef Q_OS_WIN
    bool drainOutputPipes();
    void flushPipeWriter();
    qint64 pipeWriterBytesToWrite() const;
#endif

    static bool startDetached(const QString &program, const QStringList &arguments, const QString &workingDirectory = QString(),
                              qint64 *pid = 0);

    int exitCode;
    QProcess::ExitStatus exitStatus;
    bool crashed;
#ifdef Q_OS_UNIX
    int serial;
#endif

    bool waitForStarted(int msecs = 30000);
    bool waitForReadyRead(int msecs = 30000);
    bool waitForBytesWritten(int msecs = 30000);
    bool waitForFinished(int msecs = 30000);
    bool waitForWrite(int msecs = 30000);

    qint64 bytesAvailableInChannel(const Channel *channel) const;
    qint64 readFromChannel(const Channel *channel, char *data, qint64 maxlen);
    qint64 writeToStdin(const char *data, qint64 maxlen);

    void cleanup();

#ifdef Q_OS_BLACKBERRY
    QList<QSocketNotifier *> defaultNotifiers() const;
#endif // Q_OS_BLACKBERRY

#ifdef Q_OS_UNIX
    static void initializeProcessManager();
#endif
};

QT_END_NAMESPACE

#endif // QT_NO_PROCESS

#endif // QPROCESS_P_H
