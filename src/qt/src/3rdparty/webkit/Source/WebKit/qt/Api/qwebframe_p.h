/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2007 Staikos Computing Services Inc.

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

#ifndef QWEBFRAME_P_H
#define QWEBFRAME_P_H

#include "qwebframe.h"
#include "qwebpage_p.h"

#include "EventHandler.h"
#include "GraphicsContext.h"
#include "KURL.h"
#include "PlatformString.h"
#if ENABLE(ORIENTATION_EVENTS) && ENABLE(DEVICE_ORIENTATION)
#include "qorientationsensor.h"
#endif
#include "qwebelement.h"
#include "wtf/RefPtr.h"
#include "Frame.h"
#include "ViewportArguments.h"

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
#include "texmap/TextureMapper.h"
#endif


namespace WebCore {
    class FrameLoaderClientQt;
    class FrameView;
    class HTMLFrameOwnerElement;
    class Scrollbar;
    class TextureMapperContentLayer;
}
class QWebPage;

class QWebFrameData {
public:
    QWebFrameData(WebCore::Page*, WebCore::Frame* parentFrame = 0,
                  WebCore::HTMLFrameOwnerElement* = 0,
                  const WTF::String& frameName = WTF::String());

    WebCore::KURL url;
    WTF::String name;
    WebCore::HTMLFrameOwnerElement* ownerElement;
    WebCore::Page* page;
    RefPtr<WebCore::Frame> frame;
    WebCore::FrameLoaderClientQt* frameLoaderClient;

    WTF::String referrer;
    bool allowsScrolling;
    int marginWidth;
    int marginHeight;
};

class QWebFramePrivate {
public:
    QWebFramePrivate()
        : q(0)
        , horizontalScrollBarPolicy(Qt::ScrollBarAsNeeded)
        , verticalScrollBarPolicy(Qt::ScrollBarAsNeeded)
        , frameLoaderClient(0)
        , frame(0)
        , page(0)
        , allowsScrolling(true)
        , marginWidth(-1)
        , marginHeight(-1)
#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
        , rootGraphicsLayer(0)
#endif
        {}
    void init(QWebFrame* qframe, QWebFrameData* frameData);
    void setPage(QWebPage*);

    inline QWebFrame *parentFrame() { return qobject_cast<QWebFrame*>(q->parent()); }

    WebCore::Scrollbar* horizontalScrollBar() const;
    WebCore::Scrollbar* verticalScrollBar() const;

    static WebCore::Frame* core(const QWebFrame*);
    static QWebFrame* kit(const WebCore::Frame*);

    void renderRelativeCoords(WebCore::GraphicsContext*, QFlags<QWebFrame::RenderLayer>, const QRegion& clip);
#if ENABLE(TILED_BACKING_STORE)
    void renderFromTiledBackingStore(WebCore::GraphicsContext*, const QRegion& clip);
#endif

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
    void renderCompositedLayers(WebCore::GraphicsContext*, const WebCore::IntRect& clip);
#endif
    void renderFrameExtras(WebCore::GraphicsContext*, QFlags<QWebFrame::RenderLayer>, const QRegion& clip);
    void emitUrlChanged();
    void _q_orientationChanged();

    QWebFrame *q;
    Qt::ScrollBarPolicy horizontalScrollBarPolicy;
    Qt::ScrollBarPolicy verticalScrollBarPolicy;
    WebCore::FrameLoaderClientQt *frameLoaderClient;
    WebCore::Frame *frame;
    QWebPage *page;
    WebCore::KURL url;

    bool allowsScrolling;
    int marginWidth;
    int marginHeight;
#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
    WebCore::TextureMapperContentLayer* rootGraphicsLayer;
    OwnPtr<WebCore::TextureMapper> textureMapper;
#endif

#if ENABLE(ORIENTATION_EVENTS) && ENABLE(DEVICE_ORIENTATION)
    QtMobility::QOrientationSensor m_orientation;
#endif
};

class QWebHitTestResultPrivate {
public:
    QWebHitTestResultPrivate() : isContentEditable(false), isContentSelected(false), isScrollBar(false) {}
    QWebHitTestResultPrivate(const WebCore::HitTestResult &hitTest);

    QPoint pos;
    QRect boundingRect;
    QWebElement enclosingBlock;
    QString title;
    QString linkText;
    QUrl linkUrl;
    QString linkTitle;
    QPointer<QWebFrame> linkTargetFrame;
    QWebElement linkElement;
    QString alternateText;
    QUrl imageUrl;
    QPixmap pixmap;
    bool isContentEditable;
    bool isContentSelected;
    bool isScrollBar;
    QPointer<QWebFrame> frame;
    RefPtr<WebCore::Node> innerNode;
    RefPtr<WebCore::Node> innerNonSharedNode;
};

#endif
