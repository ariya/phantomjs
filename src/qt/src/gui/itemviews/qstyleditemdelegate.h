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

#ifndef QSTYLEDITEMDELEGATE_H
#define QSTYLEDITEMDELEGATE_H

#include <QtGui/qabstractitemdelegate.h>
#include <QtCore/qstring.h>
#include <QtGui/qpixmap.h>
#include <QtCore/qvariant.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_ITEMVIEWS

class QStyledItemDelegatePrivate;
class QItemEditorFactory;

class Q_GUI_EXPORT QStyledItemDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    explicit QStyledItemDelegate(QObject *parent = 0);
    ~QStyledItemDelegate();

    // painting
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;

    // editing
    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const;

    // editor factory
    QItemEditorFactory *itemEditorFactory() const;
    void setItemEditorFactory(QItemEditorFactory *factory);

    virtual QString displayText(const QVariant &value, const QLocale &locale) const;

protected:
    virtual void initStyleOption(QStyleOptionViewItem *option,
                                const QModelIndex &index) const;

    bool eventFilter(QObject *object, QEvent *event);
    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option, const QModelIndex &index);

private:
    Q_DECLARE_PRIVATE(QStyledItemDelegate)
    Q_DISABLE_COPY(QStyledItemDelegate)

    Q_PRIVATE_SLOT(d_func(), void _q_commitDataAndCloseEditor(QWidget*))
};

#endif // QT_NO_ITEMVIEWS

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSTYLEDITEMDELEGATE_H
