/*
 * Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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
#ifndef QWebFrameAdapter_h
#define QWebFrameAdapter_h

#include "FrameLoaderClientQt.h"
#include "PlatformEvent.h"
#include "PlatformExportMacros.h"

#if ENABLE(ORIENTATION_EVENTS) && HAVE(QTSENSORS)
#include "qorientationsensor.h"
#endif // ENABLE(ORIENTATION_EVENTS).
#include "qwebelement.h"

#include <QList>
#include <QNetworkAccessManager>
#include <QRect>
#include <QSize>
#include <QUrl>
#include <wtf/ExportMacros.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
class Frame;
class GraphicsContext;
class IntRect;
class TextureMapperLayer;
}

QT_BEGIN_NAMESPACE
class QPoint;
QT_END_NAMESPACE

#if ENABLE(GESTURE_EVENTS)
class QGestureEventFacade;
#endif
class QWebFrame;
class QWebPageAdapter;
class QWebSecurityOrigin;

class WEBKIT_EXPORTDATA QWebHitTestResultPrivate {
public:
    QWebHitTestResultPrivate()
        : isContentEditable(false)
        , isContentSelected(false), isScrollBar(false)
        , innerNode(0)
        , innerNonSharedNode(0)
        , webCoreFrame(0)
    { }
    QWebHitTestResultPrivate(const WebCore::HitTestResult &);
    QWebHitTestResultPrivate(const QWebHitTestResultPrivate&);
    QWebHitTestResultPrivate& operator=(const QWebHitTestResultPrivate&);
    ~QWebHitTestResultPrivate();

    QWebElement elementForInnerNode() const;

    QPoint pos;
    QRect boundingRect;
    QWebElement enclosingBlock;
    QString title;
    QString linkText;
    QUrl linkUrl;
    QString linkTitle;
    QPointer<QObject> linkTargetFrame;
    QWebElement linkElement;
    QString alternateText;
    QUrl imageUrl;
    QUrl mediaUrl;
    QPixmap pixmap;
    bool isContentEditable;
    bool isContentSelected;
    bool isScrollBar;
    QPointer<QObject> frame;
private:
    WebCore::Node* innerNode;
    WebCore::Node* innerNonSharedNode;
    WebCore::Frame* webCoreFrame;
    friend class QWebFrameAdapter;
    friend class QWebPageAdapter;
};

class QWebFrameData {
public:
    QWebFrameData(WebCore::Page*, WebCore::Frame* parentFrame = 0, WebCore::HTMLFrameOwnerElement* = 0, const WTF::String& frameName = WTF::String());

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

class WEBKIT_EXPORTDATA QWebFrameAdapter {
public:
    enum ValueOwnership {
        QtOwnership,
        ScriptOwnership,
        AutoOwnership
    };

    enum RenderLayers {
        ContentsLayer = 0x10,
        ScrollBarLayer = 0x20,
        PanIconLayer = 0x40,

        AllLayers = 0xff
    };

    static QUrl ensureAbsoluteUrl(const QUrl&);

    QWebFrameAdapter();
    virtual ~QWebFrameAdapter();

    virtual QWebFrame* apiHandle() = 0;
    virtual QObject* handle() = 0;
    virtual void contentsSizeDidChange(const QSize&) = 0;
    virtual int scrollBarPolicy(Qt::Orientation) const = 0;
    virtual void emitUrlChanged() = 0;
    virtual void didStartProvisionalLoad() = 0;
    virtual void didClearWindowObject() = 0;
    virtual bool handleProgressFinished(QPoint*) = 0;
    virtual void emitInitialLayoutCompleted() = 0;
    virtual void emitIconChanged() = 0;
    virtual void emitLoadStarted(bool originatingLoad) = 0;
    virtual void emitLoadFinished(bool originatingLoad, bool ok) = 0;
    virtual QWebFrameAdapter* createChildFrame(QWebFrameData*) = 0;

    void load(const QNetworkRequest&, QNetworkAccessManager::Operation = QNetworkAccessManager::GetOperation, const QByteArray& body = QByteArray());
    bool hasView() const;
#if ENABLE(GESTURE_EVENTS)
    void handleGestureEvent(QGestureEventFacade*);
#endif
    QWebFrameAdapter* createFrame(QWebFrameData*);

    QVariant evaluateJavaScript(const QString& scriptSource, const QString& location);
    void addToJavaScriptWindowObject(const QString& name, QObject*, ValueOwnership);

    QString toHtml() const;
    QString toPlainText() const;

    void setContent(const QByteArray& data, const QString& mimeType, const QUrl& baseUrl);
    void setHtml(const QString& html, const QUrl& baseUrl);

    QMultiMap<QString, QString> metaData() const;

    QWebHitTestResultPrivate* hitTestContent(const QPoint&) const;
    QWebElement documentElement() const;
    QString title() const;
    void clearCoreFrame();
    QUrl baseUrl() const;
    QUrl coreFrameUrl() const;
    QUrl lastRequestedUrl() const;
    QWebSecurityOrigin securityOrigin() const;
    QString uniqueName() const;

    void renderRelativeCoords(QPainter*, int layers, const QRegion& clip);
    void renderFrameExtras(WebCore::GraphicsContext*, int layers, const QRegion& clip);
#if USE(ACCELERATED_COMPOSITING)
    void renderCompositedLayers(WebCore::GraphicsContext*, const WebCore::IntRect& clip);
#endif
#if USE(TILED_BACKING_STORE)
    void setTiledBackingStoreFrozen(bool);
    bool tiledBackingStoreFrozen() const;
    void setTiledBackingStoreContentsScale(float);
    bool renderFromTiledBackingStore(QPainter*, const QRegion& clip);
#endif

    // Called from QWebFrame as a private slot:
    void _q_orientationChanged();

    QList<QObject*> childFrames() const;
    bool hasFocus() const;
    void setFocus();

    void setScrollBarPolicy(Qt::Orientation, Qt::ScrollBarPolicy);
    void scrollToAnchor(const QString&);
    void scrollBy(int, int);
    void setScrollBarValue(Qt::Orientation, int);
    int scrollBarValue(Qt::Orientation) const;
    int scrollBarMaximum(Qt::Orientation) const;
    QRect scrollBarGeometry(Qt::Orientation) const;

    QPoint scrollPosition() const;
    QRect frameRect() const;
    QSize contentsSize() const;

    void setZoomFactor(qreal);
    void setTextSizeMultiplier(qreal);
    qreal zoomFactor() const;

    void updateBackgroundRecursively(const QColor&);

    void cancelLoad();

    // FrameView related functions
    QSize customLayoutSize() const;
    void setCustomLayoutSize(const QSize&);
    void setFixedVisibleContentRect(const QRect&);
    void setViewportSize(const QSize&);
    void setPaintsEntireContents(bool /*resizesToContents*/);
    void setDelegatesScrolling(bool /*resizesToContents*/);

    QWebPageAdapter* pageAdapter;

// protected:
    bool allowsScrolling;
    int marginWidth;
    int marginHeight;
    Qt::ScrollBarPolicy horizontalScrollBarPolicy;
    Qt::ScrollBarPolicy verticalScrollBarPolicy;
#if ENABLE(ORIENTATION_EVENTS) && HAVE(QTSENSORS)
    QOrientationSensor m_orientation;
#endif // ENABLE(ORIENTATION_EVENTS).

// private:
    void init(QWebPageAdapter*);
    void init(QWebPageAdapter*, QWebFrameData*);
    WebCore::Scrollbar* horizontalScrollBar() const;
    WebCore::Scrollbar* verticalScrollBar() const;

    WebCore::Frame *frame;
    WebCore::FrameLoaderClientQt *frameLoaderClient;
    QUrl url;

    static QWebFrameAdapter* kit(const WebCore::Frame*);

//    friend class ChromeClientQt;
};

#endif // QWebFrameAdapter_h
