/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "GraphicsLayerQt.h"

#if !defined(QT_NO_GRAPHICSVIEW)

#include "CurrentTime.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "Image.h"
#include "RefCounted.h"
#include "TranslateTransformOperation.h"
#include "UnitBezier.h"
#include <QtCore/qabstractanimation.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qset.h>
#include <QtCore/qtimer.h>
#include <QtGui/qcolor.h>
#include <QtGui/qgraphicseffect.h>
#include <QtGui/qgraphicsitem.h>
#include <QtGui/qgraphicsscene.h>
#include <QtGui/qgraphicsview.h>
#include <QtGui/qgraphicswidget.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qpixmapcache.h>
#include <QtGui/qstyleoption.h>

#if ENABLE(TILED_BACKING_STORE)
#include "TiledBackingStore.h"
#include "TiledBackingStoreClient.h"

// The minimum width/height for tiling. We use the same value as the Windows implementation.
#define GRAPHICS_LAYER_TILING_THRESHOLD 2000
#endif

#define QT_DEBUG_RECACHE 0
#define QT_DEBUG_CACHEDUMP 0

#define QT_DEBUG_FPS 0

namespace WebCore {

static const int gMinimumPixmapCacheLimit = 2048;

#ifndef QT_NO_GRAPHICSEFFECT
class MaskEffectQt : public QGraphicsEffect {
public:
    MaskEffectQt(QObject* parent, QGraphicsItem* maskLayer)
        : QGraphicsEffect(parent)
        , m_maskLayer(maskLayer)
    {
    }

    void draw(QPainter* painter)
    {
        // This is a modified clone of QGraphicsOpacityEffect.
        // It's more efficient to do it this way because:
        // (a) We don't need the QBrush abstraction - we always end up using QGraphicsItem::paint
        //     from the mask layer.
        // (b) QGraphicsOpacityEffect detaches the pixmap, which is inefficient on OpenGL.
        const QSize maskSize = sourceBoundingRect().toAlignedRect().size();
        if (!maskSize.isValid() || maskSize.isEmpty()) {
            drawSource(painter);
            return;
        }
        QPixmap maskPixmap(maskSize);

        // We need to do this so the pixmap would have hasAlpha().
        maskPixmap.fill(Qt::transparent);
        QPainter maskPainter(&maskPixmap);
        QStyleOptionGraphicsItem option;
        option.exposedRect = option.rect = maskPixmap.rect();
        maskPainter.setRenderHints(painter->renderHints(), true);
        m_maskLayer->paint(&maskPainter, &option, 0);
        maskPainter.end();

        QPoint offset;
        QPixmap srcPixmap = sourcePixmap(Qt::LogicalCoordinates, &offset, QGraphicsEffect::NoPad);

        // We have to use another intermediate pixmap, to make sure the mask applies only to this item
        // and doesn't modify pixels already painted into this paint-device.
        QPixmap pixmap(srcPixmap.size());
        pixmap.fill(Qt::transparent);

        if (pixmap.isNull())
            return;

        QPainter pixmapPainter(&pixmap);

        pixmapPainter.setRenderHints(painter->renderHints());
        pixmapPainter.setCompositionMode(QPainter::CompositionMode_Source);

        // We use drawPixmap rather than detaching, because it's more efficient on OpenGL.
        pixmapPainter.drawPixmap(0, 0, srcPixmap);
        pixmapPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        pixmapPainter.drawPixmap(0, 0, maskPixmap);

        pixmapPainter.end();
        painter->drawPixmap(offset, pixmap);
    }

    QGraphicsItem* m_maskLayer;
};
#endif // QT_NO_GRAPHICSEFFECT

class GraphicsLayerQtImpl : public QGraphicsObject
#if ENABLE(TILED_BACKING_STORE)
, public virtual TiledBackingStoreClient
#endif
{
    Q_OBJECT

public:
    // This set of flags help us defer which properties of the layer have been
    // modified by the compositor, so we can know what to look for in the next flush.
    enum ChangeMask {
        NoChanges =                 0,

        ParentChange =              (1L << 0),
        ChildrenChange =            (1L << 1),
        MaskLayerChange =           (1L << 2),
        PositionChange =            (1L << 3),

        AnchorPointChange =         (1L << 4),
        SizeChange  =               (1L << 5),
        TransformChange =           (1L << 6),
        ContentChange =             (1L << 7),

        ContentsOrientationChange = (1L << 8),
        OpacityChange =             (1L << 9),
        ContentsRectChange =        (1L << 10),

        Preserves3DChange =         (1L << 11),
        MasksToBoundsChange =       (1L << 12),
        DrawsContentChange =        (1L << 13),
        ContentsOpaqueChange =      (1L << 14),

        BackfaceVisibilityChange =  (1L << 15),
        ChildrenTransformChange =   (1L << 16),
        DisplayChange =             (1L << 17),
        BackgroundColorChange =     (1L << 18),

        DistributesOpacityChange =  (1L << 19)
    };

    // The compositor lets us special-case images and colors, so we try to do so.
    enum StaticContentType { HTMLContentType, PixmapContentType, ColorContentType, MediaContentType, Canvas3DContentType};

    const GraphicsLayerQtImpl* rootLayer() const;

    GraphicsLayerQtImpl(GraphicsLayerQt* newLayer);
    virtual ~GraphicsLayerQtImpl();

    // reimps from QGraphicsItem
    virtual QPainterPath opaqueArea() const;
    virtual QRectF boundingRect() const;
    virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);

    // We manage transforms ourselves because transform-origin acts differently in webkit and in Qt,
    // and we need it as a fallback in case we encounter an un-invertible matrix.
    void setBaseTransform(const TransformationMatrix&);
    void updateTransform();

    // let the compositor-API tell us which properties were changed
    void notifyChange(ChangeMask);

    // Actual rendering of the web-content into a QPixmap:
    // We prefer to use our own caching because it gives us a higher level of granularity than
    // QGraphicsItem cache modes - Sometimes we need to cache the contents even though the item
    // needs to be updated, e.g. when the background-color is changed.
    // TODO: investigate if QGraphicsItem caching can be improved to support that out of the box.
    QPixmap recache(const QRegion&);

    // Called when the compositor is ready for us to show the changes on screen.
    // This is called indirectly from ChromeClientQt::setNeedsOneShotDrawingSynchronization
    // (meaning the sync would happen together with the next draw) or
    // ChromeClientQt::scheduleCompositingLayerSync (meaning the sync will happen ASAP)
    void flushChanges(bool recursive = true, bool forceTransformUpdate = false);

#if ENABLE(TILED_BACKING_STORE)
    // reimplementations from TiledBackingStoreClient
    virtual void tiledBackingStorePaintBegin();
    virtual void tiledBackingStorePaint(GraphicsContext*, const IntRect&);
    virtual void tiledBackingStorePaintEnd(const Vector<IntRect>& paintedArea);
    virtual IntRect tiledBackingStoreContentsRect();
    virtual IntRect tiledBackingStoreVisibleRect();
    virtual Color tiledBackingStoreBackgroundColor() const;
#endif

    static bool allowAcceleratedCompositingCache() { return QPixmapCache::cacheLimit() > gMinimumPixmapCacheLimit; }

    void drawLayerContent(QPainter*, const QRect&);

public slots:
    // We need to notify the client (ie. the layer compositor) when the animation actually starts.
    void notifyAnimationStarted();

    // We notify WebCore of a layer changed asynchronously; otherwise we end up calling flushChanges too often.
    void notifySyncRequired();

signals:
    // Optimization: Avoid using QTimer::singleShot().
    void notifyAnimationStartedAsync();

public:
    GraphicsLayerQt* m_layer;

    TransformationMatrix m_baseTransform;
    TransformationMatrix m_transformRelativeToRootLayer;
    bool m_transformAnimationRunning;
    bool m_opacityAnimationRunning;
    bool m_blockNotifySyncRequired;
#ifndef QT_NO_GRAPHICSEFFECT
    QWeakPointer<MaskEffectQt> m_maskEffect;
#endif

    struct ContentData {
        QPixmap pixmap;
        QRegion regionToUpdate;
        bool updateAll;

        QColor contentsBackgroundColor;
        QColor backgroundColor;

        QWeakPointer<QGraphicsObject> mediaLayer;
        StaticContentType contentType;

        float opacity;

        ContentData()
            : updateAll(false)
            , contentType(HTMLContentType)
            , opacity(1.f)
        {
        }

    };

    ContentData m_pendingContent;
    ContentData m_currentContent;

    int m_changeMask;

#if ENABLE(TILED_BACKING_STORE)
    TiledBackingStore* m_tiledBackingStore;
#endif

    QSizeF m_size;
    struct {
        QPixmapCache::Key key;
        QSizeF size;
    } m_backingStore;
#ifndef QT_NO_ANIMATION
    QList<QWeakPointer<QAbstractAnimation> > m_animations;
#endif
    QTimer m_suspendTimer;

    struct State {
        GraphicsLayer* maskLayer;
        FloatPoint pos;
        FloatPoint3D anchorPoint;
        FloatSize size;
        TransformationMatrix transform;
        TransformationMatrix childrenTransform;
        Color backgroundColor;
        Color currentColor;
        GraphicsLayer::CompositingCoordinatesOrientation contentsOrientation;
        float opacity;
        QRect contentsRect;

        bool preserves3D: 1;
        bool masksToBounds: 1;
        bool drawsContent: 1;
        bool contentsOpaque: 1;
        bool backfaceVisibility: 1;
        bool distributeOpacity: 1;
        bool align: 2;

        State()
            : maskLayer(0)
            , opacity(1.f)
            , preserves3D(false)
            , masksToBounds(false)
            , drawsContent(false)
            , contentsOpaque(false)
            , backfaceVisibility(false)
            , distributeOpacity(false)
        {
        }
    } m_state;

#ifndef QT_NO_ANIMATION
    friend class AnimationQtBase;
#endif
};

inline GraphicsLayerQtImpl* toGraphicsLayerQtImpl(QGraphicsItem* item)
{
    ASSERT(item);
    return qobject_cast<GraphicsLayerQtImpl*>(item->toGraphicsObject());
}

inline GraphicsLayerQtImpl* toGraphicsLayerQtImpl(QGraphicsObject* item)
{
    return qobject_cast<GraphicsLayerQtImpl*>(item);
}

GraphicsLayerQtImpl::GraphicsLayerQtImpl(GraphicsLayerQt* newLayer)
    : QGraphicsObject(0)
    , m_layer(newLayer)
    , m_transformAnimationRunning(false)
    , m_opacityAnimationRunning(false)
    , m_blockNotifySyncRequired(false)
    , m_changeMask(NoChanges)
#if ENABLE(TILED_BACKING_STORE)
    , m_tiledBackingStore(0)
#endif
{
    // We use graphics-view for compositing-only, not for interactivity.
    setAcceptedMouseButtons(Qt::NoButton);

    // We need to have the item enabled, or else wheel events are not passed to the parent class
    // implementation of wheelEvent, where they are ignored and passed to the item below.
    setEnabled(true);

    connect(this, SIGNAL(notifyAnimationStartedAsync()), this, SLOT(notifyAnimationStarted()), Qt::QueuedConnection);
}

GraphicsLayerQtImpl::~GraphicsLayerQtImpl()
{
    // The compositor manages lifecycle of item, so we do not want the graphicsview system to delete
    // our items automatically.
    const QList<QGraphicsItem*> children = childItems();
    QList<QGraphicsItem*>::const_iterator cit;
    for (cit = children.constBegin(); cit != children.constEnd(); ++cit) {
        if (QGraphicsItem* item = *cit) {
            if (scene())
                scene()->removeItem(item);
            item->setParentItem(0);
        }
    }
#if ENABLE(TILED_BACKING_STORE)
    delete m_tiledBackingStore;
#endif
#ifndef QT_NO_ANIMATION
    // We do, however, own the animations.
    QList<QWeakPointer<QAbstractAnimation> >::iterator it;
    for (it = m_animations.begin(); it != m_animations.end(); ++it)
        if (QAbstractAnimation* anim = it->data())
            delete anim;
#endif
}

const GraphicsLayerQtImpl* GraphicsLayerQtImpl::rootLayer() const
{
    if (const GraphicsLayerQtImpl* parent = toGraphicsLayerQtImpl(parentObject()))
        return parent->rootLayer();
    return this;
}


void GraphicsLayerQtImpl::drawLayerContent(QPainter* painter, const QRect& clipRect)
{
    painter->setClipRect(clipRect, Qt::IntersectClip);
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    GraphicsContext gc(painter);
    m_layer->paintGraphicsLayerContents(gc, clipRect);
}

QPixmap GraphicsLayerQtImpl::recache(const QRegion& regionToUpdate)
{
    if (!m_layer->drawsContent() || m_size.isEmpty() || !m_size.isValid())
        return QPixmap();

#if ENABLE(TILED_BACKING_STORE)
    const bool requiresTiling = (m_state.drawsContent && m_currentContent.contentType == HTMLContentType) && (m_size.width() > GRAPHICS_LAYER_TILING_THRESHOLD || m_size.height() > GRAPHICS_LAYER_TILING_THRESHOLD);
    if (requiresTiling && !m_tiledBackingStore) {
        m_tiledBackingStore = new TiledBackingStore(this);
        m_tiledBackingStore->setTileCreationDelay(0);
        setFlag(ItemUsesExtendedStyleOption, true);
    } else if (!requiresTiling && m_tiledBackingStore) {
        delete m_tiledBackingStore;
        m_tiledBackingStore = 0;
        setFlag(ItemUsesExtendedStyleOption, false);
    }

    if (m_tiledBackingStore) {
        m_tiledBackingStore->adjustVisibleRect();
        const QVector<QRect> rects = regionToUpdate.rects();
        for (int i = 0; i < rects.size(); ++i)
           m_tiledBackingStore->invalidate(rects[i]);
        return QPixmap();
    }
#endif

    QPixmap pixmap;
    QRegion region = regionToUpdate;
    if (QPixmapCache::find(m_backingStore.key, &pixmap)) {
        if (region.isEmpty())
            return pixmap;
        QPixmapCache::remove(m_backingStore.key); // Remove the reference to the pixmap in the cache to avoid a detach.
    }

    {
        bool erased = false;

        // If the pixmap is not in the cache or the view has grown since last cached.
        if (pixmap.isNull() || m_size != m_backingStore.size) {
#if QT_DEBUG_RECACHE
            if (pixmap.isNull())
                qDebug() << "CacheMiss" << this << m_size;
#endif
            bool fill = true;
            QRegion newRegion;
            QPixmap oldPixmap = pixmap;

            // If the pixmap is two small to hold the view contents we enlarge, otherwise just use the old (large) pixmap.
            if (pixmap.width() < m_size.width() || pixmap.height() < m_size.height()) {
#if QT_DEBUG_RECACHE
                qDebug() << "CacheGrow" << this << m_size;
#endif
                pixmap = QPixmap(m_size.toSize());
                pixmap.fill(Qt::transparent);
                newRegion = QRegion(0, 0, m_size.width(), m_size.height());
            }

#if 1
            // Blit the contents of oldPixmap back into the cached pixmap as we are just adding new pixels.
            if (!oldPixmap.isNull()) {
                const QRegion cleanRegion = (QRegion(0, 0, m_size.width(), m_size.height())
                                             & QRegion(0, 0, m_backingStore.size.width(), m_backingStore.size.height())) - regionToUpdate;
                if (!cleanRegion.isEmpty()) {
#if QT_DEBUG_RECACHE
                    qDebug() << "CacheBlit" << this << cleanRegion;
#endif
                    const QRect cleanBounds(cleanRegion.boundingRect());
                    QPainter painter(&pixmap);
                    painter.setCompositionMode(QPainter::CompositionMode_Source);
                    painter.drawPixmap(cleanBounds.topLeft(), oldPixmap, cleanBounds);
                    newRegion -= cleanRegion;
                    fill = false; // We cannot just fill the pixmap.
                }
                oldPixmap = QPixmap();
            }
#endif
            region += newRegion;
            if (fill && !region.isEmpty()) { // Clear the entire pixmap with the background.
#if QT_DEBUG_RECACHE
                qDebug() << "CacheErase" << this << m_size << background;
#endif
                erased = true;
                pixmap.fill(Qt::transparent);
            }
        }
        region &= QRegion(0, 0, m_size.width(), m_size.height());

        // If we have something to draw its time to erase it and render the contents.
        if (!region.isEmpty()) {
#if QT_DEBUG_CACHEDUMP
            static int recacheCount = 0;
            ++recacheCount;
            qDebug() << "**** CACHEDUMP" << recacheCount << this << m_layer << region << m_size;
            pixmap.save(QString().sprintf("/tmp/%05d_A.png", recacheCount), "PNG");
#endif

            QPainter painter(&pixmap);
            GraphicsContext gc(&painter);

            painter.setClipRegion(region);

            if (!erased) { // Erase the area in cache that we're drawing into.
                painter.setCompositionMode(QPainter::CompositionMode_Clear);
                painter.fillRect(region.boundingRect(), Qt::transparent);

#if QT_DEBUG_CACHEDUMP
                qDebug() << "**** CACHEDUMP" << recacheCount << this << m_layer << region << m_size;
                pixmap.save(QString().sprintf("/tmp/%05d_B.png", recacheCount), "PNG");
#endif
            }

            // Render the actual contents into the cache.
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            m_layer->paintGraphicsLayerContents(gc, region.boundingRect());
            painter.end();

#if QT_DEBUG_CACHEDUMP
            qDebug() << "**** CACHEDUMP" << recacheCount << this << m_layer << region << m_size;
            pixmap.save(QString().sprintf("/tmp/%05d_C.png", recacheCount), "PNG");
#endif
        }
        m_backingStore.size = m_size; // Store the used size of the pixmap.
    }

    // Finally insert into the cache and allow a reference there.
    m_backingStore.key = QPixmapCache::insert(pixmap);
    return pixmap;
}

void GraphicsLayerQtImpl::updateTransform()
{
    if (!m_transformAnimationRunning)
        m_baseTransform = m_layer->transform();

    TransformationMatrix localTransform;

    GraphicsLayerQtImpl* parent = toGraphicsLayerQtImpl(parentObject());

    // WebCore has relative-to-size originPoint, where as the QGraphicsView has a pixel originPoint.
    // Thus, we need to convert here as we have to manage this outselves due to the fact that the
    // transformOrigin of the graphicsview is imcompatible.
    const qreal originX = m_state.anchorPoint.x() * m_size.width();
    const qreal originY = m_state.anchorPoint.y() * m_size.height();

    // We ignore QGraphicsItem::pos completely, and use transforms only, due to the fact that we
    // have to maintain that ourselves for 3D.
    localTransform
            .translate3d(originX + m_state.pos.x(), originY + m_state.pos.y(), m_state.anchorPoint.z())
            .multiply(m_baseTransform)
            .translate3d(-originX, -originY, -m_state.anchorPoint.z());

    // This is the actual 3D transform of this item, with the ancestors' transform baked in.
    m_transformRelativeToRootLayer = TransformationMatrix(parent ? parent->m_transformRelativeToRootLayer : TransformationMatrix())
                                         .multiply(localTransform);

    // Now we have enough information to determine if the layer is facing backwards.
    if (!m_state.backfaceVisibility && m_transformRelativeToRootLayer.inverse().m33() < 0) {
        setVisible(false);
        // No point in making extra calculations for invisible elements.
        return;
    }

    // The item is front-facing or backface-visibility is on.
    setVisible(true);

    // Flatten to 2D-space of this item if it doesn't preserve 3D.
    if (!m_state.preserves3D) {
        m_transformRelativeToRootLayer.setM13(0);
        m_transformRelativeToRootLayer.setM23(0);
        m_transformRelativeToRootLayer.setM31(0);
        m_transformRelativeToRootLayer.setM32(0);
        m_transformRelativeToRootLayer.setM33(1);
        m_transformRelativeToRootLayer.setM34(0);
        m_transformRelativeToRootLayer.setM43(0);
    }

    // Apply perspective for the use of this item's children. Perspective is always applied from the item's
    // center.
    if (!m_state.childrenTransform.isIdentity()) {
        m_transformRelativeToRootLayer
            .translate(m_size.width() / 2, m_size.height() /2)
            .multiply(m_state.childrenTransform)
            .translate(-m_size.width() / 2, -m_size.height() /2);
    }

    bool inverseOk = true;
    // Use QTransform::inverse to extrapolate the relative transform of this item, based on the parent's
    // transform relative to the root layer and the desired transform for this item relative to the root layer.
    const QTransform parentTransform = parent ? parent->itemTransform(rootLayer()) : QTransform();
    const QTransform transform2D = QTransform(m_transformRelativeToRootLayer) * parentTransform.inverted(&inverseOk);

    // In rare cases the transformation cannot be inversed - in that case we don't apply the transformation at
    // all, otherwise we'd flicker. FIXME: This should be amended when Qt moves to a real 3D scene-graph.
    if (!inverseOk)
        return;

    setTransform(transform2D);

    const QList<QGraphicsItem*> children = childItems();
    QList<QGraphicsItem*>::const_iterator it;
    for (it = children.constBegin(); it != children.constEnd(); ++it)
        if (GraphicsLayerQtImpl* layer= toGraphicsLayerQtImpl(*it))
            layer->updateTransform();
}

void GraphicsLayerQtImpl::setBaseTransform(const TransformationMatrix& baseTransform)
{
    m_baseTransform = baseTransform;
    updateTransform();
}

QPainterPath GraphicsLayerQtImpl::opaqueArea() const
{
    QPainterPath painterPath;

    // We try out best to return the opaque area, maybe it will help graphics-view render less items.
    if (m_currentContent.backgroundColor.isValid() && m_currentContent.backgroundColor.alpha() == 0xff)
        painterPath.addRect(boundingRect());
    else {
        if (m_state.contentsOpaque
            || (m_currentContent.contentType == ColorContentType && m_currentContent.contentsBackgroundColor.alpha() == 0xff)
            || (m_currentContent.contentType == MediaContentType)
            || (m_currentContent.contentType == PixmapContentType && !m_currentContent.pixmap.hasAlpha())) {
            painterPath.addRect(m_state.contentsRect);
        }
    }
    return painterPath;
}

QRectF GraphicsLayerQtImpl::boundingRect() const
{
    return QRectF(QPointF(0, 0), QSizeF(m_size));
}

void GraphicsLayerQtImpl::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
#if ENABLE(TILED_BACKING_STORE)
    // FIXME: There's currently no Qt API to know if a new region of an item is exposed outside of the paint event.
    // Suggested for Qt: http://bugreports.qt.nokia.com/browse/QTBUG-14877.
    if (m_tiledBackingStore)
        m_tiledBackingStore->adjustVisibleRect();
#endif

    if (m_currentContent.backgroundColor.isValid())
        painter->fillRect(option->exposedRect, QColor(m_currentContent.backgroundColor));

    switch (m_currentContent.contentType) {
    case HTMLContentType:
        if (m_state.drawsContent) {
            if (!allowAcceleratedCompositingCache())
                drawLayerContent(painter, option->exposedRect.toRect());
            else {
                QPixmap backingStore;
                // We might need to recache, in case we try to paint and the cache was purged (e.g. if it was full).
                if (!QPixmapCache::find(m_backingStore.key, &backingStore) || backingStore.size() != m_size.toSize())
                    backingStore = recache(QRegion(m_state.contentsRect));
                painter->drawPixmap(0, 0, backingStore);
            }
        }
        break;
    case PixmapContentType:
        painter->drawPixmap(m_state.contentsRect, m_currentContent.pixmap);
        break;
    case ColorContentType:
        painter->fillRect(m_state.contentsRect, m_currentContent.contentsBackgroundColor);
        break;
    case MediaContentType:
        // we don't need to paint anything: we have a QGraphicsItem from the media element
        break;
    }
}

void GraphicsLayerQtImpl::notifySyncRequired()
{
    m_blockNotifySyncRequired = false;

    if (m_layer->client())
        m_layer->client()->notifySyncRequired(m_layer);
}

void GraphicsLayerQtImpl::notifyChange(ChangeMask changeMask)
{
    m_changeMask |= changeMask;

    if (m_blockNotifySyncRequired)
        return;

    static QMetaMethod syncMethod = staticMetaObject.method(staticMetaObject.indexOfMethod("notifySyncRequired()"));
    syncMethod.invoke(this, Qt::QueuedConnection);

    m_blockNotifySyncRequired = true;
}

void GraphicsLayerQtImpl::flushChanges(bool recursive, bool forceUpdateTransform)
{
    // This is the bulk of the work. understanding what the compositor is trying to achieve, what
    // graphicsview can do, and trying to find a sane common-ground.
    if (!m_layer || m_changeMask == NoChanges)
        goto afterLayerChanges;

    if (m_changeMask & ParentChange) {
        // The WebCore compositor manages item ownership. We have to make sure graphicsview doesn't
        // try to snatch that ownership.
        if (!m_layer->parent() && !parentItem())
            setParentItem(0);
        else if (m_layer && m_layer->parent() && m_layer->parent()->platformLayer() != parentItem())
            setParentItem(m_layer->parent()->platformLayer());
    }

    if (m_changeMask & ChildrenChange) {
        // We basically do an XOR operation on the list of current children and the list of wanted
        // children, and remove/add.
        QSet<QGraphicsItem*> newChildren;
        const Vector<GraphicsLayer*> newChildrenVector = (m_layer->children());
        newChildren.reserve(newChildrenVector.size());

        for (size_t i = 0; i < newChildrenVector.size(); ++i)
            newChildren.insert(newChildrenVector[i]->platformLayer());

        const QSet<QGraphicsItem*> currentChildren = childItems().toSet();
        const QSet<QGraphicsItem*> childrenToAdd = newChildren - currentChildren;
        const QSet<QGraphicsItem*> childrenToRemove = currentChildren - newChildren;

        QSet<QGraphicsItem*>::const_iterator it;
        for (it = childrenToAdd.constBegin(); it != childrenToAdd.constEnd(); ++it) {
             if (QGraphicsItem* w = *it)
                w->setParentItem(this);
        }

        QSet<QGraphicsItem*>::const_iterator rit;
        for (rit = childrenToRemove.constBegin(); rit != childrenToRemove.constEnd(); ++rit) {
             if (GraphicsLayerQtImpl* w = toGraphicsLayerQtImpl(*rit))
                w->setParentItem(0);
        }

        // Children are ordered by z-value, let graphicsview know.
        for (size_t i = 0; i < newChildrenVector.size(); ++i) {
            if (newChildrenVector[i]->platformLayer())
                newChildrenVector[i]->platformLayer()->setZValue(i);
        }
    }

    if (m_changeMask & MaskLayerChange) {
        // We can't paint here, because we don't know if the mask layer itself is ready... we'll have
        // to wait till this layer tries to paint.
        setFlag(ItemClipsChildrenToShape, m_layer->maskLayer() || m_layer->masksToBounds());
#ifndef QT_NO_GRAPHICSEFFECT
        setGraphicsEffect(0);
        if (m_layer->maskLayer()) {
            if (GraphicsLayerQtImpl* mask = toGraphicsLayerQtImpl(m_layer->maskLayer()->platformLayer())) {
                mask->m_maskEffect = new MaskEffectQt(this, mask);
                setGraphicsEffect(mask->m_maskEffect.data());
            }
        }
#endif
    }

    if (m_changeMask & SizeChange) {
        if (m_layer->size() != m_state.size) {
            prepareGeometryChange();
            m_size = QSizeF(m_layer->size().width(), m_layer->size().height());
        }
    }

    // FIXME: This is a hack, due to a probable QGraphicsScene bug when rapidly modifying the perspective
    // but without this line we get graphic artifacts.
    if ((m_changeMask & ChildrenTransformChange) && m_state.childrenTransform != m_layer->childrenTransform())
        if (scene())
            scene()->update();

    if (m_changeMask & (ChildrenTransformChange | Preserves3DChange | TransformChange | AnchorPointChange | SizeChange | BackfaceVisibilityChange | PositionChange | ParentChange)) {
        // Due to the differences between the way WebCore handles transforms and the way Qt handles transforms,
        // all these elements affect the transforms of all the descendants.
        forceUpdateTransform = true;
    }

    if (m_changeMask & (ContentChange | DrawsContentChange | MaskLayerChange)) {
        switch (m_pendingContent.contentType) {
        case PixmapContentType:
            update();
            setFlag(ItemHasNoContents, false);
            break;

        case MediaContentType:
            setFlag(ItemHasNoContents, true);
            m_pendingContent.mediaLayer.data()->setParentItem(this);
            break;

        case ColorContentType:
            if (m_pendingContent.contentType != m_currentContent.contentType
                || m_pendingContent.contentsBackgroundColor != m_currentContent.contentsBackgroundColor)
                update();
            m_state.drawsContent = false;
            setFlag(ItemHasNoContents, false);

            // Only use ItemUsesExtendedStyleOption for HTML content as colors don't gain much from that.
            setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, false);
            break;

        case HTMLContentType:
            if (m_pendingContent.contentType != m_currentContent.contentType)
                update();
            else if (!m_state.drawsContent && m_layer->drawsContent())
                update();

            setFlag(ItemHasNoContents, !m_layer->drawsContent());
            break;
        }
    }

    if ((m_changeMask & OpacityChange) && m_state.opacity != m_layer->opacity() && !m_opacityAnimationRunning)
        setOpacity(m_layer->opacity());

    if (m_changeMask & ContentsRectChange) {
        const QRect rect(m_layer->contentsRect());
        if (m_state.contentsRect != rect) {
            m_state.contentsRect = rect;
            if (m_pendingContent.mediaLayer) {
                QGraphicsWidget* widget = qobject_cast<QGraphicsWidget*>(m_pendingContent.mediaLayer.data());
                if (widget)
                    widget->setGeometry(rect);
            }
            update();
        }
    }

    if ((m_changeMask & MasksToBoundsChange) && m_state.masksToBounds != m_layer->masksToBounds()) {
        setFlag(QGraphicsItem::ItemClipsToShape, m_layer->masksToBounds());
        setFlag(QGraphicsItem::ItemClipsChildrenToShape, m_layer->masksToBounds());
    }

    if ((m_changeMask & ContentsOpaqueChange) && m_state.contentsOpaque != m_layer->contentsOpaque())
        prepareGeometryChange();

#ifndef QT_NO_GRAPHICSEFFECT
    if (m_maskEffect)
        m_maskEffect.data()->update();
    else
#endif
    if (m_changeMask & DisplayChange) {
#ifndef QT_GRAPHICS_LAYER_NO_RECACHE_ON_DISPLAY_CHANGE
        // Recache now: all the content is ready and we don't want to wait until the paint event.
        // We only need to do this for HTML content, there's no point in caching directly composited
        // content like images or solid rectangles.
        if (m_pendingContent.contentType == HTMLContentType && allowAcceleratedCompositingCache())
            recache(m_pendingContent.regionToUpdate);
#endif
        update(m_pendingContent.regionToUpdate.boundingRect());
        m_pendingContent.regionToUpdate = QRegion();
    }

    if ((m_changeMask & BackgroundColorChange)
        && (m_pendingContent.backgroundColor != m_currentContent.backgroundColor))
        update();

    m_state.maskLayer = m_layer->maskLayer();
    m_state.pos = m_layer->position();
    m_state.anchorPoint = m_layer->anchorPoint();
    m_state.size = m_layer->size();
    m_state.transform = m_layer->transform();
    m_state.contentsOrientation =m_layer->contentsOrientation();
    m_state.opacity = m_layer->opacity();
    m_state.contentsRect = m_layer->contentsRect();
    m_state.preserves3D = m_layer->preserves3D();
    m_state.masksToBounds = m_layer->masksToBounds();
    m_state.drawsContent = m_layer->drawsContent();
    m_state.contentsOpaque = m_layer->contentsOpaque();
    m_state.backfaceVisibility = m_layer->backfaceVisibility();
    m_state.childrenTransform = m_layer->childrenTransform();
    m_currentContent.pixmap = m_pendingContent.pixmap;
    m_currentContent.contentType = m_pendingContent.contentType;
    m_currentContent.mediaLayer = m_pendingContent.mediaLayer;
    m_currentContent.backgroundColor = m_pendingContent.backgroundColor;
    m_currentContent.contentsBackgroundColor = m_pendingContent.contentsBackgroundColor;
    m_pendingContent.regionToUpdate = QRegion();
    m_changeMask = NoChanges;

afterLayerChanges:
    if (forceUpdateTransform)
        updateTransform();

    if (!recursive)
        return;

    QList<QGraphicsItem*> children = childItems();
    if (m_state.maskLayer)
        children.append(m_state.maskLayer->platformLayer());

    QList<QGraphicsItem*>::const_iterator it;
    for (it = children.constBegin(); it != children.constEnd(); ++it) {
        if (QGraphicsItem* item = *it) {
            if (GraphicsLayerQtImpl* layer = toGraphicsLayerQtImpl(item))
                layer->flushChanges(true, forceUpdateTransform);
        }
    }
}

#if ENABLE(TILED_BACKING_STORE)
/* \reimp (TiledBackingStoreClient.h)
*/
void GraphicsLayerQtImpl::tiledBackingStorePaintBegin()
{
}

/* \reimp (TiledBackingStoreClient.h)
*/
void GraphicsLayerQtImpl::tiledBackingStorePaint(GraphicsContext* gc,  const IntRect& rect)
{
    m_layer->paintGraphicsLayerContents(*gc, rect);
}

/* \reimp (TiledBackingStoreClient.h)
*/
void GraphicsLayerQtImpl::tiledBackingStorePaintEnd(const Vector<IntRect>& paintedArea)
{
    for (int i = 0; i < paintedArea.size(); ++i)
        update(QRectF(paintedArea[i]));
}

/* \reimp (TiledBackingStoreClient.h)
*/
IntRect GraphicsLayerQtImpl::tiledBackingStoreContentsRect()
{
    return m_layer->contentsRect();
}

/* \reimp (TiledBackingStoreClient.h)
*/
Color GraphicsLayerQtImpl::tiledBackingStoreBackgroundColor() const
{
    if (m_currentContent.contentType == PixmapContentType && !m_currentContent.pixmap.hasAlphaChannel())
        return Color(0, 0, 0);
    // We return a transparent color so that the tiles initialize with alpha.
    return Color(0, 0, 0, 0);
}

IntRect GraphicsLayerQtImpl::tiledBackingStoreVisibleRect()
{
    const QGraphicsView* view = scene()->views().isEmpty() ? 0 : scene()->views().first();
    if (!view)
        return mapFromScene(scene()->sceneRect()).boundingRect().toAlignedRect();

    // All we get is the viewport's visible region. We have to map it to the scene and then to item coordinates.
    return mapFromScene(view->mapToScene(view->viewport()->visibleRegion().boundingRect()).boundingRect()).boundingRect().toAlignedRect();
}
#endif

void GraphicsLayerQtImpl::notifyAnimationStarted()
{
    // WebCore notifies javascript when the animation starts. Here we're letting it know.
    m_layer->client()->notifyAnimationStarted(m_layer, /* DOM time */ WTF::currentTime());
}

GraphicsLayerQt::GraphicsLayerQt(GraphicsLayerClient* client)
    : GraphicsLayer(client)
    , m_impl(PassOwnPtr<GraphicsLayerQtImpl>(new GraphicsLayerQtImpl(this)))
{
}

GraphicsLayerQt::~GraphicsLayerQt()
{
}

// This is the hook for WebCore compositor to know that Qt implements compositing with GraphicsLayerQt.
PassOwnPtr<GraphicsLayer> GraphicsLayer::create(GraphicsLayerClient* client)
{
    return new GraphicsLayerQt(client);
}

/* \reimp (GraphicsLayer.h): The current size might change, thus we need to update the whole display.
*/
void GraphicsLayerQt::setNeedsDisplay()
{
    m_impl->m_pendingContent.regionToUpdate = QRegion(QRect(QPoint(0, 0), QSize(size().width(), size().height())));
    m_impl->notifyChange(GraphicsLayerQtImpl::DisplayChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setNeedsDisplayInRect(const FloatRect& rect)
{
    m_impl->m_pendingContent.regionToUpdate |= QRectF(rect).toAlignedRect();
    m_impl->notifyChange(GraphicsLayerQtImpl::DisplayChange);
}

void GraphicsLayerQt::setContentsNeedsDisplay()
{
    switch (m_impl->m_pendingContent.contentType) {
    case GraphicsLayerQtImpl::MediaContentType:
        if (!m_impl->m_pendingContent.mediaLayer)
            return;
        m_impl->m_pendingContent.mediaLayer.data()->update();
        break;
    default:
        setNeedsDisplay();
        break;
    }
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setName(const String& name)
{
    m_impl->setObjectName(name);
    GraphicsLayer::setName(name);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setParent(GraphicsLayer* layer)
{
    m_impl->notifyChange(GraphicsLayerQtImpl::ParentChange);
    GraphicsLayer::setParent(layer);
}

/* \reimp (GraphicsLayer.h)
*/
bool GraphicsLayerQt::setChildren(const Vector<GraphicsLayer*>& children)
{
    m_impl->notifyChange(GraphicsLayerQtImpl::ChildrenChange);
    return GraphicsLayer::setChildren(children);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::addChild(GraphicsLayer* layer)
{
    m_impl->notifyChange(GraphicsLayerQtImpl::ChildrenChange);
    GraphicsLayer::addChild(layer);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::addChildAtIndex(GraphicsLayer* layer, int index)
{
    GraphicsLayer::addChildAtIndex(layer, index);
    m_impl->notifyChange(GraphicsLayerQtImpl::ChildrenChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::addChildAbove(GraphicsLayer* layer, GraphicsLayer* sibling)
{
     GraphicsLayer::addChildAbove(layer, sibling);
     m_impl->notifyChange(GraphicsLayerQtImpl::ChildrenChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::addChildBelow(GraphicsLayer* layer, GraphicsLayer* sibling)
{

    GraphicsLayer::addChildBelow(layer, sibling);
    m_impl->notifyChange(GraphicsLayerQtImpl::ChildrenChange);
}

/* \reimp (GraphicsLayer.h)
*/
bool GraphicsLayerQt::replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild)
{
    if (GraphicsLayer::replaceChild(oldChild, newChild)) {
        m_impl->notifyChange(GraphicsLayerQtImpl::ChildrenChange);
        return true;
    }

    return false;
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::removeFromParent()
{
    if (parent())
        m_impl->notifyChange(GraphicsLayerQtImpl::ParentChange);
    GraphicsLayer::removeFromParent();
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setMaskLayer(GraphicsLayer* value)
{
    if (value == maskLayer())
        return;
    GraphicsLayer::setMaskLayer(value);
    m_impl->notifyChange(GraphicsLayerQtImpl::MaskLayerChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setPosition(const FloatPoint& value)
{
    if (value == position())
        return;
    GraphicsLayer::setPosition(value);
    m_impl->notifyChange(GraphicsLayerQtImpl::PositionChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setAnchorPoint(const FloatPoint3D& value)
{
    if (value == anchorPoint())
        return;
    GraphicsLayer::setAnchorPoint(value);
    m_impl->notifyChange(GraphicsLayerQtImpl::AnchorPointChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setSize(const FloatSize& value)
{
    if (value == size())
        return;
    GraphicsLayer::setSize(value);
    m_impl->notifyChange(GraphicsLayerQtImpl::SizeChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setTransform(const TransformationMatrix& value)
{
    if (value == transform())
        return;
    GraphicsLayer::setTransform(value);
    m_impl->notifyChange(GraphicsLayerQtImpl::TransformChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setChildrenTransform(const TransformationMatrix& value)
{
    if (value == childrenTransform())
        return;
    GraphicsLayer::setChildrenTransform(value);
    m_impl->notifyChange(GraphicsLayerQtImpl::ChildrenTransformChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setPreserves3D(bool value)
{
    if (value == preserves3D())
        return;
    GraphicsLayer::setPreserves3D(value);
    m_impl->notifyChange(GraphicsLayerQtImpl::Preserves3DChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setMasksToBounds(bool value)
{
    if (value == masksToBounds())
        return;
    GraphicsLayer::setMasksToBounds(value);
    m_impl->notifyChange(GraphicsLayerQtImpl::MasksToBoundsChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setDrawsContent(bool value)
{
    if (value == drawsContent())
        return;
    m_impl->notifyChange(GraphicsLayerQtImpl::DrawsContentChange);
    GraphicsLayer::setDrawsContent(value);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setBackgroundColor(const Color& value)
{
    if (value == m_impl->m_pendingContent.backgroundColor)
        return;
    m_impl->m_pendingContent.backgroundColor = value;
    GraphicsLayer::setBackgroundColor(value);
    m_impl->notifyChange(GraphicsLayerQtImpl::BackgroundColorChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::clearBackgroundColor()
{
    if (!m_impl->m_pendingContent.backgroundColor.isValid())
        return;
    m_impl->m_pendingContent.backgroundColor = QColor();
    GraphicsLayer::clearBackgroundColor();
    m_impl->notifyChange(GraphicsLayerQtImpl::BackgroundColorChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setContentsOpaque(bool value)
{
    if (value == contentsOpaque())
        return;
    m_impl->notifyChange(GraphicsLayerQtImpl::ContentsOpaqueChange);
    GraphicsLayer::setContentsOpaque(value);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setBackfaceVisibility(bool value)
{
    if (value == backfaceVisibility())
        return;
    GraphicsLayer::setBackfaceVisibility(value);
    m_impl->notifyChange(GraphicsLayerQtImpl::BackfaceVisibilityChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setOpacity(float value)
{
    if (value == opacity())
        return;
    GraphicsLayer::setOpacity(value);
    m_impl->notifyChange(GraphicsLayerQtImpl::OpacityChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setContentsRect(const IntRect& value)
{
    if (value == contentsRect())
        return;
    GraphicsLayer::setContentsRect(value);
    m_impl->notifyChange(GraphicsLayerQtImpl::ContentsRectChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setContentsToImage(Image* image)
{
    m_impl->notifyChange(GraphicsLayerQtImpl::ContentChange);
    m_impl->m_pendingContent.contentType = GraphicsLayerQtImpl::HTMLContentType;
    GraphicsLayer::setContentsToImage(image);
    if (image) {
        QPixmap* pxm = image->nativeImageForCurrentFrame();
        if (pxm) {
            m_impl->m_pendingContent.pixmap = *pxm;
            m_impl->m_pendingContent.contentType = GraphicsLayerQtImpl::PixmapContentType;
            return;
        }
    }
    m_impl->m_pendingContent.pixmap = QPixmap();
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setContentsBackgroundColor(const Color& color)
{
    m_impl->notifyChange(GraphicsLayerQtImpl::ContentChange);
    m_impl->m_pendingContent.contentType = GraphicsLayerQtImpl::ColorContentType;
    m_impl->m_pendingContent.contentsBackgroundColor = QColor(color);
    GraphicsLayer::setContentsBackgroundColor(color);
}

void GraphicsLayerQt::setContentsToMedia(PlatformLayer* media)
{
    if (media) {
        m_impl->m_pendingContent.contentType = GraphicsLayerQtImpl::MediaContentType;
        m_impl->m_pendingContent.mediaLayer = media->toGraphicsObject();
    } else
        m_impl->m_pendingContent.contentType = GraphicsLayerQtImpl::HTMLContentType;

    m_impl->notifyChange(GraphicsLayerQtImpl::ContentChange);
    GraphicsLayer::setContentsToMedia(media);
}

void GraphicsLayerQt::setContentsToCanvas(PlatformLayer* canvas)
{
    setContentsToMedia(canvas);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::setContentsOrientation(CompositingCoordinatesOrientation orientation)
{
    m_impl->notifyChange(GraphicsLayerQtImpl::ContentsOrientationChange);
    GraphicsLayer::setContentsOrientation(orientation);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::distributeOpacity(float o)
{
    m_impl->notifyChange(GraphicsLayerQtImpl::OpacityChange);
    m_impl->m_state.distributeOpacity = true;
}

/* \reimp (GraphicsLayer.h)
*/
float GraphicsLayerQt::accumulatedOpacity() const
{
    return m_impl->effectiveOpacity();
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::syncCompositingState()
{
    m_impl->flushChanges();
    GraphicsLayer::syncCompositingState();
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerQt::syncCompositingStateForThisLayerOnly()
{
    // We can't call flushChanges recursively here
    m_impl->flushChanges(false);
    GraphicsLayer::syncCompositingStateForThisLayerOnly();
}

/* \reimp (GraphicsLayer.h)
*/
PlatformLayer* GraphicsLayerQt::platformLayer() const
{
    return m_impl.get();
}

// Now we start dealing with WebCore animations translated to Qt animations

template <typename T>
struct KeyframeValueQt {
    const TimingFunction* timingFunction;
    T value;
};

/* Copied from AnimationBase.cpp
*/
static inline double solveEpsilon(double duration)
{
    return 1.0 / (200.0 * duration);
}

static inline double solveCubicBezierFunction(qreal p1x, qreal p1y, qreal p2x, qreal p2y, double t, double duration)
{
    UnitBezier bezier(p1x, p1y, p2x, p2y);
    return bezier.solve(t, solveEpsilon(duration));
}

static inline double solveStepsFunction(int numSteps, bool stepAtStart, double t)
{
    if (stepAtStart)
        return qMin(1.0, (floor(numSteps * t) + 1) / numSteps);
    return floor(numSteps * t) / numSteps;
}

static inline qreal applyTimingFunction(const TimingFunction* timingFunction, qreal progress, double duration)
{
    // We want the timing function to be as close as possible to what the web-developer intended, so
    // we're using the same function used by WebCore when compositing is disabled. Using easing-curves
    // would probably work for some of the cases, but wouldn't really buy us anything as we'd have to
    // convert the bezier function back to an easing curve.

    if (timingFunction->isCubicBezierTimingFunction()) {
        const CubicBezierTimingFunction* ctf = static_cast<const CubicBezierTimingFunction*>(timingFunction);
        return solveCubicBezierFunction(ctf->x1(),
                                        ctf->y1(),
                                        ctf->x2(),
                                        ctf->y2(),
                                        double(progress), double(duration) / 1000);
    } else if (timingFunction->isStepsTimingFunction()) {
        const StepsTimingFunction* stf = static_cast<const StepsTimingFunction*>(timingFunction);
        return solveStepsFunction(stf->numberOfSteps(), stf->stepAtStart(), double(progress));
    } else
        return progress;
}

// Helper functions to safely get a value out of WebCore's AnimationValue*.

#ifndef QT_NO_ANIMATION
static void webkitAnimationToQtAnimationValue(const AnimationValue* animationValue, TransformOperations& transformOperations)
{
    transformOperations = TransformOperations();
    if (!animationValue)
        return;

    if (const TransformOperations* ops = static_cast<const TransformAnimationValue*>(animationValue)->value())
        transformOperations = *ops;
}

static void webkitAnimationToQtAnimationValue(const AnimationValue* animationValue, qreal& realValue)
{
    realValue = animationValue ? static_cast<const FloatAnimationValue*>(animationValue)->value() : 0;
}

// We put a bit of the functionality in a base class to allow casting and to save some code size.

class AnimationQtBase : public QAbstractAnimation {
public:
    AnimationQtBase(GraphicsLayerQtImpl* layer, const KeyframeValueList& values, const IntSize& boxSize, const Animation* anim, const QString & name)
        : QAbstractAnimation(0)
        , m_layer(layer)
        , m_boxSize(boxSize)
        , m_duration(anim->duration() * 1000)
        , m_isAlternate(anim->direction() == Animation::AnimationDirectionAlternate)
        , m_webkitPropertyID(values.property())
        , m_webkitAnimation(anim)
        , m_keyframesName(name)
        , m_fillsForwards(false)
    {
    }


    virtual AnimatedPropertyID animatedProperty() const = 0;

    virtual void updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
    {
        QAbstractAnimation::updateState(newState, oldState);

        // For some reason we have do this asynchronously - or the animation won't work.
        if (newState == Running && oldState == Stopped && m_layer.data())
            m_layer.data()->notifyAnimationStartedAsync();
    }

    virtual int duration() const { return m_duration; }

    QWeakPointer<GraphicsLayerQtImpl> m_layer;
    IntSize m_boxSize;
    int m_duration;
    bool m_isAlternate;
    AnimatedPropertyID m_webkitPropertyID;

    // We might need this in case the same animation is added again (i.e. resumed by WebCore).
    const Animation* m_webkitAnimation;
    QString m_keyframesName;
    bool m_fillsForwards;
};

// We'd rather have a templatized QAbstractAnimation than QPropertyAnimation / QVariantAnimation;
// Since we know the types that we're dealing with, the QObject/QProperty/QVariant abstraction
// buys us very little in this case, for too much overhead.
template <typename T>
class AnimationQt : public AnimationQtBase {

public:
    AnimationQt(GraphicsLayerQtImpl* layer, const KeyframeValueList& values, const IntSize& boxSize, const Animation* anim, const QString & name)
        : AnimationQtBase(layer, values, boxSize, anim, name)
    {
        // Copying those WebCore structures is not trivial, we have to do it like this.
        for (size_t i = 0; i < values.size(); ++i) {
            const AnimationValue* animationValue = values.at(i);
            KeyframeValueQt<T> keyframeValue;
            if (animationValue->timingFunction())
                keyframeValue.timingFunction = animationValue->timingFunction();
            else
                keyframeValue.timingFunction = anim->timingFunction().get();
            webkitAnimationToQtAnimationValue(animationValue, keyframeValue.value);
            m_keyframeValues[animationValue->keyTime()] = keyframeValue;
        }
    }

protected:

    // This is the part that differs between animated properties.
    virtual void applyFrame(const T& fromValue, const T& toValue, qreal progress) = 0;

    virtual void updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
    {
#if QT_DEBUG_FPS
        if (newState == Running && oldState == Stopped) {
            qDebug("Animation Started!");
            m_fps.frames = 0;
            m_fps.duration.start();
        } else if (newState == Stopped && oldState == Running) {
            const int duration = m_fps.duration.elapsed();
            qDebug("Animation Ended! %dms [%f FPS]", duration,
                    (1000 / (((float)duration) / m_fps.frames)));
        }
#endif
        AnimationQtBase::updateState(newState, oldState);
    }

    virtual void updateCurrentTime(int currentTime)
    {
        if (!m_layer)
            return;

        qreal progress = qreal(currentLoopTime()) / duration();

        if (m_isAlternate && currentLoop()%2)
            progress = 1-progress;

        if (m_keyframeValues.isEmpty())
            return;

        // Find the current from-to keyframes in our little map.
        typename QMap<qreal, KeyframeValueQt<T> >::iterator it = m_keyframeValues.find(progress);

        // We didn't find an exact match, we try the closest match (lower bound).
        if (it == m_keyframeValues.end())
            it = m_keyframeValues.lowerBound(progress)-1;

        // We didn't find any match; use the first keyframe.
        if (it == m_keyframeValues.end())
            it = m_keyframeValues.begin();

        typename QMap<qreal, KeyframeValueQt<T> >::iterator it2 = it + 1;
        if (it2 == m_keyframeValues.end())
            it2 = it;
        const KeyframeValueQt<T>& fromKeyframe = it.value();
        const KeyframeValueQt<T>& toKeyframe = it2.value();

        const TimingFunction* timingFunc = fromKeyframe.timingFunction;
        const T& fromValue = fromKeyframe.value;
        const T& toValue = toKeyframe.value;

        // Now we have a source keyframe, origin keyframe and a timing function.
        // We can now process the progress and apply the frame.
        progress = (!progress || progress == 1 || it.key() == it2.key()) ?
            progress : applyTimingFunction(timingFunc, (progress - it.key()) / (it2.key() - it.key()), duration());
        applyFrame(fromValue, toValue, progress);
#if QT_DEBUG_FPS
        ++m_fps.frames;
#endif
    }

    QMap<qreal, KeyframeValueQt<T> > m_keyframeValues;
#if QT_DEBUG_FPS
    struct {
        QTime duration;
        int frames;
    } m_fps;
#endif
};

class TransformAnimationQt : public AnimationQt<TransformOperations> {
public:
    TransformAnimationQt(GraphicsLayerQtImpl* layer, const KeyframeValueList& values, const IntSize& boxSize, const Animation* anim, const QString & name)
        : AnimationQt<TransformOperations>(layer, values, boxSize, anim, name)
    {
    }

    ~TransformAnimationQt()
    {
        if (m_fillsForwards)
            setCurrentTime(1);
    }

    virtual AnimatedPropertyID animatedProperty() const { return AnimatedPropertyWebkitTransform; }

    // The idea is that we let WebCore manage the transform operations and Qt just manage the
    // animation heartbeat and the bottom-line QTransform. We gain performance, not by using
    // Transform instead of TransformationMatrix, but by proper caching of items that are
    // expensive for WebCore to render. We want the rest to be as close to WebCore's idea as possible.
    virtual void applyFrame(const TransformOperations& sourceOperations, const TransformOperations& targetOperations, qreal progress)
    {
        TransformationMatrix transformMatrix;

        bool validTransformLists = true;
        const int sourceOperationCount = sourceOperations.size();
        if (sourceOperationCount) {
            if (targetOperations.size() != sourceOperationCount)
                validTransformLists = false;
            else {
                for (size_t j = 0; j < sourceOperationCount && validTransformLists; ++j) {
                    if (!sourceOperations.operations()[j]->isSameType(*targetOperations.operations()[j]))
                        validTransformLists = false;
                }
            }
        }

        if (validTransformLists) {
            for (size_t i = 0; i < targetOperations.size(); ++i)
                targetOperations.operations()[i]->blend(sourceOperations.at(i), progress)->apply(transformMatrix, m_boxSize);
        } else {
            targetOperations.apply(m_boxSize, transformMatrix);
            transformMatrix.blend(m_sourceMatrix, progress);
        }

        m_layer.data()->m_layer->setTransform(transformMatrix);
        // We force the actual opacity change, otherwise it would be ignored because of the animation.
        m_layer.data()->setBaseTransform(transformMatrix);
    }

    virtual void updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
    {
        AnimationQt<TransformOperations>::updateState(newState, oldState);
        if (!m_layer)
            return;

        m_layer.data()->flushChanges(true);

        // To increase FPS, we use a less accurate caching mechanism while animation is going on
        // this is a UX choice that should probably be customizable.
        if (newState == QAbstractAnimation::Running) {
            m_sourceMatrix = m_layer.data()->m_layer->transform();
            m_layer.data()->m_transformAnimationRunning = true;
        } else if (newState == QAbstractAnimation::Stopped) {
            // We update the transform back to the default. This already takes fill-modes into account.
            m_layer.data()->m_transformAnimationRunning = false;
            if (m_layer && m_layer.data()->m_layer)
                m_layer.data()->setBaseTransform(m_layer.data()->m_layer->transform());
        }
    }

    TransformationMatrix m_sourceMatrix;
};

class OpacityAnimationQt : public AnimationQt<qreal> {
public:
    OpacityAnimationQt(GraphicsLayerQtImpl* layer, const KeyframeValueList& values, const IntSize& boxSize, const Animation* anim, const QString& name)
         : AnimationQt<qreal>(layer, values, boxSize, anim, name)
    {
    }

    ~OpacityAnimationQt()
    {
        if (m_fillsForwards)
            setCurrentTime(1);
    }

    virtual AnimatedPropertyID animatedProperty() const { return AnimatedPropertyOpacity; }

    virtual void applyFrame(const qreal& fromValue, const qreal& toValue, qreal progress)
    {
        qreal opacity = qBound(qreal(0), fromValue + (toValue - fromValue) * progress, qreal(1));

        // FIXME: This is a hack, due to a probable QGraphicsScene bug.
        // Without this the opacity change doesn't always have immediate effect.
        if (m_layer.data()->scene() && !m_layer.data()->opacity() && opacity)
            m_layer.data()->scene()->update();

        m_layer.data()->m_layer->setOpacity(opacity);
        // We force the actual opacity change, otherwise it would be ignored because of the animation.
        m_layer.data()->setOpacity(opacity);
    }

    virtual void updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
    {
        AnimationQt<qreal>::updateState(newState, oldState);

        if (m_layer)
            m_layer.data()->m_opacityAnimationRunning = (newState == QAbstractAnimation::Running);

        // If stopped, we update the opacity back to the default. This already takes fill-modes into account.
        if (newState == Stopped)
            if (m_layer && m_layer.data()->m_layer)
                m_layer.data()->setOpacity(m_layer.data()->m_layer->opacity());

    }
};

bool GraphicsLayerQt::addAnimation(const KeyframeValueList& values, const IntSize& boxSize, const Animation* anim, const String& keyframesName, double timeOffset)
{
    if (!anim->duration() || !anim->iterationCount())
        return false;

    AnimationQtBase* newAnim = 0;

    // Fixed: we might already have the Qt animation object associated with this WebCore::Animation object.
    QList<QWeakPointer<QAbstractAnimation> >::iterator it;
    for (it = m_impl->m_animations.begin(); it != m_impl->m_animations.end(); ++it) {
        if (*it) {
            AnimationQtBase* curAnimation = static_cast<AnimationQtBase*>(it->data());
            if (curAnimation && curAnimation->m_webkitAnimation == anim
                && values.property() == curAnimation->animatedProperty()) {
                newAnim = curAnimation;
                break;
            }
        }
    }

    if (!newAnim) {
        switch (values.property()) {
        case AnimatedPropertyOpacity:
            newAnim = new OpacityAnimationQt(m_impl.get(), values, boxSize, anim, keyframesName);
            break;
        case AnimatedPropertyWebkitTransform:
            newAnim = new TransformAnimationQt(m_impl.get(), values, boxSize, anim, keyframesName);
            break;
        default:
            return false;
        }

        // We make sure WebCore::Animation and QAnimation are on the same terms.
        newAnim->setLoopCount(anim->iterationCount());
        newAnim->m_fillsForwards = anim->fillsForwards();
        m_impl->m_animations.append(QWeakPointer<QAbstractAnimation>(newAnim));
        QObject::connect(&m_impl->m_suspendTimer, SIGNAL(timeout()), newAnim, SLOT(resume()));
    }

    // Flush now to avoid flicker.
    m_impl->flushChanges(false);

    // Qhen fill-mode is backwards/both, we set the value to 0 before the delay takes place.
    if (anim->fillsBackwards())
        newAnim->setCurrentTime(0);

    newAnim->start();

    // We synchronize the animation's clock to WebCore's timeOffset.
    newAnim->setCurrentTime(timeOffset * 1000);

    // We don't need to manage the animation object's lifecycle:
    // WebCore would call removeAnimations when it's time to delete.

    return true;
}

void GraphicsLayerQt::removeAnimationsForProperty(AnimatedPropertyID id)
{
    QList<QWeakPointer<QAbstractAnimation> >::iterator it;
    for (it = m_impl->m_animations.begin(); it != m_impl->m_animations.end(); ++it) {
        if (!(*it))
            continue;

        AnimationQtBase* anim = static_cast<AnimationQtBase*>(it->data());
        if (anim && anim->m_webkitPropertyID == id) {
            // We need to stop the animation right away, or it might flicker before it's deleted.
            anim->stop();
            anim->deleteLater();
            it = m_impl->m_animations.erase(it);
            --it;
        }
    }
}

void GraphicsLayerQt::removeAnimationsForKeyframes(const String& name)
{
    QList<QWeakPointer<QAbstractAnimation> >::iterator it;
    for (it = m_impl->m_animations.begin(); it != m_impl->m_animations.end(); ++it) {
        if (!(*it))
            continue;

        AnimationQtBase* anim = static_cast<AnimationQtBase*>(it->data());
        if (anim && anim->m_keyframesName == QString(name)) {
            // We need to stop the animation right away, or it might flicker before it's deleted.
            anim->stop();
            anim->deleteLater();
            it = m_impl->m_animations.erase(it);
            --it;
        }
    }
}

void GraphicsLayerQt::pauseAnimation(const String& name, double timeOffset)
{
    QList<QWeakPointer<QAbstractAnimation> >::iterator it;
    for (it = m_impl->m_animations.begin(); it != m_impl->m_animations.end(); ++it) {
        if (!(*it))
            continue;

        AnimationQtBase* anim = static_cast<AnimationQtBase*>(it->data());
        if (anim && anim->m_keyframesName == QString(name)) {
            // we synchronize the animation's clock to WebCore's timeOffset
            anim->setCurrentTime(timeOffset * 1000);
            anim->pause();
        }
    }
}

void GraphicsLayerQt::suspendAnimations(double time)
{
    if (m_impl->m_suspendTimer.isActive()) {
        m_impl->m_suspendTimer.stop();
        m_impl->m_suspendTimer.start(time * 1000);
    } else {
        QList<QWeakPointer<QAbstractAnimation> >::iterator it;
        for (it = m_impl->m_animations.begin(); it != m_impl->m_animations.end(); ++it) {
            if (QAbstractAnimation* anim = it->data())
                anim->pause();
        }
    }
}

void GraphicsLayerQt::resumeAnimations()
{
    if (m_impl->m_suspendTimer.isActive()) {
        m_impl->m_suspendTimer.stop();
        QList<QWeakPointer<QAbstractAnimation> >::iterator it;
        for (it = m_impl->m_animations.begin(); it != m_impl->m_animations.end(); ++it) {
            if (QAbstractAnimation* anim = it->data())
                anim->resume();
        }
    }
}

#endif // QT_NO_ANIMATION
}

#include <GraphicsLayerQt.moc>


#endif // QT_NO_GRAPHICSVIEW
