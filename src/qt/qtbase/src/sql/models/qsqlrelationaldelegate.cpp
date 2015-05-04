/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSql module of the Qt Toolkit.
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

#include "qglobal.h"

QT_BEGIN_NAMESPACE

/*!
    \class QSqlRelationalDelegate
    \inmodule QtSql
    \brief The QSqlRelationalDelegate class provides a delegate that is used to
    display and edit data from a QSqlRelationalTableModel.

    Unlike the default delegate, QSqlRelationalDelegate provides a
    combobox for fields that are foreign keys into other tables. To
    use the class, simply call QAbstractItemView::setItemDelegate()
    on the view with an instance of QSqlRelationalDelegate:

    \snippet relationaltablemodel/relationaltablemodel.cpp 4

    The \l{relationaltablemodel}{Relational Table Model} example
    (shown below) illustrates how to use QSqlRelationalDelegate in
    conjunction with QSqlRelationalTableModel to provide tables with
    foreign key support.

    \image relationaltable.png

    \sa QSqlRelationalTableModel, {Model/View Programming}
*/


/*!
    \fn QSqlRelationalDelegate::QSqlRelationalDelegate(QObject *parent)

    Constructs a QSqlRelationalDelegate object with the given \a
    parent.
*/

/*!
    \fn QSqlRelationalDelegate::~QSqlRelationalDelegate()

    Destroys the QSqlRelationalDelegate object and frees any
    allocated resources.
*/

/*!
    \fn QWidget *QSqlRelationalDelegate::createEditor(QWidget *parent,
                                                      const QStyleOptionViewItem &option,
                                                      const QModelIndex &index) const
    \reimp
*/

/*!
    \fn void QSqlRelationalDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                                  const QModelIndex &index) const
    \reimp
*/

QT_END_NAMESPACE
