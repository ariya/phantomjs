/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QTOOLBOX_H
#define QTOOLBOX_H

#include <QtGui/qframe.h>
#include <QtGui/qicon.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_TOOLBOX

class QToolBoxPrivate;

class Q_GUI_EXPORT QToolBox : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
    Q_PROPERTY(int count READ count)

public:
    explicit QToolBox(QWidget *parent = 0, Qt::WindowFlags f = 0);
    ~QToolBox();

    int addItem(QWidget *widget, const QString &text);
    int addItem(QWidget *widget, const QIcon &icon, const QString &text);
    int insertItem(int index, QWidget *widget, const QString &text);
    int insertItem(int index, QWidget *widget, const QIcon &icon, const QString &text);

    void removeItem(int index);

    void setItemEnabled(int index, bool enabled);
    bool isItemEnabled(int index) const;

    void setItemText(int index, const QString &text);
    QString itemText(int index) const;

    void setItemIcon(int index, const QIcon &icon);
    QIcon itemIcon(int index) const;

#ifndef QT_NO_TOOLTIP
    void setItemToolTip(int index, const QString &toolTip);
    QString itemToolTip(int index) const;
#endif

    int currentIndex() const;
    QWidget *currentWidget() const;
    QWidget *widget(int index) const;
    int indexOf(QWidget *widget) const;
    int count() const;

public Q_SLOTS:
    void setCurrentIndex(int index);
    void setCurrentWidget(QWidget *widget);

Q_SIGNALS:
    void currentChanged(int index);

protected:
    bool event(QEvent *e);
    virtual void itemInserted(int index);
    virtual void itemRemoved(int index);
    void showEvent(QShowEvent *e);
    void changeEvent(QEvent *);

#ifdef QT3_SUPPORT
public:
    QT3_SUPPORT_CONSTRUCTOR QToolBox(QWidget *parent, const char *name, Qt::WindowFlags f = 0);
    inline QT3_SUPPORT void setItemLabel(int index, const QString &text) { setItemText(index, text); }
    inline QT3_SUPPORT QString itemLabel(int index) const { return itemText(index); }
    inline QT3_SUPPORT QWidget *currentItem() const { return widget(currentIndex()); }
    inline QT3_SUPPORT void setCurrentItem(QWidget *item) { setCurrentIndex(indexOf(item)); }
    inline QT3_SUPPORT void setItemIconSet(int index, const QIcon &icon) { setItemIcon(index, icon); }
    inline QT3_SUPPORT QIcon itemIconSet(int index) const { return itemIcon(index); }
    inline QT3_SUPPORT int removeItem(QWidget *item)
    { int i = indexOf(item); removeItem(i); return i; }
    inline QT3_SUPPORT QWidget *item(int index) const { return widget(index); }
    QT3_SUPPORT void setMargin(int margin) { setContentsMargins(margin, margin, margin, margin); }
    QT3_SUPPORT int margin() const
    { int margin; int dummy; getContentsMargins(&margin, &dummy, &dummy, &dummy);  return margin; }
#endif

private:
    Q_DECLARE_PRIVATE(QToolBox)
    Q_DISABLE_COPY(QToolBox)
    Q_PRIVATE_SLOT(d_func(), void _q_buttonClicked())
    Q_PRIVATE_SLOT(d_func(), void _q_widgetDestroyed(QObject*))
};


inline int QToolBox::addItem(QWidget *item, const QString &text)
{ return insertItem(-1, item, QIcon(), text); }
inline int QToolBox::addItem(QWidget *item, const QIcon &iconSet,
                              const QString &text)
{ return insertItem(-1, item, iconSet, text); }
inline int QToolBox::insertItem(int index, QWidget *item, const QString &text)
{ return insertItem(index, item, QIcon(), text); }

#endif // QT_NO_TOOLBOX

QT_END_NAMESPACE

QT_END_HEADER

#endif // QTOOLBOX_H
