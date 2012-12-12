/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

//#define QPROCESS_DEBUG

#if defined QPROCESS_DEBUG
#include <qdebug.h>
#include <qstring.h>
#include <ctype.h>
#if !defined(Q_OS_WINCE)
#include <errno.h>
#endif

QT_BEGIN_NAMESPACE
/*
    Returns a human readable representation of the first \a len
    characters in \a data.
*/
static QByteArray qt_prettyDebug(const char *data, int len, int maxSize)
{
    if (!data) return "(null)";
    QByteArray out;
    for (int i = 0; i < len && i < maxSize; ++i) {
        char c = data[i];
        if (isprint(c)) {
            out += c;
        } else switch (c) {
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            char buf[5];
            qsnprintf(buf, sizeof(buf), "\\%3o", c);
            buf[4] = '\0';
            out += QByteArray(buf);
        }
    }

    if (len < maxSize)
        out += "...";

    return out;
}

QT_END_NAMESPACE

#endif

#include "qprocess.h"
#include "qprocess_p.h"

#include <qbytearray.h>
#include <qelapsedtimer.h>
#include <qcoreapplication.h>
#include <qsocketnotifier.h>
#include <qtimer.h>

#ifdef Q_WS_WIN
#include <private/qwineventnotifier_p.h>
#endif

#ifdef Q_OS_SYMBIAN
#include <e32std.h>
#endif

#ifndef QT_NO_PROCESS

QT_BEGIN_NAMESPACE

/*!
    \class QProcessEnvironment

    \brief The QProcessEnvironment class holds the environment variables that
    can be passed to a program.

    \ingroup io
    \ingroup misc
    \mainclass
    \reentrant
    \since 4.6

    A process's environment is composed of a set of key=value pairs known as
    environment variables. The QProcessEnvironment class wraps that concept
    and allows easy manipulation of those variables. It's meant to be used
    along with QProcess, to set the environment for child processes. It
    cannot be used to change the current process's environment.

    The environment of the calling process can be obtained using
    QProcessEnvironment::systemEnvironment().

    On Unix systems, the variable names are case-sensitive. For that reason,
    this class will not touch the names of the variables. Note as well that
    Unix environment allows both variable names and contents to contain arbitrary
    binary data (except for the NUL character), but this is not supported by
    QProcessEnvironment. This class only supports names and values that are
    encodable by the current locale settings (see QTextCodec::codecForLocale).

    On Windows, the variable names are case-insensitive. Therefore,
    QProcessEnvironment will always uppercase the names and do case-insensitive
    comparisons.

    On Windows CE, the concept of environment does not exist. This class will
    keep the values set for compatibility with other platforms, but the values
    set will have no effect on the processes being created.

    \sa QProcess, QProcess::systemEnvironment(), QProcess::setProcessEnvironment()
*/

QStringList QProcessEnvironmentPrivate::toList() const
{
    QStringList result;
    result.reserve(hash.size());
    Hash::ConstIterator it = hash.constBegin(),
                       end = hash.constEnd();
    for ( ; it != end; ++it) {
        QString data = nameToString(it.key());
        QString value = valueToString(it.value());
        data.reserve(data.length() + value.length() + 1);
        data.append(QLatin1Char('='));
        data.append(value);
        result << data;
    }
    return result;
}

QProcessEnvironment QProcessEnvironmentPrivate::fromList(const QStringList &list)
{
    QProcessEnvironment env;
    QStringList::ConstIterator it = list.constBegin(),
                              end = list.constEnd();
    for ( ; it != end; ++it) {
        int pos = it->indexOf(QLatin1Char('='));
        if (pos < 1)
            continue;

        QString value = it->mid(pos + 1);
        QString name = *it;
        name.truncate(pos);
        env.insert(name, value);
    }
    return env;
}

QStringList QProcessEnvironmentPrivate::keys() const
{
    QStringList result;
    result.reserve(hash.size());
    Hash::ConstIterator it = hash.constBegin(),
                       end = hash.constEnd();
    for ( ; it != end; ++it)
        result << nameToString(it.key());
    return result;
}

void QProcessEnvironmentPrivate::insert(const QProcessEnvironmentPrivate &other)
{
    Hash::ConstIterator it = other.hash.constBegin(),
                       end = other.hash.constEnd();
    for ( ; it != end; ++it)
        hash.insert(it.key(), it.value());

#ifdef Q_OS_UNIX
    QHash<QString, Key>::ConstIterator nit = other.nameMap.constBegin(),
                                      nend = other.nameMap.constEnd();
    for ( ; nit != nend; ++nit)
        nameMap.insert(nit.key(), nit.value());
#endif
}

/*!
    Creates a new QProcessEnvironment object. This constructor creates an
    empty environment. If set on a QProcess, this will cause the current
    environment variables to be removed.
*/
QProcessEnvironment::QProcessEnvironment()
    : d(0)
{
}

/*!
    Frees the resources associated with this QProcessEnvironment object.
*/
QProcessEnvironment::~QProcessEnvironment()
{
}

/*!
    Creates a QProcessEnvironment object that is a copy of \a other.
*/
QProcessEnvironment::QProcessEnvironment(const QProcessEnvironment &other)
    : d(other.d)
{
}

/*!
    Copies the contents of the \a other QProcessEnvironment object into this
    one.
*/
QProcessEnvironment &QProcessEnvironment::operator=(const QProcessEnvironment &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn bool QProcessEnvironment::operator !=(const QProcessEnvironment &other) const

    Returns true if this and the \a other QProcessEnvironment objects are different.

    \sa operator==()
*/

/*!
    Returns true if this and the \a other QProcessEnvironment objects are equal.

    Two QProcessEnvironment objects are considered equal if they have the same
    set of key=value pairs. The comparison of keys is done case-sensitive on
    platforms where the environment is case-sensitive.

    \sa operator!=(), contains()
*/
bool QProcessEnvironment::operator==(const QProcessEnvironment &other) const
{
    return d == other.d || (d && other.d && d->hash == other.d->hash);
}

/*!
    Returns true if this QProcessEnvironment object is empty: that is
    there are no key=value pairs set.

    \sa clear(), systemEnvironment(), insert()
*/
bool QProcessEnvironment::isEmpty() const
{
    return d ? d->hash.isEmpty() : true;
}

/*!
    Removes all key=value pairs from this QProcessEnvironment object, making
    it empty.

    \sa isEmpty(), systemEnvironment()
*/
void QProcessEnvironment::clear()
{
    if (d)
        d->hash.clear();
    // Unix: Don't clear d->nameMap, as the environment is likely to be
    // re-populated with the same keys again.
}

/*!
    Returns true if the environment variable of name \a name is found in
    this QProcessEnvironment object.

    On Windows, variable names are case-insensitive, so the key is converted
    to uppercase before searching. On other systems, names are case-sensitive
    so no trasformation is applied.

    \sa insert(), value()
*/
bool QProcessEnvironment::contains(const QString &name) const
{
    return d ? d->hash.contains(d->prepareName(name)) : false;
}

/*!
    Inserts the environment variable of name \a name and contents \a value
    into this QProcessEnvironment object. If that variable already existed,
    it is replaced by the new value.

    On Windows, variable names are case-insensitive, so this function always
    uppercases the variable name before inserting. On other systems, names
    are case-sensitive, so no transformation is applied.

    On most systems, inserting a variable with no contents will have the
    same effect for applications as if the variable had not been set at all.
    However, to guarantee that there are no incompatibilities, to remove a
    variable, please use the remove() function.

    \sa contains(), remove(), value()
*/
void QProcessEnvironment::insert(const QString &name, const QString &value)
{
    // d detaches from null
    d->hash.insert(d->prepareName(name), d->prepareValue(value));
}

/*!
    Removes the environment variable identified by \a name from this
    QProcessEnvironment object. If that variable did not exist before,
    nothing happens.

    On Windows, variable names are case-insensitive, so the key is converted
    to uppercase before searching. On other systems, names are case-sensitive
    so no trasformation is applied.

    \sa contains(), insert(), value()
*/
void QProcessEnvironment::remove(const QString &name)
{
    if (d)
        d->hash.remove(d->prepareName(name));
}

/*!
    Searches this QProcessEnvironment object for a variable identified by
    \a name and returns its value. If the variable is not found in this object,
    then \a defaultValue is returned instead.

    On Windows, variable names are case-insensitive, so the key is converted
    to uppercase before searching. On other systems, names are case-sensitive
    so no trasformation is applied.

    \sa contains(), insert(), remove()
*/
QString QProcessEnvironment::value(const QString &name, const QString &defaultValue) const
{
    if (!d)
        return defaultValue;

    QProcessEnvironmentPrivate::Hash::ConstIterator it = d->hash.constFind(d->prepareName(name));
    if (it == d->hash.constEnd())
        return defaultValue;

    return d->valueToString(it.value());
}

/*!
    Converts this QProcessEnvironment object into a list of strings, one for
    each environment variable that is set. The environment variable's name
    and its value are separated by an equal character ('=').

    The QStringList contents returned by this function are suitable for use
    with the QProcess::setEnvironment function. However, it is recommended
    to use QProcess::setProcessEnvironment instead since that will avoid
    unnecessary copying of the data.

    \sa systemEnvironment(), QProcess::systemEnvironment(), QProcess::environment(),
        QProcess::setEnvironment()
*/
QStringList QProcessEnvironment::toStringList() const
{
    return d ? d->toList() : QStringList();
}

/*!
    \since 4.8

    Returns a list containing all the variable names in this QProcessEnvironment
    object.
*/
QStringList QProcessEnvironment::keys() const
{
    return d ? d->keys() : QStringList();
}

/*!
    \overload
    \since 4.8

    Inserts the contents of \a e in this QProcessEnvironment object. Variables in
    this object that also exist in \a e will be overwritten.
*/
void QProcessEnvironment::insert(const QProcessEnvironment &e)
{
    if (!e.d)
        return;

    // d detaches from null
    d->insert(*e.d);
}

void QProcessPrivate::Channel::clear()
{
    switch (type) {
    case PipeSource:
        Q_ASSERT(process);
        process->stdinChannel.type = Normal;
        process->stdinChannel.process = 0;
        break;
    case PipeSink:
        Q_ASSERT(process);
        process->stdoutChannel.type = Normal;
        process->stdoutChannel.process = 0;
        break;
    }

    type = Normal;
    file.clear();
    process = 0;
}

/*! \fn bool QProcessPrivate::startDetached(const QString &program, const QStringList &arguments, const QString &workingDirectory, qint64 *pid)

\internal
 */

/*!
    \class QProcess

    \brief The QProcess class is used to start external programs and
    to communicate with them.

    \ingroup io

    \reentrant

    \section1 Running a Process

    To start a process, pass the name and command line arguments of
    the program you want to run as arguments to start(). Arguments
    are supplied as individual strings in a QStringList.

    For example, the following code snippet runs the analog clock
    example in the Motif style on X11 platforms by passing strings
    containing "-style" and "motif" as two items in the list of
    arguments:

    \snippet doc/src/snippets/qprocess/qprocess-simpleexecution.cpp 0
    \dots
    \snippet doc/src/snippets/qprocess/qprocess-simpleexecution.cpp 1
    \snippet doc/src/snippets/qprocess/qprocess-simpleexecution.cpp 2

    QProcess then enters the \l Starting state, and when the program
    has started, QProcess enters the \l Running state and emits
    started().

    QProcess allows you to treat a process as a sequential I/O
    device. You can write to and read from the process just as you
    would access a network connection using QTcpSocket. You can then
    write to the process's standard input by calling write(), and
    read the standard output by calling read(), readLine(), and
    getChar(). Because it inherits QIODevice, QProcess can also be
    used as an input source for QXmlReader, or for generating data to
    be uploaded using QFtp.

    \note On Windows CE and Symbian, reading and writing to a process
    is not supported.

    When the process exits, QProcess reenters the \l NotRunning state
    (the initial state), and emits finished().

    The finished() signal provides the exit code and exit status of
    the process as arguments, and you can also call exitCode() to
    obtain the exit code of the last process that finished, and
    exitStatus() to obtain its exit status. If an error occurs at
    any point in time, QProcess will emit the error() signal. You
    can also call error() to find the type of error that occurred
    last, and state() to find the current process state.

    \section1 Communicating via Channels

    Processes have two predefined output channels: The standard
    output channel (\c stdout) supplies regular console output, and
    the standard error channel (\c stderr) usually supplies the
    errors that are printed by the process. These channels represent
    two separate streams of data. You can toggle between them by
    calling setReadChannel(). QProcess emits readyRead() when data is
    available on the current read channel. It also emits
    readyReadStandardOutput() when new standard output data is
    available, and when new standard error data is available,
    readyReadStandardError() is emitted. Instead of calling read(),
    readLine(), or getChar(), you can explicitly read all data from
    either of the two channels by calling readAllStandardOutput() or
    readAllStandardError().

    The terminology for the channels can be misleading. Be aware that
    the process's output channels correspond to QProcess's
    \e read channels, whereas the process's input channels correspond
    to QProcess's \e write channels. This is because what we read
    using QProcess is the process's output, and what we write becomes
    the process's input.

    QProcess can merge the two output channels, so that standard
    output and standard error data from the running process both use
    the standard output channel. Call setProcessChannelMode() with
    MergedChannels before starting the process to activative
    this feature. You also have the option of forwarding the output of
    the running process to the calling, main process, by passing
    ForwardedChannels as the argument.

    Certain processes need special environment settings in order to
    operate. You can set environment variables for your process by
    calling setEnvironment(). To set a working directory, call
    setWorkingDirectory(). By default, processes are run in the
    current working directory of the calling process.

    \note On Symbian, setting environment or working directory
    is not supported. The working directory will always be the private
    directory of the running process.

    \note On QNX, setting the working directory may cause all
    application threads, with the exception of the QProcess caller
    thread, to temporarily freeze, owing to a limitation in
    the operating system.

    \section1 Synchronous Process API

    QProcess provides a set of functions which allow it to be used
    without an event loop, by suspending the calling thread until
    certain signals are emitted:

    \list
    \o waitForStarted() blocks until the process has started.

    \o waitForReadyRead() blocks until new data is
    available for reading on the current read channel.

    \o waitForBytesWritten() blocks until one payload of
    data has been written to the process.

    \o waitForFinished() blocks until the process has finished.
    \endlist

    Calling these functions from the main thread (the thread that
    calls QApplication::exec()) may cause your user interface to
    freeze.

    The following example runs \c gzip to compress the string "Qt
    rocks!", without an event loop:

    \snippet doc/src/snippets/process/process.cpp 0

    \section1 Notes for Windows Users

    Some Windows commands (for example, \c dir) are not provided by
    separate applications, but by the command interpreter itself.
    If you attempt to use QProcess to execute these commands directly,
    it won't work. One possible solution is to execute the command
    interpreter itself (\c{cmd.exe} on some Windows systems), and ask
    the interpreter to execute the desired command.

    \section1 Symbian Platform Security Requirements

    On Symbian, processes which use the functions kill() or terminate()
    must have the \c PowerMgmt platform security capability. If the client
    process lacks this capability, these functions will fail.

    Platform security capabilities are added via the
    \l{qmake-variable-reference.html#target-capability}{TARGET.CAPABILITY}
    qmake variable.

    \sa QBuffer, QFile, QTcpSocket
*/

/*!
    \enum QProcess::ProcessChannel

    This enum describes the process channels used by the running process.
    Pass one of these values to setReadChannel() to set the
    current read channel of QProcess.

    \value StandardOutput The standard output (stdout) of the running
           process.

    \value StandardError The standard error (stderr) of the running
           process.

    \sa setReadChannel()
*/

/*!
    \enum QProcess::ProcessChannelMode

    This enum describes the process channel modes of QProcess. Pass
    one of these values to setProcessChannelMode() to set the
    current read channel mode.

    \value SeparateChannels QProcess manages the output of the
    running process, keeping standard output and standard error data
    in separate internal buffers. You can select the QProcess's
    current read channel by calling setReadChannel(). This is the
    default channel mode of QProcess.

    \value MergedChannels QProcess merges the output of the running
    process into the standard output channel (\c stdout). The
    standard error channel (\c stderr) will not receive any data. The
    standard output and standard error data of the running process
    are interleaved.

    \value ForwardedChannels QProcess forwards the output of the
    running process onto the main process. Anything the child process
    writes to its standard output and standard error will be written
    to the standard output and standard error of the main process.

    \sa setProcessChannelMode()
*/

/*!
    \enum QProcess::ProcessError

    This enum describes the different types of errors that are
    reported by QProcess.

    \value FailedToStart The process failed to start. Either the
    invoked program is missing, or you may have insufficient
    permissions to invoke the program.

    \value Crashed The process crashed some time after starting
    successfully.

    \value Timedout The last waitFor...() function timed out. The
    state of QProcess is unchanged, and you can try calling
    waitFor...() again.

    \value WriteError An error occurred when attempting to write to the
    process. For example, the process may not be running, or it may
    have closed its input channel.

    \value ReadError An error occurred when attempting to read from
    the process. For example, the process may not be running.

    \value UnknownError An unknown error occurred. This is the default
    return value of error().

    \sa error()
*/

/*!
    \enum QProcess::ProcessState

    This enum describes the different states of QProcess.

    \value NotRunning The process is not running.

    \value Starting The process is starting, but the program has not
    yet been invoked.

    \value Running The process is running and is ready for reading and
    writing.

    \sa state()
*/

/*!
    \enum QProcess::ExitStatus

    This enum describes the different exit statuses of QProcess.

    \value NormalExit The process exited normally.

    \value CrashExit The process crashed.

    \sa exitStatus()
*/

/*!
    \fn void QProcess::error(QProcess::ProcessError error)

    This signal is emitted when an error occurs with the process. The
    specified \a error describes the type of error that occurred.
*/

/*!
    \fn void QProcess::started()

    This signal is emitted by QProcess when the process has started,
    and state() returns \l Running.
*/

/*!
    \fn void QProcess::stateChanged(QProcess::ProcessState newState)

    This signal is emitted whenever the state of QProcess changes. The
    \a newState argument is the state QProcess changed to.
*/

/*!
    \fn void QProcess::finished(int exitCode)
    \obsolete
    \overload

    Use finished(int exitCode, QProcess::ExitStatus status) instead.
*/

/*!
    \fn void QProcess::finished(int exitCode, QProcess::ExitStatus exitStatus)

    This signal is emitted when the process finishes. \a exitCode is the exit
    code of the process, and \a exitStatus is the exit status.  After the
    process has finished, the buffers in QProcess are still intact. You can
    still read any data that the process may have written before it finished.

    \sa exitStatus()
*/

/*!
    \fn void QProcess::readyReadStandardOutput()

    This signal is emitted when the process has made new data
    available through its standard output channel (\c stdout). It is
    emitted regardless of the current \l{readChannel()}{read channel}.

    \sa readAllStandardOutput(), readChannel()
*/

/*!
    \fn void QProcess::readyReadStandardError()

    This signal is emitted when the process has made new data
    available through its standard error channel (\c stderr). It is
    emitted regardless of the current \l{readChannel()}{read
    channel}.

    \sa readAllStandardError(), readChannel()
*/

/*! \internal
*/
QProcessPrivate::QProcessPrivate()
{
    processChannel = QProcess::StandardOutput;
    processChannelMode = QProcess::SeparateChannels;
    processError = QProcess::UnknownError;
    processState = QProcess::NotRunning;
    pid = 0;
    sequenceNumber = 0;
    exitCode = 0;
    exitStatus = QProcess::NormalExit;
    startupSocketNotifier = 0;
    deathNotifier = 0;
    notifier = 0;
    pipeWriter = 0;
    childStartedPipe[0] = INVALID_Q_PIPE;
    childStartedPipe[1] = INVALID_Q_PIPE;
    deathPipe[0] = INVALID_Q_PIPE;
    deathPipe[1] = INVALID_Q_PIPE;
    exitCode = 0;
    crashed = false;
    dying = false;
    emittedReadyRead = false;
    emittedBytesWritten = false;
#ifdef Q_WS_WIN
    pipeWriter = 0;
    processFinishedNotifier = 0;
#endif // Q_WS_WIN
#ifdef Q_OS_UNIX
    serial = 0;
#endif
#ifdef Q_OS_SYMBIAN
    symbianProcess = NULL;
    processLaunched = false;
#endif
}

/*! \internal
*/
QProcessPrivate::~QProcessPrivate()
{
    if (stdinChannel.process)
        stdinChannel.process->stdoutChannel.clear();
    if (stdoutChannel.process)
        stdoutChannel.process->stdinChannel.clear();
}

/*! \internal
*/
void QProcessPrivate::cleanup()
{
    q_func()->setProcessState(QProcess::NotRunning);
#ifdef Q_OS_WIN
    if (pid) {
        CloseHandle(pid->hThread);
        CloseHandle(pid->hProcess);
        delete pid;
        pid = 0;
    }
    if (processFinishedNotifier) {
        processFinishedNotifier->setEnabled(false);
        qDeleteInEventHandler(processFinishedNotifier);
        processFinishedNotifier = 0;
    }

#endif
    pid = 0;
    sequenceNumber = 0;
    dying = false;

    if (stdoutChannel.notifier) {
        stdoutChannel.notifier->setEnabled(false);
        qDeleteInEventHandler(stdoutChannel.notifier);
        stdoutChannel.notifier = 0;
    }
    if (stderrChannel.notifier) {
        stderrChannel.notifier->setEnabled(false);
        qDeleteInEventHandler(stderrChannel.notifier);
        stderrChannel.notifier = 0;
    }
    if (stdinChannel.notifier) {
        stdinChannel.notifier->setEnabled(false);
        qDeleteInEventHandler(stdinChannel.notifier);
        stdinChannel.notifier = 0;
    }
    if (startupSocketNotifier) {
        startupSocketNotifier->setEnabled(false);
        qDeleteInEventHandler(startupSocketNotifier);
        startupSocketNotifier = 0;
    }
    if (deathNotifier) {
        deathNotifier->setEnabled(false);
        qDeleteInEventHandler(deathNotifier);
        deathNotifier = 0;
    }
    if (notifier) {
        qDeleteInEventHandler(notifier);
        notifier = 0;
    }
    destroyPipe(stdoutChannel.pipe);
    destroyPipe(stderrChannel.pipe);
    destroyPipe(stdinChannel.pipe);
    destroyPipe(childStartedPipe);
    destroyPipe(deathPipe);
#ifdef Q_OS_UNIX
    serial = 0;
#endif
#ifdef Q_OS_SYMBIAN
    if (symbianProcess) {
        symbianProcess->Close();
        delete symbianProcess;
        symbianProcess = NULL;
    }
#endif
}

/*! \internal
*/
bool QProcessPrivate::_q_canReadStandardOutput()
{
    Q_Q(QProcess);
    qint64 available = bytesAvailableFromStdout();
    if (available == 0) {
        if (stdoutChannel.notifier)
            stdoutChannel.notifier->setEnabled(false);
        destroyPipe(stdoutChannel.pipe);
#if defined QPROCESS_DEBUG
        qDebug("QProcessPrivate::canReadStandardOutput(), 0 bytes available");
#endif
        return false;
    }

    char *ptr = outputReadBuffer.reserve(available);
    qint64 readBytes = readFromStdout(ptr, available);
    if (readBytes == -1) {
        processError = QProcess::ReadError;
        q->setErrorString(QProcess::tr("Error reading from process"));
        emit q->error(processError);
#if defined QPROCESS_DEBUG
        qDebug("QProcessPrivate::canReadStandardOutput(), failed to read from the process");
#endif
        return false;
    }
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::canReadStandardOutput(), read %d bytes from the process' output",
            int(readBytes));
#endif

    if (stdoutChannel.closed) {
        outputReadBuffer.chop(readBytes);
        return false;
    }

    outputReadBuffer.chop(available - readBytes);

    bool didRead = false;
    if (readBytes == 0) {
        if (stdoutChannel.notifier)
            stdoutChannel.notifier->setEnabled(false);
    } else if (processChannel == QProcess::StandardOutput) {
        didRead = true;
        if (!emittedReadyRead) {
            emittedReadyRead = true;
            emit q->readyRead();
            emittedReadyRead = false;
        }
    }
    emit q->readyReadStandardOutput();
    return didRead;
}

/*! \internal
*/
bool QProcessPrivate::_q_canReadStandardError()
{
    Q_Q(QProcess);
    qint64 available = bytesAvailableFromStderr();
    if (available == 0) {
        if (stderrChannel.notifier)
            stderrChannel.notifier->setEnabled(false);
        destroyPipe(stderrChannel.pipe);
        return false;
    }

    char *ptr = errorReadBuffer.reserve(available);
    qint64 readBytes = readFromStderr(ptr, available);
    if (readBytes == -1) {
        processError = QProcess::ReadError;
        q->setErrorString(QProcess::tr("Error reading from process"));
        emit q->error(processError);
        return false;
    }
    if (stderrChannel.closed) {
        errorReadBuffer.chop(readBytes);
        return false;
    }

    errorReadBuffer.chop(available - readBytes);

    bool didRead = false;
    if (readBytes == 0) {
        if (stderrChannel.notifier)
            stderrChannel.notifier->setEnabled(false);
    } else if (processChannel == QProcess::StandardError) {
        didRead = true;
        if (!emittedReadyRead) {
            emittedReadyRead = true;
            emit q->readyRead();
            emittedReadyRead = false;
        }
    }
    emit q->readyReadStandardError();
    return didRead;
}

/*! \internal
*/
bool QProcessPrivate::_q_canWrite()
{
    Q_Q(QProcess);
    if (stdinChannel.notifier)
        stdinChannel.notifier->setEnabled(false);

    if (writeBuffer.isEmpty()) {
#if defined QPROCESS_DEBUG
        qDebug("QProcessPrivate::canWrite(), not writing anything (empty write buffer).");
#endif
        return false;
    }

    qint64 written = writeToStdin(writeBuffer.readPointer(),
                                      writeBuffer.nextDataBlockSize());
    if (written < 0) {
        destroyPipe(stdinChannel.pipe);
        processError = QProcess::WriteError;
        q->setErrorString(QProcess::tr("Error writing to process"));
        emit q->error(processError);
        return false;
    }

#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::canWrite(), wrote %d bytes to the process input", int(written));
#endif

    if (written != 0) {
        writeBuffer.free(written);
        if (!emittedBytesWritten) {
            emittedBytesWritten = true;
            emit q->bytesWritten(written);
            emittedBytesWritten = false;
        }
    }
    if (stdinChannel.notifier && !writeBuffer.isEmpty())
        stdinChannel.notifier->setEnabled(true);
    if (writeBuffer.isEmpty() && stdinChannel.closed)
        closeWriteChannel();
    return true;
}

/*! \internal
*/
bool QProcessPrivate::_q_processDied()
{
    Q_Q(QProcess);
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::_q_processDied()");
#endif
#ifdef Q_OS_UNIX
    if (!waitForDeadChild())
        return false;
#endif
#ifdef Q_OS_WIN
    if (processFinishedNotifier)
        processFinishedNotifier->setEnabled(false);
#endif

    // the process may have died before it got a chance to report that it was
    // either running or stopped, so we will call _q_startupNotification() and
    // give it a chance to emit started() or error(FailedToStart).
    if (processState == QProcess::Starting) {
        if (!_q_startupNotification())
            return true;
    }

    if (dying) {
        // at this point we know the process is dead. prevent
        // reentering this slot recursively by calling waitForFinished()
        // or opening a dialog inside slots connected to the readyRead
        // signals emitted below.
        return true;
    }
    dying = true;

    // in case there is data in the pipe line and this slot by chance
    // got called before the read notifications, call these two slots
    // so the data is made available before the process dies.
    _q_canReadStandardOutput();
    _q_canReadStandardError();

    findExitCode();

    if (crashed) {
        exitStatus = QProcess::CrashExit;
        processError = QProcess::Crashed;
        q->setErrorString(QProcess::tr("Process crashed"));
        emit q->error(processError);
    }

    bool wasRunning = (processState == QProcess::Running);

    cleanup();

    if (wasRunning) {
        // we received EOF now:
        emit q->readChannelFinished();
        // in the future:
        //emit q->standardOutputClosed();
        //emit q->standardErrorClosed();

        emit q->finished(exitCode);
        emit q->finished(exitCode, exitStatus);
    }
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::_q_processDied() process is dead");
#endif
    return true;
}

/*! \internal
*/
bool QProcessPrivate::_q_startupNotification()
{
    Q_Q(QProcess);
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::startupNotification()");
#endif

    if (startupSocketNotifier)
        startupSocketNotifier->setEnabled(false);
    if (processStarted()) {
        q->setProcessState(QProcess::Running);
        emit q->started();
        return true;
    }

    q->setProcessState(QProcess::NotRunning);
    processError = QProcess::FailedToStart;
    emit q->error(processError);
#ifdef Q_OS_UNIX
    // make sure the process manager removes this entry
    waitForDeadChild();
    findExitCode();
#endif
    cleanup();
    return false;
}

/*! \internal
*/
void QProcessPrivate::closeWriteChannel()
{
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::closeWriteChannel()");
#endif
    if (stdinChannel.notifier) {
        extern void qDeleteInEventHandler(QObject *o);
        stdinChannel.notifier->setEnabled(false);
        if (stdinChannel.notifier) {
            qDeleteInEventHandler(stdinChannel.notifier);
            stdinChannel.notifier = 0;
        }
    }
#ifdef Q_OS_WIN
    // ### Find a better fix, feeding the process little by little
    // instead.
    flushPipeWriter();
#endif
    destroyPipe(stdinChannel.pipe);
}

/*!
    Constructs a QProcess object with the given \a parent.
*/
QProcess::QProcess(QObject *parent)
    : QIODevice(*new QProcessPrivate, parent)
{
#if defined QPROCESS_DEBUG
    qDebug("QProcess::QProcess(%p)", parent);
#endif
}

/*!
    Destructs the QProcess object, i.e., killing the process.

    Note that this function will not return until the process is
    terminated.
*/
QProcess::~QProcess()
{
    Q_D(QProcess);
    if (d->processState != NotRunning) {
        qWarning("QProcess: Destroyed while process is still running.");
        kill();
        waitForFinished();
    }
#ifdef Q_OS_UNIX
    // make sure the process manager removes this entry
    d->findExitCode();
#endif
    d->cleanup();
}

/*!
    \obsolete
    Returns the read channel mode of the QProcess. This function is
    equivalent to processChannelMode()

    \sa processChannelMode()
*/
QProcess::ProcessChannelMode QProcess::readChannelMode() const
{
    return processChannelMode();
}

/*!
    \obsolete

    Use setProcessChannelMode(\a mode) instead.

    \sa setProcessChannelMode()
*/
void QProcess::setReadChannelMode(ProcessChannelMode mode)
{
    setProcessChannelMode(mode);
}

/*!
    \since 4.2

    Returns the channel mode of the QProcess standard output and
    standard error channels.

    \sa setProcessChannelMode(), ProcessChannelMode, setReadChannel()
*/
QProcess::ProcessChannelMode QProcess::processChannelMode() const
{
    Q_D(const QProcess);
    return d->processChannelMode;
}

/*!
    \since 4.2

    Sets the channel mode of the QProcess standard output and standard
    error channels to the \a mode specified.
    This mode will be used the next time start() is called. For example:

    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 0

    \sa processChannelMode(), ProcessChannelMode, setReadChannel()
*/
void QProcess::setProcessChannelMode(ProcessChannelMode mode)
{
    Q_D(QProcess);
    d->processChannelMode = mode;
}

/*!
    Returns the current read channel of the QProcess.

    \sa setReadChannel()
*/
QProcess::ProcessChannel QProcess::readChannel() const
{
    Q_D(const QProcess);
    return d->processChannel;
}

/*!
    Sets the current read channel of the QProcess to the given \a
    channel. The current input channel is used by the functions
    read(), readAll(), readLine(), and getChar(). It also determines
    which channel triggers QProcess to emit readyRead().

    \sa readChannel()
*/
void QProcess::setReadChannel(ProcessChannel channel)
{
    Q_D(QProcess);
    if (d->processChannel != channel) {
        QByteArray buf = d->buffer.readAll();
        if (d->processChannel == QProcess::StandardOutput) {
            for (int i = buf.size() - 1; i >= 0; --i)
                d->outputReadBuffer.ungetChar(buf.at(i));
        } else {
            for (int i = buf.size() - 1; i >= 0; --i)
                d->errorReadBuffer.ungetChar(buf.at(i));
        }
    }
    d->processChannel = channel;
}

/*!
    Closes the read channel \a channel. After calling this function,
    QProcess will no longer receive data on the channel. Any data that
    has already been received is still available for reading.

    Call this function to save memory, if you are not interested in
    the output of the process.

    \sa closeWriteChannel(), setReadChannel()
*/
void QProcess::closeReadChannel(ProcessChannel channel)
{
    Q_D(QProcess);

    if (channel == StandardOutput)
        d->stdoutChannel.closed = true;
    else
        d->stderrChannel.closed = true;
}

/*!
    Schedules the write channel of QProcess to be closed. The channel
    will close once all data has been written to the process. After
    calling this function, any attempts to write to the process will
    fail.

    Closing the write channel is necessary for programs that read
    input data until the channel has been closed. For example, the
    program "more" is used to display text data in a console on both
    Unix and Windows. But it will not display the text data until
    QProcess's write channel has been closed. Example:

    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 1

    The write channel is implicitly opened when start() is called.

    \sa closeReadChannel()
*/
void QProcess::closeWriteChannel()
{
    Q_D(QProcess);
    d->stdinChannel.closed = true; // closing
    if (d->writeBuffer.isEmpty())
        d->closeWriteChannel();
}

/*!
    \since 4.2

    Redirects the process' standard input to the file indicated by \a
    fileName. When an input redirection is in place, the QProcess
    object will be in read-only mode (calling write() will result in
    error).

    If the file \a fileName does not exist at the moment start() is
    called or is not readable, starting the process will fail.

    Calling setStandardInputFile() after the process has started has no
    effect.

    \sa setStandardOutputFile(), setStandardErrorFile(),
        setStandardOutputProcess()
*/
void QProcess::setStandardInputFile(const QString &fileName)
{
    Q_D(QProcess);
    d->stdinChannel = fileName;
}

/*!
    \since 4.2

    Redirects the process' standard output to the file \a
    fileName. When the redirection is in place, the standard output
    read channel is closed: reading from it using read() will always
    fail, as will readAllStandardOutput().

    If the file \a fileName doesn't exist at the moment start() is
    called, it will be created. If it cannot be created, the starting
    will fail.

    If the file exists and \a mode is QIODevice::Truncate, the file
    will be truncated. Otherwise (if \a mode is QIODevice::Append),
    the file will be appended to.

    Calling setStandardOutputFile() after the process has started has
    no effect.

    \sa setStandardInputFile(), setStandardErrorFile(),
        setStandardOutputProcess()
*/
void QProcess::setStandardOutputFile(const QString &fileName, OpenMode mode)
{
    Q_ASSERT(mode == Append || mode == Truncate);
    Q_D(QProcess);

    d->stdoutChannel = fileName;
    d->stdoutChannel.append = mode == Append;
}

/*!
    \since 4.2

    Redirects the process' standard error to the file \a
    fileName. When the redirection is in place, the standard error
    read channel is closed: reading from it using read() will always
    fail, as will readAllStandardError(). The file will be appended to
    if \a mode is Append, otherwise, it will be truncated.

    See setStandardOutputFile() for more information on how the file
    is opened.

    Note: if setProcessChannelMode() was called with an argument of
    QProcess::MergedChannels, this function has no effect.

    \sa setStandardInputFile(), setStandardOutputFile(),
        setStandardOutputProcess()
*/
void QProcess::setStandardErrorFile(const QString &fileName, OpenMode mode)
{
    Q_ASSERT(mode == Append || mode == Truncate);
    Q_D(QProcess);

    d->stderrChannel = fileName;
    d->stderrChannel.append = mode == Append;
}

/*!
    \since 4.2

    Pipes the standard output stream of this process to the \a
    destination process' standard input.

    The following shell command:
    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 2

    Can be accomplished with QProcesses with the following code:
    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 3
*/
void QProcess::setStandardOutputProcess(QProcess *destination)
{
    QProcessPrivate *dfrom = d_func();
    QProcessPrivate *dto = destination->d_func();
    dfrom->stdoutChannel.pipeTo(dto);
    dto->stdinChannel.pipeFrom(dfrom);
}

#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)

/*!
    \since 4.7

    Returns the additional native command line arguments for the program.

    \note This function is available only on the Windows and Symbian
    platforms.

    \sa setNativeArguments()
*/
QString QProcess::nativeArguments() const
{
    Q_D(const QProcess);
    return d->nativeArguments;
}

/*!
    \since 4.7
    \overload

    Sets additional native command line \a arguments for the program.

    On operating systems where the system API for passing command line
    \a arguments to a subprocess natively uses a single string, one can
    conceive command lines which cannot be passed via QProcess's portable
    list-based API. In such cases this function must be used to set a
    string which is \e appended to the string composed from the usual
    argument list, with a delimiting space.

    \note This function is available only on the Windows and Symbian
    platforms.

    \sa nativeArguments()
*/
void QProcess::setNativeArguments(const QString &arguments)
{
    Q_D(QProcess);
    d->nativeArguments = arguments;
}

#endif

/*!
    If QProcess has been assigned a working directory, this function returns
    the working directory that the QProcess will enter before the program has
    started. Otherwise, (i.e., no directory has been assigned,) an empty
    string is returned, and QProcess will use the application's current
    working directory instead.

    \sa setWorkingDirectory()
*/
QString QProcess::workingDirectory() const
{
    Q_D(const QProcess);
    return d->workingDirectory;
}

/*!
    Sets the working directory to \a dir. QProcess will start the
    process in this directory. The default behavior is to start the
    process in the working directory of the calling process.

    \note The working directory setting is ignored on Symbian;
    the private directory of the process is considered its working
    directory.

    \note On QNX, this may cause all application threads to
    temporarily freeze.

    \sa workingDirectory(), start()
*/
void QProcess::setWorkingDirectory(const QString &dir)
{
    Q_D(QProcess);
    d->workingDirectory = dir;
}

/*!
    Returns the native process identifier for the running process, if
    available.  If no process is currently running, 0 is returned.
*/
Q_PID QProcess::pid() const
{
    Q_D(const QProcess);
    return d->pid;
}

/*! \reimp

    This function operates on the current read channel.

    \sa readChannel(), setReadChannel()
*/
bool QProcess::canReadLine() const
{
    Q_D(const QProcess);
    const QRingBuffer *readBuffer = (d->processChannel == QProcess::StandardError)
                                    ? &d->errorReadBuffer
                                    : &d->outputReadBuffer;
    return readBuffer->canReadLine() || QIODevice::canReadLine();
}

/*!
    Closes all communication with the process and kills it. After calling this
    function, QProcess will no longer emit readyRead(), and data can no
    longer be read or written.
*/
void QProcess::close()
{
    emit aboutToClose();
    while (waitForBytesWritten(-1))
        ;
    kill();
    waitForFinished(-1);
    QIODevice::close();
}

/*! \reimp

   Returns true if the process is not running, and no more data is available
   for reading; otherwise returns false.
*/
bool QProcess::atEnd() const
{
    Q_D(const QProcess);
    const QRingBuffer *readBuffer = (d->processChannel == QProcess::StandardError)
                                    ? &d->errorReadBuffer
                                    : &d->outputReadBuffer;
    return QIODevice::atEnd() && (!isOpen() || readBuffer->isEmpty());
}

/*! \reimp
*/
bool QProcess::isSequential() const
{
    return true;
}

/*! \reimp
*/
qint64 QProcess::bytesAvailable() const
{
    Q_D(const QProcess);
    const QRingBuffer *readBuffer = (d->processChannel == QProcess::StandardError)
                                    ? &d->errorReadBuffer
                                    : &d->outputReadBuffer;
#if defined QPROCESS_DEBUG
    qDebug("QProcess::bytesAvailable() == %i (%s)", readBuffer->size(),
           (d->processChannel == QProcess::StandardError) ? "stderr" : "stdout");
#endif
    return readBuffer->size() + QIODevice::bytesAvailable();
}

/*! \reimp
*/
qint64 QProcess::bytesToWrite() const
{
    Q_D(const QProcess);
    qint64 size = d->writeBuffer.size();
#ifdef Q_OS_WIN
    size += d->pipeWriterBytesToWrite();
#endif
    return size;
}

/*!
    Returns the type of error that occurred last.

    \sa state()
*/
QProcess::ProcessError QProcess::error() const
{
    Q_D(const QProcess);
    return d->processError;
}

/*!
    Returns the current state of the process.

    \sa stateChanged(), error()
*/
QProcess::ProcessState QProcess::state() const
{
    Q_D(const QProcess);
    return d->processState;
}

/*!
    \deprecated
    Sets the environment that QProcess will use when starting a process to the
    \a environment specified which consists of a list of key=value pairs.

    For example, the following code adds the \c{C:\\BIN} directory to the list of
    executable paths (\c{PATHS}) on Windows:

    \snippet doc/src/snippets/qprocess-environment/main.cpp 0

    \note This function is less efficient than the setProcessEnvironment()
    function.

    \sa environment(), setProcessEnvironment(), systemEnvironment()
*/
void QProcess::setEnvironment(const QStringList &environment)
{
    setProcessEnvironment(QProcessEnvironmentPrivate::fromList(environment));
}

/*!
    \deprecated
    Returns the environment that QProcess will use when starting a
    process, or an empty QStringList if no environment has been set
    using setEnvironment() or setEnvironmentHash(). If no environment
    has been set, the environment of the calling process will be used.

    \note The environment settings are ignored on Windows CE and Symbian,
    as there is no concept of an environment.

    \sa processEnvironment(), setEnvironment(), systemEnvironment()
*/
QStringList QProcess::environment() const
{
    Q_D(const QProcess);
    return d->environment.toStringList();
}

/*!
    \since 4.6
    Sets the environment that QProcess will use when starting a process to the
    \a environment object.

    For example, the following code adds the \c{C:\\BIN} directory to the list of
    executable paths (\c{PATHS}) on Windows and sets \c{TMPDIR}:

    \snippet doc/src/snippets/qprocess-environment/main.cpp 1

    Note how, on Windows, environment variable names are case-insensitive.

    \sa processEnvironment(), QProcessEnvironment::systemEnvironment(), setEnvironment()
*/
void QProcess::setProcessEnvironment(const QProcessEnvironment &environment)
{
    Q_D(QProcess);
    d->environment = environment;
}

/*!
    \since 4.6
    Returns the environment that QProcess will use when starting a
    process, or an empty object if no environment has been set using
    setEnvironment() or setProcessEnvironment(). If no environment has
    been set, the environment of the calling process will be used.

    \note The environment settings are ignored on Windows CE,
    as there is no concept of an environment.

    \sa setProcessEnvironment(), setEnvironment(), QProcessEnvironment::isEmpty()
*/
QProcessEnvironment QProcess::processEnvironment() const
{
    Q_D(const QProcess);
    return d->environment;
}

/*!
    Blocks until the process has started and the started() signal has
    been emitted, or until \a msecs milliseconds have passed.

    Returns true if the process was started successfully; otherwise
    returns false (if the operation timed out or if an error
    occurred).

    This function can operate without an event loop. It is
    useful when writing non-GUI applications and when performing
    I/O operations in a non-GUI thread.

    \warning Calling this function from the main (GUI) thread
    might cause your user interface to freeze.

    If msecs is -1, this function will not time out.

    \sa started(), waitForReadyRead(), waitForBytesWritten(), waitForFinished()
*/
bool QProcess::waitForStarted(int msecs)
{
    Q_D(QProcess);
    if (d->processState == QProcess::Running)
        return true;

    return d->waitForStarted(msecs);
}

/*! \reimp
*/
bool QProcess::waitForReadyRead(int msecs)
{
    Q_D(QProcess);

    if (d->processState == QProcess::NotRunning)
        return false;
    if (d->processChannel == QProcess::StandardOutput && d->stdoutChannel.closed)
        return false;
    if (d->processChannel == QProcess::StandardError && d->stderrChannel.closed)
        return false;
    return d->waitForReadyRead(msecs);
}

/*! \reimp
*/
bool QProcess::waitForBytesWritten(int msecs)
{
    Q_D(QProcess);
    if (d->processState == QProcess::NotRunning)
        return false;
    if (d->processState == QProcess::Starting) {
        QElapsedTimer stopWatch;
        stopWatch.start();
        bool started = waitForStarted(msecs);
        if (!started)
            return false;
        if (msecs != -1)
            msecs -= stopWatch.elapsed();
    }

    return d->waitForBytesWritten(msecs);
}

/*!
    Blocks until the process has finished and the finished() signal
    has been emitted, or until \a msecs milliseconds have passed.

    Returns true if the process finished; otherwise returns false (if
    the operation timed out, if an error occurred, or if this QProcess
    is already finished).

    This function can operate without an event loop. It is
    useful when writing non-GUI applications and when performing
    I/O operations in a non-GUI thread.

    \warning Calling this function from the main (GUI) thread
    might cause your user interface to freeze.

    If msecs is -1, this function will not time out.

    \sa finished(), waitForStarted(), waitForReadyRead(), waitForBytesWritten()
*/
bool QProcess::waitForFinished(int msecs)
{
    Q_D(QProcess);
    if (d->processState == QProcess::NotRunning)
        return false;
    if (d->processState == QProcess::Starting) {
        QElapsedTimer stopWatch;
        stopWatch.start();
        bool started = waitForStarted(msecs);
        if (!started)
            return false;
        if (msecs != -1)
            msecs -= stopWatch.elapsed();
    }

    return d->waitForFinished(msecs);
}

/*!
    Sets the current state of the QProcess to the \a state specified.

    \sa state()
*/
void QProcess::setProcessState(ProcessState state)
{
    Q_D(QProcess);
    if (d->processState == state)
        return;
    d->processState = state;
    emit stateChanged(state);
}

/*!
  This function is called in the child process context just before the
    program is executed on Unix or Mac OS X (i.e., after \e fork(), but before
    \e execve()). Reimplement this function to do last minute initialization
    of the child process. Example:

    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 4

    You cannot exit the process (by calling exit(), for instance) from
    this function. If you need to stop the program before it starts
    execution, your workaround is to emit finished() and then call
    exit().

    \warning This function is called by QProcess on Unix and Mac OS X
    only. On Windows and QNX, it is not called.
*/
void QProcess::setupChildProcess()
{
}

/*! \reimp
*/
qint64 QProcess::readData(char *data, qint64 maxlen)
{
    Q_D(QProcess);
    QRingBuffer *readBuffer = (d->processChannel == QProcess::StandardError)
                              ? &d->errorReadBuffer
                              : &d->outputReadBuffer;

    if (maxlen == 1 && !readBuffer->isEmpty()) {
        int c = readBuffer->getChar();
        if (c == -1) {
#if defined QPROCESS_DEBUG
            qDebug("QProcess::readData(%p \"%s\", %d) == -1",
                   data, qt_prettyDebug(data, 1, maxlen).constData(), 1);
#endif
            return -1;
        }
        *data = (char) c;
#if defined QPROCESS_DEBUG
        qDebug("QProcess::readData(%p \"%s\", %d) == 1",
               data, qt_prettyDebug(data, 1, maxlen).constData(), 1);
#endif
        return 1;
    }

    qint64 bytesToRead = qint64(qMin(readBuffer->size(), (int)maxlen));
    qint64 readSoFar = 0;
    while (readSoFar < bytesToRead) {
        const char *ptr = readBuffer->readPointer();
        int bytesToReadFromThisBlock = qMin<qint64>(bytesToRead - readSoFar,
                                            readBuffer->nextDataBlockSize());
        memcpy(data + readSoFar, ptr, bytesToReadFromThisBlock);
        readSoFar += bytesToReadFromThisBlock;
        readBuffer->free(bytesToReadFromThisBlock);
    }

#if defined QPROCESS_DEBUG
    qDebug("QProcess::readData(%p \"%s\", %lld) == %lld",
           data, qt_prettyDebug(data, readSoFar, 16).constData(), maxlen, readSoFar);
#endif
    if (!readSoFar && d->processState == QProcess::NotRunning)
        return -1;              // EOF
    return readSoFar;
}

/*! \reimp
*/
qint64 QProcess::writeData(const char *data, qint64 len)
{
    Q_D(QProcess);

#if defined(Q_OS_WINCE)
    Q_UNUSED(data);
    Q_UNUSED(len);
    d->processError = QProcess::WriteError;
    setErrorString(tr("Error writing to process"));
    emit error(d->processError);
    return -1;
#endif

    if (d->stdinChannel.closed) {
#if defined QPROCESS_DEBUG
    qDebug("QProcess::writeData(%p \"%s\", %lld) == 0 (write channel closing)",
           data, qt_prettyDebug(data, len, 16).constData(), len);
#endif
        return 0;
    }

    if (len == 1) {
        d->writeBuffer.putChar(*data);
        if (d->stdinChannel.notifier)
            d->stdinChannel.notifier->setEnabled(true);
#if defined QPROCESS_DEBUG
    qDebug("QProcess::writeData(%p \"%s\", %lld) == 1 (written to buffer)",
           data, qt_prettyDebug(data, len, 16).constData(), len);
#endif
        return 1;
    }

    char *dest = d->writeBuffer.reserve(len);
    memcpy(dest, data, len);
    if (d->stdinChannel.notifier)
        d->stdinChannel.notifier->setEnabled(true);
#if defined QPROCESS_DEBUG
    qDebug("QProcess::writeData(%p \"%s\", %lld) == %lld (written to buffer)",
           data, qt_prettyDebug(data, len, 16).constData(), len, len);
#endif
    return len;
}

/*!
    Regardless of the current read channel, this function returns all
    data available from the standard output of the process as a
    QByteArray.

    \sa readyReadStandardOutput(), readAllStandardError(), readChannel(), setReadChannel()
*/
QByteArray QProcess::readAllStandardOutput()
{
    ProcessChannel tmp = readChannel();
    setReadChannel(StandardOutput);
    QByteArray data = readAll();
    setReadChannel(tmp);
    return data;
}

/*!
    Regardless of the current read channel, this function returns all
    data available from the standard error of the process as a
    QByteArray.

    \sa readyReadStandardError(), readAllStandardOutput(), readChannel(), setReadChannel()
*/
QByteArray QProcess::readAllStandardError()
{
    ProcessChannel tmp = readChannel();
    setReadChannel(StandardError);
    QByteArray data = readAll();
    setReadChannel(tmp);
    return data;
}

/*!
    Starts the given \a program in a new process, if none is already
    running, passing the command line arguments in \a arguments. The OpenMode
    is set to \a mode.

    The QProcess object will immediately enter the Starting state. If the
    process starts successfully, QProcess will emit started(); otherwise,
    error() will be emitted. If the QProcess object is already running a
    process, a warning may be printed at the console, and the existing
    process will continue running.

    \note Processes are started asynchronously, which means the started()
    and error() signals may be delayed. Call waitForStarted() to make
    sure the process has started (or has failed to start) and those signals
    have been emitted.

    \note No further splitting of the arguments is performed.

    \bold{Windows:} Arguments that contain spaces are wrapped in quotes.

    \sa pid(), started(), waitForStarted()
*/
void QProcess::start(const QString &program, const QStringList &arguments, OpenMode mode)
{
    Q_D(QProcess);
    if (d->processState != NotRunning) {
        qWarning("QProcess::start: Process is already running");
        return;
    }

#if defined QPROCESS_DEBUG
    qDebug() << "QProcess::start(" << program << ',' << arguments << ',' << mode << ')';
#endif

    d->outputReadBuffer.clear();
    d->errorReadBuffer.clear();

    if (d->stdinChannel.type != QProcessPrivate::Channel::Normal)
        mode &= ~WriteOnly;     // not open for writing
    if (d->stdoutChannel.type != QProcessPrivate::Channel::Normal &&
        (d->stderrChannel.type != QProcessPrivate::Channel::Normal ||
         d->processChannelMode == MergedChannels))
        mode &= ~ReadOnly;      // not open for reading
    if (mode == 0)
        mode = Unbuffered;
    QIODevice::open(mode);

    d->stdinChannel.closed = false;
    d->stdoutChannel.closed = false;
    d->stderrChannel.closed = false;

    d->program = program;
    d->arguments = arguments;

    d->exitCode = 0;
    d->exitStatus = NormalExit;
    d->processError = QProcess::UnknownError;
    d->errorString.clear();
    d->startProcess();
}


static QStringList parseCombinedArgString(const QString &program)
{
    QStringList args;
    QString tmp;
    int quoteCount = 0;
    bool inQuote = false;

    // handle quoting. tokens can be surrounded by double quotes
    // "hello world". three consecutive double quotes represent
    // the quote character itself.
    for (int i = 0; i < program.size(); ++i) {
        if (program.at(i) == QLatin1Char('"')) {
            ++quoteCount;
            if (quoteCount == 3) {
                // third consecutive quote
                quoteCount = 0;
                tmp += program.at(i);
            }
            continue;
        }
        if (quoteCount) {
            if (quoteCount == 1)
                inQuote = !inQuote;
            quoteCount = 0;
        }
        if (!inQuote && program.at(i).isSpace()) {
            if (!tmp.isEmpty()) {
                args += tmp;
                tmp.clear();
            }
        } else {
            tmp += program.at(i);
        }
    }
    if (!tmp.isEmpty())
        args += tmp;

    return args;
}

/*!
    \overload

    Starts the program \a program in a new process, if one is not already
    running. \a program is a single string of text containing both the
    program name and its arguments. The arguments are separated by one or
    more spaces. For example:

    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 5

    The \a program string can also contain quotes, to ensure that arguments
    containing spaces are correctly supplied to the new process. For example:

    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 6

    If the QProcess object is already running a process, a warning may be
    printed at the console, and the existing process will continue running.

    Note that, on Windows, quotes need to be both escaped and quoted.
    For example, the above code would be specified in the following
    way to ensure that \c{"My Documents"} is used as the argument to
    the \c dir executable:

    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 7

    The OpenMode is set to \a mode.
*/
void QProcess::start(const QString &program, OpenMode mode)
{
    QStringList args = parseCombinedArgString(program);
    if (args.isEmpty()) {
        Q_D(QProcess);
        d->processError = QProcess::FailedToStart;
        setErrorString(tr("No program defined"));
        emit error(d->processError);
        return;
    }

    QString prog = args.first();
    args.removeFirst();

    start(prog, args, mode);
}

/*!
    Attempts to terminate the process.

    The process may not exit as a result of calling this function (it is given
    the chance to prompt the user for any unsaved files, etc).

    On Windows, terminate() posts a WM_CLOSE message to all toplevel windows
    of the process and then to the main thread of the process itself. On Unix
    and Mac OS X the SIGTERM signal is sent.

    Console applications on Windows that do not run an event loop, or whose
    event loop does not handle the WM_CLOSE message, can only be terminated by
    calling kill().

    On Symbian, this function requires platform security capability
    \c PowerMgmt. If absent, the process will panic with KERN-EXEC 46.

    \note Terminating running processes from other processes will typically
    cause a panic in Symbian due to platform security.

    \sa {Symbian Platform Security Requirements}
    \sa kill()
*/
void QProcess::terminate()
{
    Q_D(QProcess);
    d->terminateProcess();
}

/*!
    Kills the current process, causing it to exit immediately.

    On Windows, kill() uses TerminateProcess, and on Unix and Mac OS X, the
    SIGKILL signal is sent to the process.

    On Symbian, this function requires platform security capability
    \c PowerMgmt. If absent, the process will panic with KERN-EXEC 46.

    \note Killing running processes from other processes will typically
    cause a panic in Symbian due to platform security.

    \sa {Symbian Platform Security Requirements}
    \sa terminate()
*/
void QProcess::kill()
{
    Q_D(QProcess);
    d->killProcess();
}

/*!
    Returns the exit code of the last process that finished.
*/
int QProcess::exitCode() const
{
    Q_D(const QProcess);
    return d->exitCode;
}

/*!
    \since 4.1

    Returns the exit status of the last process that finished.

    On Windows, if the process was terminated with TerminateProcess()
    from another application this function will still return NormalExit
    unless the exit code is less than 0.
*/
QProcess::ExitStatus QProcess::exitStatus() const
{
    Q_D(const QProcess);
    return d->exitStatus;
}

/*!
    Starts the program \a program with the arguments \a arguments in a
    new process, waits for it to finish, and then returns the exit
    code of the process. Any data the new process writes to the
    console is forwarded to the calling process.

    The environment and working directory are inherited from the calling
    process.

    On Windows, arguments that contain spaces are wrapped in quotes.

    If the process cannot be started, -2 is returned. If the process
    crashes, -1 is returned. Otherwise, the process' exit code is
    returned.
*/
int QProcess::execute(const QString &program, const QStringList &arguments)
{
    QProcess process;
    process.setReadChannelMode(ForwardedChannels);
    process.start(program, arguments);
    if (!process.waitForFinished(-1))
        return -2;
    return process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
}

/*!
    \overload

    Starts the program \a program in a new process. \a program is a
    single string of text containing both the program name and its
    arguments. The arguments are separated by one or more spaces.
*/
int QProcess::execute(const QString &program)
{
    QProcess process;
    process.setReadChannelMode(ForwardedChannels);
    process.start(program);
    if (!process.waitForFinished(-1))
        return -2;
    return process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
}

/*!
    Starts the program \a program with the arguments \a arguments in a
    new process, and detaches from it. Returns true on success;
    otherwise returns false. If the calling process exits, the
    detached process will continue to live.

    Note that arguments that contain spaces are not passed to the
    process as separate arguments.

    \bold{Unix:} The started process will run in its own session and act
    like a daemon.

    \bold{Windows:} Arguments that contain spaces are wrapped in quotes.
    The started process will run as a regular standalone process.

    The process will be started in the directory \a workingDirectory.

    \note On QNX, this may cause all application threads to
    temporarily freeze.

    If the function is successful then *\a pid is set to the process
    identifier of the started process.
*/
bool QProcess::startDetached(const QString &program,
			     const QStringList &arguments,
			     const QString &workingDirectory,
                             qint64 *pid)
{
    return QProcessPrivate::startDetached(program,
					  arguments,
					  workingDirectory,
					  pid);
}

/*!
    Starts the program \a program with the given \a arguments in a
    new process, and detaches from it. Returns true on success;
    otherwise returns false. If the calling process exits, the
    detached process will continue to live.

    \note Arguments that contain spaces are not passed to the
    process as separate arguments.

    \bold{Unix:} The started process will run in its own session and act
    like a daemon.

    \bold{Windows:} Arguments that contain spaces are wrapped in quotes.
    The started process will run as a regular standalone process.
*/
bool QProcess::startDetached(const QString &program,
			     const QStringList &arguments)
{
    return QProcessPrivate::startDetached(program, arguments);
}

/*!
    \overload

    Starts the program \a program in a new process. \a program is a
    single string of text containing both the program name and its
    arguments. The arguments are separated by one or more spaces.

    The \a program string can also contain quotes, to ensure that arguments
    containing spaces are correctly supplied to the new process.
*/
bool QProcess::startDetached(const QString &program)
{
    QStringList args = parseCombinedArgString(program);
    if (args.isEmpty())
        return false;

    QString prog = args.first();
    args.removeFirst();

    return QProcessPrivate::startDetached(prog, args);
}

QT_BEGIN_INCLUDE_NAMESPACE
#if defined(Q_OS_MAC) && !defined(Q_OS_IOS)
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
#elif defined(Q_OS_WINCE) || defined(Q_OS_SYMBIAN) || (defined(Q_OS_MAC) && defined(Q_OS_IOS))
  static char *qt_empty_environ[] = { 0 };
#define environ qt_empty_environ
#elif !defined(Q_OS_WIN)
  extern char **environ;
#endif
QT_END_INCLUDE_NAMESPACE

/*!
    \since 4.1

    Returns the environment of the calling process as a list of
    key=value pairs. Example:

    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 8

    This function does not cache the system environment. Therefore, it's
    possible to obtain an updated version of the environment if low-level C
    library functions like \tt setenv ot \tt putenv have been called.

    However, note that repeated calls to this function will recreate the
    list of environment variables, which is a non-trivial operation.

    \note For new code, it is recommended to use QProcessEnvironment::systemEnvironment()

    \sa QProcessEnvironment::systemEnvironment(), environment(), setEnvironment()
*/
QStringList QProcess::systemEnvironment()
{
    QStringList tmp;
    char *entry = 0;
    int count = 0;
    while ((entry = environ[count++]))
        tmp << QString::fromLocal8Bit(entry);
    return tmp;
}

/*!
    \fn QProcessEnvironment QProcessEnvironment::systemEnvironment()

    \since 4.6

    \brief The systemEnvironment function returns the environment of
    the calling process.

    It is returned as a QProcessEnvironment. This function does not
    cache the system environment. Therefore, it's possible to obtain
    an updated version of the environment if low-level C library
    functions like \tt setenv ot \tt putenv have been called.

    However, note that repeated calls to this function will recreate the
    QProcessEnvironment object, which is a non-trivial operation.

    \sa QProcess::systemEnvironment()
*/

/*!
    \typedef Q_PID
    \relates QProcess

    Typedef for the identifiers used to represent processes on the underlying
    platform. On Unix and Symbian, this corresponds to \l qint64; on Windows, it
    corresponds to \c{_PROCESS_INFORMATION*}.

    \sa QProcess::pid()
*/

QT_END_NAMESPACE

#include "moc_qprocess.cpp"

#endif // QT_NO_PROCESS

