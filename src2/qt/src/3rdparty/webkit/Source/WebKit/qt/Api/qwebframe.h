/*
    Copyright (C) 2008,2009 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef QWEBFRAME_H
#define QWEBFRAME_H

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>
#include <QtGui/qicon.h>
#include <QtScript/qscriptengine.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include "qwebkitglobal.h"

QT_BEGIN_NAMESPACE
class QRect;
class QPoint;
class QPainter;
class QPixmap;
class QMouseEvent;
class QWheelEvent;
class QNetworkRequest;
class QRegion;
class QPrinter;
QT_END_NAMESPACE

class QWebNetworkRequest;
class QWebFramePrivate;
class QWebPage;
class QWebHitTestResult;
class QWebHistoryItem;
class QWebSecurityOrigin;
class QWebElement;
class QWebElementCollection;
class QWebScriptWorld;

class DumpRenderTreeSupportQt;
namespace WebCore {
    class WidgetPrivate;
    class FrameLoaderClientQt;
    class ChromeClientQt;
    class PlatformLayerProxyQt;
}
class QWebFrameData;
class QWebHitTestResultPrivate;
class QWebFrame;

class QWEBKIT_EXPORT QWebHitTestResult {
public:
    QWebHitTestResult();
    QWebHitTestResult(const QWebHitTestResult &other);
    QWebHitTestResult &operator=(const QWebHitTestResult &other);
    ~QWebHitTestResult();

    bool isNull() const;

    QPoint pos() const;
    QRect boundingRect() const;
    QWebElement enclosingBlockElement() const;
    QString title() const;

    QString linkText() const;
    QUrl linkUrl() const;
    QUrl linkTitle() const;
    QWebFrame *linkTargetFrame() const;
    QWebElement linkElement() const;

    QString alternateText() const; // for img, area, input and applet

    QUrl imageUrl() const;
    QPixmap pixmap() const;

    bool isContentEditable() const;
    bool isContentSelected() const;

    QWebElement element() const;

    QWebFrame *frame() const;

private:
    QWebHitTestResult(QWebHitTestResultPrivate *priv);
    QWebHitTestResultPrivate *d;

    friend class QWebFrame;
    friend class QWebPagePrivate;
    friend class QWebPage;
};

class QWEBKIT_EXPORT QWebFrame : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal textSizeMultiplier READ textSizeMultiplier WRITE setTextSizeMultiplier DESIGNABLE false)
    Q_PROPERTY(qreal zoomFactor READ zoomFactor WRITE setZoomFactor)
    Q_PROPERTY(QString title READ title)
    Q_PROPERTY(QUrl url READ url WRITE setUrl)
    Q_PROPERTY(QUrl requestedUrl READ requestedUrl)
    Q_PROPERTY(QUrl baseUrl READ baseUrl)
    Q_PROPERTY(QIcon icon READ icon)
    Q_PROPERTY(QSize contentsSize READ contentsSize)
    Q_PROPERTY(QPoint scrollPosition READ scrollPosition WRITE setScrollPosition)
    Q_PROPERTY(bool focus READ hasFocus)
private:
    QWebFrame(QWebPage *parent, QWebFrameData *frameData);
    QWebFrame(QWebFrame *parent, QWebFrameData *frameData);
    ~QWebFrame();

public:
    QWebPage *page() const;

    void load(const QUrl &url);
    void load(const QNetworkRequest &request,
              QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
              const QByteArray &body = QByteArray());
    void setHtml(const QString &html, const QUrl &baseUrl = QUrl());
    void setContent(const QByteArray &data, const QString &mimeType = QString(), const QUrl &baseUrl = QUrl());

    void addToJavaScriptWindowObject(const QString &name, QObject *object);
    void addToJavaScriptWindowObject(const QString &name, QObject *object, QScriptEngine::ValueOwnership ownership);
    QString toHtml() const;
    QString toPlainText() const;
    QString renderTreeDump() const;

    QString title() const;
    void setUrl(const QUrl &url);
    QUrl url() const;
    QUrl requestedUrl() const;
    QUrl baseUrl() const;
    QIcon icon() const;
    QMultiMap<QString, QString> metaData() const;

    QString frameName() const;

    QWebFrame *parentFrame() const;
    QList<QWebFrame*> childFrames() const;

    Qt::ScrollBarPolicy scrollBarPolicy(Qt::Orientation orientation) const;
    void setScrollBarPolicy(Qt::Orientation orientation, Qt::ScrollBarPolicy policy);

    void setScrollBarValue(Qt::Orientation orientation, int value);
    int scrollBarValue(Qt::Orientation orientation) const;
    int scrollBarMinimum(Qt::Orientation orientation) const;
    int scrollBarMaximum(Qt::Orientation orientation) const;
    QRect scrollBarGeometry(Qt::Orientation orientation) const;

    void scroll(int, int);
    QPoint scrollPosition() const;
    void setScrollPosition(const QPoint &pos);

    void scrollToAnchor(const QString& anchor);

    enum RenderLayer {
        ContentsLayer = 0x10,
        ScrollBarLayer = 0x20,
        PanIconLayer = 0x40,

        AllLayers = 0xff
    };

    void render(QPainter*);
    void render(QPainter*, const QRegion& clip);
    void render(QPainter*, RenderLayer layer, const QRegion& clip = QRegion());

    void setTextSizeMultiplier(qreal factor);
    qreal textSizeMultiplier() const;

    qreal zoomFactor() const;
    void setZoomFactor(qreal factor);

    bool hasFocus() const;
    void setFocus();

    QPoint pos() const;
    QRect geometry() const;
    QSize contentsSize() const;

    QWebElement documentElement() const;
    QWebElementCollection findAllElements(const QString &selectorQuery) const;
    QWebElement findFirstElement(const QString &selectorQuery) const;

    QWebHitTestResult hitTestContent(const QPoint &pos) const;

    virtual bool event(QEvent *);

    QWebSecurityOrigin securityOrigin() const;

#ifndef QT_NO_PRINTER
    struct PrintCallback {
        /// height of header in points
        virtual qreal headerHeight() const = 0;
        /// height of footer in points
        virtual qreal footerHeight() const = 0;
        /// header contents (in HTML) on page @p page
        virtual QString header(int page, int numPages) = 0;
        /// footer contents (in HTML) on page @p page
        virtual QString footer(int page, int numPages) = 0;
    };
#endif

public Q_SLOTS:
    QVariant evaluateJavaScript(const QString& scriptSource, const QString& file = QString());
#ifndef QT_NO_PRINTER
    void print(QPrinter *printer) const;
    void print(QPrinter *printer, PrintCallback *callback) const;
#endif

Q_SIGNALS:
    void javaScriptWindowObjectCleared();

    void provisionalLoad();
    void titleChanged(const QString &title);
    void urlChanged(const QUrl &url);

    void initialLayoutCompleted();

    void iconChanged();

    void contentsSizeChanged(const QSize &size);

    void loadStarted();
    void loadFinished(bool ok);

    void pageChanged();

private:
    friend class QGraphicsWebView;
    friend class QWebPage;
    friend class QWebPagePrivate;
    friend class QWebFramePrivate;
    friend class DumpRenderTreeSupportQt;
    friend class WebCore::WidgetPrivate;
    friend class WebCore::FrameLoaderClientQt;
    friend class WebCore::ChromeClientQt;
    friend class WebCore::PlatformLayerProxyQt;
    QWebFramePrivate *d;
    Q_PRIVATE_SLOT(d, void _q_orientationChanged())
};

#endif
