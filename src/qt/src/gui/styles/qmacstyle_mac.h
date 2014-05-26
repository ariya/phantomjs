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

#ifndef QMACSTYLE_MAC_H
#define QMACSTYLE_MAC_H

#include <QtGui/qwindowsstyle.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#if defined(Q_WS_MAC) && !defined(QT_NO_STYLE_MAC)

class QPalette;

#if defined(QT_PLUGIN)
#define Q_GUI_EXPORT_STYLE_MAC
#else
#define Q_GUI_EXPORT_STYLE_MAC Q_GUI_EXPORT
#endif

class QPushButton;
class QStyleOptionButton;
class QMacStylePrivate;
class Q_GUI_EXPORT_STYLE_MAC QMacStyle : public QWindowsStyle
{
    Q_OBJECT
public:
    QMacStyle();
    virtual ~QMacStyle();

    void polish(QWidget *w);
    void unpolish(QWidget *w);

    void polish(QApplication*);
    void unpolish(QApplication*);

    void polish(QPalette &pal);

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                       const QWidget *w = 0) const;
    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p,
                     const QWidget *w = 0) const;
    QRect subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget = 0) const;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
                            const QWidget *w = 0) const;
    SubControl hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                               const QPoint &pt, const QWidget *w = 0) const;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc,
                         const QWidget *w = 0) const;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt,
                           const QSize &contentsSize, const QWidget *w = 0) const;

    int pixelMetric(PixelMetric pm, const QStyleOption *opt = 0, const QWidget *widget = 0) const;

    QPalette standardPalette() const;

    virtual int styleHint(StyleHint sh, const QStyleOption *opt = 0, const QWidget *w = 0,
                          QStyleHintReturn *shret = 0) const;

    enum FocusRectPolicy { FocusEnabled, FocusDisabled, FocusDefault };
    static void setFocusRectPolicy(QWidget *w, FocusRectPolicy policy);
    static FocusRectPolicy focusRectPolicy(const QWidget *w);

    enum WidgetSizePolicy { SizeSmall, SizeLarge, SizeMini, SizeDefault
#ifdef QT3_SUPPORT
                            , SizeNone = SizeDefault
#endif
    };
    static void setWidgetSizePolicy(const QWidget *w, WidgetSizePolicy policy);
    static WidgetSizePolicy widgetSizePolicy(const QWidget *w);

    QPixmap standardPixmap(StandardPixmap sp, const QStyleOption *opt,
                           const QWidget *widget = 0) const;

    QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                const QStyleOption *opt) const;

    virtual void drawItemText(QPainter *p, const QRect &r, int flags, const QPalette &pal,
                              bool enabled, const QString &text, QPalette::ColorRole textRole  = QPalette::NoRole) const;

    bool event(QEvent *e);

protected Q_SLOTS:
    QIcon standardIconImplementation(StandardPixmap standardIcon, const QStyleOption *opt = 0,
                                     const QWidget *widget = 0) const;
    int layoutSpacingImplementation(QSizePolicy::ControlType control1,
                                    QSizePolicy::ControlType control2, 
                                    Qt::Orientation orientation,
                                    const QStyleOption *option = 0, 
                                    const QWidget *widget = 0) const;

private:
    Q_DISABLE_COPY(QMacStyle)

    QMacStylePrivate *d;

    friend bool qt_mac_buttonIsRenderedFlat(const QPushButton *pushButton, const QStyleOptionButton *option);
};

#endif // Q_WS_MAC

QT_END_NAMESPACE

QT_END_HEADER

#endif // QMACSTYLE_H
