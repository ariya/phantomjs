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

#include "qundostack.h"
#include "qundoview.h"

#ifndef QT_NO_UNDOVIEW

#include "qundogroup.h"
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qpointer.h>
#include <QtGui/qicon.h>
#include <private/qlistview_p.h>

QT_BEGIN_NAMESPACE

class QUndoModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    QUndoModel(QObject *parent = 0);

    QUndoStack *stack() const;

    virtual QModelIndex index(int row, int column,
                const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QModelIndex selectedIndex() const;
    QItemSelectionModel *selectionModel() const;

    QString emptyLabel() const;
    void setEmptyLabel(const QString &label);

    void setCleanIcon(const QIcon &icon);
    QIcon cleanIcon() const;

public slots:
    void setStack(QUndoStack *stack);

private slots:
    void stackChanged();
    void stackDestroyed(QObject *obj);
    void setStackCurrentIndex(const QModelIndex &index);

private:
    QUndoStack *m_stack;
    QItemSelectionModel *m_sel_model;
    QString m_emty_label;
    QIcon m_clean_icon;
};

QUndoModel::QUndoModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_stack = 0;
    m_sel_model = new QItemSelectionModel(this, this);
    connect(m_sel_model, SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(setStackCurrentIndex(QModelIndex)));
    m_emty_label = tr("<empty>");
}

QItemSelectionModel *QUndoModel::selectionModel() const
{
    return m_sel_model;
}

QUndoStack *QUndoModel::stack() const
{
    return m_stack;
}

void QUndoModel::setStack(QUndoStack *stack)
{
    if (m_stack == stack)
        return;

    if (m_stack != 0) {
        disconnect(m_stack, SIGNAL(cleanChanged(bool)), this, SLOT(stackChanged()));
        disconnect(m_stack, SIGNAL(indexChanged(int)), this, SLOT(stackChanged()));
        disconnect(m_stack, SIGNAL(destroyed(QObject*)), this, SLOT(stackDestroyed(QObject*)));
    }
    m_stack = stack;
    if (m_stack != 0) {
        connect(m_stack, SIGNAL(cleanChanged(bool)), this, SLOT(stackChanged()));
        connect(m_stack, SIGNAL(indexChanged(int)), this, SLOT(stackChanged()));
        connect(m_stack, SIGNAL(destroyed(QObject*)), this, SLOT(stackDestroyed(QObject*)));
    }

    stackChanged();
}

void QUndoModel::stackDestroyed(QObject *obj)
{
    if (obj != m_stack)
        return;
    m_stack = 0;

    stackChanged();
}

void QUndoModel::stackChanged()
{
    beginResetModel();
    endResetModel();
    m_sel_model->setCurrentIndex(selectedIndex(), QItemSelectionModel::ClearAndSelect);
}

void QUndoModel::setStackCurrentIndex(const QModelIndex &index)
{
    if (m_stack == 0)
        return;

    if (index == selectedIndex())
        return;

    if (index.column() != 0)
        return;

    m_stack->setIndex(index.row());
}

QModelIndex QUndoModel::selectedIndex() const
{
    return m_stack == 0 ? QModelIndex() : createIndex(m_stack->index(), 0);
}

QModelIndex QUndoModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_stack == 0)
        return QModelIndex();

    if (parent.isValid())
        return QModelIndex();

    if (column != 0)
        return QModelIndex();

    if (row < 0 || row > m_stack->count())
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex QUndoModel::parent(const QModelIndex&) const
{
    return QModelIndex();
}

int QUndoModel::rowCount(const QModelIndex &parent) const
{
    if (m_stack == 0)
        return 0;

    if (parent.isValid())
        return 0;

    return m_stack->count() + 1;
}

int QUndoModel::columnCount(const QModelIndex&) const
{
    return 1;
}

QVariant QUndoModel::data(const QModelIndex &index, int role) const
{
    if (m_stack == 0)
        return QVariant();

    if (index.column() != 0)
        return QVariant();

    if (index.row() < 0 || index.row() > m_stack->count())
        return QVariant();

    if (role == Qt::DisplayRole) {
        if (index.row() == 0)
            return m_emty_label;
        return m_stack->text(index.row() - 1);
    } else if (role == Qt::DecorationRole) {
        if (index.row() == m_stack->cleanIndex() && !m_clean_icon.isNull())
            return m_clean_icon;
        return QVariant();
    }

    return QVariant();
}

QString QUndoModel::emptyLabel() const
{
    return m_emty_label;
}

void QUndoModel::setEmptyLabel(const QString &label)
{
    m_emty_label = label;
    stackChanged();
}

void QUndoModel::setCleanIcon(const QIcon &icon)
{
    m_clean_icon = icon;
    stackChanged();
}

QIcon QUndoModel::cleanIcon() const
{
    return m_clean_icon;
}

/*!
    \class QUndoView
    \brief The QUndoView class displays the contents of a QUndoStack.
    \since 4.2

    \ingroup advanced
    \inmodule QtWidgets

    QUndoView is a QListView which displays the list of commands pushed on an undo stack.
    The most recently executed command is always selected. Selecting a different command
    results in a call to QUndoStack::setIndex(), rolling the state of the document
    backwards or forward to the new command.

    The stack can be set explicitly with setStack(). Alternatively, a QUndoGroup object can
    be set with setGroup(). The view will then update itself automatically whenever the
    active stack of the group changes.

    \image qundoview.png
*/

class QUndoViewPrivate : public QListViewPrivate
{
    Q_DECLARE_PUBLIC(QUndoView)
public:
    QUndoViewPrivate() :
#ifndef QT_NO_UNDOGROUP
        group(0),
#endif
        model(0) {}

#ifndef QT_NO_UNDOGROUP
    QPointer<QUndoGroup> group;
#endif
    QUndoModel *model;

    void init();
};

void QUndoViewPrivate::init()
{
    Q_Q(QUndoView);

    model = new QUndoModel(q);
    q->setModel(model);
    q->setSelectionModel(model->selectionModel());
}

/*!
    Constructs a new view with parent \a parent.
*/

QUndoView::QUndoView(QWidget *parent)
    : QListView(*new QUndoViewPrivate(), parent)
{
    Q_D(QUndoView);
    d->init();
}

/*!
    Constructs a new view with parent \a parent and sets the observed stack to \a stack.
*/

QUndoView::QUndoView(QUndoStack *stack, QWidget *parent)
    : QListView(*new QUndoViewPrivate(), parent)
{
    Q_D(QUndoView);
    d->init();
    setStack(stack);
}

#ifndef QT_NO_UNDOGROUP

/*!
    Constructs a new view with parent \a parent and sets the observed group to \a group.

    The view will update itself autmiatically whenever the active stack of the group changes.
*/

QUndoView::QUndoView(QUndoGroup *group, QWidget *parent)
    : QListView(*new QUndoViewPrivate(), parent)
{
    Q_D(QUndoView);
    d->init();
    setGroup(group);
}

#endif // QT_NO_UNDOGROUP

/*!
    Destroys this view.
*/

QUndoView::~QUndoView()
{
}

/*!
    Returns the stack currently displayed by this view. If the view is looking at a
    QUndoGroup, this the group's active stack.

    \sa setStack(), setGroup()
*/

QUndoStack *QUndoView::stack() const
{
    Q_D(const QUndoView);
    return d->model->stack();
}

/*!
    Sets the stack displayed by this view to \a stack. If \a stack is 0, the view
    will be empty.

    If the view was previously looking at a QUndoGroup, the group is set to 0.

    \sa stack(), setGroup()
*/

void QUndoView::setStack(QUndoStack *stack)
{
    Q_D(QUndoView);
#ifndef QT_NO_UNDOGROUP
    setGroup(0);
#endif
    d->model->setStack(stack);
}

#ifndef QT_NO_UNDOGROUP

/*!
    Sets the group displayed by this view to \a group. If \a group is 0, the view will
    be empty.

    The view will update itself autmiatically whenever the active stack of the group changes.

    \sa group(), setStack()
*/

void QUndoView::setGroup(QUndoGroup *group)
{
    Q_D(QUndoView);

    if (d->group == group)
        return;

    if (d->group != 0) {
        disconnect(d->group, SIGNAL(activeStackChanged(QUndoStack*)),
                d->model, SLOT(setStack(QUndoStack*)));
    }

    d->group = group;

    if (d->group != 0) {
        connect(d->group, SIGNAL(activeStackChanged(QUndoStack*)),
                d->model, SLOT(setStack(QUndoStack*)));
        d->model->setStack(d->group->activeStack());
    } else {
        d->model->setStack(0);
    }
}

/*!
    Returns the group displayed by this view.

    If the view is not looking at group, this function returns 0.

    \sa setGroup(), setStack()
*/

QUndoGroup *QUndoView::group() const
{
    Q_D(const QUndoView);
    return d->group;
}

#endif // QT_NO_UNDOGROUP

/*!
    \property QUndoView::emptyLabel
    \brief the label used for the empty state.

    The empty label is the topmost element in the list of commands, which represents
    the state of the document before any commands were pushed on the stack. The default
    is the string "<empty>".
*/

void QUndoView::setEmptyLabel(const QString &label)
{
    Q_D(QUndoView);
    d->model->setEmptyLabel(label);
}

QString QUndoView::emptyLabel() const
{
    Q_D(const QUndoView);
    return d->model->emptyLabel();
}

/*!
    \property QUndoView::cleanIcon
    \brief the icon used to represent the clean state.

    A stack may have a clean state set with QUndoStack::setClean(). This is usually
    the state of the document at the point it was saved. QUndoView can display an
    icon in the list of commands to show the clean state. If this property is
    a null icon, no icon is shown. The default value is the null icon.
*/

void QUndoView::setCleanIcon(const QIcon &icon)
{
    Q_D(const QUndoView);
    d->model->setCleanIcon(icon);

}

QIcon QUndoView::cleanIcon() const
{
    Q_D(const QUndoView);
    return d->model->cleanIcon();
}

QT_END_NAMESPACE

#include "qundoview.moc"

#endif // QT_NO_UNDOVIEW
