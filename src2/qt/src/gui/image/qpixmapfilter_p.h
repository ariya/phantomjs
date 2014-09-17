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

#ifndef QPIXMAPFILTER_H
#define QPIXMAPFILTER_H

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

#include <QtCore/qnamespace.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qgraphicseffect.h>

#ifndef QT_NO_GRAPHICSEFFECT
QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QPainter;
class QPixmapData;

class QPixmapFilterPrivate;

class Q_GUI_EXPORT QPixmapFilter : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPixmapFilter)
public:
    virtual ~QPixmapFilter() = 0;

    enum FilterType {
        ConvolutionFilter,
        ColorizeFilter,
        DropShadowFilter,
        BlurFilter,

        UserFilter = 1024
    };

    FilterType type() const;

    virtual QRectF boundingRectFor(const QRectF &rect) const;

    virtual void draw(QPainter *painter, const QPointF &p, const QPixmap &src, const QRectF &srcRect = QRectF()) const = 0;

protected:
    QPixmapFilter(QPixmapFilterPrivate &d, FilterType type, QObject *parent);
    QPixmapFilter(FilterType type, QObject *parent);
};

class QPixmapConvolutionFilterPrivate;

class Q_GUI_EXPORT QPixmapConvolutionFilter : public QPixmapFilter
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPixmapConvolutionFilter)

public:
    QPixmapConvolutionFilter(QObject *parent = 0);
    ~QPixmapConvolutionFilter();

    void setConvolutionKernel(const qreal *matrix, int rows, int columns);

    QRectF boundingRectFor(const QRectF &rect) const;
    void draw(QPainter *painter, const QPointF &dest, const QPixmap &src, const QRectF &srcRect = QRectF()) const;

private:
    friend class QGLPixmapConvolutionFilter;
    friend class QVGPixmapConvolutionFilter;
    const qreal *convolutionKernel() const;
    int rows() const;
    int columns() const;
};

class QPixmapBlurFilterPrivate;

class Q_GUI_EXPORT QPixmapBlurFilter : public QPixmapFilter
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPixmapBlurFilter)

public:
    QPixmapBlurFilter(QObject *parent = 0);
    ~QPixmapBlurFilter();

    void setRadius(qreal radius);
    void setBlurHints(QGraphicsBlurEffect::BlurHints hints);

    qreal radius() const;
    QGraphicsBlurEffect::BlurHints blurHints() const;

    QRectF boundingRectFor(const QRectF &rect) const;
    void draw(QPainter *painter, const QPointF &dest, const QPixmap &src, const QRectF &srcRect = QRectF()) const;

private:
    friend class QGLPixmapBlurFilter;
};

class QPixmapColorizeFilterPrivate;

class Q_GUI_EXPORT QPixmapColorizeFilter : public QPixmapFilter
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPixmapColorizeFilter)

public:
    QPixmapColorizeFilter(QObject *parent = 0);

    void setColor(const QColor& color);
    QColor color() const;

    void setStrength(qreal strength);
    qreal strength() const;

    void draw(QPainter *painter, const QPointF &dest, const QPixmap &src, const QRectF &srcRect = QRectF()) const;
};

class QPixmapDropShadowFilterPrivate;

class Q_GUI_EXPORT QPixmapDropShadowFilter : public QPixmapFilter
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPixmapDropShadowFilter)

public:
    QPixmapDropShadowFilter(QObject *parent = 0);
    ~QPixmapDropShadowFilter();

    QRectF boundingRectFor(const QRectF &rect) const;
    void draw(QPainter *p, const QPointF &pos, const QPixmap &px, const QRectF &src = QRectF()) const;

    qreal blurRadius() const;
    void setBlurRadius(qreal radius);

    QColor color() const;
    void setColor(const QColor &color);

    QPointF offset() const;
    void setOffset(const QPointF &offset);
    inline void setOffset(qreal dx, qreal dy) { setOffset(QPointF(dx, dy)); }
};

QT_END_NAMESPACE

QT_END_HEADER

#endif //QT_NO_GRAPHICSEFFECT
#endif // QPIXMAPFILTER_H
