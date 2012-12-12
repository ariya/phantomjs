/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QFRAME_H
#define QFRAME_H

#include <QtGui/qwidget.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QFramePrivate;

class Q_GUI_EXPORT QFrame : public QWidget
{
    Q_OBJECT

    Q_ENUMS(Shape Shadow)
    Q_PROPERTY(Shape frameShape READ frameShape WRITE setFrameShape)
    Q_PROPERTY(Shadow frameShadow READ frameShadow WRITE setFrameShadow)
    Q_PROPERTY(int lineWidth READ lineWidth WRITE setLineWidth)
    Q_PROPERTY(int midLineWidth READ midLineWidth WRITE setMidLineWidth)
    Q_PROPERTY(int frameWidth READ frameWidth)
    Q_PROPERTY(QRect frameRect READ frameRect WRITE setFrameRect DESIGNABLE false)

public:
    explicit QFrame(QWidget* parent = 0, Qt::WindowFlags f = 0);
    ~QFrame();

    int frameStyle() const;
    void setFrameStyle(int);

    int frameWidth() const;

    QSize sizeHint() const;

    enum Shape {
        NoFrame  = 0, // no frame
        Box = 0x0001, // rectangular box
        Panel = 0x0002, // rectangular panel
        WinPanel = 0x0003, // rectangular panel (Windows)
        HLine = 0x0004, // horizontal line
        VLine = 0x0005, // vertical line
        StyledPanel = 0x0006 // rectangular panel depending on the GUI style

#if defined(QT3_SUPPORT) && !defined(Q_MOC_RUN)
        ,PopupPanel = StyledPanel, // rectangular panel depending on the GUI style
        MenuBarPanel = StyledPanel,
        ToolBarPanel = StyledPanel,
        LineEditPanel = StyledPanel,
        TabWidgetPanel = StyledPanel,
        GroupBoxPanel = StyledPanel
#endif
    };
    enum Shadow {
        Plain = 0x0010, // plain line
        Raised = 0x0020, // raised shadow effect
        Sunken = 0x0030 // sunken shadow effect
    };

    enum StyleMask {
        Shadow_Mask = 0x00f0, // mask for the shadow
        Shape_Mask = 0x000f // mask for the shape
#if defined(QT3_SUPPORT)
        ,MShadow = Shadow_Mask,
        MShape = Shape_Mask
#endif
    };

    Shape frameShape() const;
    void setFrameShape(Shape);
    Shadow frameShadow() const;
    void setFrameShadow(Shadow);

    int lineWidth() const;
    void setLineWidth(int);

    int midLineWidth() const;
    void setMidLineWidth(int);

    QRect frameRect() const;
    void setFrameRect(const QRect &);

protected:
    bool event(QEvent *e);
    void paintEvent(QPaintEvent *);
    void changeEvent(QEvent *);
    void drawFrame(QPainter *);

#ifdef QT3_SUPPORT
public:
    QT3_SUPPORT_CONSTRUCTOR QFrame(QWidget* parent, const char* name, Qt::WindowFlags f = 0);
#endif

protected:
    QFrame(QFramePrivate &dd, QWidget* parent = 0, Qt::WindowFlags f = 0);

private:
    Q_DISABLE_COPY(QFrame)
    Q_DECLARE_PRIVATE(QFrame)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QFRAME_H
