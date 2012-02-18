/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QDIRMODEL_H
#define QDIRMODEL_H

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qdir.h>
#include <QtGui/qfileiconprovider.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_DIRMODEL

class QDirModelPrivate;

class Q_GUI_EXPORT QDirModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(bool resolveSymlinks READ resolveSymlinks WRITE setResolveSymlinks)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY(bool lazyChildCount READ lazyChildCount WRITE setLazyChildCount)

public:
    enum Roles {
        FileIconRole = Qt::DecorationRole,
        FilePathRole = Qt::UserRole + 1,
        FileNameRole
    };

    QDirModel(const QStringList &nameFilters, QDir::Filters filters,
              QDir::SortFlags sort, QObject *parent = 0);
    explicit QDirModel(QObject *parent = 0);
    ~QDirModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    bool hasChildren(const QModelIndex &index = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent);
    Qt::DropActions supportedDropActions() const;

    // QDirModel specific API

    void setIconProvider(QFileIconProvider *provider);
    QFileIconProvider *iconProvider() const;

    void setNameFilters(const QStringList &filters);
    QStringList nameFilters() const;

    void setFilter(QDir::Filters filters);
    QDir::Filters filter() const;

    void setSorting(QDir::SortFlags sort);
    QDir::SortFlags sorting() const;

    void setResolveSymlinks(bool enable);
    bool resolveSymlinks() const;

    void setReadOnly(bool enable);
    bool isReadOnly() const;

    void setLazyChildCount(bool enable);
    bool lazyChildCount() const;

    QModelIndex index(const QString &path, int column = 0) const;

    bool isDir(const QModelIndex &index) const;
    QModelIndex mkdir(const QModelIndex &parent, const QString &name);
    bool rmdir(const QModelIndex &index);
    bool remove(const QModelIndex &index);

    QString filePath(const QModelIndex &index) const;
    QString fileName(const QModelIndex &index) const;
    QIcon fileIcon(const QModelIndex &index) const;
    QFileInfo fileInfo(const QModelIndex &index) const;

#ifdef Q_NO_USING_KEYWORD
    inline QObject *parent() const { return QObject::parent(); }
#else
    using QObject::parent;
#endif

public Q_SLOTS:
    void refresh(const QModelIndex &parent = QModelIndex());

protected:
    QDirModel(QDirModelPrivate &, QObject *parent = 0);
    friend class QFileDialogPrivate;

private:
    Q_DECLARE_PRIVATE(QDirModel)
    Q_DISABLE_COPY(QDirModel)
    Q_PRIVATE_SLOT(d_func(), void _q_refresh())
};

#endif // QT_NO_DIRMODEL

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDIRMODEL_H
