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

#ifndef QGROUPBOX_H
#define QGROUPBOX_H

#include <QtGui/qframe.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_GROUPBOX

class QGroupBoxPrivate;
class QStyleOptionGroupBox;
class Q_GUI_EXPORT QGroupBox : public QWidget
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

#ifdef QT3_SUPPORT
public:
    QT3_SUPPORT_CONSTRUCTOR QGroupBox(QWidget* parent, const char* name);
    QT3_SUPPORT_CONSTRUCTOR QGroupBox(const QString &title, QWidget* parent, const char* name);
#endif

private:
    Q_DISABLE_COPY(QGroupBox)
    Q_DECLARE_PRIVATE(QGroupBox)
    Q_PRIVATE_SLOT(d_func(), void _q_setChildrenEnabled(bool b))
};

#endif // QT_NO_GROUPBOX

QT_END_NAMESPACE

QT_END_HEADER

#endif // QGROUPBOX_H
