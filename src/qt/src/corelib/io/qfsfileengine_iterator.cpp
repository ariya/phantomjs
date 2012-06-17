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

#include "qfsfileengine_iterator_p.h"
#include "qfileinfo_p.h"
#include "qvariant.h"

#ifndef QT_NO_FSFILEENGINE

QT_BEGIN_NAMESPACE

QFSFileEngineIterator::QFSFileEngineIterator(QDir::Filters filters, const QStringList &filterNames)
    : QAbstractFileEngineIterator(filters, filterNames)
    , done(false)
{
}

QFSFileEngineIterator::~QFSFileEngineIterator()
{
}

bool QFSFileEngineIterator::hasNext() const
{
    if (!done && !nativeIterator) {
        nativeIterator.reset(new QFileSystemIterator(QFileSystemEntry(path()),
                    filters(), nameFilters()));
        advance();
    }

    return !done;
}

QString QFSFileEngineIterator::next()
{
    if (!hasNext())
        return QString();

    advance();
    return currentFilePath();
}

void QFSFileEngineIterator::advance() const
{
    currentInfo = nextInfo;

    QFileSystemEntry entry;
    QFileSystemMetaData data;
    if (nativeIterator->advance(entry, data)) {
        nextInfo = QFileInfo(new QFileInfoPrivate(entry, data));
    } else {
        done = true;
        nativeIterator.reset();
    }
}

QString QFSFileEngineIterator::currentFileName() const
{
    return currentInfo.fileName();
}

QFileInfo QFSFileEngineIterator::currentFileInfo() const
{
    return currentInfo;
}

QT_END_NAMESPACE

#endif // QT_NO_FSFILEENGINE
