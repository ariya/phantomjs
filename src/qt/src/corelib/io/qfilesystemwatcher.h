/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QFILESYSTEMWATCHER_H
#define QFILESYSTEMWATCHER_H

#include <QtCore/qobject.h>

#ifndef QT_NO_FILESYSTEMWATCHER

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class QFileSystemWatcherPrivate;

class Q_CORE_EXPORT QFileSystemWatcher : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QFileSystemWatcher)

public:
    QFileSystemWatcher(QObject *parent = 0);
    QFileSystemWatcher(const QStringList &paths, QObject *parent = 0);
    ~QFileSystemWatcher();

    void addPath(const QString &file);
    void addPaths(const QStringList &files);
    void removePath(const QString &file);
    void removePaths(const QStringList &files);

    QStringList files() const;
    QStringList directories() const;

Q_SIGNALS:
    void fileChanged(const QString &path);
    void directoryChanged(const QString &path);

private:
    Q_PRIVATE_SLOT(d_func(), void _q_fileChanged(const QString &path, bool removed))
    Q_PRIVATE_SLOT(d_func(), void _q_directoryChanged(const QString &path, bool removed))
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_FILESYSTEMWATCHER
#endif // QFILESYSTEMWATCHER_H
