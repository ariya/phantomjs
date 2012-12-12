/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#ifndef QFTP_H
#define QFTP_H

#include <QtCore/qstring.h>
#include <QtNetwork/qurlinfo.h>
#include <QtCore/qobject.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Network)

#ifndef QT_NO_FTP

class QFtpPrivate;

class Q_NETWORK_EXPORT QFtp : public QObject
{
    Q_OBJECT

public:
    explicit QFtp(QObject *parent = 0);
    virtual ~QFtp();

    enum State {
        Unconnected,
        HostLookup,
        Connecting,
        Connected,
        LoggedIn,
        Closing
    };
    enum Error {
        NoError,
        UnknownError,
        HostNotFound,
        ConnectionRefused,
        NotConnected
    };
    enum Command {
        None,
        SetTransferMode,
        SetProxy,
        ConnectToHost,
        Login,
        Close,
        List,
        Cd,
        Get,
        Put,
        Remove,
        Mkdir,
        Rmdir,
        Rename,
        RawCommand
    };
    enum TransferMode {
        Active,
        Passive
    };
    enum TransferType {
        Binary,
        Ascii
    };

    int setProxy(const QString &host, quint16 port);
    int connectToHost(const QString &host, quint16 port=21);
    int login(const QString &user = QString(), const QString &password = QString());
    int close();
    int setTransferMode(TransferMode mode);
    int list(const QString &dir = QString());
    int cd(const QString &dir);
    int get(const QString &file, QIODevice *dev=0, TransferType type = Binary);
    int put(const QByteArray &data, const QString &file, TransferType type = Binary);
    int put(QIODevice *dev, const QString &file, TransferType type = Binary);
    int remove(const QString &file);
    int mkdir(const QString &dir);
    int rmdir(const QString &dir);
    int rename(const QString &oldname, const QString &newname);

    int rawCommand(const QString &command);

    qint64 bytesAvailable() const;
    qint64 read(char *data, qint64 maxlen);
#ifdef QT3_SUPPORT
    inline QT3_SUPPORT qint64 readBlock(char *data, quint64 maxlen)
    { return read(data, qint64(maxlen)); }
#endif
    QByteArray readAll();

    int currentId() const;
    QIODevice* currentDevice() const;
    Command currentCommand() const;
    bool hasPendingCommands() const;
    void clearPendingCommands();

    State state() const;

    Error error() const;
    QString errorString() const;

public Q_SLOTS:
    void abort();

Q_SIGNALS:
    void stateChanged(int);
    void listInfo(const QUrlInfo&);
    void readyRead();
    void dataTransferProgress(qint64, qint64);
    void rawCommandReply(int, const QString&);

    void commandStarted(int);
    void commandFinished(int, bool);
    void done(bool);

#ifdef QT3_SUPPORT
public:
    QT3_SUPPORT_CONSTRUCTOR QFtp(QObject *parent, const char *name);
#endif

private:
    Q_DISABLE_COPY(QFtp)
    Q_DECLARE_PRIVATE(QFtp)

    Q_PRIVATE_SLOT(d_func(), void _q_startNextCommand())
    Q_PRIVATE_SLOT(d_func(), void _q_piFinished(const QString&))
    Q_PRIVATE_SLOT(d_func(), void _q_piError(int, const QString&))
    Q_PRIVATE_SLOT(d_func(), void _q_piConnectState(int))
    Q_PRIVATE_SLOT(d_func(), void _q_piFtpReply(int, const QString&))
};

#endif // QT_NO_FTP

QT_END_NAMESPACE

QT_END_HEADER

#endif // QFTP_H
