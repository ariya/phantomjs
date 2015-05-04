/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QFORMLAYOUT_H
#define QFORMLAYOUT_H

#include <QtWidgets/QLayout>

QT_BEGIN_NAMESPACE


class QFormLayoutPrivate;

class Q_WIDGETS_EXPORT QFormLayout : public QLayout
{
    Q_OBJECT
    Q_ENUMS(FieldGrowthPolicy RowWrapPolicy ItemRole)
    Q_DECLARE_PRIVATE(QFormLayout)
    Q_PROPERTY(FieldGrowthPolicy fieldGrowthPolicy READ fieldGrowthPolicy WRITE setFieldGrowthPolicy RESET resetFieldGrowthPolicy)
    Q_PROPERTY(RowWrapPolicy rowWrapPolicy READ rowWrapPolicy WRITE setRowWrapPolicy RESET resetRowWrapPolicy)
    Q_PROPERTY(Qt::Alignment labelAlignment READ labelAlignment WRITE setLabelAlignment RESET resetLabelAlignment)
    Q_PROPERTY(Qt::Alignment formAlignment READ formAlignment WRITE setFormAlignment RESET resetFormAlignment)
    Q_PROPERTY(int horizontalSpacing READ horizontalSpacing WRITE setHorizontalSpacing)
    Q_PROPERTY(int verticalSpacing READ verticalSpacing WRITE setVerticalSpacing)

public:
    enum FieldGrowthPolicy {
        FieldsStayAtSizeHint,
        ExpandingFieldsGrow,
        AllNonFixedFieldsGrow
    };

    enum RowWrapPolicy {
        DontWrapRows,
        WrapLongRows,
        WrapAllRows
    };

    enum ItemRole {
        LabelRole = 0,
        FieldRole = 1,
        SpanningRole = 2
    };

    explicit QFormLayout(QWidget *parent = 0);
    ~QFormLayout();

    void setFieldGrowthPolicy(FieldGrowthPolicy policy);
    FieldGrowthPolicy fieldGrowthPolicy() const;
    void setRowWrapPolicy(RowWrapPolicy policy);
    RowWrapPolicy rowWrapPolicy() const;
    void setLabelAlignment(Qt::Alignment alignment);
    Qt::Alignment labelAlignment() const;
    void setFormAlignment(Qt::Alignment alignment);
    Qt::Alignment formAlignment() const;

    void setHorizontalSpacing(int spacing);
    int horizontalSpacing() const;
    void setVerticalSpacing(int spacing);
    int verticalSpacing() const;

    int spacing() const;
    void setSpacing(int);

    void addRow(QWidget *label, QWidget *field);
    void addRow(QWidget *label, QLayout *field);
    void addRow(const QString &labelText, QWidget *field);
    void addRow(const QString &labelText, QLayout *field);
    void addRow(QWidget *widget);
    void addRow(QLayout *layout);

    void insertRow(int row, QWidget *label, QWidget *field);
    void insertRow(int row, QWidget *label, QLayout *field);
    void insertRow(int row, const QString &labelText, QWidget *field);
    void insertRow(int row, const QString &labelText, QLayout *field);
    void insertRow(int row, QWidget *widget);
    void insertRow(int row, QLayout *layout);

    void setItem(int row, ItemRole role, QLayoutItem *item);
    void setWidget(int row, ItemRole role, QWidget *widget);
    void setLayout(int row, ItemRole role, QLayout *layout);

    QLayoutItem *itemAt(int row, ItemRole role) const;
    void getItemPosition(int index, int *rowPtr, ItemRole *rolePtr) const;
    void getWidgetPosition(QWidget *widget, int *rowPtr, ItemRole *rolePtr) const;
    void getLayoutPosition(QLayout *layout, int *rowPtr, ItemRole *rolePtr) const;
    QWidget *labelForField(QWidget *field) const;
    QWidget *labelForField(QLayout *field) const;

    // reimplemented from QLayout
    void addItem(QLayoutItem *item);
    QLayoutItem *itemAt(int index) const;
    QLayoutItem *takeAt(int index);

    void setGeometry(const QRect &rect);
    QSize minimumSize() const;
    QSize sizeHint() const;
    void invalidate();

    bool hasHeightForWidth() const;
    int heightForWidth(int width) const;
    Qt::Orientations expandingDirections() const;
    int count() const;

    int rowCount() const;

#if 0
    void dump() const;
#endif

private:
    void resetFieldGrowthPolicy();
    void resetRowWrapPolicy();
    void resetLabelAlignment();
    void resetFormAlignment();
};

QT_END_NAMESPACE

#endif
