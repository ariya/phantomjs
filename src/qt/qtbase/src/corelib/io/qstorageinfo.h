/****************************************************************************
**
** Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
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

#ifndef QSTORAGEINFO_H
#define QSTORAGEINFO_H

#include <QtCore/qbytearray.h>
#include <QtCore/qdir.h>
#include <QtCore/qlist.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qstring.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QStorageInfoPrivate;
class Q_CORE_EXPORT QStorageInfo
{
public:
    QStorageInfo();
    explicit QStorageInfo(const QString &path);
    explicit QStorageInfo(const QDir &dir);
    QStorageInfo(const QStorageInfo &other);
    ~QStorageInfo();

    QStorageInfo &operator=(const QStorageInfo &other);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QStorageInfo &operator=(QStorageInfo &&other)
    { qSwap(d, other.d); return *this; }
#endif

    inline void swap(QStorageInfo &other)
    { qSwap(d, other.d); }

    void setPath(const QString &path);

    QString rootPath() const;
    QByteArray device() const;
    QByteArray fileSystemType() const;
    QString name() const;
    QString displayName() const;

    qint64 bytesTotal() const;
    qint64 bytesFree() const;
    qint64 bytesAvailable() const;

    inline bool isRoot() const;
    bool isReadOnly() const;
    bool isReady() const;
    bool isValid() const;

    void refresh();

    static QList<QStorageInfo> mountedVolumes();
    static QStorageInfo root();

private:
    friend class QStorageInfoPrivate;
    friend bool operator==(const QStorageInfo &first, const QStorageInfo &second);
    QExplicitlySharedDataPointer<QStorageInfoPrivate> d;
};

inline bool operator==(const QStorageInfo &first, const QStorageInfo &second)
{
    if (first.d == second.d)
        return true;
    return first.device() == second.device();
}

inline bool operator!=(const QStorageInfo &first, const QStorageInfo &second)
{
    return !(first == second);
}

inline bool QStorageInfo::isRoot() const
{ return *this == QStorageInfo::root(); }

Q_DECLARE_SHARED(QStorageInfo)

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QStorageInfo)

#endif // QSTORAGEINFO_H
