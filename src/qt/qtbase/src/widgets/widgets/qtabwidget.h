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

#ifndef QTABWIDGET_H
#define QTABWIDGET_H

#include <QtWidgets/qwidget.h>
#include <QtGui/qicon.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_TABWIDGET

class QTabBar;
class QTabWidgetPrivate;
class QStyleOptionTabWidgetFrame;

class Q_WIDGETS_EXPORT QTabWidget : public QWidget
{
    Q_OBJECT
    Q_ENUMS(TabPosition TabShape)
    Q_PROPERTY(TabPosition tabPosition READ tabPosition WRITE setTabPosition)
    Q_PROPERTY(TabShape tabShape READ tabShape WRITE setTabShape)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
    Q_PROPERTY(int count READ count)
    Q_PROPERTY(QSize iconSize READ iconSize WRITE setIconSize)
    Q_PROPERTY(Qt::TextElideMode elideMode READ elideMode WRITE setElideMode)
    Q_PROPERTY(bool usesScrollButtons READ usesScrollButtons WRITE setUsesScrollButtons)
    Q_PROPERTY(bool documentMode READ documentMode WRITE setDocumentMode)
    Q_PROPERTY(bool tabsClosable READ tabsClosable WRITE setTabsClosable)
    Q_PROPERTY(bool movable READ isMovable WRITE setMovable)

public:
    explicit QTabWidget(QWidget *parent = 0);
    ~QTabWidget();

    int addTab(QWidget *widget, const QString &);
    int addTab(QWidget *widget, const QIcon& icon, const QString &label);

    int insertTab(int index, QWidget *widget, const QString &);
    int insertTab(int index, QWidget *widget, const QIcon& icon, const QString &label);

    void removeTab(int index);

    bool isTabEnabled(int index) const;
    void setTabEnabled(int index, bool);

    QString tabText(int index) const;
    void setTabText(int index, const QString &);

    QIcon tabIcon(int index) const;
    void setTabIcon(int index, const QIcon & icon);

#ifndef QT_NO_TOOLTIP
    void setTabToolTip(int index, const QString & tip);
    QString tabToolTip(int index) const;
#endif

#ifndef QT_NO_WHATSTHIS
    void setTabWhatsThis(int index, const QString &text);
    QString tabWhatsThis(int index) const;
#endif

    int currentIndex() const;
    QWidget *currentWidget() const;
    QWidget *widget(int index) const;
    int indexOf(QWidget *widget) const;
    int count() const;

    enum TabPosition { North, South, West, East };
    TabPosition tabPosition() const;
    void setTabPosition(TabPosition);

    bool tabsClosable() const;
    void setTabsClosable(bool closeable);

    bool isMovable() const;
    void setMovable(bool movable);

    enum TabShape { Rounded, Triangular };
    TabShape tabShape() const;
    void setTabShape(TabShape s);

    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    int heightForWidth(int width) const;
    bool hasHeightForWidth() const;

    void setCornerWidget(QWidget * w, Qt::Corner corner = Qt::TopRightCorner);
    QWidget * cornerWidget(Qt::Corner corner = Qt::TopRightCorner) const;

    Qt::TextElideMode elideMode() const;
    void setElideMode(Qt::TextElideMode);

    QSize iconSize() const;
    void setIconSize(const QSize &size);

    bool usesScrollButtons() const;
    void setUsesScrollButtons(bool useButtons);

    bool documentMode() const;
    void setDocumentMode(bool set);

    void clear();

    QTabBar* tabBar() const;

public Q_SLOTS:
    void setCurrentIndex(int index);
    void setCurrentWidget(QWidget *widget);

Q_SIGNALS:
    void currentChanged(int index);
    void tabCloseRequested(int index);
    void tabBarClicked(int index);
    void tabBarDoubleClicked(int index);

protected:
    virtual void tabInserted(int index);
    virtual void tabRemoved(int index);

    void showEvent(QShowEvent *);
    void resizeEvent(QResizeEvent *);
    void keyPressEvent(QKeyEvent *);
    void paintEvent(QPaintEvent *);
    void setTabBar(QTabBar *);
    void changeEvent(QEvent *);
    bool event(QEvent *);
    void initStyleOption(QStyleOptionTabWidgetFrame *option) const;


private:
    Q_DECLARE_PRIVATE(QTabWidget)
    Q_DISABLE_COPY(QTabWidget)
    Q_PRIVATE_SLOT(d_func(), void _q_showTab(int))
    Q_PRIVATE_SLOT(d_func(), void _q_removeTab(int))
    Q_PRIVATE_SLOT(d_func(), void _q_tabMoved(int, int))
    void setUpLayout(bool = false);
};

#endif // QT_NO_TABWIDGET

QT_END_NAMESPACE

#endif // QTABWIDGET_H
