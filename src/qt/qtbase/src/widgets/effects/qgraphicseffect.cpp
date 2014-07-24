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

/*!
    \class QGraphicsEffect
    \brief The QGraphicsEffect class is the base class for all graphics
           effects.
    \since 4.6
    \ingroup multimedia
    \ingroup graphicsview-api
    \inmodule QtWidgets

    Effects alter the appearance of elements by hooking into the rendering
    pipeline and operating between the source (e.g., a QGraphicsPixmapItem)
    and the destination device (e.g., QGraphicsView's viewport). Effects can be
    disabled by calling setEnabled(false). If effects are disabled, the source
    is rendered directly.

    To add a visual effect to a QGraphicsItem, for example, you can use one of
    the standard effects, or alternately, create your own effect by creating a
    subclass of QGraphicsEffect. The effect can then be installed on the item
    using QGraphicsItem::setGraphicsEffect().

    Qt provides the following standard effects:

    \list
    \li QGraphicsBlurEffect - blurs the item by a given radius
    \li QGraphicsDropShadowEffect - renders a dropshadow behind the item
    \li QGraphicsColorizeEffect - renders the item in shades of any given color
    \li QGraphicsOpacityEffect - renders the item with an opacity
    \endlist

    \table
    \row
    \li{2,1} \image graphicseffect-plain.png
    \row
    \li \image graphicseffect-blur.png
    \li \image graphicseffect-colorize.png
    \row
    \li \image graphicseffect-opacity.png
    \li \image graphicseffect-drop-shadow.png
    \endtable

    \image graphicseffect-widget.png

    For more information on how to use each effect, refer to the specific
    effect's documentation.

    To create your own custom effect, create a subclass of QGraphicsEffect (or
    any other existing effects) and reimplement the virtual function draw().
    This function is called whenever the effect needs to redraw. The draw()
    function takes the painter with which to draw as an argument. For more
    information, refer to the documenation for draw(). In the draw() function
    you can call sourcePixmap() to get a pixmap of the graphics effect source
    which you can then process.

    If your effect changes, use update() to request for a redraw. If your
    custom effect changes the bounding rectangle of the source, e.g., a radial
    glow effect may need to apply an extra margin, you can reimplement the
    virtual boundingRectFor() function, and call updateBoundingRect()
    to notify the framework whenever this rectangle changes. The virtual
    sourceChanged() function is called to notify the effects that
    the source has changed in some way - e.g., if the source is a
    QGraphicsRectItem and its rectangle parameters have changed.

    \sa QGraphicsItem::setGraphicsEffect(), QWidget::setGraphicsEffect()
*/

#include "qgraphicseffect_p.h"
#include "private/qgraphicsitem_p.h"

#include <QtWidgets/qgraphicsitem.h>

#include <QtGui/qimage.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpaintengine.h>
#include <QtCore/qrect.h>
#include <QtCore/qdebug.h>
#include <private/qdrawhelper_p.h>

#ifndef QT_NO_GRAPHICSEFFECT
QT_BEGIN_NAMESPACE

/*!
    \internal
    \class QGraphicsEffectSource
    \brief The QGraphicsEffectSource class represents the source on which a
           QGraphicsEffect is installed on.

    When a QGraphicsEffect is installed on a QGraphicsItem, for example, this
    class will act as a wrapper around QGraphicsItem. Then, calling update() is
    effectively the same as calling QGraphicsItem::update().

    QGraphicsEffectSource also provides a pixmap() function which creates a
    pixmap with the source painted into it.

    \sa QGraphicsItem::setGraphicsEffect(), QWidget::setGraphicsEffect()
*/

/*!
    \internal
*/
QGraphicsEffectSource::QGraphicsEffectSource(QGraphicsEffectSourcePrivate &dd, QObject *parent)
    : QObject(dd, parent)
{}

/*!
    Destroys the effect source.
*/
QGraphicsEffectSource::~QGraphicsEffectSource()
{}

/*!
    Returns the bounding rectangle of the source mapped to the given \a system.

    \sa draw()
*/
QRectF QGraphicsEffectSource::boundingRect(Qt::CoordinateSystem system) const
{
    return d_func()->boundingRect(system);
}

/*!
    Returns the bounding rectangle of the source mapped to the given \a system.

    Calling this function with Qt::DeviceCoordinates outside of
    QGraphicsEffect::draw() will give undefined results, as there is no device
    context available.

    \sa draw()
*/
QRectF QGraphicsEffect::sourceBoundingRect(Qt::CoordinateSystem system) const
{
    Q_D(const QGraphicsEffect);
    if (d->source)
        return d->source->boundingRect(system);
    return QRectF();
}

/*!
    Returns a pointer to the item if this source is a QGraphicsItem; otherwise
    returns 0.

    \sa widget()
*/
const QGraphicsItem *QGraphicsEffectSource::graphicsItem() const
{
    return d_func()->graphicsItem();
}

/*!
    Returns a pointer to the widget if this source is a QWidget; otherwise
    returns 0.

    \sa graphicsItem()
*/
const QWidget *QGraphicsEffectSource::widget() const
{
    return d_func()->widget();
}

/*!
    Returns a pointer to the style options (used when drawing the source) if
    available; otherwise returns 0.

    \sa graphicsItem(), widget()
*/
const QStyleOption *QGraphicsEffectSource::styleOption() const
{
    return d_func()->styleOption();
}

/*!
    Draws the source using the given \a painter.

    This function should only be called from QGraphicsEffect::draw().

    \sa QGraphicsEffect::draw()
*/
void QGraphicsEffectSource::draw(QPainter *painter)
{
    Q_D(const QGraphicsEffectSource);

    QPixmap pm;
    if (QPixmapCache::find(d->m_cacheKey, &pm)) {
        QTransform restoreTransform;
        if (d->m_cachedSystem == Qt::DeviceCoordinates) {
            restoreTransform = painter->worldTransform();
            painter->setWorldTransform(QTransform());
        }

        painter->drawPixmap(d->m_cachedOffset, pm);

        if (d->m_cachedSystem == Qt::DeviceCoordinates)
            painter->setWorldTransform(restoreTransform);
    } else {
        d_func()->draw(painter);
    }
}

/*!
    Draws the source directly using the given \a painter.

    This function should only be called from QGraphicsEffect::draw().

    For example:

    \snippet code/src_gui_effects_qgraphicseffect.cpp 0

    \sa QGraphicsEffect::draw()
*/
void QGraphicsEffect::drawSource(QPainter *painter)
{
    Q_D(const QGraphicsEffect);
    if (d->source)
        d->source->draw(painter);
}

/*!
    Schedules a redraw of the source. Call this function whenever the source
    needs to be redrawn.

    \sa QGraphicsEffect::updateBoundingRect(), QWidget::update(),
        QGraphicsItem::update(),
*/
void QGraphicsEffectSource::update()
{
    d_func()->update();
}

/*!
    Returns \c true if the source effectively is a pixmap, e.g., a
    QGraphicsPixmapItem.

    This function is useful for optimization purposes. For instance, there's no
    point in drawing the source in device coordinates to avoid pixmap scaling
    if this function returns \c true - the source pixmap will be scaled anyways.
*/
bool QGraphicsEffectSource::isPixmap() const
{
    return d_func()->isPixmap();
}

/*!
    Returns \c true if the source effectively is a pixmap, e.g., a
    QGraphicsPixmapItem.

    This function is useful for optimization purposes. For instance, there's no
    point in drawing the source in device coordinates to avoid pixmap scaling
    if this function returns \c true - the source pixmap will be scaled anyways.
*/
bool QGraphicsEffect::sourceIsPixmap() const
{
    return source() ? source()->isPixmap() : false;
}

/*!
    Returns a pixmap with the source painted into it.

    The \a system specifies which coordinate system to be used for the source.
    The optional \a offset parameter returns the offset where the pixmap should
    be painted at using the current painter.

    The \a mode determines how much of the effect the pixmap will contain.
    By default, the pixmap will contain the whole effect.

    The returned pixmap is bound to the current painter's device rectangle when
    \a system is Qt::DeviceCoordinates.

    \sa QGraphicsEffect::draw(), boundingRect()
*/
QPixmap QGraphicsEffectSource::pixmap(Qt::CoordinateSystem system, QPoint *offset, QGraphicsEffect::PixmapPadMode mode) const
{
    Q_D(const QGraphicsEffectSource);

    // Shortcut, no cache for childless pixmap items...
    const QGraphicsItem *item = graphicsItem();
    if (system == Qt::LogicalCoordinates && mode == QGraphicsEffect::NoPad && item && isPixmap()) {
        const QGraphicsPixmapItem *pixmapItem = static_cast<const QGraphicsPixmapItem *>(item);
        if (offset)
            *offset = pixmapItem->offset().toPoint();
        return pixmapItem->pixmap();
    }

    if (system == Qt::DeviceCoordinates && item
        && !static_cast<const QGraphicsItemEffectSourcePrivate *>(d_func())->info) {
        qWarning("QGraphicsEffectSource::pixmap: Not yet implemented, lacking device context");
        return QPixmap();
    }

    QPixmap pm;
    if (item && d->m_cachedSystem == system && d->m_cachedMode == mode)
        QPixmapCache::find(d->m_cacheKey, &pm);

    if (pm.isNull()) {
        pm = d->pixmap(system, &d->m_cachedOffset, mode);
        d->m_cachedSystem = system;
        d->m_cachedMode = mode;

        d->invalidateCache();
        d->m_cacheKey = QPixmapCache::insert(pm);
    }

    if (offset)
        *offset = d->m_cachedOffset;

    return pm;
}

/*!
    Returns a pixmap with the source painted into it.

    The \a system specifies which coordinate system to be used for the source.
    The optional \a offset parameter returns the offset where the pixmap should
    be painted at using the current painter. For control on how the pixmap is
    padded use the \a mode parameter.

    The returned pixmap is clipped to the current painter's device rectangle when
    \a system is Qt::DeviceCoordinates.

    Calling this function with Qt::DeviceCoordinates outside of
    QGraphicsEffect::draw() will give undefined results, as there is no device
    context available.

    \sa draw(), boundingRect()
*/
QPixmap QGraphicsEffect::sourcePixmap(Qt::CoordinateSystem system, QPoint *offset, QGraphicsEffect::PixmapPadMode mode) const
{
    Q_D(const QGraphicsEffect);
    if (d->source)
        return d->source->pixmap(system, offset, mode);
    return QPixmap();
}

QGraphicsEffectSourcePrivate::~QGraphicsEffectSourcePrivate()
{
    invalidateCache();
}

void QGraphicsEffectSourcePrivate::setCachedOffset(const QPoint &offset)
{
    m_cachedOffset = offset;
}

void QGraphicsEffectSourcePrivate::invalidateCache(InvalidateReason reason) const
{
    if (m_cachedMode != QGraphicsEffect::PadToEffectiveBoundingRect
        && (reason == EffectRectChanged
            || (reason == TransformChanged && m_cachedSystem == Qt::LogicalCoordinates))) {
        return;
    }

    QPixmapCache::remove(m_cacheKey);
}

/*!
    Constructs a new QGraphicsEffect instance having the
    specified \a parent.
*/
QGraphicsEffect::QGraphicsEffect(QObject *parent)
    : QObject(*new QGraphicsEffectPrivate, parent)
{
}

/*!
    \internal
*/
QGraphicsEffect::QGraphicsEffect(QGraphicsEffectPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}

/*!
    Removes the effect from the source, and destroys the graphics effect.
*/
QGraphicsEffect::~QGraphicsEffect()
{
    Q_D(QGraphicsEffect);
    d->setGraphicsEffectSource(0);
}

/*!
    Returns the effective bounding rectangle for this effect, i.e., the
    bounding rectangle of the source in device coordinates, adjusted by
    any margins applied by the effect itself.

    \sa boundingRectFor(), updateBoundingRect()
*/
QRectF QGraphicsEffect::boundingRect() const
{
    Q_D(const QGraphicsEffect);
    if (d->source)
        return boundingRectFor(d->source->boundingRect());
    return QRectF();
}

/*!
    Returns the effective bounding rectangle for this effect, given the
    provided \a rect in the device coordinates. When writing
    you own custom effect, you must call updateBoundingRect() whenever any
    parameters are changed that may cause this this function to return a
    different value.

    \sa sourceBoundingRect()
*/
QRectF QGraphicsEffect::boundingRectFor(const QRectF &rect) const
{
    return rect;
}

/*!
    \property QGraphicsEffect::enabled
    \brief whether the effect is enabled or not.

    If an effect is disabled, the source will be rendered with as normal, with
    no interference from the effect. If the effect is enabled, the source will
    be rendered with the effect applied.

    This property is enabled by default.

    Using this property, you can disable certain effects on slow platforms, in
    order to ensure that the user interface is responsive.
*/
bool QGraphicsEffect::isEnabled() const
{
    Q_D(const QGraphicsEffect);
    return d->isEnabled;
}

void QGraphicsEffect::setEnabled(bool enable)
{
    Q_D(QGraphicsEffect);
    if (d->isEnabled == enable)
        return;

    d->isEnabled = enable;
    if (d->source) {
        d->source->d_func()->effectBoundingRectChanged();
        d->source->d_func()->invalidateCache();
    }
    emit enabledChanged(enable);
}

/*!
    \fn void QGraphicsEffect::enabledChanged(bool enabled)

    This signal is emitted whenever the effect is enabled or disabled.
    The \a enabled parameter holds the effects's new enabled state.

    \sa isEnabled()
*/

/*!
    Schedules a redraw of the effect. Call this function whenever the effect
    needs to be redrawn. This function does not trigger a redraw of the source.

    \sa updateBoundingRect()
*/
void QGraphicsEffect::update()
{
    Q_D(QGraphicsEffect);
    if (d->source)
        d->source->update();
}

/*!
    \internal

    Returns a pointer to the source, which provides extra context information
    that can be useful for the effect.

    \sa draw()
*/
QGraphicsEffectSource *QGraphicsEffect::source() const
{
    Q_D(const QGraphicsEffect);
    return d->source;
}

/*!
    This function notifies the effect framework when the effect's bounding
    rectangle has changed. As a custom effect author, you must call this
    function whenever you change any parameters that will cause the virtual
    boundingRectFor() function to return a different value.

    This function will call update() if this is necessary.

    \sa boundingRectFor(), boundingRect(), sourceBoundingRect()
*/
void QGraphicsEffect::updateBoundingRect()
{
    Q_D(QGraphicsEffect);
    if (d->source) {
        d->source->d_func()->effectBoundingRectChanged();
        d->source->d_func()->invalidateCache(QGraphicsEffectSourcePrivate::EffectRectChanged);
    }
}

/*!
    \fn virtual void QGraphicsEffect::draw(QPainter *painter) = 0

    This pure virtual function draws the effect and is called whenever the
    source needs to be drawn.

    Reimplement this function in a QGraphicsEffect subclass to provide the
    effect's drawing implementation, using \a painter.

    For example:

    \snippet code/src_gui_effects_qgraphicseffect.cpp 1

    This function should not be called explicitly by the user, since it is
    meant for reimplementation purposes only.
*/

/*!
    \enum QGraphicsEffect::ChangeFlag

    This enum describes what has changed in QGraphicsEffectSource.

    \value SourceAttached The effect is installed on a source.
    \value SourceDetached The effect is uninstalled on a source.
    \value SourceBoundingRectChanged The bounding rect of the source has
           changed.
    \value SourceInvalidated The visual appearance of the source has changed.
*/

/*!
    \enum QGraphicsEffect::PixmapPadMode

    This enum describes how the pixmap returned from sourcePixmap should be
    padded.

    \value NoPad The pixmap should not receive any additional
           padding.
    \value PadToTransparentBorder The pixmap should be padded
           to ensure it has a completely transparent border.
    \value PadToEffectiveBoundingRect The pixmap should be padded to
           match the effective bounding rectangle of the effect.
*/

/*!
    This virtual function is called by QGraphicsEffect to notify the effect
    that the source has changed. If the effect applies any cache, then this
    cache must be purged in order to reflect the new appearance of the source.

    The \a flags describes what has changed.
*/
void QGraphicsEffect::sourceChanged(ChangeFlags flags)
{
    Q_UNUSED(flags);
}

/*!
    \class QGraphicsColorizeEffect
    \brief The QGraphicsColorizeEffect class provides a colorize effect.
    \since 4.6
    \inmodule QtWidgets

    A colorize effect renders the source with a tint of its color(). The color
    can be modified using the setColor() function.

    By default, the color is light blue (QColor(0, 0, 192)).

    \image graphicseffect-colorize.png

    \sa QGraphicsDropShadowEffect, QGraphicsBlurEffect, QGraphicsOpacityEffect
*/

/*!
    Constructs a new QGraphicsColorizeEffect instance.
    The \a parent parameter is passed to QGraphicsEffect's constructor.
*/
QGraphicsColorizeEffect::QGraphicsColorizeEffect(QObject *parent)
    : QGraphicsEffect(*new QGraphicsColorizeEffectPrivate, parent)
{
}

/*!
    Destroys the effect.
*/
QGraphicsColorizeEffect::~QGraphicsColorizeEffect()
{
}

/*!
    \property QGraphicsColorizeEffect::color
    \brief the color of the effect.

    By default, the color is light blue (QColor(0, 0, 192)).
*/
QColor QGraphicsColorizeEffect::color() const
{
    Q_D(const QGraphicsColorizeEffect);
    return d->filter->color();
}

void QGraphicsColorizeEffect::setColor(const QColor &color)
{
    Q_D(QGraphicsColorizeEffect);
    if (d->filter->color() == color)
        return;

    d->filter->setColor(color);
    update();
    emit colorChanged(color);
}

/*!
    \property QGraphicsColorizeEffect::strength
    \brief the strength of the effect.

    By default, the strength is 1.0.
    A strength 0.0 equals to no effect, while 1.0 means full colorization.
*/
qreal QGraphicsColorizeEffect::strength() const
{
    Q_D(const QGraphicsColorizeEffect);
    return d->filter->strength();
}

void QGraphicsColorizeEffect::setStrength(qreal strength)
{
    Q_D(QGraphicsColorizeEffect);
    if (qFuzzyCompare(d->filter->strength(), strength))
        return;

    d->filter->setStrength(strength);
    d->opaque = !qFuzzyIsNull(strength);
    update();
    emit strengthChanged(strength);
}

/*! \fn void QGraphicsColorizeEffect::strengthChanged(qreal strength)
  This signal is emitted whenever setStrength() changes the colorize
  strength property. \a strength contains the new strength value of
  the colorize effect.
 */

/*!
    \fn void QGraphicsColorizeEffect::colorChanged(const QColor &color)

    This signal is emitted whenever the effect's color changes.
    The \a color parameter holds the effect's new color.
*/

/*!
    \reimp
*/
void QGraphicsColorizeEffect::draw(QPainter *painter)
{
    Q_D(QGraphicsColorizeEffect);

    if (!d->opaque) {
        drawSource(painter);
        return;
    }

    QPoint offset;
    if (sourceIsPixmap()) {
        // No point in drawing in device coordinates (pixmap will be scaled anyways).
        const QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset, NoPad);
        if (!pixmap.isNull())
            d->filter->draw(painter, offset, pixmap);

        return;
    }

    // Draw pixmap in deviceCoordinates to avoid pixmap scaling.
    const QPixmap pixmap = sourcePixmap(Qt::DeviceCoordinates, &offset);
    if (pixmap.isNull())
        return;

    QTransform restoreTransform = painter->worldTransform();
    painter->setWorldTransform(QTransform());
    d->filter->draw(painter, offset, pixmap);
    painter->setWorldTransform(restoreTransform);
}

/*!
    \class QGraphicsBlurEffect
    \brief The QGraphicsBlurEffect class provides a blur effect.
    \since 4.6
    \inmodule QtWidgets

    A blur effect blurs the source. This effect is useful for reducing details,
    such as when the source loses focus and you want to draw attention to other
    elements. The level of detail can be modified using the setBlurRadius()
    function. Use setBlurHints() to choose the blur hints.

    By default, the blur radius is 5 pixels. The blur radius is specified in
    device coordinates.

    \image graphicseffect-blur.png

    \sa QGraphicsDropShadowEffect, QGraphicsColorizeEffect, QGraphicsOpacityEffect
*/

/*!
    \enum QGraphicsBlurEffect::BlurHint
    \since 4.6

    This enum describes the possible hints that can be used to control how
    blur effects are applied. The hints might not have an effect in all the
    paint engines.

    \value PerformanceHint Indicates that rendering performance is the most important factor,
    at the potential cost of lower quality.

    \value QualityHint Indicates that rendering quality is the most important factor,
    at the potential cost of lower performance.

    \value AnimationHint Indicates that the blur radius is going to be animated, hinting
    that the implementation can keep a cache of blurred verisons of the source.
    Do not use this hint if the source is going to be dynamically changing.

    \sa blurHints(), setBlurHints()
*/


/*!
    Constructs a new QGraphicsBlurEffect instance.
    The \a parent parameter is passed to QGraphicsEffect's constructor.
*/
QGraphicsBlurEffect::QGraphicsBlurEffect(QObject *parent)
    : QGraphicsEffect(*new QGraphicsBlurEffectPrivate, parent)
{
    Q_D(QGraphicsBlurEffect);
    d->filter->setBlurHints(QGraphicsBlurEffect::PerformanceHint);
}

/*!
    Destroys the effect.
*/
QGraphicsBlurEffect::~QGraphicsBlurEffect()
{
}

/*!
    \property QGraphicsBlurEffect::blurRadius
    \brief the blur radius of the effect.

    Using a smaller radius results in a sharper appearance, whereas a bigger
    radius results in a more blurred appearance.

    By default, the blur radius is 5 pixels.

    The radius is given in device coordinates, meaning it is
    unaffected by scale.
*/
qreal QGraphicsBlurEffect::blurRadius() const
{
    Q_D(const QGraphicsBlurEffect);
    return d->filter->radius();
}

void QGraphicsBlurEffect::setBlurRadius(qreal radius)
{
    Q_D(QGraphicsBlurEffect);
    if (qFuzzyCompare(d->filter->radius(), radius))
        return;

    d->filter->setRadius(radius);
    updateBoundingRect();
    emit blurRadiusChanged(radius);
}

/*!
    \fn void QGraphicsBlurEffect::blurRadiusChanged(qreal radius)

    This signal is emitted whenever the effect's blur radius changes.
    The \a radius parameter holds the effect's new blur radius.
*/

/*!
    \property QGraphicsBlurEffect::blurHints
    \brief the blur hint of the effect.

    Use the PerformanceHint hint to say that you want a faster blur,
    the QualityHint hint to say that you prefer a higher quality blur,
    or the AnimationHint when you want to animate the blur radius.

    By default, the blur hint is PerformanceHint.
*/
QGraphicsBlurEffect::BlurHints QGraphicsBlurEffect::blurHints() const
{
    Q_D(const QGraphicsBlurEffect);
    return d->filter->blurHints();
}

void QGraphicsBlurEffect::setBlurHints(QGraphicsBlurEffect::BlurHints hints)
{
    Q_D(QGraphicsBlurEffect);
    if (d->filter->blurHints() == hints)
        return;

    d->filter->setBlurHints(hints);
    emit blurHintsChanged(hints);
}

/*!
    \fn void QGraphicsBlurEffect::blurHintsChanged(QGraphicsBlurEffect::BlurHints hints)

    This signal is emitted whenever the effect's blur hints changes.
    The \a hints parameter holds the effect's new blur hints.
*/

/*!
    \reimp
*/
QRectF QGraphicsBlurEffect::boundingRectFor(const QRectF &rect) const
{
    Q_D(const QGraphicsBlurEffect);
    return d->filter->boundingRectFor(rect);
}

/*!
    \reimp
*/
void QGraphicsBlurEffect::draw(QPainter *painter)
{
    Q_D(QGraphicsBlurEffect);
    if (d->filter->radius() < 1) {
        drawSource(painter);
        return;
    }

    PixmapPadMode mode = PadToEffectiveBoundingRect;

    QPoint offset;
    QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset, mode);
    if (pixmap.isNull())
        return;

    d->filter->draw(painter, offset, pixmap);
}

/*!
    \class QGraphicsDropShadowEffect
    \brief The QGraphicsDropShadowEffect class provides a drop shadow effect.
    \since 4.6
    \inmodule QtWidgets

    A drop shadow effect renders the source with a drop shadow. The color of
    the drop shadow can be modified using the setColor() function. The drop
    shadow offset can be modified using the setOffset() function and the blur
    radius of the drop shadow can be changed with the setBlurRadius()
    function.

    By default, the drop shadow is a semi-transparent dark gray
    (QColor(63, 63, 63, 180)) shadow, blurred with a radius of 1 at an offset
    of 8 pixels towards the lower right. The drop shadow offset is specified
    in device coordinates.

    \image graphicseffect-drop-shadow.png

    \sa QGraphicsBlurEffect, QGraphicsColorizeEffect, QGraphicsOpacityEffect
*/

/*!
    Constructs a new QGraphicsDropShadowEffect instance.
    The \a parent parameter is passed to QGraphicsEffect's constructor.
*/
QGraphicsDropShadowEffect::QGraphicsDropShadowEffect(QObject *parent)
    : QGraphicsEffect(*new QGraphicsDropShadowEffectPrivate, parent)
{
}

/*!
    Destroys the effect.
*/
QGraphicsDropShadowEffect::~QGraphicsDropShadowEffect()
{
}

/*!
    \property QGraphicsDropShadowEffect::offset
    \brief the shadow offset in pixels.

    By default, the offset is 8 pixels towards the lower right.

    The offset is given in device coordinates, which means it is
    unaffected by scale.

    \sa xOffset(), yOffset(), blurRadius(), color()
*/
QPointF QGraphicsDropShadowEffect::offset() const
{
    Q_D(const QGraphicsDropShadowEffect);
    return d->filter->offset();
}

void QGraphicsDropShadowEffect::setOffset(const QPointF &offset)
{
    Q_D(QGraphicsDropShadowEffect);
    if (d->filter->offset() == offset)
        return;

    d->filter->setOffset(offset);
    updateBoundingRect();
    emit offsetChanged(offset);
}

/*!
    \property QGraphicsDropShadowEffect::xOffset
    \brief the horizontal shadow offset in pixels.

    By default, the horizontal shadow offset is 8 pixels.



    \sa yOffset(), offset()
*/

/*!
    \property QGraphicsDropShadowEffect::yOffset
    \brief the vertical shadow offset in pixels.

    By default, the vertical shadow offset is 8 pixels.

    \sa xOffset(), offset()
*/

/*!
    \fn void QGraphicsDropShadowEffect::offsetChanged(const QPointF &offset)

    This signal is emitted whenever the effect's shadow offset changes.
    The \a offset parameter holds the effect's new shadow offset.
*/

/*!
    \property QGraphicsDropShadowEffect::blurRadius
    \brief the blur radius in pixels of the drop shadow.

    Using a smaller radius results in a sharper shadow, whereas using a bigger
    radius results in a more blurred shadow.

    By default, the blur radius is 1 pixel.

    \sa color(), offset()
*/
qreal QGraphicsDropShadowEffect::blurRadius() const
{
    Q_D(const QGraphicsDropShadowEffect);
    return d->filter->blurRadius();
}

void QGraphicsDropShadowEffect::setBlurRadius(qreal blurRadius)
{
    Q_D(QGraphicsDropShadowEffect);
    if (qFuzzyCompare(d->filter->blurRadius(), blurRadius))
        return;

    d->filter->setBlurRadius(blurRadius);
    updateBoundingRect();
    emit blurRadiusChanged(blurRadius);
}

/*!
    \fn void QGraphicsDropShadowEffect::blurRadiusChanged(qreal blurRadius)

    This signal is emitted whenever the effect's blur radius changes.
    The \a blurRadius parameter holds the effect's new blur radius.
*/

/*!
    \property QGraphicsDropShadowEffect::color
    \brief the color of the drop shadow.

    By default, the drop color is a semi-transparent dark gray
    (QColor(63, 63, 63, 180)).

    \sa offset(), blurRadius()
*/
QColor QGraphicsDropShadowEffect::color() const
{
    Q_D(const QGraphicsDropShadowEffect);
    return d->filter->color();
}

void QGraphicsDropShadowEffect::setColor(const QColor &color)
{
    Q_D(QGraphicsDropShadowEffect);
    if (d->filter->color() == color)
        return;

    d->filter->setColor(color);
    update();
    emit colorChanged(color);
}

/*!
    \fn void QGraphicsDropShadowEffect::colorChanged(const QColor &color)

    This signal is emitted whenever the effect's color changes.
    The \a color parameter holds the effect's new color.
*/

/*!
    \reimp
*/
QRectF QGraphicsDropShadowEffect::boundingRectFor(const QRectF &rect) const
{
    Q_D(const QGraphicsDropShadowEffect);
    return d->filter->boundingRectFor(rect);
}

/*!
    \reimp
*/
void QGraphicsDropShadowEffect::draw(QPainter *painter)
{
    Q_D(QGraphicsDropShadowEffect);
    if (d->filter->blurRadius() <= 0 && d->filter->offset().isNull()) {
        drawSource(painter);
        return;
    }

    PixmapPadMode mode = PadToEffectiveBoundingRect;

    // Draw pixmap in device coordinates to avoid pixmap scaling.
    QPoint offset;
    const QPixmap pixmap = sourcePixmap(Qt::DeviceCoordinates, &offset, mode);
    if (pixmap.isNull())
        return;

    QTransform restoreTransform = painter->worldTransform();
    painter->setWorldTransform(QTransform());
    d->filter->draw(painter, offset, pixmap);
    painter->setWorldTransform(restoreTransform);
}

/*!
    \class QGraphicsOpacityEffect
    \brief The QGraphicsOpacityEffect class provides an opacity effect.
    \since 4.6
    \inmodule QtWidgets

    An opacity effect renders the source with an opacity. This effect is useful
    for making the source semi-transparent, similar to a fade-in/fade-out
    sequence. The opacity can be modified using the setOpacity() function.

    By default, the opacity is 0.7.

    \image graphicseffect-opacity.png

    \sa QGraphicsDropShadowEffect, QGraphicsBlurEffect, QGraphicsColorizeEffect
*/

/*!
    Constructs a new QGraphicsOpacityEffect instance.
    The \a parent parameter is passed to QGraphicsEffect's constructor.
*/
QGraphicsOpacityEffect::QGraphicsOpacityEffect(QObject *parent)
    : QGraphicsEffect(*new QGraphicsOpacityEffectPrivate, parent)
{
}

/*!
    Destroys the effect.
*/
QGraphicsOpacityEffect::~QGraphicsOpacityEffect()
{
}

/*!
    \property QGraphicsOpacityEffect::opacity
    \brief the opacity of the effect.

    The value should be in the range of 0.0 to 1.0, where 0.0 is
    fully transparent and 1.0 is fully opaque.

    By default, the opacity is 0.7.

    \sa setOpacityMask()
*/
qreal QGraphicsOpacityEffect::opacity() const
{
    Q_D(const QGraphicsOpacityEffect);
    return d->opacity;
}

void QGraphicsOpacityEffect::setOpacity(qreal opacity)
{
    Q_D(QGraphicsOpacityEffect);
    opacity = qBound(qreal(0.0), opacity, qreal(1.0));

    if (qFuzzyCompare(d->opacity, opacity))
        return;

    d->opacity = opacity;
    if ((d->isFullyTransparent = qFuzzyIsNull(d->opacity)))
        d->isFullyOpaque = 0;
    else
        d->isFullyOpaque = qFuzzyIsNull(d->opacity - 1);
    update();
    emit opacityChanged(opacity);
}

/*!
    \fn void QGraphicsOpacityEffect::opacityChanged(qreal opacity)

    This signal is emitted whenever the effect's opacity changes.
    The \a opacity parameter holds the effect's new opacity.
*/

/*!
    \property QGraphicsOpacityEffect::opacityMask
    \brief the opacity mask of the effect.

    An opacity mask allows you apply opacity to portions of an element.

    For example:

    \snippet code/src_gui_effects_qgraphicseffect.cpp 2

    There is no opacity mask by default.

    \sa setOpacity()
*/
QBrush QGraphicsOpacityEffect::opacityMask() const
{
    Q_D(const QGraphicsOpacityEffect);
    return d->opacityMask;
}

void QGraphicsOpacityEffect::setOpacityMask(const QBrush &mask)
{
    Q_D(QGraphicsOpacityEffect);
    if (d->opacityMask == mask)
        return;

    d->opacityMask = mask;
    d->hasOpacityMask = (mask.style() != Qt::NoBrush);
    update();

    emit opacityMaskChanged(mask);
}

/*!
    \fn void QGraphicsOpacityEffect::opacityMaskChanged(const QBrush &mask)

    This signal is emitted whenever the effect's opacity mask changes.
    The \a mask parameter holds the effect's new opacity mask.
*/

/*!
    \reimp
*/
void QGraphicsOpacityEffect::draw(QPainter *painter)
{
    Q_D(QGraphicsOpacityEffect);

    // Transparent; nothing to draw.
    if (d->isFullyTransparent)
        return;

    // Opaque; draw directly without going through a pixmap.
    if (d->isFullyOpaque && !d->hasOpacityMask) {
        drawSource(painter);
        return;
    }

    QPoint offset;
    Qt::CoordinateSystem system = sourceIsPixmap() ? Qt::LogicalCoordinates : Qt::DeviceCoordinates;
    QPixmap pixmap = sourcePixmap(system, &offset, QGraphicsEffect::NoPad);
    if (pixmap.isNull())
        return;

    painter->save();
    painter->setOpacity(d->opacity);

    if (d->hasOpacityMask) {
        QPainter pixmapPainter(&pixmap);
        pixmapPainter.setRenderHints(painter->renderHints());
        pixmapPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        if (system == Qt::DeviceCoordinates) {
            QTransform worldTransform = painter->worldTransform();
            worldTransform *= QTransform::fromTranslate(-offset.x(), -offset.y());
            pixmapPainter.setWorldTransform(worldTransform);
            pixmapPainter.fillRect(sourceBoundingRect(), d->opacityMask);
        } else {
            pixmapPainter.translate(-offset);
            pixmapPainter.fillRect(pixmap.rect(), d->opacityMask);
        }
    }

    if (system == Qt::DeviceCoordinates)
        painter->setWorldTransform(QTransform());

    painter->drawPixmap(offset, pixmap);
    painter->restore();
}


QT_END_NAMESPACE

#endif //QT_NO_GRAPHICSEFFECT
