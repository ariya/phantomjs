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

#ifndef QGROUPBOX_H
#define QGROUPBOX_H

#include <QtWidgets/qframe.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_GROUPBOX

class QGroupBoxPrivate;
class QStyleOptionGroupBox;
class Q_WIDGETS_EXPORT QGroupBox : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
    Q_PROPERTY(bool flat READ isFlat WRITE setFlat)
    Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable)
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked DESIGNABLE isCheckable NOTIFY toggled USER true)
public:
    explicit QGroupBox(QWidget* parent=0);
    explicit QGroupBox(const QString &title, QWidget* parent=0);
    ~QGroupBox();

    QString title() const;
    void setTitle(const QString &title);

    Qt::Alignment alignment() const;
    void setAlignment(int alignment);

    QSize minimumSizeHint() const;

    bool isFlat() const;
    void setFlat(bool flat);
    bool isCheckable() const;
    void setCheckable(bool checkable);
    bool isChecked() const;

public Q_SLOTS:
    void setChecked(bool checked);

Q_SIGNALS:
    void clicked(bool checked = false);
    void toggled(bool);

protected:
    bool event(QEvent *event);
    void childEvent(QChildEvent *event);
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
    void focusInEvent(QFocusEvent *event);
    void changeEvent(QEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void initStyleOption(QStyleOptionGroupBox *option) const;


private:
    Q_DISABLE_COPY(QGroupBox)
    Q_DECLARE_PRIVATE(QGroupBox)
    Q_PRIVATE_SLOT(d_func(), void _q_setChildrenEnabled(bool b))
};

#endif // QT_NO_GROUPBOX

QT_END_NAMESPACE

#endif // QGROUPBOX_H
