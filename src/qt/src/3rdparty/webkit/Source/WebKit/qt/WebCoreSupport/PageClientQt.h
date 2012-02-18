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

#ifndef PageClientQt_h
#define PageClientQt_h

#include "FrameView.h"
#include "GraphicsContext.h"
#include "IntRect.h"
#include "QWebPageClient.h"
#include "TiledBackingStore.h"
#include "qgraphicswebview.h"
#include "qwebframe.h"
#include "qwebframe_p.h"
#include "qwebpage.h"
#include "qwebpage_p.h"
#include <QtCore/qmetaobject.h>
#include <QtGui/qgraphicsscene.h>
#include <QtGui/qgraphicsview.h>
#include <QtGui/qgraphicswidget.h>
#include <QtGui/qscrollbar.h>
#include <QtGui/qstyleoption.h>
#include <QtGui/qwidget.h>

#include <Settings.h>

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
#include "texmap/TextureMapperPlatformLayer.h"
#endif

namespace WebCore {

class PageClientQWidget : public QWebPageClient {
public:
    PageClientQWidget(QWidget* newView, QWebPage* newPage)
        : view(newView)
        , page(newPage)
#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
        , syncTimer(this, &PageClientQWidget::syncLayers)
        , platformLayerProxy(0)
#endif
    {
        Q_ASSERT(view);
    }
    virtual ~PageClientQWidget();

    virtual bool isQWidgetClient() const { return true; }

    virtual void scroll(int dx, int dy, const QRect&);
    virtual void update(const QRect& dirtyRect);
    virtual void setInputMethodEnabled(bool enable);
    virtual bool inputMethodEnabled() const;
    virtual void setInputMethodHints(Qt::InputMethodHints hints);

#ifndef QT_NO_CURSOR
    virtual QCursor cursor() const;
    virtual void updateCursor(const QCursor& cursor);
#endif

    virtual QPalette palette() const;
    virtual int screenNumber() const;
    virtual QWidget* ownerWidget() const;
    virtual QRect geometryRelativeToOwnerWidget() const;

    virtual QObject* pluginParent() const;

    virtual QStyle* style() const;

    virtual bool viewResizesToContentsEnabled() const { return false; }

    virtual QRectF windowRect() const;

    QWidget* view;
    QWebPage* page;

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
    virtual void setRootGraphicsLayer(TextureMapperPlatformLayer* layer);
    virtual void markForSync(bool scheduleSync);
    void syncLayers(Timer<PageClientQWidget>*);
#endif

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
    virtual bool allowsAcceleratedCompositing() const { return true; }
#else
    virtual bool allowsAcceleratedCompositing() const { return false; }
#endif

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
    Timer<PageClientQWidget> syncTimer;
    PlatformLayerProxyQt* platformLayerProxy;
#endif
};

#if !defined(QT_NO_GRAPHICSVIEW)
// the overlay is here for one reason only: to have the scroll-bars and other
// extra UI elements appear on top of any QGraphicsItems created by CSS compositing layers
class QGraphicsItemOverlay : public QGraphicsObject {
    public:
    QGraphicsItemOverlay(QGraphicsWidget* view, QWebPage* p)
            :QGraphicsObject(view)
            , q(view)
            , page(p)
    {
        setPos(0, 0);
        setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
        setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }

    QRectF boundingRect() const
    {
        return q->boundingRect();
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* options, QWidget*)
    {
        page->mainFrame()->render(painter, static_cast<QWebFrame::RenderLayer>(QWebFrame::AllLayers&(~QWebFrame::ContentsLayer)), options->exposedRect.toRect());
    }

    void prepareGraphicsItemGeometryChange()
    {
        prepareGeometryChange();
    }

    QGraphicsWidget* q;
    QWebPage* page;
};


class PageClientQGraphicsWidget : public QWebPageClient {
public:
    PageClientQGraphicsWidget(QGraphicsWebView* newView, QWebPage* newPage)
        : view(newView)
        , page(newPage)
        , viewResizesToContents(false)
#if USE(ACCELERATED_COMPOSITING)
#if USE(TEXTURE_MAPPER)
        , platformLayerProxy(0)
#endif
        , shouldSync(false)
#endif
        , overlay(0)
    {
       Q_ASSERT(view);
#if USE(ACCELERATED_COMPOSITING)
        // the overlay and stays alive for the lifetime of
        // this QGraphicsWebView as the scrollbars are needed when there's no compositing
        view->setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
        syncMetaMethod = view->metaObject()->method(view->metaObject()->indexOfMethod("syncLayers()"));
#endif
    }

    virtual ~PageClientQGraphicsWidget();

    virtual bool isQWidgetClient() const { return false; }

    virtual void scroll(int dx, int dy, const QRect&);
    virtual void update(const QRect& dirtyRect);
    virtual void setInputMethodEnabled(bool enable);
    virtual bool inputMethodEnabled() const;
    virtual void setInputMethodHints(Qt::InputMethodHints hints);

#ifndef QT_NO_CURSOR
    virtual QCursor cursor() const;
    virtual void updateCursor(const QCursor& cursor);
#endif

    virtual QPalette palette() const;
    virtual int screenNumber() const;
    virtual QWidget* ownerWidget() const;
    virtual QRect geometryRelativeToOwnerWidget() const;

    virtual QObject* pluginParent() const;

    virtual QStyle* style() const;

    virtual bool viewResizesToContentsEnabled() const { return viewResizesToContents; }

    void createOrDeleteOverlay();

#if ENABLE(TILED_BACKING_STORE)
    void updateTiledBackingStoreScale();
    virtual QRectF graphicsItemVisibleRect() const;
#endif

#if USE(ACCELERATED_COMPOSITING)
    virtual void setRootGraphicsLayer(PlatformLayer* layer);
    virtual void markForSync(bool scheduleSync);
    void syncLayers();

    // QGraphicsWebView can render composited layers
    virtual bool allowsAcceleratedCompositing() const { return true; }
#endif

    virtual QRectF windowRect() const;

    QGraphicsWebView* view;
    QWebPage* page;
    bool viewResizesToContents;

#if USE(ACCELERATED_COMPOSITING)
#if USE(TEXTURE_MAPPER)
    PlatformLayerProxyQt* platformLayerProxy;
#else
    QWeakPointer<QGraphicsObject> rootGraphicsLayer;
#endif
    // we have to flush quite often, so we use a meta-method instead of QTimer::singleShot for putting the event in the queue
    QMetaMethod syncMetaMethod;

    // we need to sync the layers if we get a special call from the WebCore
    // compositor telling us to do so. We'll get that call from ChromeClientQt
    bool shouldSync;
#endif
    // the overlay gets instantiated when the root layer is attached, and get deleted when it's detached
    QGraphicsItemOverlay* overlay;

    // we need to put the root graphics layer behind the overlay (which contains the scrollbar)
    enum { RootGraphicsLayerZValue, OverlayZValue };
};
#endif // QT_NO_GRAPHICSVIEW

}
#endif // PageClientQt
