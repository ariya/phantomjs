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

#ifndef QDYNAMICDOCKWIDGET_H
#define QDYNAMICDOCKWIDGET_H

#include <QtGui/qwidget.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_DOCKWIDGET

class QDockAreaLayout;
class QDockWidgetPrivate;
class QMainWindow;
class QStyleOptionDockWidget;

class Q_GUI_EXPORT QDockWidget : public QWidget
{
    Q_OBJECT

    Q_FLAGS(DockWidgetFeatures)
    Q_PROPERTY(bool floating READ isFloating WRITE setFloating)
    Q_PROPERTY(DockWidgetFeatures features READ features WRITE setFeatures NOTIFY featuresChanged)
    Q_PROPERTY(Qt::DockWidgetAreas allowedAreas READ allowedAreas
               WRITE setAllowedAreas NOTIFY allowedAreasChanged)
    Q_PROPERTY(QString windowTitle READ windowTitle WRITE setWindowTitle DESIGNABLE true)

public:
    explicit QDockWidget(const QString &title, QWidget *parent = 0, Qt::WindowFlags flags = 0);
    explicit QDockWidget(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~QDockWidget();

    QWidget *widget() const;
    void setWidget(QWidget *widget);

    enum DockWidgetFeature {
        DockWidgetClosable    = 0x01,
        DockWidgetMovable     = 0x02,
        DockWidgetFloatable   = 0x04,
        DockWidgetVerticalTitleBar = 0x08,

        DockWidgetFeatureMask = 0x0f,
        AllDockWidgetFeatures = DockWidgetClosable|DockWidgetMovable|DockWidgetFloatable, // ### remove in 5.0
        NoDockWidgetFeatures  = 0x00,

        Reserved              = 0xff
    };
    Q_DECLARE_FLAGS(DockWidgetFeatures, DockWidgetFeature)

    void setFeatures(DockWidgetFeatures features);
    DockWidgetFeatures features() const;

    void setFloating(bool floating);
    inline bool isFloating() const { return isWindow(); }

    void setAllowedAreas(Qt::DockWidgetAreas areas);
    Qt::DockWidgetAreas allowedAreas() const;

    void setTitleBarWidget(QWidget *widget);
    QWidget *titleBarWidget() const;

    inline bool isAreaAllowed(Qt::DockWidgetArea area) const
    { return (allowedAreas() & area) == area; }

#ifndef QT_NO_ACTION
    QAction *toggleViewAction() const;
#endif

Q_SIGNALS:
    void featuresChanged(QDockWidget::DockWidgetFeatures features);
    void topLevelChanged(bool topLevel);
    void allowedAreasChanged(Qt::DockWidgetAreas allowedAreas);
    void visibilityChanged(bool visible);
    void dockLocationChanged(Qt::DockWidgetArea area);

protected:
    void changeEvent(QEvent *event);
    void closeEvent(QCloseEvent *event);
    void paintEvent(QPaintEvent *event);
    bool event(QEvent *event);
    void initStyleOption(QStyleOptionDockWidget *option) const;

private:
    Q_DECLARE_PRIVATE(QDockWidget)
    Q_DISABLE_COPY(QDockWidget)
    Q_PRIVATE_SLOT(d_func(), void _q_toggleView(bool))
    Q_PRIVATE_SLOT(d_func(), void _q_toggleTopLevel())
    friend class QDockAreaLayout;
    friend class QDockWidgetItem;
    friend class QMainWindowLayout;
    friend class QDockWidgetLayout;
    friend class QDockAreaLayoutInfo;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QDockWidget::DockWidgetFeatures)

#endif // QT_NO_DOCKWIDGET

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDYNAMICDOCKWIDGET_H
