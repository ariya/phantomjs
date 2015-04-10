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

#include <private/qgtkglobal_p.h>
#include <QtCore/qsize.h>
#include <QtCore/qrect.h>
#include <QtCore/qpoint.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qpainter.h>

QT_BEGIN_NAMESPACE

class QGtkPainter
{
public:
    QGtkPainter();
    virtual ~QGtkPainter();

    void reset(QPainter *painter = 0);

    void setAlphaSupport(bool value) { m_alpha = value; }
    void setClipRect(const QRect &rect) { m_cliprect = rect; }
    void setFlipHorizontal(bool value) { m_hflipped = value; }
    void setFlipVertical(bool value) { m_vflipped = value; }
    void setUsePixmapCache(bool value) { m_usePixmapCache = value; }

    virtual void paintBoxGap(GtkWidget *gtkWidget, const gchar* part, const QRect &rect,
                             GtkStateType state, GtkShadowType shadow, GtkPositionType gap_side, gint x,
                             gint width, GtkStyle *style) = 0;
    virtual void paintBox(GtkWidget *gtkWidget, const gchar* part,
                          const QRect &rect, GtkStateType state, GtkShadowType shadow, GtkStyle *style,
                          const QString &pmKey = QString()) = 0;
    virtual void paintHline(GtkWidget *gtkWidget, const gchar* part, const QRect &rect, GtkStateType state, GtkStyle *style,
                            int x1, int x2, int y, const QString &pmKey = QString()) = 0;
    virtual void paintVline(GtkWidget *gtkWidget, const gchar* part, const QRect &rect, GtkStateType state, GtkStyle *style,
                            int y1, int y2, int x, const QString &pmKey = QString()) = 0;
    virtual void paintExpander(GtkWidget *gtkWidget, const gchar* part, const QRect &rect, GtkStateType state,
                               GtkExpanderStyle expander_state, GtkStyle *style, const QString &pmKey = QString()) = 0;
    virtual void paintFocus(GtkWidget *gtkWidget, const gchar* part, const QRect &rect, GtkStateType state, GtkStyle *style,
                            const QString &pmKey = QString()) = 0;
    virtual void paintResizeGrip(GtkWidget *gtkWidget, const gchar* part, const QRect &rect, GtkStateType state, GtkShadowType shadow,
                                 GdkWindowEdge edge, GtkStyle *style, const QString &pmKey = QString()) = 0;
    virtual void paintArrow(GtkWidget *gtkWidget, const gchar* part, const QRect &arrowrect, GtkArrowType arrow_type, GtkStateType state, GtkShadowType shadow,
                            gboolean fill, GtkStyle *style, const QString &pmKey = QString()) = 0;
    virtual void paintHandle(GtkWidget *gtkWidget, const gchar* part, const QRect &rect,
                             GtkStateType state, GtkShadowType shadow, GtkOrientation orientation, GtkStyle *style) = 0;
    virtual void paintSlider(GtkWidget *gtkWidget, const gchar* part, const QRect &rect, GtkStateType state, GtkShadowType shadow,
                             GtkStyle *style, GtkOrientation orientation, const QString &pmKey = QString()) = 0;
    virtual void paintShadow(GtkWidget *gtkWidget, const gchar* part, const QRect &rect, GtkStateType state, GtkShadowType shadow,
                             GtkStyle *style, const QString &pmKey = QString()) = 0;
    virtual void paintFlatBox(GtkWidget *gtkWidget, const gchar* part, const QRect &rect, GtkStateType state, GtkShadowType shadow, GtkStyle *style, const QString & = QString()) = 0;
    virtual void paintExtention(GtkWidget *gtkWidget, const gchar *part, const QRect &rect, GtkStateType state, GtkShadowType shadow,
                                GtkPositionType gap_pos, GtkStyle *style) = 0;
    virtual void paintOption(GtkWidget *gtkWidget, const QRect &rect, GtkStateType state, GtkShadowType shadow, GtkStyle *style, const QString &detail) = 0;
    virtual void paintCheckbox(GtkWidget *gtkWidget, const QRect &rect, GtkStateType state, GtkShadowType shadow, GtkStyle *style, const QString &detail) = 0;

protected:
    static QString uniqueName(const QString &key, GtkStateType state, GtkShadowType shadow, const QSize &size, GtkWidget *widget = 0);

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
