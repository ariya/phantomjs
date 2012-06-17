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

#include "qresource.h"
#include "qresource_iterator_p.h"

#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

QResourceFileEngineIterator::QResourceFileEngineIterator(QDir::Filters filters,
                                                         const QStringList &filterNames)
    : QAbstractFileEngineIterator(filters, filterNames), index(-1)
{
}

QResourceFileEngineIterator::~QResourceFileEngineIterator()
{
}

QString QResourceFileEngineIterator::next()
{
    if (!hasNext())
        return QString();
    ++index;
    return currentFilePath();
}

bool QResourceFileEngineIterator::hasNext() const
{
    if (index == -1) {
        // Lazy initialization of the iterator
        QResource resource(path());
        if (!resource.isValid())
            return false;

        // Initialize and move to the next entry.
        entries = resource.children();
        index = 0;
    }

    return index < entries.size();
}

QString QResourceFileEngineIterator::currentFileName() const
{
    if (index <= 0 || index > entries.size())
        return QString();
    return entries.at(index - 1);
}

QT_END_NAMESPACE
