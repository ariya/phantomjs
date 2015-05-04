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

#ifndef QDIR_P_H
#define QDIR_P_H

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

#include "qfilesystementry_p.h"
#include "qfilesystemmetadata_p.h"

QT_BEGIN_NAMESPACE

class QDirPrivate : public QSharedData
{
public:
    explicit QDirPrivate(const QString &path, const QStringList &nameFilters_ = QStringList(),
                         QDir::SortFlags sort_ = QDir::SortFlags(QDir::Name | QDir::IgnoreCase),
                         QDir::Filters filters_ = QDir::AllEntries);

    explicit QDirPrivate(const QDirPrivate &copy);

    bool exists() const;

    void initFileEngine();
    void initFileLists(const QDir &dir) const;

    static void sortFileList(QDir::SortFlags, QFileInfoList &, QStringList *, QFileInfoList *);

    static inline QChar getFilterSepChar(const QString &nameFilter);

    static inline QStringList splitFilters(const QString &nameFilter, QChar sep = 0);

    void setPath(const QString &path);

    void clearFileLists();

    void resolveAbsoluteEntry() const;

    mutable bool fileListsInitialized;
    mutable QStringList files;
    mutable QFileInfoList fileInfos;

    QStringList nameFilters;
    QDir::SortFlags sort;
    QDir::Filters filters;

    QScopedPointer<QAbstractFileEngine> fileEngine;

    QFileSystemEntry dirEntry;
    mutable QFileSystemEntry absoluteDirEntry;
    mutable QFileSystemMetaData metaData;
};

QT_END_NAMESPACE

#endif
