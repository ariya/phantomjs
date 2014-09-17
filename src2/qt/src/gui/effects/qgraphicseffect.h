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

#ifndef QGRAPHICSEFFECT_H
#define QGRAPHICSEFFECT_H

#include <QtCore/qobject.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtGui/qcolor.h>
#include <QtGui/qbrush.h>

#ifndef QT_NO_GRAPHICSEFFECT
QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QGraphicsItem;
class QStyleOption;
class QPainter;
class QPixmap;

class QGraphicsEffectSource;

class QGraphicsEffectPrivate;
class Q_GUI_EXPORT QGraphicsEffect : public QObject
{
    Q_OBJECT
    Q_FLAGS(ChangeFlags)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
public:
    enum ChangeFlag {
        SourceAttached = 0x1,
        SourceDetached = 0x2,
        SourceBoundingRectChanged = 0x4,
        SourceInvalidated = 0x8
    };
    Q_DECLARE_FLAGS(ChangeFlags, ChangeFlag)

    enum PixmapPadMode {
        NoPad,
        PadToTransparentBorder,
        PadToEffectiveBoundingRect
    };

    QGraphicsEffect(QObject *parent = 0);
    virtual ~QGraphicsEffect();

    virtual QRectF boundingRectFor(const QRectF &sourceRect) const;
    QRectF boundingRect() const;

    bool isEnabled() const;

public Q_SLOTS:
    void setEnabled(bool enable);
    void update();

Q_SIGNALS:
    void enabledChanged(bool enabled);

protected:
    QGraphicsEffect(QGraphicsEffectPrivate &d, QObject *parent = 0);
    virtual void draw(QPainter *painter) = 0;
    virtual void sourceChanged(ChangeFlags flags);
    void updateBoundingRect();

    bool sourceIsPixmap() const;
    QRectF sourceBoundingRect(Qt::CoordinateSystem system = Qt::LogicalCoordinates) const;
    void drawSource(QPainter *painter);
    QPixmap sourcePixmap(Qt::CoordinateSystem system = Qt::LogicalCoordinates,
                         QPoint *offset = 0,
                         PixmapPadMode mode = PadToEffectiveBoundingRect) const;

private:
    Q_DECLARE_PRIVATE(QGraphicsEffect)
    Q_DISABLE_COPY(QGraphicsEffect)
    friend class QGraphicsItem;
    friend class QGraphicsItemPrivate;
    friend class QGraphicsScenePrivate;
    friend class QWidget;
    friend class QWidgetPrivate;

public:
    QGraphicsEffectSource *source() const; // internal

};
Q_DECLARE_OPERATORS_FOR_FLAGS(QGraphicsEffect::ChangeFlags)

class QGraphicsColorizeEffectPrivate;
class Q_GUI_EXPORT QGraphicsColorizeEffect: public QGraphicsEffect
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qreal strength READ strength WRITE setStrength NOTIFY strengthChanged)
public:
    QGraphicsColorizeEffect(QObject *parent = 0);
    ~QGraphicsColorizeEffect();

    QColor color() const;
    qreal strength() const;

public Q_SLOTS:
    void setColor(const QColor &c);
    void setStrength(qreal strength);

Q_SIGNALS:
    void colorChanged(const QColor &color);
    void strengthChanged(qreal strength);

protected:
    void draw(QPainter *painter);

private:
    Q_DECLARE_PRIVATE(QGraphicsColorizeEffect)
    Q_DISABLE_COPY(QGraphicsColorizeEffect)
};

class QGraphicsBlurEffectPrivate;
class Q_GUI_EXPORT QGraphicsBlurEffect: public QGraphicsEffect
{
    Q_OBJECT
    Q_FLAGS(BlurHint BlurHints)
    Q_PROPERTY(qreal blurRadius READ blurRadius WRITE setBlurRadius NOTIFY blurRadiusChanged)
    Q_PROPERTY(BlurHints blurHints READ blurHints WRITE setBlurHints NOTIFY blurHintsChanged)
public:
    enum BlurHint {
        PerformanceHint = 0x00,
        QualityHint = 0x01,
        AnimationHint = 0x02
    };
    Q_DECLARE_FLAGS(BlurHints, BlurHint)

    QGraphicsBlurEffect(QObject *parent = 0);
    ~QGraphicsBlurEffect();

    QRectF boundingRectFor(const QRectF &rect) const;
    qreal blurRadius() const;
    BlurHints blurHints() const;

public Q_SLOTS:
    void setBlurRadius(qreal blurRadius);
    void setBlurHints(BlurHints hints);

Q_SIGNALS:
    void blurRadiusChanged(qreal blurRadius);
    void blurHintsChanged(BlurHints hints);

protected:
    void draw(QPainter *painter);

private:
    Q_DECLARE_PRIVATE(QGraphicsBlurEffect)
    Q_DISABLE_COPY(QGraphicsBlurEffect)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QGraphicsBlurEffect::BlurHints)

class QGraphicsDropShadowEffectPrivate;
class Q_GUI_EXPORT QGraphicsDropShadowEffect: public QGraphicsEffect
{
    Q_OBJECT
    Q_PROPERTY(QPointF offset READ offset WRITE setOffset NOTIFY offsetChanged)
    Q_PROPERTY(qreal xOffset READ xOffset WRITE setXOffset NOTIFY offsetChanged)
    Q_PROPERTY(qreal yOffset READ yOffset WRITE setYOffset NOTIFY offsetChanged)
    Q_PROPERTY(qreal blurRadius READ blurRadius WRITE setBlurRadius NOTIFY blurRadiusChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
public:
    QGraphicsDropShadowEffect(QObject *parent = 0);
    ~QGraphicsDropShadowEffect();

    QRectF boundingRectFor(const QRectF &rect) const;
    QPointF offset() const;

    inline qreal xOffset() const
    { return offset().x(); }

    inline qreal yOffset() const
    { return offset().y(); }

    qreal blurRadius() const;
    QColor color() const;

public Q_SLOTS:
    void setOffset(const QPointF &ofs);

    inline void setOffset(qreal dx, qreal dy)
    { setOffset(QPointF(dx, dy)); }

    inline void setOffset(qreal d)
    { setOffset(QPointF(d, d)); }

    inline void setXOffset(qreal dx)
    { setOffset(QPointF(dx, yOffset())); }

    inline void setYOffset(qreal dy)
    { setOffset(QPointF(xOffset(), dy)); }

    void setBlurRadius(qreal blurRadius);
    void setColor(const QColor &color);

Q_SIGNALS:
    void offsetChanged(const QPointF &offset);
    void blurRadiusChanged(qreal blurRadius);
    void colorChanged(const QColor &color);

protected:
    void draw(QPainter *painter);

private:
    Q_DECLARE_PRIVATE(QGraphicsDropShadowEffect)
    Q_DISABLE_COPY(QGraphicsDropShadowEffect)
};

class QGraphicsOpacityEffectPrivate;
class Q_GUI_EXPORT QGraphicsOpacityEffect: public QGraphicsEffect
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)
    Q_PROPERTY(QBrush opacityMask READ opacityMask WRITE setOpacityMask NOTIFY opacityMaskChanged)
public:
    QGraphicsOpacityEffect(QObject *parent = 0);
    ~QGraphicsOpacityEffect();

    qreal opacity() const;
    QBrush opacityMask() const;

public Q_SLOTS:
    void setOpacity(qreal opacity);
    void setOpacityMask(const QBrush &mask);

Q_SIGNALS:
    void opacityChanged(qreal opacity);
    void opacityMaskChanged(const QBrush &mask);

protected:
    void draw(QPainter *painter);

private:
    Q_DECLARE_PRIVATE(QGraphicsOpacityEffect)
    Q_DISABLE_COPY(QGraphicsOpacityEffect)
};

QT_END_NAMESPACE

QT_END_HEADER
#endif //QT_NO_GRAPHICSEFFECT

#endif // QGRAPHICSEFFECT_H

