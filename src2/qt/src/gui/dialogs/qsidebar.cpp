/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qsidebar_p.h"
#include "qfilesystemmodel.h"

#ifndef QT_NO_FILEDIALOG

#include <qaction.h>
#include <qurl.h>
#include <qmenu.h>
#include <qmimedata.h>
#include <qevent.h>
#include <qdebug.h>
#include <qfileiconprovider.h>
#include <qfiledialog.h>

QT_BEGIN_NAMESPACE

void QSideBarDelegate::initStyleOption(QStyleOptionViewItem *option,
                                         const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option,index);
    QVariant value = index.data(QUrlModel::EnabledRole);
    if (value.isValid()) {
        //If the bookmark/entry is not enabled then we paint it in gray
        if (!qvariant_cast<bool>(value))
            option->state &= ~QStyle::State_Enabled;
    }
}

/*!
    QUrlModel lets you have indexes from a QFileSystemModel to a list.  When QFileSystemModel
    changes them QUrlModel will automatically update.

    Example usage: File dialog sidebar and combo box
 */
QUrlModel::QUrlModel(QObject *parent) : QStandardItemModel(parent), showFullPath(false), fileSystemModel(0)
{
}

/*!
    \reimp
*/
QStringList QUrlModel::mimeTypes() const
{
    return QStringList(QLatin1String("text/uri-list"));
}

/*!
    \reimp
*/
Qt::ItemFlags QUrlModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QStandardItemModel::flags(index);
    if (index.isValid()) {
        flags &= ~Qt::ItemIsEditable;
        // ### some future version could support "moving" urls onto a folder
        flags &= ~Qt::ItemIsDropEnabled;
    }

    if (index.data(Qt::DecorationRole).isNull())
        flags &= ~Qt::ItemIsEnabled;

    return flags;
}

/*!
    \reimp
*/
QMimeData *QUrlModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QUrl> list;
    for (int i = 0; i < indexes.count(); ++i) {
        if (indexes.at(i).column() == 0)
           list.append(indexes.at(i).data(UrlRole).toUrl());
    }
    QMimeData *data = new QMimeData();
    data->setUrls(list);
    return data;
}

#ifndef QT_NO_DRAGANDDROP

/*!
    Decide based upon the data if it should be accepted or not

    We only accept dirs and not files
*/
bool QUrlModel::canDrop(QDragEnterEvent *event)
{
    if (!event->mimeData()->formats().contains(mimeTypes().first()))
        return false;

    const QList<QUrl> list = event->mimeData()->urls();
    for (int i = 0; i < list.count(); ++i) {
        QModelIndex idx = fileSystemModel->index(list.at(0).toLocalFile());
        if (!fileSystemModel->isDir(idx))
            return false;
    }
    return true;
}

/*!
    \reimp
*/
bool QUrlModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                 int row, int column, const QModelIndex &parent)
{
    if (!data->formats().contains(mimeTypes().first()))
        return false;
    Q_UNUSED(action);
    Q_UNUSED(column);
    Q_UNUSED(parent);
    addUrls(data->urls(), row);
    return true;
}

#endif // QT_NO_DRAGANDDROP

/*!
    \reimp

    If the role is the UrlRole then handle otherwise just pass to QStandardItemModel
*/
bool QUrlModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (value.type() == QVariant::Url) {
        QUrl url = value.toUrl();
        QModelIndex dirIndex = fileSystemModel->index(url.toLocalFile());
        //On windows the popup display the "C:\", convert to nativeSeparators
        if (showFullPath)
            QStandardItemModel::setData(index, QDir::toNativeSeparators(fileSystemModel->data(dirIndex, QFileSystemModel::FilePathRole).toString()));
        else {
            QStandardItemModel::setData(index, QDir::toNativeSeparators(fileSystemModel->data(dirIndex, QFileSystemModel::FilePathRole).toString()), Qt::ToolTipRole);
            QStandardItemModel::setData(index, fileSystemModel->data(dirIndex).toString());
        }
        QStandardItemModel::setData(index, fileSystemModel->data(dirIndex, Qt::DecorationRole),
                                               Qt::DecorationRole);
        QStandardItemModel::setData(index, url, UrlRole);
        return true;
    }
    return QStandardItemModel::setData(index, value, role);
}

void QUrlModel::setUrl(const QModelIndex &index, const QUrl &url, const QModelIndex &dirIndex)
{
    setData(index, url, UrlRole);
    if (url.path().isEmpty()) {
        setData(index, fileSystemModel->myComputer());
        setData(index, fileSystemModel->myComputer(Qt::DecorationRole), Qt::DecorationRole);
    } else {
        QString newName;
        if (showFullPath) {
            //On windows the popup display the "C:\", convert to nativeSeparators
            newName = QDir::toNativeSeparators(dirIndex.data(QFileSystemModel::FilePathRole).toString());
        } else {
            newName = dirIndex.data().toString();
        }

        QIcon newIcon = qvariant_cast<QIcon>(dirIndex.data(Qt::DecorationRole));
        if (!dirIndex.isValid()) {
            newIcon = fileSystemModel->iconProvider()->icon(QFileIconProvider::Folder);
            newName = QFileInfo(url.toLocalFile()).fileName();
            if (!invalidUrls.contains(url))
                invalidUrls.append(url);
            //The bookmark is invalid then we set to false the EnabledRole
            setData(index, false, EnabledRole);
        } else {
            //The bookmark is valid then we set to true the EnabledRole
            setData(index, true, EnabledRole);
        }

        // Make sure that we have at least 32x32 images
        const QSize size = newIcon.actualSize(QSize(32,32));
        if (size.width() < 32) {
            QPixmap smallPixmap = newIcon.pixmap(QSize(32, 32));
            newIcon.addPixmap(smallPixmap.scaledToWidth(32, Qt::SmoothTransformation));
        }

        if (index.data().toString() != newName)
            setData(index, newName);
        QIcon oldIcon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        if (oldIcon.cacheKey() != newIcon.cacheKey())
            setData(index, newIcon, Qt::DecorationRole);
    }
}

void QUrlModel::setUrls(const QList<QUrl> &list)
{
    removeRows(0, rowCount());
    invalidUrls.clear();
    watching.clear();
    addUrls(list, 0);
}

/*!
    Add urls \a list into the list at \a row.  If move then movie
    existing ones to row.

    \sa dropMimeData()
*/
void QUrlModel::addUrls(const QList<QUrl> &list, int row, bool move)
{
    if (row == -1)
        row = rowCount();
    row = qMin(row, rowCount());
    for (int i = list.count() - 1; i >= 0; --i) {
        QUrl url = list.at(i);
        if (!url.isValid() || url.scheme() != QLatin1String("file"))
            continue;
        //this makes sure the url is clean
        const QString cleanUrl = QDir::cleanPath(url.toLocalFile());
        url = QUrl::fromLocalFile(cleanUrl);

        for (int j = 0; move && j < rowCount(); ++j) {
            QString local = index(j, 0).data(UrlRole).toUrl().toLocalFile();
#if defined(Q_OS_WIN)
            if (index(j, 0).data(UrlRole).toUrl().toLocalFile().toLower() == cleanUrl.toLower()) {
#else
            if (index(j, 0).data(UrlRole).toUrl().toLocalFile() == cleanUrl) {
#endif
                removeRow(j);
                if (j <= row)
                    row--;
                break;
            }
        }
        row = qMax(row, 0);
        QModelIndex idx = fileSystemModel->index(cleanUrl);
        if (!fileSystemModel->isDir(idx))
            continue;
        insertRows(row, 1);
        setUrl(index(row, 0), url, idx);
        watching.append(qMakePair(idx, cleanUrl));
    }
}

/*!
    Return the complete list of urls in a QList.
*/
QList<QUrl> QUrlModel::urls() const
{
    QList<QUrl> list;
    for (int i = 0; i < rowCount(); ++i)
        list.append(data(index(i, 0), UrlRole).toUrl());
    return list;
}

/*!
    QFileSystemModel to get index's from, clears existing rows
*/
void QUrlModel::setFileSystemModel(QFileSystemModel *model)
{
    if (model == fileSystemModel)
        return;
    if (fileSystemModel != 0) {
        disconnect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(dataChanged(QModelIndex,QModelIndex)));
        disconnect(model, SIGNAL(layoutChanged()),
            this, SLOT(layoutChanged()));
        disconnect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(layoutChanged()));
    }
    fileSystemModel = model;
    if (fileSystemModel != 0) {
        connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(dataChanged(QModelIndex,QModelIndex)));
        connect(model, SIGNAL(layoutChanged()),
            this, SLOT(layoutChanged()));
        connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(layoutChanged()));
    }
    clear();
    insertColumns(0, 1);
}

/*
    If one of the index's we are watching has changed update our internal data
*/
void QUrlModel::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    QModelIndex parent = topLeft.parent();
    for (int i = 0; i < watching.count(); ++i) {
        QModelIndex index = watching.at(i).first;
        if (index.model() && topLeft.model()) {
            Q_ASSERT(index.model() == topLeft.model());
        }
        if (   index.row() >= topLeft.row()
            && index.row() <= bottomRight.row()
            && index.column() >= topLeft.column()
            && index.column() <= bottomRight.column()
            && index.parent() == parent) {
                changed(watching.at(i).second);
        }
    }
}

/*!
    Re-get all of our data, anything could have changed!
 */
void QUrlModel::layoutChanged()
{
    QStringList paths;
    for (int i = 0; i < watching.count(); ++i)
        paths.append(watching.at(i).second);
    watching.clear();
    for (int i = 0; i < paths.count(); ++i) {
        QString path = paths.at(i);
        QModelIndex newIndex = fileSystemModel->index(path);
        watching.append(QPair<QModelIndex, QString>(newIndex, path));
        if (newIndex.isValid())
            changed(path);
     }
}

/*!
    The following path changed data update our copy of that data

    \sa layoutChanged() dataChanged()
*/
void QUrlModel::changed(const QString &path)
{
    for (int i = 0; i < rowCount(); ++i) {
        QModelIndex idx = index(i, 0);
        if (idx.data(UrlRole).toUrl().toLocalFile() == path) {
            setData(idx, idx.data(UrlRole).toUrl());
        }
    }
}

QSidebar::QSidebar(QWidget *parent) : QListView(parent)
{
}

void QSidebar::init(QFileSystemModel *model, const QList<QUrl> &newUrls)
{
    // ### TODO make icon size dynamic
    setIconSize(QSize(24,24));
    setUniformItemSizes(true);
    urlModel = new QUrlModel(this);
    urlModel->setFileSystemModel(model);
    setModel(urlModel);
    setItemDelegate(new QSideBarDelegate(this));

    connect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(clicked(QModelIndex)));
#ifndef QT_NO_DRAGANDDROP
    setDragDropMode(QAbstractItemView::DragDrop);
#endif
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(showContextMenu(QPoint)));
    urlModel->setUrls(newUrls);
    setCurrentIndex(this->model()->index(0,0));
}

QSidebar::~QSidebar()
{
}

#ifndef QT_NO_DRAGANDDROP
void QSidebar::dragEnterEvent(QDragEnterEvent *event)
{
    if (urlModel->canDrop(event))
        QListView::dragEnterEvent(event);
}
#endif // QT_NO_DRAGANDDROP

QSize QSidebar::sizeHint() const
{
    if (model())
        return QListView::sizeHintForIndex(model()->index(0, 0)) + QSize(2 * frameWidth(), 2 * frameWidth());
    return QListView::sizeHint();
}

void QSidebar::selectUrl(const QUrl &url)
{
    disconnect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
               this, SLOT(clicked(QModelIndex)));

    selectionModel()->clear();
    for (int i = 0; i < model()->rowCount(); ++i) {
        if (model()->index(i, 0).data(QUrlModel::UrlRole).toUrl() == url) {
            selectionModel()->select(model()->index(i, 0), QItemSelectionModel::Select);
            break;
        }
    }

    connect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(clicked(QModelIndex)));
}

#ifndef QT_NO_MENU
/*!
    \internal

    \sa removeEntry()
*/
void QSidebar::showContextMenu(const QPoint &position)
{
    QList<QAction *> actions;
    if (indexAt(position).isValid()) {
        QAction *action = new QAction(QFileDialog::tr("Remove"), this);
        if (indexAt(position).data(QUrlModel::UrlRole).toUrl().path().isEmpty())
            action->setEnabled(false);
        connect(action, SIGNAL(triggered()), this, SLOT(removeEntry()));
        actions.append(action);
    }
    if (actions.count() > 0)
        QMenu::exec(actions, mapToGlobal(position));
}
#endif // QT_NO_MENU

/*!
    \internal

    \sa showContextMenu()
*/
void QSidebar::removeEntry()
{
    QList<QModelIndex> idxs = selectionModel()->selectedIndexes();
    QList<QPersistentModelIndex> indexes;
    for (int i = 0; i < idxs.count(); i++)
        indexes.append(idxs.at(i));

    for (int i = 0; i < indexes.count(); ++i)
        if (!indexes.at(i).data(QUrlModel::UrlRole).toUrl().path().isEmpty())
            model()->removeRow(indexes.at(i).row());
}

/*!
    \internal

    \sa goToUrl()
*/
void QSidebar::clicked(const QModelIndex &index)
{
    QUrl url = model()->index(index.row(), 0).data(QUrlModel::UrlRole).toUrl();
    emit goToUrl(url);
    selectUrl(url);
}

/*!
    \reimp
    Don't automatically select something
 */
void QSidebar::focusInEvent(QFocusEvent *event)
{
    QAbstractScrollArea::focusInEvent(event);
    viewport()->update();
}

/*!
    \reimp
 */
bool QSidebar::event(QEvent * event)
{
    if (event->type() == QEvent::KeyRelease) {
        QKeyEvent* ke = (QKeyEvent*) event;
        if (ke->key() == Qt::Key_Delete) {
            removeEntry();
            return true;
        }
    }
    return QListView::event(event);
}

QT_END_NAMESPACE

#endif
