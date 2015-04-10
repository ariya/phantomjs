/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qdirmodel.h"

#ifndef QT_NO_DIRMODEL
#include <qstack.h>
#include <qfile.h>
#include <qfilesystemmodel.h>
#include <qurl.h>
#include <qmimedata.h>
#include <qpair.h>
#include <qvector.h>
#include <qobject.h>
#include <qdatetime.h>
#include <qlocale.h>
#include <qstyle.h>
#include <qapplication.h>
#include <private/qabstractitemmodel_p.h>
#include <qdebug.h>

/*!
    \enum QDirModel::Roles
    \value FileIconRole
    \value FilePathRole
    \value FileNameRole
*/

QT_BEGIN_NAMESPACE

class QDirModelPrivate : public QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QDirModel)

public:
    struct QDirNode
    {
        QDirNode() : parent(0), populated(false), stat(false) {}
        ~QDirNode() { children.clear(); }
        QDirNode *parent;
        QFileInfo info;
        QIcon icon; // cache the icon
        mutable QVector<QDirNode> children;
        mutable bool populated; // have we read the children
        mutable bool stat;
    };

    QDirModelPrivate()
        : resolveSymlinks(true),
          readOnly(true),
          lazyChildCount(false),
          allowAppendChild(true),
          iconProvider(&defaultProvider),
          shouldStat(true) // ### This is set to false by QFileDialog
    { }

    void init();
    QDirNode *node(int row, QDirNode *parent) const;
    QVector<QDirNode> children(QDirNode *parent, bool stat) const;

    void _q_refresh();

    void savePersistentIndexes();
    void restorePersistentIndexes();

    QFileInfoList entryInfoList(const QString &path) const;
    QStringList entryList(const QString &path) const;

    QString name(const QModelIndex &index) const;
    QString size(const QModelIndex &index) const;
    QString type(const QModelIndex &index) const;
    QString time(const QModelIndex &index) const;

    void appendChild(QDirModelPrivate::QDirNode *parent, const QString &path) const;
    static QFileInfo resolvedInfo(QFileInfo info);

    inline QDirNode *node(const QModelIndex &index) const;
    inline void populate(QDirNode *parent) const;
    inline void clear(QDirNode *parent) const;

    void invalidate();

    mutable QDirNode root;
    bool resolveSymlinks;
    bool readOnly;
    bool lazyChildCount;
    bool allowAppendChild;

    QDir::Filters filters;
    QDir::SortFlags sort;
    QStringList nameFilters;

    QFileIconProvider *iconProvider;
    QFileIconProvider defaultProvider;

    struct SavedPersistent {
        QString path;
        int column;
        QPersistentModelIndexData *data;
        QPersistentModelIndex index;
    };
    QList<SavedPersistent> savedPersistent;
    QPersistentModelIndex toBeRefreshed;

    bool shouldStat; // use the "carefull not to stat directories" mode
};

void qt_setDirModelShouldNotStat(QDirModelPrivate *modelPrivate)
{
    modelPrivate->shouldStat = false;
}

QDirModelPrivate::QDirNode *QDirModelPrivate::node(const QModelIndex &index) const
{
    QDirModelPrivate::QDirNode *n =
        static_cast<QDirModelPrivate::QDirNode*>(index.internalPointer());
    Q_ASSERT(n);
    return n;
}

void QDirModelPrivate::populate(QDirNode *parent) const
{
    Q_ASSERT(parent);
    parent->children = children(parent, parent->stat);
    parent->populated = true;
}

void QDirModelPrivate::clear(QDirNode *parent) const
{
     Q_ASSERT(parent);
     parent->children.clear();
     parent->populated = false;
}

void QDirModelPrivate::invalidate()
{
    QStack<const QDirNode*> nodes;
    nodes.push(&root);
    while (!nodes.empty()) {
        const QDirNode *current = nodes.pop();
        current->stat = false;
        const QVector<QDirNode> children = current->children;
        for (int i = 0; i < children.count(); ++i)
            nodes.push(&children.at(i));
    }
}

/*!
    \class QDirModel
    \obsolete
    \brief The QDirModel class provides a data model for the local filesystem.

    \ingroup model-view
    \inmodule QtWidgets

    The usage of QDirModel is not recommended anymore. The
    QFileSystemModel class is a more performant alternative.

    This class provides access to the local filesystem, providing functions
    for renaming and removing files and directories, and for creating new
    directories. In the simplest case, it can be used with a suitable display
    widget as part of a browser or filer.

    QDirModel keeps a cache with file information. The cache needs to be
    updated with refresh().

    QDirModel can be accessed using the standard interface provided by
    QAbstractItemModel, but it also provides some convenience functions
    that are specific to a directory model. The fileInfo() and isDir()
    functions provide information about the underlying files and directories
    related to items in the model.

    Directories can be created and removed using mkdir(), rmdir(), and the
    model will be automatically updated to take the changes into account.

    \note QDirModel requires an instance of a GUI application.

    \sa nameFilters(), setFilter(), filter(), QListView, QTreeView, QFileSystemModel,
    {Dir View Example}, {Model Classes}
*/

/*!
    Constructs a new directory model with the given \a parent.
    Only those files matching the \a nameFilters and the
    \a filters are included in the model. The sort order is given by the
    \a sort flags.
*/

QDirModel::QDirModel(const QStringList &nameFilters,
                     QDir::Filters filters,
                     QDir::SortFlags sort,
                     QObject *parent)
    : QAbstractItemModel(*new QDirModelPrivate, parent)
{
    Q_D(QDirModel);
    // we always start with QDir::drives()
    d->nameFilters = nameFilters.isEmpty() ? QStringList(QLatin1String("*")) : nameFilters;
    d->filters = filters;
    d->sort = sort;
    d->root.parent = 0;
    d->root.info = QFileInfo();
    d->clear(&d->root);
}

/*!
  Constructs a directory model with the given \a parent.
*/

QDirModel::QDirModel(QObject *parent)
    : QAbstractItemModel(*new QDirModelPrivate, parent)
{
    Q_D(QDirModel);
    d->init();
}

/*!
    \internal
*/
QDirModel::QDirModel(QDirModelPrivate &dd, QObject *parent)
    : QAbstractItemModel(dd, parent)
{
    Q_D(QDirModel);
    d->init();
}

/*!
  Destroys this directory model.
*/

QDirModel::~QDirModel()
{

}

/*!
  Returns the model item index for the item in the \a parent with the
  given \a row and \a column.

*/

QModelIndex QDirModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const QDirModel);
    // note that rowCount does lazy population
    if (column < 0 || column >= columnCount(parent) || row < 0 || parent.column() > 0)
        return QModelIndex();
    // make sure the list of children is up to date
    QDirModelPrivate::QDirNode *p = (d->indexValid(parent) ? d->node(parent) : &d->root);
    Q_ASSERT(p);
    if (!p->populated)
        d->populate(p); // populate without stat'ing
    if (row >= p->children.count())
        return QModelIndex();
    // now get the internal pointer for the index
    QDirModelPrivate::QDirNode *n = d->node(row, d->indexValid(parent) ? p : 0);
    Q_ASSERT(n);

    return createIndex(row, column, n);
}

/*!
  Return the parent of the given \a child model item.
*/

QModelIndex QDirModel::parent(const QModelIndex &child) const
{
    Q_D(const QDirModel);

    if (!d->indexValid(child))
        return QModelIndex();
    QDirModelPrivate::QDirNode *node = d->node(child);
    QDirModelPrivate::QDirNode *par = (node ? node->parent : 0);
    if (par == 0) // parent is the root node
        return QModelIndex();

    // get the parent's row
    const QVector<QDirModelPrivate::QDirNode> children =
        par->parent ? par->parent->children : d->root.children;
    Q_ASSERT(children.count() > 0);
    int row = (par - &(children.at(0)));
    Q_ASSERT(row >= 0);

    return createIndex(row, 0, par);
}

/*!
  Returns the number of rows in the \a parent model item.

*/

int QDirModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const QDirModel);
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid()) {
        if (!d->root.populated) // lazy population
            d->populate(&d->root);
        return d->root.children.count();
    }
    if (parent.model() != this)
        return 0;
    QDirModelPrivate::QDirNode *p = d->node(parent);
    if (p->info.isDir() && !p->populated) // lazy population
        d->populate(p);
    return p->children.count();
}

/*!
  Returns the number of columns in the \a parent model item.

*/

int QDirModel::columnCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;
    return 4;
}

/*!
  Returns the data for the model item \a index with the given \a role.
*/
QVariant QDirModel::data(const QModelIndex &index, int role) const
{
    Q_D(const QDirModel);
    if (!d->indexValid(index))
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case 0: return d->name(index);
        case 1: return d->size(index);
        case 2: return d->type(index);
        case 3: return d->time(index);
        default:
            qWarning("data: invalid display value column %d", index.column());
            return QVariant();
        }
    }

    if (index.column() == 0) {
        if (role == FileIconRole)
            return fileIcon(index);
        if (role == FilePathRole)
            return filePath(index);
        if (role == FileNameRole)
            return fileName(index);
    }

    if (index.column() == 1 && Qt::TextAlignmentRole == role) {
        return Qt::AlignRight;
    }
    return QVariant();
}

/*!
  Sets the data for the model item \a index with the given \a role to
  the data referenced by the \a value. Returns \c true if successful;
  otherwise returns \c false.

  \sa Qt::ItemDataRole
*/

bool QDirModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_D(QDirModel);
    if (!d->indexValid(index) || index.column() != 0
        || (flags(index) & Qt::ItemIsEditable) == 0 || role != Qt::EditRole)
        return false;

    QDirModelPrivate::QDirNode *node = d->node(index);
    QDir dir = node->info.dir();
    QString name = value.toString();
    if (dir.rename(node->info.fileName(), name)) {
        node->info = QFileInfo(dir, name);
        QModelIndex sibling = index.sibling(index.row(), 3);
        emit dataChanged(index, sibling);

        d->toBeRefreshed = index.parent();
        QMetaObject::invokeMethod(this, "_q_refresh", Qt::QueuedConnection);

        return true;
    }

    return false;
}

/*!
  Returns the data stored under the given \a role for the specified \a section
  of the header with the given \a orientation.
*/

QVariant QDirModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role != Qt::DisplayRole)
            return QVariant();
        switch (section) {
        case 0: return tr("Name");
        case 1: return tr("Size");
        case 2: return
#ifdef Q_OS_MAC
                       tr("Kind", "Match OS X Finder");
#else
                       tr("Type", "All other platforms");
#endif
        // Windows   - Type
        // OS X      - Kind
        // Konqueror - File Type
        // Nautilus  - Type
        case 3: return tr("Date Modified");
        default: return QVariant();
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

/*!
  Returns \c true if the \a parent model item has children; otherwise
  returns \c false.
*/

bool QDirModel::hasChildren(const QModelIndex &parent) const
{
    Q_D(const QDirModel);
    if (parent.column() > 0)
        return false;

    if (!parent.isValid()) // the invalid index is the "My Computer" item
        return true; // the drives
    QDirModelPrivate::QDirNode *p = d->node(parent);
    Q_ASSERT(p);

    if (d->lazyChildCount) // optimization that only checks for children if the node has been populated
        return p->info.isDir();
    return p->info.isDir() && rowCount(parent) > 0;
}

/*!
  Returns the item flags for the given \a index in the model.

  \sa Qt::ItemFlags
*/
Qt::ItemFlags QDirModel::flags(const QModelIndex &index) const
{
    Q_D(const QDirModel);
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (!d->indexValid(index))
        return flags;
    flags |= Qt::ItemIsDragEnabled;
    if (d->readOnly)
        return flags;
    QDirModelPrivate::QDirNode *node = d->node(index);
    if ((index.column() == 0) && node->info.isWritable()) {
        flags |= Qt::ItemIsEditable;
        if (fileInfo(index).isDir()) // is directory and is editable
            flags |= Qt::ItemIsDropEnabled;
    }
    return flags;
}

/*!
  Sort the model items in the \a column using the \a order given.
  The order is a value defined in \l Qt::SortOrder.
*/

void QDirModel::sort(int column, Qt::SortOrder order)
{
    QDir::SortFlags sort = QDir::DirsFirst | QDir::IgnoreCase;
    if (order == Qt::DescendingOrder)
        sort |= QDir::Reversed;

    switch (column) {
    case 0:
        sort |= QDir::Name;
        break;
    case 1:
        sort |= QDir::Size;
        break;
    case 2:
        sort |= QDir::Type;
        break;
    case 3:
        sort |= QDir::Time;
        break;
    default:
        break;
    }

    setSorting(sort);
}

/*!
    Returns a list of MIME types that can be used to describe a list of items
    in the model.
*/

QStringList QDirModel::mimeTypes() const
{
    return QStringList(QLatin1String("text/uri-list"));
}

/*!
    Returns an object that contains a serialized description of the specified
    \a indexes. The format used to describe the items corresponding to the
    indexes is obtained from the mimeTypes() function.

    If the list of indexes is empty, 0 is returned rather than a serialized
    empty list.
*/

QMimeData *QDirModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QUrl> urls;
    QList<QModelIndex>::const_iterator it = indexes.begin();
    for (; it != indexes.end(); ++it)
        if ((*it).column() == 0)
            urls << QUrl::fromLocalFile(filePath(*it));
    QMimeData *data = new QMimeData();
    data->setUrls(urls);
    return data;
}

/*!
    Handles the \a data supplied by a drag and drop operation that ended with
    the given \a action over the row in the model specified by the \a row and
    \a column and by the \a parent index.

    Returns \c true if the drop was successful, and false otherwise.

    \sa supportedDropActions()
*/

bool QDirModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                             int /* row */, int /* column */, const QModelIndex &parent)
{
    Q_D(QDirModel);
    if (!d->indexValid(parent) || isReadOnly())
        return false;

    bool success = true;
    QString to = filePath(parent) + QDir::separator();
    QModelIndex _parent = parent;

    QList<QUrl> urls = data->urls();
    QList<QUrl>::const_iterator it = urls.constBegin();

    switch (action) {
    case Qt::CopyAction:
        for (; it != urls.constEnd(); ++it) {
            QString path = (*it).toLocalFile();
            success = QFile::copy(path, to + QFileInfo(path).fileName()) && success;
        }
        break;
    case Qt::LinkAction:
        for (; it != urls.constEnd(); ++it) {
            QString path = (*it).toLocalFile();
            success = QFile::link(path, to + QFileInfo(path).fileName()) && success;
        }
        break;
    case Qt::MoveAction:
        for (; it != urls.constEnd(); ++it) {
            QString path = (*it).toLocalFile();
            if (QFile::copy(path, to + QFileInfo(path).fileName())
               && QFile::remove(path)) {
                QModelIndex idx=index(QFileInfo(path).path());
                if (idx.isValid()) {
                    refresh(idx);
                    //the previous call to refresh may invalidate the _parent. so recreate a new QModelIndex
                    _parent = index(to);
                }
            } else {
                success = false;
            }
        }
        break;
    default:
        return false;
    }

    if (success)
        refresh(_parent);

    return success;
}

/*!
  Returns the drop actions supported by this model.

  \sa Qt::DropActions
*/

Qt::DropActions QDirModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction; // FIXME: LinkAction is not supported yet
}

/*!
  Sets the \a provider of file icons for the directory model.

*/

void QDirModel::setIconProvider(QFileIconProvider *provider)
{
    Q_D(QDirModel);
    d->iconProvider = provider;
}

/*!
  Returns the file icon provider for this directory model.
*/

QFileIconProvider *QDirModel::iconProvider() const
{
    Q_D(const QDirModel);
    return d->iconProvider;
}

/*!
  Sets the name \a filters for the directory model.
*/

void QDirModel::setNameFilters(const QStringList &filters)
{
    Q_D(QDirModel);
    d->nameFilters = filters;
    emit layoutAboutToBeChanged();
    if (d->shouldStat)
       refresh(QModelIndex());
    else
        d->invalidate();
    emit layoutChanged();
}

/*!
  Returns a list of filters applied to the names in the model.
*/

QStringList QDirModel::nameFilters() const
{
    Q_D(const QDirModel);
    return d->nameFilters;
}

/*!
  Sets the directory model's filter to that specified by \a filters.

  Note that the filter you set should always include the QDir::AllDirs enum value,
  otherwise QDirModel won't be able to read the directory structure.

  \sa QDir::Filters
*/

void QDirModel::setFilter(QDir::Filters filters)
{
    Q_D(QDirModel);
    d->filters = filters;
    emit layoutAboutToBeChanged();
    if (d->shouldStat)
        refresh(QModelIndex());
    else
        d->invalidate();
    emit layoutChanged();
}

/*!
  Returns the filter specification for the directory model.

  \sa QDir::Filters
*/

QDir::Filters QDirModel::filter() const
{
    Q_D(const QDirModel);
    return d->filters;
}

/*!
  Sets the directory model's sorting order to that specified by \a sort.

  \sa QDir::SortFlags
*/

void QDirModel::setSorting(QDir::SortFlags sort)
{
    Q_D(QDirModel);
    d->sort = sort;
    emit layoutAboutToBeChanged();
    if (d->shouldStat)
        refresh(QModelIndex());
    else
        d->invalidate();
    emit layoutChanged();
}

/*!
  Returns the sorting method used for the directory model.

  \sa QDir::SortFlags
*/

QDir::SortFlags QDirModel::sorting() const
{
    Q_D(const QDirModel);
    return d->sort;
}

/*!
    \property QDirModel::resolveSymlinks
    \brief Whether the directory model should resolve symbolic links

    This is only relevant on operating systems that support symbolic
    links.
*/
void QDirModel::setResolveSymlinks(bool enable)
{
    Q_D(QDirModel);
    d->resolveSymlinks = enable;
}

bool QDirModel::resolveSymlinks() const
{
    Q_D(const QDirModel);
    return d->resolveSymlinks;
}

/*!
  \property QDirModel::readOnly
  \brief Whether the directory model allows writing to the file system

  If this property is set to false, the directory model will allow renaming, copying
  and deleting of files and directories.

  This property is \c true by default
*/

void QDirModel::setReadOnly(bool enable)
{
    Q_D(QDirModel);
    d->readOnly = enable;
}

bool QDirModel::isReadOnly() const
{
    Q_D(const QDirModel);
    return d->readOnly;
}

/*!
  \property QDirModel::lazyChildCount
  \brief Whether the directory model optimizes the hasChildren function
  to only check if the item is a directory.

  If this property is set to false, the directory model will make sure that a directory
  actually containes any files before reporting that it has children.
  Otherwise the directory model will report that an item has children if the item
  is a directory.

  This property is \c false by default
*/

void QDirModel::setLazyChildCount(bool enable)
{
    Q_D(QDirModel);
    d->lazyChildCount = enable;
}

bool QDirModel::lazyChildCount() const
{
    Q_D(const QDirModel);
    return d->lazyChildCount;
}

/*!
  QDirModel caches file information. This function updates the
  cache. The \a parent parameter is the directory from which the
  model is updated; the default value will update the model from
  root directory of the file system (the entire model).
*/

void QDirModel::refresh(const QModelIndex &parent)
{
    Q_D(QDirModel);

    QDirModelPrivate::QDirNode *n = d->indexValid(parent) ? d->node(parent) : &(d->root);

    int rows = n->children.count();
    if (rows == 0) {
        emit layoutAboutToBeChanged();
        n->stat = true; // make sure that next time we read all the info
        n->populated = false;
        emit layoutChanged();
        return;
    }

    emit layoutAboutToBeChanged();
    d->savePersistentIndexes();
    d->rowsAboutToBeRemoved(parent, 0, rows - 1);
    n->stat = true; // make sure that next time we read all the info
    d->clear(n);
    d->rowsRemoved(parent, 0, rows - 1);
    d->restorePersistentIndexes();
    emit layoutChanged();
}

/*!
    \overload

    Returns the model item index for the given \a path.
*/

QModelIndex QDirModel::index(const QString &path, int column) const
{
    Q_D(const QDirModel);

    if (path.isEmpty() || path == QCoreApplication::translate("QFileDialog", "My Computer"))
        return QModelIndex();

    QString absolutePath = QDir(path).absolutePath();
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    absolutePath = absolutePath.toLower();
    // On Windows, "filename......." and "filename" are equivalent
    if (absolutePath.endsWith(QLatin1Char('.'))) {
        int i;
        for (i = absolutePath.count() - 1; i >= 0; --i) {
            if (absolutePath.at(i) != QLatin1Char('.'))
                break;
        }
        absolutePath = absolutePath.left(i+1);
    }
#endif

    QStringList pathElements = absolutePath.split(QLatin1Char('/'), QString::SkipEmptyParts);
    if ((pathElements.isEmpty() || !QFileInfo(path).exists())
#if !defined(Q_OS_WIN) || defined(Q_OS_WINCE)
        && path != QLatin1String("/")
#endif
        )
        return QModelIndex();

    QModelIndex idx; // start with "My Computer"
    if (!d->root.populated) // make sure the root is populated
        d->populate(&d->root);

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    if (absolutePath.startsWith(QLatin1String("//"))) { // UNC path
        QString host = pathElements.first();
        int r = 0;
        for (; r < d->root.children.count(); ++r)
            if (d->root.children.at(r).info.fileName() == host)
                break;
        bool childAppended = false;
        if (r >= d->root.children.count() && d->allowAppendChild) {
            d->appendChild(&d->root, QLatin1String("//") + host);
            childAppended = true;
        }
        idx = index(r, 0, QModelIndex());
        pathElements.pop_front();
        if (childAppended)
            emit const_cast<QDirModel*>(this)->layoutChanged();
    } else
#endif
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    if (pathElements.at(0).endsWith(QLatin1Char(':'))) {
        pathElements[0] += QLatin1Char('/');
    }
#else
    // add the "/" item, since it is a valid path element on unix
    pathElements.prepend(QLatin1String("/"));
#endif

    for (int i = 0; i < pathElements.count(); ++i) {
        Q_ASSERT(!pathElements.at(i).isEmpty());
        QString element = pathElements.at(i);
        QDirModelPrivate::QDirNode *parent = (idx.isValid() ? d->node(idx) : &d->root);

        Q_ASSERT(parent);
        if (!parent->populated)
            d->populate(parent);

        // search for the element in the child nodes first
        int row = -1;
        for (int j = parent->children.count() - 1; j >= 0; --j) {
            const QFileInfo& fi = parent->children.at(j).info;
            QString childFileName;
            childFileName = idx.isValid() ? fi.fileName() : fi.absoluteFilePath();
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
            childFileName = childFileName.toLower();
#endif
            if (childFileName == element) {
                if (i == pathElements.count() - 1)
                    parent->children[j].stat = true;
                row = j;
                break;
            }
        }

        // we couldn't find the path element, we create a new node since we _know_ that the path is valid
        if (row == -1) {
#if defined(Q_OS_WINCE)
            QString newPath;
            if (parent->info.isRoot())
                newPath = parent->info.absoluteFilePath() + element;
            else
                newPath = parent->info.absoluteFilePath() + QLatin1Char('/') + element;
#else
            QString newPath = parent->info.absoluteFilePath() + QLatin1Char('/') + element;
#endif
            if (!d->allowAppendChild || !QFileInfo(newPath).isDir())
                return QModelIndex();
            d->appendChild(parent, newPath);
            row = parent->children.count() - 1;
            if (i == pathElements.count() - 1) // always stat children of  the last element
                parent->children[row].stat = true;
            emit const_cast<QDirModel*>(this)->layoutChanged();
        }

        Q_ASSERT(row >= 0);
        idx = createIndex(row, 0, static_cast<void*>(&parent->children[row]));
        Q_ASSERT(idx.isValid());
    }

    if (column != 0)
        return idx.sibling(idx.row(), column);
    return idx;
}

/*!
  Returns \c true if the model item \a index represents a directory;
  otherwise returns \c false.
*/

bool QDirModel::isDir(const QModelIndex &index) const
{
    Q_D(const QDirModel);
    Q_ASSERT(d->indexValid(index));
    QDirModelPrivate::QDirNode *node = d->node(index);
    return node->info.isDir();
}

/*!
  Create a directory with the \a name in the \a parent model item.
*/

QModelIndex QDirModel::mkdir(const QModelIndex &parent, const QString &name)
{
    Q_D(QDirModel);
    if (!d->indexValid(parent) || isReadOnly())
        return QModelIndex();

    QDirModelPrivate::QDirNode *p = d->node(parent);
    QString path = p->info.absoluteFilePath();
    // For the indexOf() method to work, the new directory has to be a direct child of
    // the parent directory.

    QDir newDir(name);
    QDir dir(path);
    if (newDir.isRelative())
        newDir = QDir(path + QLatin1Char('/') + name);
    QString childName = newDir.dirName(); // Get the singular name of the directory
    newDir.cdUp();

    if (newDir.absolutePath() != dir.absolutePath() || !dir.mkdir(name))
        return QModelIndex(); // nothing happened

    refresh(parent);

    QStringList entryList = d->entryList(path);
    int r = entryList.indexOf(childName);
    QModelIndex i = index(r, 0, parent); // return an invalid index

    return i;
}

/*!
  Removes the directory corresponding to the model item \a index in the
  directory model and \b{deletes the corresponding directory from the
  file system}, returning true if successful. If the directory cannot be
  removed, false is returned.

  \warning This function deletes directories from the file system; it does
  \b{not} move them to a location where they can be recovered.

  \sa remove()
*/

bool QDirModel::rmdir(const QModelIndex &index)
{
    Q_D(QDirModel);
    if (!d->indexValid(index) || isReadOnly())
        return false;

    QDirModelPrivate::QDirNode *n = d_func()->node(index);
    if (!n->info.isDir()) {
        qWarning("rmdir: the node is not a directory");
        return false;
    }

    QModelIndex par = parent(index);
    QDirModelPrivate::QDirNode *p = d_func()->node(par);
    QDir dir = p->info.dir(); // parent dir
    QString path = n->info.absoluteFilePath();
    if (!dir.rmdir(path))
        return false;

    refresh(par);

    return true;
}

/*!
  Removes the model item \a index from the directory model and \b{deletes the
  corresponding file from the file system}, returning true if successful. If the
  item cannot be removed, false is returned.

  \warning This function deletes files from the file system; it does \b{not}
  move them to a location where they can be recovered.

  \sa rmdir()
*/

bool QDirModel::remove(const QModelIndex &index)
{
    Q_D(QDirModel);
    if (!d->indexValid(index) || isReadOnly())
        return false;

    QDirModelPrivate::QDirNode *n = d_func()->node(index);
    if (n->info.isDir())
        return false;

    QModelIndex par = parent(index);
    QDirModelPrivate::QDirNode *p = d_func()->node(par);
    QDir dir = p->info.dir(); // parent dir
    QString path = n->info.absoluteFilePath();
    if (!dir.remove(path))
        return false;

    refresh(par);

    return true;
}

/*!
  Returns the path of the item stored in the model under the
  \a index given.

*/

QString QDirModel::filePath(const QModelIndex &index) const
{
    Q_D(const QDirModel);
    if (d->indexValid(index)) {
        QFileInfo fi = fileInfo(index);
        if (d->resolveSymlinks && fi.isSymLink())
            fi = d->resolvedInfo(fi);
        return QDir::cleanPath(fi.absoluteFilePath());
    }
    return QString(); // root path
}

/*!
  Returns the name of the item stored in the model under the
  \a index given.

*/

QString QDirModel::fileName(const QModelIndex &index) const
{
    Q_D(const QDirModel);
    if (!d->indexValid(index))
        return QString();
    QFileInfo info = fileInfo(index);
    if (info.isRoot())
        return info.absoluteFilePath();
    if (d->resolveSymlinks && info.isSymLink())
        info = d->resolvedInfo(info);
    return info.fileName();
}

/*!
  Returns the icons for the item stored in the model under the given
  \a index.
*/

QIcon QDirModel::fileIcon(const QModelIndex &index) const
{
    Q_D(const QDirModel);
    if (!d->indexValid(index))
        return d->iconProvider->icon(QFileIconProvider::Computer);
    QDirModelPrivate::QDirNode *node = d->node(index);
    if (node->icon.isNull())
        node->icon = d->iconProvider->icon(node->info);
    return node->icon;
}

/*!
  Returns the file information for the specified model \a index.

  \b{Note:} If the model index represents a symbolic link in the
  underlying filing system, the file information returned will contain
  information about the symbolic link itself, regardless of whether
  resolveSymlinks is enabled or not.

  \sa QFileInfo::symLinkTarget()
*/

QFileInfo QDirModel::fileInfo(const QModelIndex &index) const
{
    Q_D(const QDirModel);
    Q_ASSERT(d->indexValid(index));

    QDirModelPrivate::QDirNode *node = d->node(index);
    return node->info;
}

/*!
  \fn QObject *QDirModel::parent() const
  \internal
*/

/*
  The root node is never seen outside the model.
*/

void QDirModelPrivate::init()
{
    filters = QDir::AllEntries | QDir::NoDotAndDotDot;
    sort = QDir::Name;
    nameFilters << QLatin1String("*");
    root.parent = 0;
    root.info = QFileInfo();
    clear(&root);
    roleNames.insertMulti(QDirModel::FileIconRole, QByteArrayLiteral("fileIcon")); // == Qt::decoration
    roleNames.insert(QDirModel::FilePathRole, QByteArrayLiteral("filePath"));
    roleNames.insert(QDirModel::FileNameRole, QByteArrayLiteral("fileName"));
}

QDirModelPrivate::QDirNode *QDirModelPrivate::node(int row, QDirNode *parent) const
{
    if (row < 0)
        return 0;

    bool isDir = !parent || parent->info.isDir();
    QDirNode *p = (parent ? parent : &root);
    if (isDir && !p->populated)
        populate(p); // will also resolve symlinks

    if (row >= p->children.count()) {
        qWarning("node: the row does not exist");
        return 0;
    }

    return const_cast<QDirNode*>(&p->children.at(row));
}

QVector<QDirModelPrivate::QDirNode> QDirModelPrivate::children(QDirNode *parent, bool stat) const
{
    Q_ASSERT(parent);
    QFileInfoList infoList;
    if (parent == &root) {
        parent = 0;
        infoList = QDir::drives();
    } else if (parent->info.isDir()) {
        //resolve directory links only if requested.
        if (parent->info.isSymLink() && resolveSymlinks) {
            QString link = parent->info.symLinkTarget();
            if (link.size() > 1 && link.at(link.size() - 1) == QDir::separator())
                link.chop(1);
            if (stat)
                infoList = entryInfoList(link);
            else
                infoList = QDir(link).entryInfoList(nameFilters, QDir::AllEntries | QDir::System);
        } else {
            if (stat)
                infoList = entryInfoList(parent->info.absoluteFilePath());
            else
                infoList = QDir(parent->info.absoluteFilePath()).entryInfoList(nameFilters, QDir::AllEntries | QDir::System);
        }
    }

    QVector<QDirNode> nodes(infoList.count());
    for (int i = 0; i < infoList.count(); ++i) {
        QDirNode &node = nodes[i];
        node.parent = parent;
        node.info = infoList.at(i);
        node.populated = false;
        node.stat = shouldStat;
    }

    return nodes;
}

void QDirModelPrivate::_q_refresh()
{
    Q_Q(QDirModel);
    q->refresh(toBeRefreshed);
    toBeRefreshed = QModelIndex();
}

void QDirModelPrivate::savePersistentIndexes()
{
    Q_Q(QDirModel);
    savedPersistent.clear();
    foreach (QPersistentModelIndexData *data, persistent.indexes) {
        SavedPersistent saved;
        QModelIndex index = data->index;
        saved.path = q->filePath(index);
        saved.column = index.column();
        saved.data = data;
        saved.index = index;
        savedPersistent.append(saved);
    }
}

void QDirModelPrivate::restorePersistentIndexes()
{
    Q_Q(QDirModel);
    bool allow = allowAppendChild;
    allowAppendChild = false;
    for (int i = 0; i < savedPersistent.count(); ++i) {
        QPersistentModelIndexData *data = savedPersistent.at(i).data;
        QString path = savedPersistent.at(i).path;
        int column = savedPersistent.at(i).column;
        QModelIndex idx = q->index(path, column);
        if (idx != data->index || data->model == 0) {
            //data->model may be equal to 0 if the model is getting destroyed
            persistent.indexes.remove(data->index);
            data->index = idx;
            data->model = q;
            if (idx.isValid())
                persistent.indexes.insert(idx, data);
        }
    }
    savedPersistent.clear();
    allowAppendChild = allow;
}

QFileInfoList QDirModelPrivate::entryInfoList(const QString &path) const
{
    const QDir dir(path);
    return dir.entryInfoList(nameFilters, filters, sort);
}

QStringList QDirModelPrivate::entryList(const QString &path) const
{
    const QDir dir(path);
    return dir.entryList(nameFilters, filters, sort);
}

QString QDirModelPrivate::name(const QModelIndex &index) const
{
    const QDirNode *n = node(index);
    const QFileInfo info = n->info;
    if (info.isRoot()) {
        QString name = info.absoluteFilePath();
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
        if (name.startsWith(QLatin1Char('/'))) // UNC host
            return info.fileName();
        if (name.endsWith(QLatin1Char('/')))
            name.chop(1);
#endif
        return name;
    }
    return info.fileName();
}

QString QDirModelPrivate::size(const QModelIndex &index) const
{
    const QDirNode *n = node(index);
    if (n->info.isDir()) {
#ifdef Q_OS_MAC
        return QLatin1String("--");
#else
        return QLatin1String("");
#endif
    // Windows   - ""
    // OS X      - "--"
    // Konqueror - "4 KB"
    // Nautilus  - "9 items" (the number of children)
    }

    // According to the Si standard KB is 1000 bytes, KiB is 1024
    // but on windows sizes are calulated by dividing by 1024 so we do what they do.
    const quint64 kb = 1024;
    const quint64 mb = 1024 * kb;
    const quint64 gb = 1024 * mb;
    const quint64 tb = 1024 * gb;
    quint64 bytes = n->info.size();
    if (bytes >= tb)
        return QFileSystemModel::tr("%1 TB").arg(QLocale().toString(qreal(bytes) / tb, 'f', 3));
    if (bytes >= gb)
        return QFileSystemModel::tr("%1 GB").arg(QLocale().toString(qreal(bytes) / gb, 'f', 2));
    if (bytes >= mb)
        return QFileSystemModel::tr("%1 MB").arg(QLocale().toString(qreal(bytes) / mb, 'f', 1));
    if (bytes >= kb)
        return QFileSystemModel::tr("%1 KB").arg(QLocale().toString(bytes / kb));
    return QFileSystemModel::tr("%1 byte(s)").arg(QLocale().toString(bytes));
}

QString QDirModelPrivate::type(const QModelIndex &index) const
{
    return iconProvider->type(node(index)->info);
}

QString QDirModelPrivate::time(const QModelIndex &index) const
{
#ifndef QT_NO_DATESTRING
    return node(index)->info.lastModified().toString(Qt::LocalDate);
#else
    Q_UNUSED(index);
    return QString();
#endif
}

void QDirModelPrivate::appendChild(QDirModelPrivate::QDirNode *parent, const QString &path) const
{
    QDirModelPrivate::QDirNode node;
    node.populated = false;
    node.stat = shouldStat;
    node.parent = (parent == &root ? 0 : parent);
    node.info = QFileInfo(path);
    node.info.setCaching(true);

    // The following append(node) may reallocate the vector, thus
    // we need to update the pointers to the childnodes parent.
    QDirModelPrivate *that = const_cast<QDirModelPrivate *>(this);
    that->savePersistentIndexes();
    parent->children.append(node);
    for (int i = 0; i < parent->children.count(); ++i) {
        QDirNode *childNode = &parent->children[i];
        for (int j = 0; j < childNode->children.count(); ++j)
            childNode->children[j].parent = childNode;
    }
    that->restorePersistentIndexes();
}

QFileInfo QDirModelPrivate::resolvedInfo(QFileInfo info)
{
#ifdef Q_OS_WIN
    // On windows, we cannot create a shortcut to a shortcut.
    return QFileInfo(info.symLinkTarget());
#else
    QStringList paths;
    do {
        QFileInfo link(info.symLinkTarget());
        if (link.isRelative())
            info.setFile(info.absolutePath(), link.filePath());
        else
            info = link;
        if (paths.contains(info.absoluteFilePath()))
            return QFileInfo();
        paths.append(info.absoluteFilePath());
    } while (info.isSymLink());
    return info;
#endif
}

QT_END_NAMESPACE

#include "moc_qdirmodel.cpp"

#endif // QT_NO_DIRMODEL
