/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"

#include "PageClientQt.h"
#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
#include "TextureMapperQt.h"
#include "texmap/TextureMapperPlatformLayer.h"
#endif
#include <QGraphicsScene>
#include <QGraphicsView>
#if defined(Q_WS_X11)
#include <QX11Info>
#endif

#ifdef QT_OPENGL_LIB
#include "opengl/TextureMapperGL.h"
#include <QGLWidget>
#endif

namespace WebCore {

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)    
class PlatformLayerProxyQt : public QObject, public virtual TextureMapperLayerClient {
public:
    PlatformLayerProxyQt(QWebFrame* frame, TextureMapperContentLayer* layer, QObject* object)
        : QObject(object)
        , m_frame(frame)
        , m_layer(layer)
    {
        if (m_layer)
            m_layer->setPlatformLayerClient(this);
        m_frame->d->rootGraphicsLayer = m_layer;
    }

    void setTextureMapper(PassOwnPtr<TextureMapper> textureMapper)
    {
        m_frame->d->textureMapper = textureMapper;
    }

    virtual ~PlatformLayerProxyQt()
    {
        if (m_layer)
            m_layer->setPlatformLayerClient(0);
        if (m_frame->d)
            m_frame->d->rootGraphicsLayer = 0;
    }

    virtual TextureMapper* textureMapper()
    {
        return m_frame->d->textureMapper.get();
    }

    // Since we just paint the composited tree and never create a special item for it, we don't have to handle its size changes.
    void setSizeChanged(const IntSize&) { }

private:
    QWebFrame* m_frame;
    TextureMapperContentLayer* m_layer;
};

class PlatformLayerProxyQWidget : public PlatformLayerProxyQt {
public:
    PlatformLayerProxyQWidget(QWebFrame* frame, TextureMapperContentLayer* layer, QWidget* widget)
        : PlatformLayerProxyQt(frame, layer, widget)
        , m_widget(widget)
    {
        if (m_widget)
            m_widget->installEventFilter(this);

        if (textureMapper())
            return;

        setTextureMapper(TextureMapperQt::create());
    }

    // We don't want a huge region-clip on the compositing layers; instead we unite the rectangles together
    // and clear them when the paint actually occurs.
    bool eventFilter(QObject* object, QEvent* event)
    {
        if (object == m_widget && event->type() == QEvent::Paint)
            m_dirtyRect = QRect();
        return QObject::eventFilter(object, event);
    }

    void setNeedsDisplay()
    {
        if (m_widget)
            m_widget->update();
    }

    void setNeedsDisplayInRect(const IntRect& rect)
    {
        m_dirtyRect |= rect;
        m_widget->update(m_dirtyRect);
    }

private:
    QRect m_dirtyRect;
    QWidget* m_widget;
};

#if !defined(QT_NO_GRAPHICSVIEW)
class PlatformLayerProxyQGraphicsObject : public PlatformLayerProxyQt {
public:
    PlatformLayerProxyQGraphicsObject(QWebFrame* frame, TextureMapperContentLayer* layer, QGraphicsObject* object)
        : PlatformLayerProxyQt(frame, layer, object)
        , m_graphicsItem(object)
    {
        if (textureMapper())
            return;

#ifdef QT_OPENGL_LIB
        QGraphicsView* view = object->scene()->views()[0];
        if (view && view->viewport() && view->viewport()->inherits("QGLWidget")) {
            setTextureMapper(TextureMapperGL::create());
            return;
        }
#endif
        setTextureMapper(TextureMapperQt::create());
    }

    void setNeedsDisplay()
    {
        if (m_graphicsItem)
            m_graphicsItem->update();
    }

    void setNeedsDisplayInRect(const IntRect& rect)
    {
        if (m_graphicsItem)
            m_graphicsItem->update(QRectF(rect));
    }

private:
    QGraphicsItem* m_graphicsItem;
};
#endif // QT_NO_GRAPHICSVIEW

void PageClientQWidget::setRootGraphicsLayer(TextureMapperPlatformLayer* layer)
{
    if (layer) {
        platformLayerProxy = new PlatformLayerProxyQWidget(page->mainFrame(), static_cast<TextureMapperContentLayer*>(layer), view);
        return;
    }
    delete platformLayerProxy;
    platformLayerProxy = 0;
}

void PageClientQWidget::markForSync(bool scheduleSync)
{
    syncTimer.startOneShot(0);
}

void PageClientQWidget::syncLayers(Timer<PageClientQWidget>*)
{
    QWebFramePrivate::core(page->mainFrame())->view()->syncCompositingStateIncludingSubframes();
}
#endif

void PageClientQWidget::scroll(int dx, int dy, const QRect& rectToScroll)
{
    view->scroll(qreal(dx), qreal(dy), rectToScroll);
}

void PageClientQWidget::update(const QRect & dirtyRect)
{
    view->update(dirtyRect);
}

void PageClientQWidget::setInputMethodEnabled(bool enable)
{
    view->setAttribute(Qt::WA_InputMethodEnabled, enable);
}

bool PageClientQWidget::inputMethodEnabled() const
{
    return view->testAttribute(Qt::WA_InputMethodEnabled);
}

void PageClientQWidget::setInputMethodHints(Qt::InputMethodHints hints)
{
    view->setInputMethodHints(hints);
}

PageClientQWidget::~PageClientQWidget()
{
#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
    delete platformLayerProxy;
#endif
}

#ifndef QT_NO_CURSOR
QCursor PageClientQWidget::cursor() const
{
    return view->cursor();
}

void PageClientQWidget::updateCursor(const QCursor& cursor)
{
    view->setCursor(cursor);
}
#endif

QPalette PageClientQWidget::palette() const
{
    return view->palette();
}

int PageClientQWidget::screenNumber() const
{
#if defined(Q_WS_X11)
    return view->x11Info().screen();
#endif
    return 0;
}

QWidget* PageClientQWidget::ownerWidget() const
{
    return view;
}

QRect PageClientQWidget::geometryRelativeToOwnerWidget() const
{
    return view->geometry();
}

QObject* PageClientQWidget::pluginParent() const
{
    return view;
}

QStyle* PageClientQWidget::style() const
{
    return view->style();
}

QRectF PageClientQWidget::windowRect() const
{
    return QRectF(view->window()->geometry());
}

#if !defined(QT_NO_GRAPHICSVIEW)
PageClientQGraphicsWidget::~PageClientQGraphicsWidget()
{
    delete overlay;
#if USE(ACCELERATED_COMPOSITING)
#if USE(TEXTURE_MAPPER)
    delete platformLayerProxy;
#else
    if (!rootGraphicsLayer)
        return;
    // we don't need to delete the root graphics layer. The lifecycle is managed in GraphicsLayerQt.cpp.
    rootGraphicsLayer.data()->setParentItem(0);
    view->scene()->removeItem(rootGraphicsLayer.data());
#endif
#endif
}

void PageClientQGraphicsWidget::scroll(int dx, int dy, const QRect& rectToScroll)
{
    view->scroll(qreal(dx), qreal(dy), rectToScroll);
}

void PageClientQGraphicsWidget::update(const QRect& dirtyRect)
{
    view->update(dirtyRect);

    createOrDeleteOverlay();
    if (overlay)
        overlay->update(QRectF(dirtyRect));
#if USE(ACCELERATED_COMPOSITING)
    syncLayers();
#endif
}

void PageClientQGraphicsWidget::createOrDeleteOverlay()
{
    // We don't use an overlay with TextureMapper. Instead, the overlay is drawn inside QWebFrame.
#if !USE(TEXTURE_MAPPER)
    bool useOverlay = false;
    if (!viewResizesToContents) {
#if USE(ACCELERATED_COMPOSITING)
        useOverlay = useOverlay || rootGraphicsLayer;
#endif
#if ENABLE(TILED_BACKING_STORE)
        useOverlay = useOverlay || QWebFramePrivate::core(page->mainFrame())->tiledBackingStore();
#endif
    }
    if (useOverlay == !!overlay)
        return;

    if (useOverlay) {
        overlay = new QGraphicsItemOverlay(view, page);
        overlay->setZValue(OverlayZValue);
    } else {
        // Changing the overlay might be done inside paint events.
        overlay->deleteLater();
        overlay = 0;
    }
#endif // !USE(TEXTURE_MAPPER)
}

#if USE(ACCELERATED_COMPOSITING)
void PageClientQGraphicsWidget::syncLayers()
{
    if (shouldSync) {
        QWebFramePrivate::core(page->mainFrame())->view()->syncCompositingStateIncludingSubframes();
        shouldSync = false;
    }
}

#if USE(TEXTURE_MAPPER)
void PageClientQGraphicsWidget::setRootGraphicsLayer(TextureMapperPlatformLayer* layer)
{
    if (layer) {
        platformLayerProxy = new PlatformLayerProxyQGraphicsObject(page->mainFrame(), static_cast<TextureMapperContentLayer*>(layer), view);
        return;
    }
    delete platformLayerProxy;
    platformLayerProxy = 0;
}
#else
void PageClientQGraphicsWidget::setRootGraphicsLayer(QGraphicsObject* layer)
{
    if (rootGraphicsLayer) {
        rootGraphicsLayer.data()->setParentItem(0);
        view->scene()->removeItem(rootGraphicsLayer.data());
        QWebFramePrivate::core(page->mainFrame())->view()->syncCompositingStateIncludingSubframes();
    }

    rootGraphicsLayer = layer;

    if (layer) {
        layer->setParentItem(view);
        layer->setZValue(RootGraphicsLayerZValue);
    }
    createOrDeleteOverlay();
}
#endif

void PageClientQGraphicsWidget::markForSync(bool scheduleSync)
{
    shouldSync = true;
    if (scheduleSync)
        syncMetaMethod.invoke(view, Qt::QueuedConnection);
}

#endif

#if ENABLE(TILED_BACKING_STORE)
void PageClientQGraphicsWidget::updateTiledBackingStoreScale()
{
    WebCore::TiledBackingStore* backingStore = QWebFramePrivate::core(page->mainFrame())->tiledBackingStore();
    if (!backingStore)
        return;
    backingStore->setContentsScale(view->scale());
}
#endif

void PageClientQGraphicsWidget::setInputMethodEnabled(bool enable)
{
    view->setFlag(QGraphicsItem::ItemAcceptsInputMethod, enable);
}

bool PageClientQGraphicsWidget::inputMethodEnabled() const
{
    return view->flags() & QGraphicsItem::ItemAcceptsInputMethod;
}

void PageClientQGraphicsWidget::setInputMethodHints(Qt::InputMethodHints hints)
{
    view->setInputMethodHints(hints);
}

#ifndef QT_NO_CURSOR
QCursor PageClientQGraphicsWidget::cursor() const
{
    return view->cursor();
}

void PageClientQGraphicsWidget::updateCursor(const QCursor& cursor)
{
    view->setCursor(cursor);
}
#endif

QPalette PageClientQGraphicsWidget::palette() const
{
    return view->palette();
}

int PageClientQGraphicsWidget::screenNumber() const
{
#if defined(Q_WS_X11)
    if (QGraphicsScene* scene = view->scene()) {
        const QList<QGraphicsView*> views = scene->views();

        if (!views.isEmpty())
            return views.at(0)->x11Info().screen();
    }
#endif

    return 0;
}

QWidget* PageClientQGraphicsWidget::ownerWidget() const
{
    if (QGraphicsScene* scene = view->scene()) {
        const QList<QGraphicsView*> views = scene->views();
        return views.value(0);
    }
    return 0;
}

QRect PageClientQGraphicsWidget::geometryRelativeToOwnerWidget() const
{
    if (!view->scene())
        return QRect();

    QList<QGraphicsView*> views = view->scene()->views();
    if (views.isEmpty())
        return QRect();

    QGraphicsView* graphicsView = views.at(0);
    return graphicsView->mapFromScene(view->boundingRect()).boundingRect();
}

#if ENABLE(TILED_BACKING_STORE)
QRectF PageClientQGraphicsWidget::graphicsItemVisibleRect() const
{
    if (!view->scene())
        return QRectF();

    QList<QGraphicsView*> views = view->scene()->views();
    if (views.isEmpty())
        return QRectF();

    QGraphicsView* graphicsView = views.at(0);
    int xOffset = graphicsView->horizontalScrollBar()->value();
    int yOffset = graphicsView->verticalScrollBar()->value();
    return view->mapRectFromScene(QRectF(QPointF(xOffset, yOffset), graphicsView->viewport()->size()));
}
#endif

QObject* PageClientQGraphicsWidget::pluginParent() const
{
    return view;
}

QStyle* PageClientQGraphicsWidget::style() const
{
    return view->style();
}

QRectF PageClientQGraphicsWidget::windowRect() const
{
    if (!view->scene())
        return QRectF();

    // The sceneRect is a good approximation of the size of the application, independent of the view.
    return view->scene()->sceneRect();
}
#endif // QT_NO_GRAPHICSVIEW

} // namespace WebCore
