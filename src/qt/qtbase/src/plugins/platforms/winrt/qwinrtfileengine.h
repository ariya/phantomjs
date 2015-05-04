/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWINRTFILEENGINE_H
#define QWINRTFILEENGINE_H

#include <private/qabstractfileengine_p.h>

QT_BEGIN_NAMESPACE

namespace ABI {
    namespace Windows {
        namespace Storage {
            struct IStorageItem;
        }
    }
}

class QWinRTFileEngineHandlerPrivate;
class QWinRTFileEngineHandler : public QAbstractFileEngineHandler
{
public:
    QWinRTFileEngineHandler();
    ~QWinRTFileEngineHandler();
    QAbstractFileEngine *create(const QString &fileName) const Q_DECL_OVERRIDE;

    static void registerFile(const QString &fileName, ABI::Windows::Storage::IStorageItem *file);
    static ABI::Windows::Storage::IStorageItem *registeredFile(const QString &fileName);

private:
    QScopedPointer<QWinRTFileEngineHandlerPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTFileEngineHandler)
};

class QWinRTFileEnginePrivate;
class QWinRTFileEngine : public QAbstractFileEngine
{
public:
    QWinRTFileEngine(const QString &fileName, ABI::Windows::Storage::IStorageItem *file);
    ~QWinRTFileEngine();

    bool open(QIODevice::OpenMode openMode) Q_DECL_OVERRIDE;
    bool close() Q_DECL_OVERRIDE;
    qint64 size() const Q_DECL_OVERRIDE;
    qint64 pos() const Q_DECL_OVERRIDE;
    bool seek(qint64 pos) Q_DECL_OVERRIDE;
    bool remove() Q_DECL_OVERRIDE;
    bool copy(const QString &newName) Q_DECL_OVERRIDE;
    bool rename(const QString &newName) Q_DECL_OVERRIDE;
    bool renameOverwrite(const QString &newName) Q_DECL_OVERRIDE;
    FileFlags fileFlags(FileFlags type=FileInfoAll) const Q_DECL_OVERRIDE;
    bool setPermissions(uint perms) Q_DECL_OVERRIDE;
    QString fileName(FileName type=DefaultName) const Q_DECL_OVERRIDE;
    QDateTime fileTime(FileTime type) const Q_DECL_OVERRIDE;

    qint64 read(char *data, qint64 maxlen) Q_DECL_OVERRIDE;
    qint64 write(const char *data, qint64 len) Q_DECL_OVERRIDE;

private:
    QScopedPointer<QWinRTFileEnginePrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTFileEngine)
};

QT_END_NAMESPACE

#endif // QWINRTFILEENGINE_H
