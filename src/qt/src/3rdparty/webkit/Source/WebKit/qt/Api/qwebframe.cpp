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

#include "config.h"
#include "qwebframe.h"

#if USE(JSC)
#include "BridgeJSC.h"
#include "CallFrame.h"
#elif USE(V8)
#include "V8Binding.h"
#endif
#include "Document.h"
#include "DocumentLoader.h"
#include "DragData.h"
#include "Element.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoaderClientQt.h"
#include "FrameTree.h"
#include "FrameView.h"
#if USE(JSC)
#include "GCController.h"
#elif USE(V8)
#include "V8GCController.h"
#endif
#include "GraphicsContext.h"
#include "HTMLMetaElement.h"
#include "HitTestResult.h"
#include "HTTPParsers.h"
#include "IconDatabase.h"
#include "InspectorController.h"
#if USE(JSC)
#include "JSDOMBinding.h"
#include "JSDOMWindowBase.h"
#include "JSLock.h"
#include "JSObject.h"
#elif USE(V8)
#include "V8DOMWrapper.h"
#include "V8DOMWindowShell.h"
#endif
#include "NetworkingContext.h"
#include "NodeList.h"
#include "Page.h"
#include "PlatformMouseEvent.h"
#include "PlatformWheelEvent.h"
#include "PrintContext.h"
#if USE(JSC)
#include "PutPropertySlot.h"
#endif
#include "RenderLayer.h"
#include "RenderTreeAsText.h"
#include "RenderView.h"
#include "ResourceRequest.h"
#include "ScriptController.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "Scrollbar.h"
#include "Settings.h"
#include "SelectionController.h"
#include "SubstituteData.h"
#include "SVGSMILElement.h"
#include "TiledBackingStore.h"
#include "htmlediting.h"
#include "markup.h"
#if USE(JSC)
#include "qt_instance.h"
#include "qt_runtime.h"
#endif
#include "qwebelement.h"
#include "qwebframe_p.h"
#include "qwebpage.h"
#include "qwebpage_p.h"
#include "qwebsecurityorigin.h"
#include "qwebsecurityorigin_p.h"
#include "qwebscriptworld.h"
#include "qwebscriptworld_p.h"
#if USE(JSC)
#include "runtime_object.h"
#include "runtime_root.h"
#endif
#if USE(TEXTURE_MAPPER)
#include "texmap/TextureMapper.h"
#include "texmap/TextureMapperPlatformLayer.h"
#endif
#include "wtf/HashMap.h"
#include <QMultiMap>
#include <qdebug.h>
#include <qevent.h>
#include <qfileinfo.h>
#include <qpainter.h>
#include <qprinter.h>
#include <qregion.h>
#include <qnetworkrequest.h>

using namespace WebCore;

// from text/qfont.cpp
QT_BEGIN_NAMESPACE
extern Q_GUI_EXPORT int qt_defaultDpi();
QT_END_NAMESPACE

bool QWEBKIT_EXPORT qtwebkit_webframe_scrollOverflow(QWebFrame* qFrame, int dx, int dy, const QPoint& pos)
{
    WebCore::Frame* frame = QWebFramePrivate::core(qFrame);
    if (!frame || !frame->document() || !frame->view() || !frame->eventHandler())
        return false;

    QPoint contentsPos = frame->view()->windowToContents(pos);
    Node* node = frame->document()->elementFromPoint(contentsPos.x(), contentsPos.y());
    if (!node)
        return false;

    RenderObject* renderer = node->renderer();
    if (!renderer)
        return false;

    if (renderer->isListBox())
        return false;

    RenderLayer* renderLayer = renderer->enclosingLayer();
    if (!renderLayer)
        return false;

    bool scrolledHorizontal = false;
    bool scrolledVertical = false;

    do {
        if (dx > 0)
            scrolledHorizontal = renderLayer->scroll(ScrollRight, ScrollByPixel, dx);
        else if (dx < 0)
            scrolledHorizontal = renderLayer->scroll(ScrollLeft, ScrollByPixel, qAbs(dx));

        if (dy > 0)
            scrolledVertical = renderLayer->scroll(ScrollDown, ScrollByPixel, dy);
        else if (dy < 0)
            scrolledVertical = renderLayer->scroll(ScrollUp, ScrollByPixel, qAbs(dy));

        if (scrolledHorizontal || scrolledVertical)
            return true;

        renderLayer = renderLayer->parent();
    } while (renderLayer);

    return false;
}


/*!
  \internal
  Scrolls nested frames starting at this frame, \a dx pixels to the right 
  and \a dy pixels downward. Both \a dx and \a dy may be negative. First attempts
  to scroll elements with CSS overflow at position pos, followed by this frame. If this 
  frame doesn't scroll, attempts to scroll the parent
*/
void QWEBKIT_EXPORT qtwebkit_webframe_scrollRecursively(QWebFrame* qFrame, int dx, int dy, const QPoint& pos)
{
    if (!qFrame)
        return;

    if (qtwebkit_webframe_scrollOverflow(qFrame, dx, dy, pos))
        return;

    bool scrollHorizontal = false;
    bool scrollVertical = false;

    do {
        if (dx > 0)  // scroll right
            scrollHorizontal = qFrame->scrollBarValue(Qt::Horizontal) < qFrame->scrollBarMaximum(Qt::Horizontal);
        else if (dx < 0)  // scroll left
            scrollHorizontal = qFrame->scrollBarValue(Qt::Horizontal) > qFrame->scrollBarMinimum(Qt::Horizontal);

        if (dy > 0)  // scroll down
            scrollVertical = qFrame->scrollBarValue(Qt::Vertical) < qFrame->scrollBarMaximum(Qt::Vertical);
        else if (dy < 0) //scroll up
            scrollVertical = qFrame->scrollBarValue(Qt::Vertical) > qFrame->scrollBarMinimum(Qt::Vertical);

        if (scrollHorizontal || scrollVertical) {
            qFrame->scroll(dx, dy);
            return;
        }

        qFrame = qFrame->parentFrame();
    } while (qFrame);
}

static inline ResourceRequestCachePolicy cacheLoadControlToCachePolicy(uint cacheLoadControl)
{
    switch (cacheLoadControl) {
    case QNetworkRequest::AlwaysNetwork:
        return WebCore::ReloadIgnoringCacheData;
    case QNetworkRequest::PreferCache:
        return WebCore::ReturnCacheDataElseLoad;
    case QNetworkRequest::AlwaysCache:
        return WebCore::ReturnCacheDataDontLoad;
    default:
        break;
    }
    return WebCore::UseProtocolCachePolicy;
}

QWebFrameData::QWebFrameData(WebCore::Page* parentPage, WebCore::Frame* parentFrame,
                             WebCore::HTMLFrameOwnerElement* ownerFrameElement,
                             const WTF::String& frameName)
    : name(frameName)
    , ownerElement(ownerFrameElement)
    , page(parentPage)
    , allowsScrolling(true)
    , marginWidth(0)
    , marginHeight(0)
{
    frameLoaderClient = new FrameLoaderClientQt();
    frame = Frame::create(page, ownerElement, frameLoaderClient);

    // FIXME: All of the below should probably be moved over into WebCore
    frame->tree()->setName(name);
    if (parentFrame)
        parentFrame->tree()->appendChild(frame);
}

void QWebFramePrivate::init(QWebFrame *qframe, QWebFrameData *frameData)
{
    q = qframe;

    allowsScrolling = frameData->allowsScrolling;
    marginWidth = frameData->marginWidth;
    marginHeight = frameData->marginHeight;
    frame = frameData->frame.get();
    frameLoaderClient = frameData->frameLoaderClient;
    frameLoaderClient->setFrame(qframe, frame);

    frame->init();
}

void QWebFramePrivate::setPage(QWebPage* newPage)
{
    if (page == newPage)
        return;

    // The QWebFrame is created as a child of QWebPage or a parent QWebFrame.
    // That adds it to QObject's internal children list and ensures it will be
    // deleted when parent QWebPage is deleted. Reparent if needed.
    if (q->parent() == qobject_cast<QObject*>(page))
        q->setParent(newPage);

    page = newPage;
    emit q->pageChanged();
}

WebCore::Scrollbar* QWebFramePrivate::horizontalScrollBar() const
{
    if (!frame->view())
        return 0;
    return frame->view()->horizontalScrollbar();
}

WebCore::Scrollbar* QWebFramePrivate::verticalScrollBar() const
{
    if (!frame->view())
        return 0;
    return frame->view()->verticalScrollbar();
}

#if ENABLE(TILED_BACKING_STORE)
void QWebFramePrivate::renderFromTiledBackingStore(GraphicsContext* context, const QRegion& clip)
{
    ASSERT(frame->tiledBackingStore());

    if (!frame->view() || !frame->contentRenderer())
        return;

    QVector<QRect> vector = clip.rects();
    if (vector.isEmpty())
        return;

    QPainter* painter = context->platformContext();

    WebCore::FrameView* view = frame->view();
    
    int scrollX = view->scrollX();
    int scrollY = view->scrollY();
    context->translate(-scrollX, -scrollY);

    for (int i = 0; i < vector.size(); ++i) {
        const QRect& clipRect = vector.at(i);

        painter->save();
        
        QRect rect = clipRect.translated(scrollX, scrollY);
        painter->setClipRect(rect, Qt::IntersectClip);

        frame->tiledBackingStore()->paint(context, rect);

        painter->restore();
    }

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
    renderCompositedLayers(context, IntRect(clip.boundingRect()));
    renderFrameExtras(context, QFlags<QWebFrame::RenderLayer>(QWebFrame::ScrollBarLayer) | QWebFrame::PanIconLayer, clip);
#endif
}
#endif

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
void QWebFramePrivate::renderCompositedLayers(GraphicsContext* context, const IntRect& clip)
{
    if (!rootGraphicsLayer)
        return;

    textureMapper->setGraphicsContext(context);
    textureMapper->setImageInterpolationQuality(context->imageInterpolationQuality());
    textureMapper->setTextDrawingMode(context->textDrawingMode());
    QPainter* painter = context->platformContext();
    FrameView* view = frame->view();
    painter->save();
    painter->beginNativePainting();
    TextureMapperContentLayer::PaintOptions options;
    options.visibleRect = clip;
    options.targetRect = view->frameRect();
    options.viewportSize = view->size();
    options.opacity = painter->opacity();
    rootGraphicsLayer->paint(textureMapper.get(), options);
    painter->endNativePainting();
    painter->restore();
}
#endif

void QWebFramePrivate::renderRelativeCoords(GraphicsContext* context, QFlags<QWebFrame::RenderLayer> layers, const QRegion& clip)
{
    if (!frame->view() || !frame->contentRenderer())
        return;

    QVector<QRect> vector = clip.rects();
    if (vector.isEmpty())
        return;

    QPainter* painter = context->platformContext();

    WebCore::FrameView* view = frame->view();
    view->updateLayoutAndStyleIfNeededRecursive();

    if (layers & QWebFrame::ContentsLayer) {
        for (int i = 0; i < vector.size(); ++i) {
            const QRect& clipRect = vector.at(i);

            QRect rect = clipRect.intersected(view->frameRect());

            context->save();
            painter->setClipRect(clipRect, Qt::IntersectClip);

            int x = view->x();
            int y = view->y();

            int scrollX = view->scrollX();
            int scrollY = view->scrollY();

            context->translate(x, y);
            rect.translate(-x, -y);
            context->translate(-scrollX, -scrollY);
            rect.translate(scrollX, scrollY);
            context->clip(view->visibleContentRect());

            view->paintContents(context, rect);

            context->restore();
        }
#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
        renderCompositedLayers(context, IntRect(clip.boundingRect()));
#endif
    }
    renderFrameExtras(context, layers, clip);
#if ENABLE(INSPECTOR)
    if (frame->page()->inspectorController()->highlightedNode()) {
        context->save();
        frame->page()->inspectorController()->drawNodeHighlight(*context);
        context->restore();
    }
#endif
}

void QWebFramePrivate::renderFrameExtras(GraphicsContext* context, QFlags<QWebFrame::RenderLayer> layers, const QRegion& clip)
{
    if (!(layers & (QWebFrame::PanIconLayer | QWebFrame::ScrollBarLayer)))
        return;
    QPainter* painter = context->platformContext();
    WebCore::FrameView* view = frame->view();
    QVector<QRect> vector = clip.rects();
    for (int i = 0; i < vector.size(); ++i) {
        const QRect& clipRect = vector.at(i);

        QRect intersectedRect = clipRect.intersected(view->frameRect());

        painter->save();
        painter->setClipRect(clipRect, Qt::IntersectClip);

        int x = view->x();
        int y = view->y();

        if (layers & QWebFrame::ScrollBarLayer
            && !view->scrollbarsSuppressed()
            && (view->horizontalScrollbar() || view->verticalScrollbar())) {

            QRect rect = intersectedRect;
            context->translate(x, y);
            rect.translate(-x, -y);
            view->paintScrollbars(context, rect);
            context->translate(-x, -y);
        }

#if ENABLE(PAN_SCROLLING)
        if (layers & QWebFrame::PanIconLayer)
            view->paintPanScrollIcon(context);
#endif

        painter->restore();
    }
}

void QWebFramePrivate::emitUrlChanged()
{
    url = frame->document()->url();
    emit q->urlChanged(url);
}

void QWebFramePrivate::_q_orientationChanged()
{
#if ENABLE(ORIENTATION_EVENTS) && ENABLE(DEVICE_ORIENTATION)
    int orientation;
    WebCore::Frame* frame = core(q);

    switch (m_orientation.reading()->orientation()) {
    case QtMobility::QOrientationReading::TopUp:
        orientation = 0;
        break;
    case QtMobility::QOrientationReading::TopDown:
        orientation = 180;
        break;
    case QtMobility::QOrientationReading::LeftUp:
        orientation = -90;
        break;
    case QtMobility::QOrientationReading::RightUp:
        orientation = 90;
        break;
    case QtMobility::QOrientationReading::FaceUp:
    case QtMobility::QOrientationReading::FaceDown:
        // WebCore unable to handle it
    default:
        return;
    }
    frame->sendOrientationChangeEvent(orientation);
#endif
}
/*!
    \class QWebFrame
    \since 4.4
    \brief The QWebFrame class represents a frame in a web page.

    \inmodule QtWebKit

    QWebFrame represents a frame inside a web page. Each QWebPage
    object contains at least one frame, the main frame, obtained using
    QWebPage::mainFrame(). Additional frames will be created for HTML
    \c{<frame>} or \c{<iframe>} elements.

    A frame can be loaded using load() or setUrl(). Alternatively, if you have
    the HTML content readily available, you can use setHtml() instead.

    The page() function returns a pointer to the web page object. See
    \l{QWebView}{Elements of QWebView} for an explanation of how web
    frames are related to a web page and web view.

    The QWebFrame class also offers methods to retrieve both the URL currently
    loaded by the frame (see url()) as well as the URL originally requested
    to be loaded (see requestedUrl()). These methods make possible the retrieval
    of the URL before and after a DNS resolution or a redirection occurs during
    the load process. The requestedUrl() also matches to the URL added to the
    frame history (\l{QWebHistory}) if load is successful.

    The title of an HTML frame can be accessed with the title() property.
    Additionally, a frame may also specify an icon, which can be accessed
    using the icon() property. If the title or the icon changes, the
    corresponding titleChanged() and iconChanged() signals will be emitted.
    The zoomFactor() property can be used to change the overall size
    of the content displayed in the frame.

    QWebFrame objects are created and controlled by the web page. You
    can connect to the web page's \l{QWebPage::}{frameCreated()} signal
    to be notified when a new frame is created.

    There are multiple ways to programmatically examine the contents of a frame.
    The hitTestContent() function can be used to find elements by coordinate.
    For access to the underlying DOM tree, there is documentElement(),
    findAllElements() and findFirstElement().

    A QWebFrame can be printed onto a QPrinter using the print() function.
    This function is marked as a slot and can be conveniently connected to
    \l{QPrintPreviewDialog}'s \l{QPrintPreviewDialog::}{paintRequested()}
    signal.

    \sa QWebPage
*/

/*!
    \enum QWebFrame::RenderLayer

    This enum describes the layers available for rendering using \l{QWebFrame::}{render()}.
    The layers can be OR-ed together from the following list:

    \value ContentsLayer The web content of the frame
    \value ScrollBarLayer The scrollbars of the frame
    \value PanIconLayer The icon used when panning the frame

    \value AllLayers Includes all the above layers
*/

QWebFrame::QWebFrame(QWebPage *parent, QWebFrameData *frameData)
    : QObject(parent)
    , d(new QWebFramePrivate)
{
    d->page = parent;
    d->init(this, frameData);

    if (!frameData->url.isEmpty()) {
        WebCore::ResourceRequest request(frameData->url, frameData->referrer);
        d->frame->loader()->load(request, frameData->name, false);
    }
#if ENABLE(ORIENTATION_EVENTS) && ENABLE(DEVICE_ORIENTATION)
    connect(&d->m_orientation, SIGNAL(readingChanged()), this, SLOT(_q_orientationChanged()));
    d->m_orientation.start();
#endif
}

QWebFrame::QWebFrame(QWebFrame *parent, QWebFrameData *frameData)
    : QObject(parent)
    , d(new QWebFramePrivate)
{
    d->page = parent->d->page;
    d->init(this, frameData);
#if ENABLE(ORIENTATION_EVENTS) && ENABLE(DEVICE_ORIENTATION)
    connect(&d->m_orientation, SIGNAL(readingChanged()), this, SLOT(_q_orientationChanged()));
    d->m_orientation.start();
#endif
}

QWebFrame::~QWebFrame()
{
    if (d->frame && d->frame->loader() && d->frame->loader()->client())
        static_cast<FrameLoaderClientQt*>(d->frame->loader()->client())->m_webFrame = 0;

    delete d;
}

/*!
    Make \a object available under \a name from within the frame's JavaScript
    context. The \a object will be inserted as a child of the frame's window
    object.

    Qt properties will be exposed as JavaScript properties and slots as
    JavaScript methods.
    The interaction between C++ and JavaScript is explained in the documentation of the \l{The QtWebKit Bridge}{QtWebKit bridge}.

    If you want to ensure that your QObjects remain accessible after loading a
    new URL, you should add them in a slot connected to the
    javaScriptWindowObjectCleared() signal.

    If Javascript is not enabled for this page, then this method does nothing.

    The \a object will never be explicitly deleted by QtWebKit.
*/
void QWebFrame::addToJavaScriptWindowObject(const QString &name, QObject *object)
{
    addToJavaScriptWindowObject(name, object, QScriptEngine::QtOwnership);
}

/*!
    \fn void QWebFrame::addToJavaScriptWindowObject(const QString &name, QObject *object, QScriptEngine::ValueOwnership own)
    \overload

    Make \a object available under \a name from within the frame's JavaScript
    context. The \a object will be inserted as a child of the frame's window
    object.

    Qt properties will be exposed as JavaScript properties and slots as
    JavaScript methods.
    The interaction between C++ and JavaScript is explained in the documentation of the \l{The QtWebKit Bridge}{QtWebKit bridge}.

    If you want to ensure that your QObjects remain accessible after loading a
    new URL, you should add them in a slot connected to the
    javaScriptWindowObjectCleared() signal.

    If Javascript is not enabled for this page, then this method does nothing.

    The ownership of \a object is specified using \a own.
*/
void QWebFrame::addToJavaScriptWindowObject(const QString &name, QObject *object, QScriptEngine::ValueOwnership ownership)
{
    if (!page()->settings()->testAttribute(QWebSettings::JavascriptEnabled))
        return;
#if USE(JSC)
    JSC::JSLock lock(JSC::SilenceAssertionsOnly);
    JSDOMWindow* window = toJSDOMWindow(d->frame, mainThreadNormalWorld());
    JSC::Bindings::RootObject* root;
    if (ownership == QScriptEngine::QtOwnership)
        root = d->frame->script()->cacheableBindingRootObject();
    else
        root = d->frame->script()->bindingRootObject();

    if (!window) {
        qDebug() << "Warning: couldn't get window object";
        return;
    }
    if (!root) {
        qDebug() << "Warning: couldn't get root object";
        return;
    }

    JSC::ExecState* exec = window->globalExec();

    JSC::JSObject* runtimeObject =
            JSC::Bindings::QtInstance::getQtInstance(object, root, ownership)->createRuntimeObject(exec);

    JSC::PutPropertySlot slot;
    window->put(exec, JSC::Identifier(exec, reinterpret_cast_ptr<const UChar*>(name.constData()), name.length()), runtimeObject, slot);
#elif USE(V8)
    QScriptEngine* engine = d->frame->script()->qtScriptEngine();
    if (!engine)
        return;
    QScriptValue v = engine->newQObject(object, ownership);
    engine->globalObject().property("window").setProperty(name, v);
#endif
}

/*!
    Returns the frame's content as HTML, enclosed in HTML and BODY tags.

    \sa setHtml(), toPlainText()
*/
QString QWebFrame::toHtml() const
{
    if (!d->frame->document())
        return QString();
    return createMarkup(d->frame->document());
}

/*!
    Returns the content of this frame converted to plain text, completely
    stripped of all HTML formatting.

    \sa toHtml()
*/
QString QWebFrame::toPlainText() const
{
    if (d->frame->view() && d->frame->view()->layoutPending())
        d->frame->view()->layout();

    Element *documentElement = d->frame->document()->documentElement();
    if (documentElement)
        return documentElement->innerText();
    return QString();
}

/*!
    Returns a dump of the rendering tree. This is mainly useful for debugging
    html.
*/
QString QWebFrame::renderTreeDump() const
{
    if (d->frame->view() && d->frame->view()->layoutPending())
        d->frame->view()->layout();

    return externalRepresentation(d->frame);
}

/*!
    \property QWebFrame::title
    \brief the title of the frame as defined by the HTML &lt;title&gt; element

    \sa titleChanged()
*/

QString QWebFrame::title() const
{
    if (d->frame->document())
        return d->frame->loader()->documentLoader()->title().string();
    return QString();
}

/*!
    \since 4.5
    \brief Returns the meta data in this frame as a QMultiMap

    The meta data consists of the name and content attributes of the
    of the \c{<meta>} tags in the HTML document.

    For example:

    \code
    <html>
        <head>
            <meta name="description" content="This document is a tutorial about Qt development">
            <meta name="keywords" content="Qt, WebKit, Programming">
        </head>
        ...
    </html>
    \endcode

    Given the above HTML code the metaData() function will return a map with two entries:
    \table
    \header \o Key
            \o Value
    \row    \o "description"
            \o "This document is a tutorial about Qt development"
    \row    \o "keywords"
            \o "Qt, WebKit, Programming"
    \endtable

    This function returns a multi map to support multiple meta tags with the same attribute name.
*/
QMultiMap<QString, QString> QWebFrame::metaData() const
{
    if (!d->frame->document())
       return QMap<QString, QString>();

    QMultiMap<QString, QString> map;
    Document* doc = d->frame->document();
    RefPtr<NodeList> list = doc->getElementsByTagName("meta");
    unsigned len = list->length();
    for (unsigned i = 0; i < len; i++) {
        HTMLMetaElement* meta = static_cast<HTMLMetaElement*>(list->item(i));
        map.insert(meta->name(), meta->content());
    }
    return map;
}

static inline void clearCoreFrame(WebCore::Frame* frame)
{
    WebCore::DocumentLoader* documentLoader = frame->loader()->activeDocumentLoader();
    Q_ASSERT(documentLoader);
    documentLoader->writer()->begin();
    documentLoader->writer()->end();
}

static inline bool isCoreFrameClear(WebCore::Frame* frame)
{
    return frame->document()->url().isEmpty();
}

static inline QUrl ensureAbsoluteUrl(const QUrl &url)
{
    if (!url.isValid() || !url.isRelative())
        return url;

    // This contains the URL with absolute path but without 
    // the query and the fragment part.
    QUrl baseUrl = QUrl::fromLocalFile(QFileInfo(url.toLocalFile()).absoluteFilePath()); 

    // The path is removed so the query and the fragment parts are there.
    QString pathRemoved = url.toString(QUrl::RemovePath);
    QUrl toResolve(pathRemoved);
    
    return baseUrl.resolved(toResolve);
}

/*!
    \property QWebFrame::url
    \brief the url of the frame currently viewed

    Setting this property clears the view and loads the URL.

    By default, this property contains an empty, invalid URL.

    \sa urlChanged()
*/

void QWebFrame::setUrl(const QUrl &url)
{
    clearCoreFrame(d->frame);
    const QUrl absolute = ensureAbsoluteUrl(url);
    d->url = absolute;
    load(absolute);
}

QUrl QWebFrame::url() const
{
    return d->url;
}

/*!
    \since 4.6
    \property QWebFrame::requestedUrl

    The URL requested to loaded by the frame currently viewed. The URL may differ from
    the one returned by url() if a DNS resolution or a redirection occurs.

    \sa url(), setUrl()
*/
QUrl QWebFrame::requestedUrl() const
{
    return d->frameLoaderClient->lastRequestedUrl();
}
/*!
    \since 4.6
    \property QWebFrame::baseUrl
    \brief the base URL of the frame, can be used to resolve relative URLs
    \since 4.6
*/

QUrl QWebFrame::baseUrl() const
{
    if (isCoreFrameClear(d->frame))
        return QUrl(d->url).resolved(QUrl());
    return d->frame->document()->baseURL();
}

/*!
    \property QWebFrame::icon
    \brief the icon associated with this frame

    \sa iconChanged(), QWebSettings::iconForUrl()
*/

QIcon QWebFrame::icon() const
{
    return QWebSettings::iconForUrl(d->frame->document()->url());
}

/*!
  The name of this frame as defined by the parent frame.
*/
QString QWebFrame::frameName() const
{
    return d->frame->tree()->uniqueName();
}

/*!
  The web page that contains this frame.

  \sa pageChanged()
*/
QWebPage *QWebFrame::page() const
{
    return d->page;
}

/*!
  Loads \a url into this frame.

  \note The view remains the same until enough data has arrived to display the new \a url.

  \sa setUrl(), setHtml(), setContent()
*/
void QWebFrame::load(const QUrl &url)
{
    // The load() overload ensures that the url is absolute.
    load(QNetworkRequest(url));
}

/*!
  Loads a network request, \a req, into this frame, using the method specified in \a
  operation.

  \a body is optional and is only used for POST operations.

  \note The view remains the same until enough data has arrived to display the new content.

  \sa setUrl()
*/
void QWebFrame::load(const QNetworkRequest &req,
                     QNetworkAccessManager::Operation operation,
                     const QByteArray &body)
{
    if (d->parentFrame())
        d->page->d->insideOpenCall = true;

    QUrl url = ensureAbsoluteUrl(req.url());

    WebCore::ResourceRequest request(url);

    switch (operation) {
        case QNetworkAccessManager::HeadOperation:
            request.setHTTPMethod("HEAD");
            break;
        case QNetworkAccessManager::GetOperation:
            request.setHTTPMethod("GET");
            break;
        case QNetworkAccessManager::PutOperation:
            request.setHTTPMethod("PUT");
            break;
        case QNetworkAccessManager::PostOperation:
            request.setHTTPMethod("POST");
            break;
        case QNetworkAccessManager::DeleteOperation:
            request.setHTTPMethod("DELETE");
            break;
        case QNetworkAccessManager::CustomOperation:
            request.setHTTPMethod(req.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray().constData());
            break;
        case QNetworkAccessManager::UnknownOperation:
            // eh?
            break;
    }

    QVariant cacheLoad = req.attribute(QNetworkRequest::CacheLoadControlAttribute);
    if (cacheLoad.isValid()) {
        bool ok;
        uint cacheLoadValue = cacheLoad.toUInt(&ok);
        if (ok)
            request.setCachePolicy(cacheLoadControlToCachePolicy(cacheLoadValue));
    }

    QList<QByteArray> httpHeaders = req.rawHeaderList();
    for (int i = 0; i < httpHeaders.size(); ++i) {
        const QByteArray &headerName = httpHeaders.at(i);
        request.addHTTPHeaderField(QString::fromLatin1(headerName), QString::fromLatin1(req.rawHeader(headerName)));
    }

    if (!body.isEmpty())
        request.setHTTPBody(WebCore::FormData::create(body.constData(), body.size()));

    d->frame->loader()->load(request, false);

    if (d->parentFrame())
        d->page->d->insideOpenCall = false;
}

/*!
  Sets the content of this frame to \a html. \a baseUrl is optional and used to resolve relative
  URLs in the document, such as referenced images or stylesheets.

  The \a html is loaded immediately; external objects are loaded asynchronously.

  If a script in the \a html runs longer than the default script timeout (currently 10 seconds),
  for example due to being blocked by a modal JavaScript alert dialog, this method will return
  as soon as possible after the timeout and any subsequent \a html will be loaded asynchronously.

  When using this method WebKit assumes that external resources such as JavaScript programs or style
  sheets are encoded in UTF-8 unless otherwise specified. For example, the encoding of an external
  script can be specified through the charset attribute of the HTML script tag. It is also possible
  for the encoding to be specified by web server.

  This is a convenience function equivalent to setContent(html, "text/html", baseUrl).

  \note This method will not affect session or global history for the frame.

  \warning This function works only for HTML, for other mime types (i.e. XHTML, SVG)
  setContent() should be used instead.

  \sa toHtml(), setContent(), load()
*/
void QWebFrame::setHtml(const QString &html, const QUrl &baseUrl)
{
    KURL kurl(baseUrl);
    WebCore::ResourceRequest request(kurl);
    const QByteArray utf8 = html.toUtf8();
    WTF::RefPtr<WebCore::SharedBuffer> data = WebCore::SharedBuffer::create(utf8.constData(), utf8.length());
    WebCore::SubstituteData substituteData(data, WTF::String("text/html"), WTF::String("utf-8"), KURL());
    d->frame->loader()->load(request, substituteData, false);
}

/*!
  Sets the content of this frame to the specified content \a data. If the \a mimeType argument
  is empty it is currently assumed that the content is HTML but in future versions we may introduce
  auto-detection.

  External objects referenced in the content are located relative to \a baseUrl.

  The \a data is loaded immediately; external objects are loaded asynchronously.

  \note This method will not affect session or global history for the frame.

  \sa toHtml(), setHtml()
*/
void QWebFrame::setContent(const QByteArray &data, const QString &mimeType, const QUrl &baseUrl)
{
    KURL kurl(baseUrl);
    WebCore::ResourceRequest request(kurl);
    WTF::RefPtr<WebCore::SharedBuffer> buffer = WebCore::SharedBuffer::create(data.constData(), data.length());
    QString actualMimeType;
    WTF::String encoding;
    if (mimeType.isEmpty())
        actualMimeType = QLatin1String("text/html");
    else {
        actualMimeType = extractMIMETypeFromMediaType(mimeType);
        encoding = extractCharsetFromMediaType(mimeType);
    }
    WebCore::SubstituteData substituteData(buffer, WTF::String(actualMimeType), encoding, KURL());
    d->frame->loader()->load(request, substituteData, false);
}

/*!
  Returns the parent frame of this frame, or 0 if the frame is the web pages
  main frame.

  This is equivalent to qobject_cast<QWebFrame*>(frame->parent()).

  \sa childFrames()
*/
QWebFrame *QWebFrame::parentFrame() const
{
    return d->parentFrame();
}

/*!
  Returns a list of all frames that are direct children of this frame.

  \sa parentFrame()
*/
QList<QWebFrame*> QWebFrame::childFrames() const
{
    QList<QWebFrame*> rc;
    if (d->frame) {
        FrameTree *tree = d->frame->tree();
        for (Frame *child = tree->firstChild(); child; child = child->tree()->nextSibling()) {
            FrameLoader *loader = child->loader();
            QWebFrame* webFrame = qobject_cast<QWebFrame*>(loader->networkingContext()->originatingObject());
            if (webFrame)
                rc.append(webFrame);
        }

    }
    return rc;
}

/*!
    Returns the scrollbar policy for the scrollbar defined by \a orientation.
*/
Qt::ScrollBarPolicy QWebFrame::scrollBarPolicy(Qt::Orientation orientation) const
{
    if (orientation == Qt::Horizontal)
        return d->horizontalScrollBarPolicy;
    return d->verticalScrollBarPolicy;
}

/*!
    Sets the scrollbar policy for the scrollbar defined by \a orientation to \a policy.
*/
void QWebFrame::setScrollBarPolicy(Qt::Orientation orientation, Qt::ScrollBarPolicy policy)
{
    Q_ASSERT((int)ScrollbarAuto == (int)Qt::ScrollBarAsNeeded);
    Q_ASSERT((int)ScrollbarAlwaysOff == (int)Qt::ScrollBarAlwaysOff);
    Q_ASSERT((int)ScrollbarAlwaysOn == (int)Qt::ScrollBarAlwaysOn);

    if (orientation == Qt::Horizontal) {
        d->horizontalScrollBarPolicy = policy;
        if (d->frame->view()) {
            d->frame->view()->setHorizontalScrollbarMode((ScrollbarMode)policy, policy != Qt::ScrollBarAsNeeded /* lock */);
            d->frame->view()->updateCanHaveScrollbars();
        }
    } else {
        d->verticalScrollBarPolicy = policy;
        if (d->frame->view()) {
            d->frame->view()->setVerticalScrollbarMode((ScrollbarMode)policy, policy != Qt::ScrollBarAsNeeded /* lock */);
            d->frame->view()->updateCanHaveScrollbars();
        }
    }
}

/*!
  Sets the current \a value for the scrollbar with orientation \a orientation.

  The scrollbar forces the \a value to be within the legal range: minimum <= value <= maximum.

  Changing the value also updates the thumb position.

  \sa scrollBarMinimum(), scrollBarMaximum()
*/
void QWebFrame::setScrollBarValue(Qt::Orientation orientation, int value)
{
    Scrollbar *sb;
    sb = (orientation == Qt::Horizontal) ? d->horizontalScrollBar() : d->verticalScrollBar();
    if (sb) {
        if (value < 0)
            value = 0;
        else if (value > scrollBarMaximum(orientation))
            value = scrollBarMaximum(orientation);
        sb->scrollableArea()->scrollToOffsetWithoutAnimation(orientation == Qt::Horizontal ? HorizontalScrollbar : VerticalScrollbar, value);
    }
}

/*!
  Returns the current value for the scrollbar with orientation \a orientation, or 0
  if no scrollbar is found for \a orientation.

  \sa scrollBarMinimum(), scrollBarMaximum()
*/
int QWebFrame::scrollBarValue(Qt::Orientation orientation) const
{
    Scrollbar *sb;
    sb = (orientation == Qt::Horizontal) ? d->horizontalScrollBar() : d->verticalScrollBar();
    if (sb)
        return sb->value();
    return 0;
}

/*!
  Returns the maximum value for the scrollbar with orientation \a orientation, or 0
  if no scrollbar is found for \a orientation.

  \sa scrollBarMinimum()
*/
int QWebFrame::scrollBarMaximum(Qt::Orientation orientation) const
{
    Scrollbar *sb;
    sb = (orientation == Qt::Horizontal) ? d->horizontalScrollBar() : d->verticalScrollBar();
    if (sb)
        return sb->maximum();
    return 0;
}

/*!
  Returns the minimum value for the scrollbar with orientation \a orientation.

  The minimum value is always 0.

  \sa scrollBarMaximum()
*/
int QWebFrame::scrollBarMinimum(Qt::Orientation orientation) const
{
    Q_UNUSED(orientation)
    return 0;
}

/*!
  \since 4.6
  Returns the geometry for the scrollbar with orientation \a orientation.

  If the scrollbar does not exist an empty rect is returned.
*/
QRect QWebFrame::scrollBarGeometry(Qt::Orientation orientation) const
{
    Scrollbar *sb;
    sb = (orientation == Qt::Horizontal) ? d->horizontalScrollBar() : d->verticalScrollBar();
    if (sb)
        return sb->frameRect();
    return QRect();
}

/*!
  \since 4.5
  Scrolls the frame \a dx pixels to the right and \a dy pixels downward. Both
  \a dx and \a dy may be negative.

  \sa QWebFrame::scrollPosition
*/

void QWebFrame::scroll(int dx, int dy)
{
    if (!d->frame->view())
        return;

    d->frame->view()->scrollBy(IntSize(dx, dy));
}

/*!
  \property QWebFrame::scrollPosition
  \since 4.5
  \brief the position the frame is currently scrolled to.
*/

QPoint QWebFrame::scrollPosition() const
{
    if (!d->frame->view())
        return QPoint(0, 0);

    IntSize ofs = d->frame->view()->scrollOffset();
    return QPoint(ofs.width(), ofs.height());
}

void QWebFrame::setScrollPosition(const QPoint &pos)
{
    QPoint current = scrollPosition();
    int dx = pos.x() - current.x();
    int dy = pos.y() - current.y();
    scroll(dx, dy);
}

/*!
  \since 4.7
  Scrolls the frame to the given \a anchor name.
*/
void QWebFrame::scrollToAnchor(const QString& anchor)
{
    FrameView *view = d->frame->view();
    if (view)
        view->scrollToAnchor(anchor);
}

/*!
  \since 4.6
  Render the \a layer of the frame using \a painter clipping to \a clip.

  \sa print()
*/

void QWebFrame::render(QPainter* painter, RenderLayer layer, const QRegion& clip)
{
    GraphicsContext context(painter);
    if (context.paintingDisabled() && !context.updatingControlTints())
        return;

    if (!clip.isEmpty())
        d->renderRelativeCoords(&context, layer, clip);
    else if (d->frame->view())
        d->renderRelativeCoords(&context, layer, QRegion(d->frame->view()->frameRect()));
}

/*!
  Render the frame into \a painter clipping to \a clip.
*/
void QWebFrame::render(QPainter* painter, const QRegion& clip)
{
    GraphicsContext context(painter);
    if (context.paintingDisabled() && !context.updatingControlTints())
        return;

    d->renderRelativeCoords(&context, AllLayers, clip);
}

/*!
  Render the frame into \a painter.
*/
void QWebFrame::render(QPainter* painter)
{
    if (!d->frame->view())
        return;

    GraphicsContext context(painter);
    if (context.paintingDisabled() && !context.updatingControlTints())
        return;

    d->renderRelativeCoords(&context, AllLayers, QRegion(d->frame->view()->frameRect()));
}

/*!
    \property QWebFrame::textSizeMultiplier
    \brief the scaling factor for all text in the frame
    \obsolete

    Use setZoomFactor instead, in combination with the ZoomTextOnly attribute in
    QWebSettings.

    \note Setting this property also enables the ZoomTextOnly attribute in
    QWebSettings.
*/

/*!
    Sets the value of the multiplier used to scale the text in a Web frame to
    the \a factor specified.
*/
void QWebFrame::setTextSizeMultiplier(qreal factor)
{
    page()->settings()->setAttribute(QWebSettings::ZoomTextOnly, true);

    d->frame->setPageAndTextZoomFactors(1, factor);
}

/*!
    Returns the value of the multiplier used to scale the text in a Web frame.
*/
qreal QWebFrame::textSizeMultiplier() const
{
    return page()->settings()->testAttribute(QWebSettings::ZoomTextOnly) ? d->frame->textZoomFactor() : d->frame->pageZoomFactor();
}

/*!
    \property QWebFrame::zoomFactor
    \since 4.5
    \brief the zoom factor for the frame
*/

void QWebFrame::setZoomFactor(qreal factor)
{
    if (page()->settings()->testAttribute(QWebSettings::ZoomTextOnly))
        d->frame->setTextZoomFactor(factor);
    else
        d->frame->setPageZoomFactor(factor);
}

qreal QWebFrame::zoomFactor() const
{
    return page()->settings()->testAttribute(QWebSettings::ZoomTextOnly) ? d->frame->textZoomFactor() : d->frame->pageZoomFactor();
}

/*!
    \property QWebFrame::focus
    \since 4.6

    Returns true if this frame has keyboard input focus; otherwise, returns false.
*/
bool QWebFrame::hasFocus() const
{
    WebCore::Frame* ff = d->frame->page()->focusController()->focusedFrame();
    return ff && QWebFramePrivate::kit(ff) == this;
}

/*!
    \since 4.6

    Gives keyboard input focus to this frame.
*/
void QWebFrame::setFocus()
{
    QWebFramePrivate::core(this)->page()->focusController()->setFocusedFrame(QWebFramePrivate::core(this));
}

/*!
    Returns the position of the frame relative to it's parent frame.
*/
QPoint QWebFrame::pos() const
{
    if (!d->frame->view())
        return QPoint();

    return d->frame->view()->frameRect().location();
}

/*!
    Return the geometry of the frame relative to it's parent frame.
*/
QRect QWebFrame::geometry() const
{
    if (!d->frame->view())
        return QRect();
    return d->frame->view()->frameRect();
}

/*!
    \property QWebFrame::contentsSize
    \brief the size of the contents in this frame

    \sa contentsSizeChanged()
*/
QSize QWebFrame::contentsSize() const
{
    FrameView *view = d->frame->view();
    if (!view)
        return QSize();
    return QSize(view->contentsWidth(), view->contentsHeight());
}

/*!
    \since 4.6

    Returns the document element of this frame.

    The document element provides access to the entire structured
    content of the frame.
*/
QWebElement QWebFrame::documentElement() const
{
    WebCore::Document *doc = d->frame->document();
    if (!doc)
        return QWebElement();
    return QWebElement(doc->documentElement());
}

/*!
    \since 4.6
    Returns a new list of elements matching the given CSS selector \a selectorQuery.
    If there are no matching elements, an empty list is returned.

    \l{http://www.w3.org/TR/REC-CSS2/selector.html#q1}{Standard CSS2 selector} syntax is
    used for the query.

    \sa QWebElement::findAll()
*/
QWebElementCollection QWebFrame::findAllElements(const QString &selectorQuery) const
{
    return documentElement().findAll(selectorQuery);
}

/*!
    \since 4.6
    Returns the first element in the frame's document that matches the
    given CSS selector \a selectorQuery. If there is no matching element, a
    null element is returned.

    \l{http://www.w3.org/TR/REC-CSS2/selector.html#q1}{Standard CSS2 selector} syntax is
    used for the query.

    \sa QWebElement::findFirst()
*/
QWebElement QWebFrame::findFirstElement(const QString &selectorQuery) const
{
    return documentElement().findFirst(selectorQuery);
}

/*!
    Performs a hit test on the frame contents at the given position \a pos and returns the hit test result.
*/
QWebHitTestResult QWebFrame::hitTestContent(const QPoint &pos) const
{
    if (!d->frame->view() || !d->frame->contentRenderer())
        return QWebHitTestResult();

    HitTestResult result = d->frame->eventHandler()->hitTestResultAtPoint(d->frame->view()->windowToContents(pos), /*allowShadowContent*/ false, /*ignoreClipping*/ true);

    if (result.scrollbar())
        return QWebHitTestResult();

    return QWebHitTestResult(new QWebHitTestResultPrivate(result));
}

/*! \reimp
*/
bool QWebFrame::event(QEvent *e)
{
    return QObject::event(e);
}

#ifndef QT_NO_PRINTER
/*!
    Prints the frame to the given \a printer.

    \sa render()
*/
void QWebFrame::print(QPrinter *printer) const
{
    QPainter painter;
    if (!painter.begin(printer))
        return;

    const qreal zoomFactorX = (qreal)printer->logicalDpiX() / qt_defaultDpi();
    const qreal zoomFactorY = (qreal)printer->logicalDpiY() / qt_defaultDpi();

    PrintContext printContext(d->frame);
    float pageHeight = 0;

    QRect qprinterRect = printer->pageRect();

    IntRect pageRect(0, 0,
                     int(qprinterRect.width() / zoomFactorX),
                     int(qprinterRect.height() / zoomFactorY));

    printContext.begin(pageRect.width());

    printContext.computePageRects(pageRect, /* headerHeight */ 0, /* footerHeight */ 0, /* userScaleFactor */ 1.0, pageHeight);

    int docCopies;
    int pageCopies;
    if (printer->collateCopies()) {
        docCopies = 1;
        pageCopies = printer->numCopies();
    } else {
        docCopies = printer->numCopies();
        pageCopies = 1;
    }

    int fromPage = printer->fromPage();
    int toPage = printer->toPage();
    bool ascending = true;

    if (fromPage == 0 && toPage == 0) {
        fromPage = 1;
        toPage = printContext.pageCount();
    }
    // paranoia check
    fromPage = qMax(1, fromPage);
    toPage = qMin(static_cast<int>(printContext.pageCount()), toPage);
    if (toPage < fromPage) {
        // if the user entered a page range outside the actual number
        // of printable pages, just return
        return;
    }

    if (printer->pageOrder() == QPrinter::LastPageFirst) {
        int tmp = fromPage;
        fromPage = toPage;
        toPage = tmp;
        ascending = false;
    }

    painter.scale(zoomFactorX, zoomFactorY);
    GraphicsContext ctx(&painter);

    for (int i = 0; i < docCopies; ++i) {
        int page = fromPage;
        while (true) {
            for (int j = 0; j < pageCopies; ++j) {
                if (printer->printerState() == QPrinter::Aborted
                    || printer->printerState() == QPrinter::Error) {
                    printContext.end();
                    return;
                }
                printContext.spoolPage(ctx, page - 1, pageRect.width());
                if (j < pageCopies - 1)
                    printer->newPage();
            }

            if (page == toPage)
                break;

            if (ascending)
                ++page;
            else
                --page;

            printer->newPage();
        }

        if ( i < docCopies - 1)
            printer->newPage();
    }

    printContext.end();
}
#endif // QT_NO_PRINTER

/*!
    Evaluates the JavaScript defined by \a scriptSource using this frame as context
    and returns the result of the last executed statement.

    \sa addToJavaScriptWindowObject(), javaScriptWindowObjectCleared()
*/
QVariant QWebFrame::evaluateJavaScript(const QString& scriptSource)
{
    ScriptController *proxy = d->frame->script();
    QVariant rc;
    if (proxy) {
#if USE(JSC)
        int distance = 0;
        JSC::JSValue v = d->frame->script()->executeScript(ScriptSourceCode(scriptSource)).jsValue();

        rc = JSC::Bindings::convertValueToQVariant(proxy->globalObject(mainThreadNormalWorld())->globalExec(), v, QMetaType::Void, &distance);
#elif USE(V8)
        QScriptEngine* engine = d->frame->script()->qtScriptEngine();
        if (!engine)
            return rc;
        rc = engine->evaluate(scriptSource).toVariant();
#endif
    }
    return rc;
}

/*!
    \since 4.5

    Returns the frame's security origin.
*/
QWebSecurityOrigin QWebFrame::securityOrigin() const
{
    QWebFrame* that = const_cast<QWebFrame*>(this);
    QWebSecurityOriginPrivate* priv = new QWebSecurityOriginPrivate(QWebFramePrivate::core(that)->document()->securityOrigin());
    return QWebSecurityOrigin(priv);
}

WebCore::Frame* QWebFramePrivate::core(const QWebFrame* webFrame)
{
    return webFrame->d->frame;
}

QWebFrame* QWebFramePrivate::kit(const WebCore::Frame* coreFrame)
{
    return qobject_cast<QWebFrame*>(coreFrame->loader()->networkingContext()->originatingObject());
}


/*!
    \fn void QWebFrame::javaScriptWindowObjectCleared()

    This signal is emitted whenever the global window object of the JavaScript
    environment is cleared, e.g., before starting a new load.

    If you intend to add QObjects to a QWebFrame using
    addToJavaScriptWindowObject(), you should add them in a slot connected
    to this signal. This ensures that your objects remain accessible when
    loading new URLs.
*/

/*!
    \fn void QWebFrame::provisionalLoad()
    \internal
*/

/*!
    \fn void QWebFrame::titleChanged(const QString &title)

    This signal is emitted whenever the title of the frame changes.
    The \a title string specifies the new title.

    \sa title()
*/

/*!
    \fn void QWebFrame::urlChanged(const QUrl &url)

    This signal is emitted with the URL of the frame when the frame's title is
    received. The new URL is specified by \a url.

    \sa url()
*/

/*!
    \fn void QWebFrame::initialLayoutCompleted()

    This signal is emitted when the frame is laid out the first time.
    This is the first time you will see contents displayed on the frame.

    \note A frame can be laid out multiple times.
*/

/*!
  \fn void QWebFrame::iconChanged()

  This signal is emitted when the icon ("favicon") associated with the frame
  has been loaded.

  \sa icon()
*/

/*!
  \fn void QWebFrame::contentsSizeChanged(const QSize &size)
  \since 4.6

  This signal is emitted when the frame's contents size changes
  to \a size.

  \sa contentsSize()
*/

/*!
    \fn void QWebFrame::loadStarted()
    \since 4.6

    This signal is emitted when a new load of this frame is started.

    \sa loadFinished()
*/

/*!
    \fn void QWebFrame::loadFinished(bool ok)
    \since 4.6

    This signal is emitted when a load of this frame is finished.
    \a ok will indicate whether the load was successful or any error occurred.

    \sa loadStarted()
*/

/*!
    \fn void QWebFrame::pageChanged()
    \since 4.7

    This signal is emitted when this frame has been moved to a different QWebPage.

    \sa page()
*/

/*!
    \class QWebHitTestResult
    \since 4.4
    \brief The QWebHitTestResult class provides information about the web
    page content after a hit test.

    \inmodule QtWebKit

    QWebHitTestResult is returned by QWebFrame::hitTestContent() to provide
    information about the content of the web page at the specified position.
*/

/*!
    \internal
*/
QWebHitTestResult::QWebHitTestResult(QWebHitTestResultPrivate *priv)
    : d(priv)
{
}

QWebHitTestResultPrivate::QWebHitTestResultPrivate(const WebCore::HitTestResult &hitTest)
    : isContentEditable(false)
    , isContentSelected(false)
    , isScrollBar(false)
{
    if (!hitTest.innerNode())
        return;
    pos = hitTest.point();
    WebCore::TextDirection dir;
    title = hitTest.title(dir);
    linkText = hitTest.textContent();
    linkUrl = hitTest.absoluteLinkURL();
    linkTitle = hitTest.titleDisplayString();
    alternateText = hitTest.altDisplayString();
    imageUrl = hitTest.absoluteImageURL();
    innerNode = hitTest.innerNode();
    innerNonSharedNode = hitTest.innerNonSharedNode();
    boundingRect = innerNonSharedNode ? innerNonSharedNode->renderer()->absoluteBoundingBoxRect(true) : IntRect();
    WebCore::Image *img = hitTest.image();
    if (img) {
        QPixmap *pix = img->nativeImageForCurrentFrame();
        if (pix)
            pixmap = *pix;
    }
    WebCore::Frame *wframe = hitTest.targetFrame();
    if (wframe)
        linkTargetFrame = QWebFramePrivate::kit(wframe);
    linkElement = QWebElement(hitTest.URLElement());

    isContentEditable = hitTest.isContentEditable();
    isContentSelected = hitTest.isSelected();
    isScrollBar = hitTest.scrollbar();

    if (innerNonSharedNode && innerNonSharedNode->document()
        && innerNonSharedNode->document()->frame())
        frame = QWebFramePrivate::kit(innerNonSharedNode->document()->frame());

    enclosingBlock = QWebElement(WebCore::enclosingBlock(innerNode.get()));
}

/*!
    Constructs a null hit test result.
*/
QWebHitTestResult::QWebHitTestResult()
    : d(0)
{
}

/*!
    Constructs a hit test result from \a other.
*/
QWebHitTestResult::QWebHitTestResult(const QWebHitTestResult &other)
    : d(0)
{
    if (other.d)
        d = new QWebHitTestResultPrivate(*other.d);
}

/*!
    Assigns the \a other hit test result to this.
*/
QWebHitTestResult &QWebHitTestResult::operator=(const QWebHitTestResult &other)
{
    if (this != &other) {
        if (other.d) {
            if (!d)
                d = new QWebHitTestResultPrivate;
            *d = *other.d;
        } else {
            delete d;
            d = 0;
        }
    }
    return *this;
}

/*!
    Destructor.
*/
QWebHitTestResult::~QWebHitTestResult()
{
    delete d;
}

/*!
    Returns true if the hit test result is null; otherwise returns false.
*/
bool QWebHitTestResult::isNull() const
{
    return !d;
}

/*!
    Returns the position where the hit test occured.
*/
QPoint QWebHitTestResult::pos() const
{
    if (!d)
        return QPoint();
    return d->pos;
}

/*!
    \since 4.5
    Returns the bounding rect of the element.
*/
QRect QWebHitTestResult::boundingRect() const
{
    if (!d)
        return QRect();
    return d->boundingRect;
}

/*!
    \since 4.6
    Returns the block element that encloses the element hit.

    A block element is an element that is rendered using the
    CSS "block" style. This includes for example text
    paragraphs.
*/
QWebElement QWebHitTestResult::enclosingBlockElement() const
{
    if (!d)
        return QWebElement();
    return d->enclosingBlock;
}

/*!
    Returns the title of the nearest enclosing HTML element.
*/
QString QWebHitTestResult::title() const
{
    if (!d)
        return QString();
    return d->title;
}

/*!
    Returns the text of the link.
*/
QString QWebHitTestResult::linkText() const
{
    if (!d)
        return QString();
    return d->linkText;
}

/*!
    Returns the url to which the link points to.
*/
QUrl QWebHitTestResult::linkUrl() const
{
    if (!d)
        return QUrl();
    return d->linkUrl;
}

/*!
    Returns the title of the link.
*/
QUrl QWebHitTestResult::linkTitle() const
{
    if (!d)
        return QUrl();
    return d->linkTitle;
}

/*!
  \since 4.6
  Returns the element that represents the link.

  \sa linkTargetFrame()
*/
QWebElement QWebHitTestResult::linkElement() const
{
    if (!d)
        return QWebElement();
    return d->linkElement;
}

/*!
    Returns the frame that will load the link if it is activated.

    \sa linkElement()
*/
QWebFrame *QWebHitTestResult::linkTargetFrame() const
{
    if (!d)
        return 0;
    return d->linkTargetFrame;
}

/*!
    Returns the alternate text of the element. This corresponds to the HTML alt attribute.
*/
QString QWebHitTestResult::alternateText() const
{
    if (!d)
        return QString();
    return d->alternateText;
}

/*!
    Returns the url of the image.
*/
QUrl QWebHitTestResult::imageUrl() const
{
    if (!d)
        return QUrl();
    return d->imageUrl;
}

/*!
    Returns a QPixmap containing the image. A null pixmap is returned if the
    element being tested is not an image.
*/
QPixmap QWebHitTestResult::pixmap() const
{
    if (!d)
        return QPixmap();
    return d->pixmap;
}

/*!
    Returns true if the content is editable by the user; otherwise returns false.
*/
bool QWebHitTestResult::isContentEditable() const
{
    if (!d)
        return false;
    return d->isContentEditable;
}

/*!
    Returns true if the content tested is part of the selection; otherwise returns false.
*/
bool QWebHitTestResult::isContentSelected() const
{
    if (!d)
        return false;
    return d->isContentSelected;
}

/*!
    \since 4.6
    Returns the underlying DOM element as QWebElement.
*/
QWebElement QWebHitTestResult::element() const
{
    if (!d || !d->innerNonSharedNode || !d->innerNonSharedNode->isElementNode())
        return QWebElement();

    return QWebElement(static_cast<WebCore::Element*>(d->innerNonSharedNode.get()));
}

/*!
    Returns the frame the hit test was executed in.
*/
QWebFrame *QWebHitTestResult::frame() const
{
    if (!d)
        return 0;
    return d->frame;
}

#include "moc_qwebframe.cpp"
