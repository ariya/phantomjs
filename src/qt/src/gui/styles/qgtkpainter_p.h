/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QGTKPAINTER_H
#define QGTKPAINTER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#if !defined(QT_NO_STYLE_GTK)

#include <QtGui/QCleanlooksStyle>
#include <QtGui/QPainter>
#include <QtGui/QPalette>
#include <QtGui/QFont>
#include <private/qgtkstyle_p.h>

QT_BEGIN_NAMESPACE

class QGtkPainter
{

public:
    QGtkPainter(QPainter *painter);
    GtkStyle *getStyle(GtkWidget *gtkWidget);
    GtkStateType gtkState(const QStyleOption *option);

    void setAlphaSupport(bool value) { m_alpha = value; }
    void setClipRect(const QRect &rect) { m_cliprect = rect; }
    void setFlipHorizontal(bool value) { m_hflipped = value; }
    void setFlipVertical(bool value) { m_vflipped = value; }
    void setUsePixmapCache(bool value) { m_usePixmapCache = value; }

    void paintBoxGap(GtkWidget *gtkWidget, const gchar* part, const QRect &rect,
                     GtkStateType state, GtkShadowType shadow, GtkPositionType gap_side, gint x,
                     gint width, GtkStyle *style);
    void paintBox(GtkWidget *gtkWidget, const gchar* part,
                  const QRect &rect, GtkStateType state, GtkShadowType shadow, GtkStyle *style,
                  const QString &pmKey = QString());
    void paintHline(GtkWidget *gtkWidget, const gchar* part, const QRect &rect, GtkStateType state, GtkStyle *style,
                    int x1, int x2, int y, const QString &pmKey = QString());
    void paintVline(GtkWidget *gtkWidget, const gchar* part, const QRect &rect, GtkStateType state, GtkStyle *style,
                    int y1, int y2, int x, const QString &pmKey = QString());
    void paintExpander(GtkWidget *gtkWidget, const gchar* part, const QRect &rect, GtkStateType state,
                       GtkExpanderStyle expander_state, GtkStyle *style, const QString &pmKey = QString());
    void paintFocus(GtkWidget *gtkWidget, const gchar* part, const QRect &rect, GtkStateType state, GtkStyle *style,
                    const QString &pmKey = QString());
    void paintResizeGrip(GtkWidget *gtkWidget, const gchar* part, const QRect &rect, GtkStateType state, GtkShadowType shadow,
                         GdkWindowEdge edge, GtkStyle *style, const QString &pmKey = QString());
    void paintArrow(GtkWidget *gtkWidget, const gchar* part, const QRect &arrowrect, GtkArrowType arrow_type, GtkStateType state, GtkShadowType shadow,
                    gboolean fill, GtkStyle *style, const QString &pmKey = QString());
    void paintHandle(GtkWidget *gtkWidget, const gchar* part, const QRect &rect,
                     GtkStateType state, GtkShadowType shadow, GtkOrientation orientation, GtkStyle *style);
    void paintSlider(GtkWidget *gtkWidget, const gchar* part, const QRect &rect, GtkStateType state, GtkShadowType shadow,
                     GtkStyle *style, GtkOrientation orientation, const QString &pmKey = QString());
    void paintShadow(GtkWidget *gtkWidget, const gchar* part, const QRect &rect, GtkStateType state, GtkShadowType shadow,
                     GtkStyle *style, const QString &pmKey = QString());
    void paintFlatBox(GtkWidget *gtkWidget, const gchar* part, const QRect &rect, GtkStateType state, GtkShadowType shadow, GtkStyle *style, const QString & = QString());
    void paintExtention(GtkWidget *gtkWidget, const gchar *part, const QRect &rect, GtkStateType state, GtkShadowType shadow,
                        GtkPositionType gap_pos, GtkStyle *style);
    void paintOption(GtkWidget *gtkWidget, const QRect &rect, GtkStateType state, GtkShadowType shadow, GtkStyle *style, const QString &detail);
    void paintCheckbox(GtkWidget *gtkWidget, const QRect &rect, GtkStateType state, GtkShadowType shadow, GtkStyle *style, const QString &detail);

    static QPixmap getIcon(const char* iconName, GtkIconSize size = GTK_ICON_SIZE_BUTTON);
private:
    QPixmap renderTheme(uchar *bdata, uchar *wdata, const QRect&);

    GtkWidget *m_window;
    QPainter *m_painter;
    bool m_alpha;
    bool m_hflipped;
    bool m_vflipped;
    bool m_usePixmapCache;
    QRect m_cliprect;

};

QT_END_NAMESPACE

#endif //!defined(QT_NO_STYLE_QGTK)

#endif // QGTKPAINTER_H
