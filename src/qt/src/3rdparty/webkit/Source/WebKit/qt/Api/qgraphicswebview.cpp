/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2009 Girish Ramakrishnan <girish@forwardbias.in>

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
#include "qgraphicswebview.h"

#if !defined(QT_NO_GRAPHICSVIEW)

#include "qwebframe.h"
#include "qwebframe_p.h"
#include "qwebpage.h"
#include "qwebpage_p.h"
#include "PageClientQt.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "IntRect.h"
#include "TiledBackingStore.h"
#include <QtCore/qmetaobject.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qtimer.h>
#include <QtGui/qapplication.h>
#include <QtGui/qgraphicsscene.h>
#include <QtGui/qgraphicssceneevent.h>
#include <QtGui/qgraphicsview.h>
#include <QtGui/qpixmapcache.h>
#include <QtGui/qscrollbar.h>
#include <QtGui/qstyleoption.h>
#include <QtGui/qinputcontext.h>
#if defined(Q_WS_X11)
#include <QX11Info>
#endif
#include <Settings.h>

using namespace WebCore;

class QGraphicsWebViewPrivate {
public:
    QGraphicsWebViewPrivate(QGraphicsWebView* parent)
        : q(parent)
        , page(0)
        , resizesToContents(false)
        , renderHints(QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform) {}

    virtual ~QGraphicsWebViewPrivate();

    void syncLayers();

    void updateResizesToContentsForPage();

    void detachCurrentPage();

    void _q_doLoadFinished(bool success);
    void _q_contentsSizeChanged(const QSize&);
    void _q_scaleChanged();

    void _q_pageDestroyed();

    QGraphicsWebView* q;
    QWebPage* page;
    bool resizesToContents;
    QPainter::RenderHints renderHints;

    QGraphicsItemOverlay* overlay() const
    {
        if (!page || !page->d->client)
            return 0;
        return pageClient()->overlay;
    }

    PageClientQGraphicsWidget* pageClient() const
    {
        return static_cast<WebCore::PageClientQGraphicsWidget*> (page->d->client.get());
    } 
};

QGraphicsWebViewPrivate::~QGraphicsWebViewPrivate()
{
    detachCurrentPage();
}

void QGraphicsWebViewPrivate::syncLayers()
{
#if USE(ACCELERATED_COMPOSITING)
    pageClient()->syncLayers();
#endif
}

void QGraphicsWebViewPrivate::_q_doLoadFinished(bool success)
{
    // If the page had no title, still make sure it gets the signal
    if (q->title().isEmpty())
        emit q->urlChanged(q->url());

    emit q->loadFinished(success);
}

void QGraphicsWebViewPrivate::_q_pageDestroyed()
{
    page = 0;
    q->setPage(0);
}

void QGraphicsWebViewPrivate::updateResizesToContentsForPage()
{
    ASSERT(page);
    pageClient()->viewResizesToContents = resizesToContents;
    if (resizesToContents) {
        // resizes to contents mode requires preferred contents size to be set
        if (!page->preferredContentsSize().isValid())
            page->setPreferredContentsSize(QSize(960, 800));

        QObject::connect(page->mainFrame(), SIGNAL(contentsSizeChanged(QSize)),
            q, SLOT(_q_contentsSizeChanged(const QSize&)), Qt::UniqueConnection);
    } else {
        QObject::disconnect(page->mainFrame(), SIGNAL(contentsSizeChanged(QSize)),
                         q, SLOT(_q_contentsSizeChanged(const QSize&)));
    }
    page->d->page->mainFrame()->view()->setPaintsEntireContents(resizesToContents);
    page->d->page->mainFrame()->view()->setDelegatesScrolling(resizesToContents);
}

void QGraphicsWebViewPrivate::_q_contentsSizeChanged(const QSize& size)
{
    if (!resizesToContents)
        return;
    q->setGeometry(QRectF(q->geometry().topLeft(), size));
}

void QGraphicsWebViewPrivate::_q_scaleChanged()
{
#if ENABLE(TILED_BACKING_STORE)
    if (!page)
        return;
    pageClient()->updateTiledBackingStoreScale();
#endif
}

/*!
    \class QGraphicsWebView
    \brief The QGraphicsWebView class allows Web content to be added to a GraphicsView.
    \since 4.6

    An instance of this class renders Web content from a URL or supplied as data, using
    features of the QtWebKit module.

    If the width and height of the item are not set, they will default to 800 and 600,
    respectively. If the Web page contents is larger than that, scrollbars will be shown
    if not disabled explicitly.

    \section1 Browser Features

    Many of the functions, signals and properties provided by QWebView are also available
    for this item, making it simple to adapt existing code to use QGraphicsWebView instead
    of QWebView.

    The item uses a QWebPage object to perform the rendering of Web content, and this can
    be obtained with the page() function, enabling the document itself to be accessed and
    modified.

    As with QWebView, the item records the browsing history using a QWebHistory object,
    accessible using the history() function. The QWebSettings object that defines the
    configuration of the browser can be obtained with the settings() function, enabling
    features like plugin support to be customized for each item.

    \sa QWebView, QGraphicsTextItem
*/

/*!
    \fn void QGraphicsWebView::titleChanged(const QString &title)

    This signal is emitted whenever the \a title of the main frame changes.

    \sa title()
*/

/*!
    \fn void QGraphicsWebView::urlChanged(const QUrl &url)

    This signal is emitted when the \a url of the view changes.

    \sa url(), load()
*/

/*!
    \fn void QGraphicsWebView::iconChanged()

    This signal is emitted whenever the icon of the page is loaded or changes.

    In order for icons to be loaded, you will need to set an icon database path
    using QWebSettings::setIconDatabasePath().

    \sa icon(), QWebSettings::setIconDatabasePath()
*/

/*!
    \fn void QGraphicsWebView::loadStarted()

    This signal is emitted when a new load of the page is started.

    \sa loadProgress(), loadFinished()
*/

/*!
    \fn void QGraphicsWebView::loadFinished(bool ok)

    This signal is emitted when a load of the page is finished.
    \a ok will indicate whether the load was successful or any error occurred.

    \sa loadStarted()
*/

/*!
    Constructs an empty QGraphicsWebView with parent \a parent.

    \sa load()
*/
QGraphicsWebView::QGraphicsWebView(QGraphicsItem* parent)
    : QGraphicsWidget(parent)
    , d(new QGraphicsWebViewPrivate(this))
{
    setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
    setAcceptDrops(true);
    setAcceptHoverEvents(true);
    setAcceptTouchEvents(true);
    setFocusPolicy(Qt::StrongFocus);
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
#if ENABLE(TILED_BACKING_STORE)
    QObject::connect(this, SIGNAL(scaleChanged()), this, SLOT(_q_scaleChanged()));
#endif
}

/*!
    Destroys the item.
*/
QGraphicsWebView::~QGraphicsWebView()
{
    delete d;
}

/*!
    Returns a pointer to the underlying web page.

    \sa setPage()
*/
QWebPage* QGraphicsWebView::page() const
{
    if (!d->page) {
        QGraphicsWebView* that = const_cast<QGraphicsWebView*>(this);
        QWebPage* page = new QWebPage(that);

        // Default to not having a background, in the case
        // the page doesn't provide one.
        QPalette palette = QApplication::palette();
        palette.setBrush(QPalette::Base, QColor::fromRgbF(0, 0, 0, 0));
        page->setPalette(palette);

        that->setPage(page);
    }

    return d->page;
}

/*! \reimp
*/
void QGraphicsWebView::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
    QPainter::RenderHints oldHints = painter->renderHints();
    painter->setRenderHints(oldHints | d->renderHints);
#if ENABLE(TILED_BACKING_STORE)
    if (WebCore::TiledBackingStore* backingStore = QWebFramePrivate::core(page()->mainFrame())->tiledBackingStore()) {
        // FIXME: We should set the backing store viewport earlier than in paint
        backingStore->adjustVisibleRect();
        // QWebFrame::render is a public API, bypass it for tiled rendering so behavior does not need to change.
        WebCore::GraphicsContext context(painter); 
        page()->mainFrame()->d->renderFromTiledBackingStore(&context, option->exposedRect.toAlignedRect());
        painter->setRenderHints(oldHints);
        return;
    } 
#endif
#if USE(ACCELERATED_COMPOSITING) && !USE(TEXTURE_MAPPER)
    page()->mainFrame()->render(painter, d->overlay() ? QWebFrame::ContentsLayer : QWebFrame::AllLayers, option->exposedRect.toAlignedRect());
#else
    page()->mainFrame()->render(painter, QWebFrame::AllLayers, option->exposedRect.toRect());
#endif
    painter->setRenderHints(oldHints);
}

/*! \reimp
*/
bool QGraphicsWebView::sceneEvent(QEvent* event)
{
    // Re-implemented in order to allows fixing event-related bugs in patch releases.

    if (d->page && (event->type() == QEvent::TouchBegin
                || event->type() == QEvent::TouchEnd
                || event->type() == QEvent::TouchUpdate)) {
        d->page->event(event);

        // Always return true so that we'll receive also TouchUpdate and TouchEnd events
        return true;
    }

    return QGraphicsWidget::sceneEvent(event);
}

/*! \reimp
*/
QVariant QGraphicsWebView::itemChange(GraphicsItemChange change, const QVariant& value)
{
    switch (change) {
    // Differently from QWebView, it is interesting to QGraphicsWebView to handle
    // post mouse cursor change notifications. Reason: 'ItemCursorChange' is sent
    // as the first action in QGraphicsItem::setCursor implementation, and at that
    // item widget's cursor has not been effectively changed yet.
    // After cursor is properly set (at 'ItemCursorHasChanged' emission time), we
    // fire 'CursorChange'.
    case ItemCursorChange:
        return value;
    case ItemCursorHasChanged: {
            QEvent event(QEvent::CursorChange);
            QApplication::sendEvent(this, &event);
            return value;
        }
    default:
        break;
    }

    return QGraphicsWidget::itemChange(change, value);
}

/*! \reimp
*/
QSizeF QGraphicsWebView::sizeHint(Qt::SizeHint which, const QSizeF& constraint) const
{
    if (which == Qt::PreferredSize)
        return QSizeF(800, 600); // ###
    return QGraphicsWidget::sizeHint(which, constraint);
}

/*! \reimp
*/
QVariant QGraphicsWebView::inputMethodQuery(Qt::InputMethodQuery query) const
{
    if (d->page)
        return d->page->inputMethodQuery(query);
    return QVariant();
}

/*!
    \property QGraphicsWebView::renderHints
    \since 4.8
    \brief the default render hints for the view

    These hints are used to initialize QPainter before painting the Web page.

    QPainter::TextAntialiasing and QPainter::SmoothPixmapTransform are enabled by default and will be
    used to render the item in addition of what has been set on the painter given by QGraphicsScene.

    \note This property is not available on Symbian. However, the getter and
    setter functions can still be used directly.

    \sa QPainter::renderHints()
*/

/*!
    \since 4.8
    Returns the render hints used by the view to render content.

    \sa QPainter::renderHints()
*/
QPainter::RenderHints QGraphicsWebView::renderHints() const
{
    return d->renderHints;
}

/*!
    \since 4.8
    Sets the render hints used by the view to the specified \a hints.

    \sa QPainter::setRenderHints()
*/
void QGraphicsWebView::setRenderHints(QPainter::RenderHints hints)
{
    if (hints == d->renderHints)
        return;
    d->renderHints = hints;
    update();
}

/*!
    \since 4.8
    If \a enabled is true, enables the specified render \a hint; otherwise
    disables it.

    \sa renderHints, QPainter::renderHints()
*/
void QGraphicsWebView::setRenderHint(QPainter::RenderHint hint, bool enabled)
{
    QPainter::RenderHints oldHints = d->renderHints;
    if (enabled)
        d->renderHints |= hint;
    else
        d->renderHints &= ~hint;
    if (oldHints != d->renderHints)
        update();
}

/*! \reimp
*/
bool QGraphicsWebView::event(QEvent* event)
{
    // Re-implemented in order to allows fixing event-related bugs in patch releases.

    if (d->page) {
        if (event->type() == QEvent::PaletteChange)
            d->page->setPalette(palette());
#ifndef QT_NO_CONTEXTMENU
        if (event->type() == QEvent::GraphicsSceneContextMenu) {
            if (!isEnabled())
                return false;

            QGraphicsSceneContextMenuEvent* ev = static_cast<QGraphicsSceneContextMenuEvent*>(event);
            QContextMenuEvent fakeEvent(QContextMenuEvent::Reason(ev->reason()), ev->pos().toPoint());
            if (d->page->swallowContextMenuEvent(&fakeEvent)) {
                event->accept();
                return true;
            }
            d->page->updatePositionDependentActions(fakeEvent.pos());
        } else
#endif // QT_NO_CONTEXTMENU
        {
#ifndef QT_NO_CURSOR
            if (event->type() == QEvent::CursorChange) {
                // An unsetCursor will set the cursor to Qt::ArrowCursor.
                // Thus this cursor change might be a QWidget::unsetCursor()
                // If this is not the case and it came from WebCore, the
                // QWebPageClient already has set its cursor internally
                // to Qt::ArrowCursor, so updating the cursor is always
                // right, as it falls back to the last cursor set by
                // WebCore.
                // FIXME: Add a QEvent::CursorUnset or similar to Qt.
                if (cursor().shape() == Qt::ArrowCursor)
                    d->page->d->client->resetCursor();
            }
#endif
        }
    }
    return QGraphicsWidget::event(event);
}

void QGraphicsWebViewPrivate::detachCurrentPage()
{
    if (!page)
        return;

    page->d->view.clear();
    page->d->client = nullptr;

    // if the page was created by us, we own it and need to
    // destroy it as well.

    if (page->parent() == q)
        delete page;
    else
        page->disconnect(q);

    page = 0;
}

/*!
    Makes \a page the new web page of the web graphicsitem.

    The parent QObject of the provided page remains the owner
    of the object. If the current document is a child of the web
    view, it will be deleted.

    \sa page()
*/
void QGraphicsWebView::setPage(QWebPage* page)
{
    if (d->page == page)
        return;

    d->detachCurrentPage();
    d->page = page;

    if (!d->page)
        return;

    d->page->d->client = new PageClientQGraphicsWidget(this, page); // set the page client

    if (d->overlay())
        d->overlay()->prepareGraphicsItemGeometryChange();

    QSize size = geometry().size().toSize();
    page->setViewportSize(size);

    if (d->resizesToContents)
        d->updateResizesToContentsForPage();

    QWebFrame* mainFrame = d->page->mainFrame();

    connect(mainFrame, SIGNAL(titleChanged(QString)),
            this, SIGNAL(titleChanged(QString)));
    connect(mainFrame, SIGNAL(iconChanged()),
            this, SIGNAL(iconChanged()));
    connect(mainFrame, SIGNAL(urlChanged(QUrl)),
            this, SIGNAL(urlChanged(QUrl)));
    connect(d->page, SIGNAL(loadStarted()),
            this, SIGNAL(loadStarted()));
    connect(d->page, SIGNAL(loadProgress(int)),
            this, SIGNAL(loadProgress(int)));
    connect(d->page, SIGNAL(loadFinished(bool)),
            this, SLOT(_q_doLoadFinished(bool)));
    connect(d->page, SIGNAL(statusBarMessage(QString)),
            this, SIGNAL(statusBarMessage(QString)));
    connect(d->page, SIGNAL(linkClicked(QUrl)),
            this, SIGNAL(linkClicked(QUrl)));
    connect(d->page, SIGNAL(destroyed()),
            this, SLOT(_q_pageDestroyed()));
#if !defined(QT_NO_IM) && (defined(Q_WS_X11) || defined(Q_WS_QWS) || defined(Q_OS_SYMBIAN))
    connect(d->page, SIGNAL(microFocusChanged()),
            this, SLOT(updateMicroFocus()));
#endif
}

/*!
    \property QGraphicsWebView::url
    \brief the url of the web page currently viewed

    Setting this property clears the view and loads the URL.

    By default, this property contains an empty, invalid URL.

    \sa load(), urlChanged()
*/

void QGraphicsWebView::setUrl(const QUrl &url)
{
    page()->mainFrame()->setUrl(url);
}

QUrl QGraphicsWebView::url() const
{
    if (d->page)
        return d->page->mainFrame()->url();

    return QUrl();
}

/*!
    \property QGraphicsWebView::title
    \brief the title of the web page currently viewed

    By default, this property contains an empty string.

    \sa titleChanged()
*/
QString QGraphicsWebView::title() const
{
    if (d->page)
        return d->page->mainFrame()->title();

    return QString();
}

/*!
    \property QGraphicsWebView::icon
    \brief the icon associated with the web page currently viewed

    By default, this property contains a null icon.

    \sa iconChanged(), QWebSettings::iconForUrl()
*/
QIcon QGraphicsWebView::icon() const
{
    if (d->page)
        return d->page->mainFrame()->icon();

    return QIcon();
}

/*!
    \property QGraphicsWebView::zoomFactor
    \brief the zoom factor for the view
*/

void QGraphicsWebView::setZoomFactor(qreal factor)
{
    if (factor == page()->mainFrame()->zoomFactor())
        return;

    page()->mainFrame()->setZoomFactor(factor);
}

qreal QGraphicsWebView::zoomFactor() const
{
    return page()->mainFrame()->zoomFactor();
}

/*! \reimp
*/
void QGraphicsWebView::updateGeometry()
{
    if (d->overlay())
        d->overlay()->prepareGraphicsItemGeometryChange();

    QGraphicsWidget::updateGeometry();

    if (!d->page)
        return;

    QSize size = geometry().size().toSize();
    d->page->setViewportSize(size);
}

/*! \reimp
*/
void QGraphicsWebView::setGeometry(const QRectF& rect)
{
    QGraphicsWidget::setGeometry(rect);

    if (d->overlay())
        d->overlay()->prepareGraphicsItemGeometryChange();

    if (!d->page)
        return;

    // NOTE: call geometry() as setGeometry ensures that
    // the geometry is within legal bounds (minimumSize, maximumSize)
    QSize size = geometry().size().toSize();
    d->page->setViewportSize(size);
}

/*!
    Convenience slot that stops loading the document.

    \sa reload(), loadFinished()
*/
void QGraphicsWebView::stop()
{
    if (d->page)
        d->page->triggerAction(QWebPage::Stop);
}

/*!
    Convenience slot that loads the previous document in the list of documents
    built by navigating links. Does nothing if there is no previous document.

    \sa forward()
*/
void QGraphicsWebView::back()
{
    if (d->page)
        d->page->triggerAction(QWebPage::Back);
}

/*!
    Convenience slot that loads the next document in the list of documents
    built by navigating links. Does nothing if there is no next document.

    \sa back()
*/
void QGraphicsWebView::forward()
{
    if (d->page)
        d->page->triggerAction(QWebPage::Forward);
}

/*!
    Reloads the current document.

    \sa stop(), loadStarted()
*/
void QGraphicsWebView::reload()
{
    if (d->page)
        d->page->triggerAction(QWebPage::Reload);
}

/*!
    Loads the specified \a url and displays it.

    \note The view remains the same until enough data has arrived to display the new \a url.

    \sa setUrl(), url(), urlChanged()
*/
void QGraphicsWebView::load(const QUrl& url)
{
    page()->mainFrame()->load(url);
}

/*!
    \fn void QGraphicsWebView::load(const QNetworkRequest &request, QNetworkAccessManager::Operation operation, const QByteArray &body)

    Loads a network request, \a request, using the method specified in \a operation.

    \a body is optional and is only used for POST operations.

    \note The view remains the same until enough data has arrived to display the new url.

    \sa url(), urlChanged()
*/

void QGraphicsWebView::load(const QNetworkRequest& request,
                    QNetworkAccessManager::Operation operation,
                    const QByteArray& body)
{
    page()->mainFrame()->load(request, operation, body);
}

/*!
    Sets the content of the web view to the specified \a html.

    External objects such as stylesheets or images referenced in the HTML
    document are located relative to \a baseUrl.

    The \a html is loaded immediately; external objects are loaded asynchronously.

    When using this method, WebKit assumes that external resources such as
    JavaScript programs or style sheets are encoded in UTF-8 unless otherwise
    specified. For example, the encoding of an external script can be specified
    through the charset attribute of the HTML script tag. Alternatively, the
    encoding can also be specified by the web server.

    This is a convenience function equivalent to setContent(html, "text/html", baseUrl).

    \warning This function works only for HTML, for other mime types (i.e. XHTML, SVG)
    setContent() should be used instead.

    \sa load(), setContent(), QWebFrame::toHtml(), QWebFrame::setContent()
*/
void QGraphicsWebView::setHtml(const QString& html, const QUrl& baseUrl)
{
    page()->mainFrame()->setHtml(html, baseUrl);
}

/*!
    Sets the content of the web graphicsitem to the specified content \a data. If the \a mimeType argument
    is empty it is currently assumed that the content is HTML but in future versions we may introduce
    auto-detection.

    External objects referenced in the content are located relative to \a baseUrl.

    The \a data is loaded immediately; external objects are loaded asynchronously.

    \sa load(), setHtml(), QWebFrame::toHtml()
*/
void QGraphicsWebView::setContent(const QByteArray& data, const QString& mimeType, const QUrl& baseUrl)
{
    page()->mainFrame()->setContent(data, mimeType, baseUrl);
}

/*!
    Returns a pointer to the view's history of navigated web pages.

    It is equivalent to

    \snippet webkitsnippets/qtwebkit_qwebview_snippet.cpp 0
*/
QWebHistory* QGraphicsWebView::history() const
{
    return page()->history();
}

/*!
    \property QGraphicsWebView::modified
    \brief whether the document was modified by the user

    Parts of HTML documents can be editable for example through the
    \c{contenteditable} attribute on HTML elements.

    By default, this property is false.
*/
bool QGraphicsWebView::isModified() const
{
    if (d->page)
        return d->page->isModified();
    return false;
}

/*!
    Returns a pointer to the view/page specific settings object.

    It is equivalent to

    \snippet webkitsnippets/qtwebkit_qwebview_snippet.cpp 1

    \sa QWebSettings::globalSettings()
*/
QWebSettings* QGraphicsWebView::settings() const
{
    return page()->settings();
}

/*!
    Returns a pointer to a QAction that encapsulates the specified web action \a action.
*/
QAction *QGraphicsWebView::pageAction(QWebPage::WebAction action) const
{
#ifdef QT_NO_ACTION
    Q_UNUSED(action)
    return 0;
#else
    return page()->action(action);
#endif
}

/*!
    Triggers the specified \a action. If it is a checkable action the specified
    \a checked state is assumed.

    \sa pageAction()
*/
void QGraphicsWebView::triggerPageAction(QWebPage::WebAction action, bool checked)
{
    page()->triggerAction(action, checked);
}

/*!
    Finds the specified string, \a subString, in the page, using the given \a options.

    If the HighlightAllOccurrences flag is passed, the function will highlight all occurrences
    that exist in the page. All subsequent calls will extend the highlight, rather than
    replace it, with occurrences of the new string.

    If the HighlightAllOccurrences flag is not passed, the function will select an occurrence
    and all subsequent calls will replace the current occurrence with the next one.

    To clear the selection, just pass an empty string.

    Returns true if \a subString was found; otherwise returns false.

    \sa QWebPage::selectedText(), QWebPage::selectionChanged()
*/
bool QGraphicsWebView::findText(const QString &subString, QWebPage::FindFlags options)
{
    if (d->page)
        return d->page->findText(subString, options);
    return false;
}

/*!
    \property QGraphicsWebView::resizesToContents
    \brief whether the size of the QGraphicsWebView and its viewport changes to match the contents size
    \since 4.7 

    If this property is set, the QGraphicsWebView will automatically change its
    size to match the size of the main frame contents. As a result the top level frame
    will never have scrollbars. It will also make CSS fixed positioning to behave like absolute positioning
    with elements positioned relative to the document instead of the viewport.

    This property should be used in conjunction with the QWebPage::preferredContentsSize property.
    If not explicitly set, the preferredContentsSize is automatically set to a reasonable value.

    \sa QWebPage::setPreferredContentsSize()
*/
void QGraphicsWebView::setResizesToContents(bool enabled)
{
    if (d->resizesToContents == enabled)
        return;
    d->resizesToContents = enabled;
    if (d->page)
        d->updateResizesToContentsForPage();
}

bool QGraphicsWebView::resizesToContents() const
{
    return d->resizesToContents;
}

/*!
    \property QGraphicsWebView::tiledBackingStoreFrozen
    \brief whether the tiled backing store updates its contents
    \since 4.7 

    If the tiled backing store is enabled using QWebSettings::TiledBackingStoreEnabled attribute, this property
    can be used to disable backing store updates temporarily. This can be useful for example for running
    a smooth animation that changes the scale of the QGraphicsWebView.
 
    When the backing store is unfrozen, its contents will be automatically updated to match the current
    state of the document. If the QGraphicsWebView scale was changed, the backing store is also
    re-rendered using the new scale.
 
    If the tiled backing store is not enabled, this property does nothing.

    \sa QWebSettings::TiledBackingStoreEnabled
    \sa QGraphicsObject::scale
*/
bool QGraphicsWebView::isTiledBackingStoreFrozen() const
{
#if ENABLE(TILED_BACKING_STORE)
    WebCore::TiledBackingStore* backingStore = QWebFramePrivate::core(page()->mainFrame())->tiledBackingStore();
    if (!backingStore)
        return false;
    return backingStore->contentsFrozen();
#else
    return false;
#endif
}

void QGraphicsWebView::setTiledBackingStoreFrozen(bool frozen)
{
#if ENABLE(TILED_BACKING_STORE)
    WebCore::TiledBackingStore* backingStore = QWebFramePrivate::core(page()->mainFrame())->tiledBackingStore();
    if (!backingStore)
        return;
    backingStore->setContentsFrozen(frozen);
#else
    UNUSED_PARAM(frozen);
#endif
}

/*! \reimp
*/
void QGraphicsWebView::hoverMoveEvent(QGraphicsSceneHoverEvent* ev)
{
    if (d->page) {
        const bool accepted = ev->isAccepted();
        QMouseEvent me = QMouseEvent(QEvent::MouseMove,
                ev->pos().toPoint(), Qt::NoButton,
                Qt::NoButton, Qt::NoModifier);
        d->page->event(&me);
        ev->setAccepted(accepted);
    }

    if (!ev->isAccepted())
        QGraphicsItem::hoverMoveEvent(ev);
}

/*! \reimp
*/
void QGraphicsWebView::hoverLeaveEvent(QGraphicsSceneHoverEvent* ev)
{
    Q_UNUSED(ev);
}

/*! \reimp
*/
void QGraphicsWebView::mouseMoveEvent(QGraphicsSceneMouseEvent* ev)
{
    if (d->page) {
        const bool accepted = ev->isAccepted();
        d->page->event(ev);
        ev->setAccepted(accepted);
    }

    if (!ev->isAccepted())
        QGraphicsItem::mouseMoveEvent(ev);
}

/*! \reimp
*/
void QGraphicsWebView::mousePressEvent(QGraphicsSceneMouseEvent* ev)
{
    if (d->page) {
        const bool accepted = ev->isAccepted();
        d->page->event(ev);
        ev->setAccepted(accepted);
    }

    if (!ev->isAccepted())
        QGraphicsItem::mousePressEvent(ev);
}

/*! \reimp
*/
void QGraphicsWebView::mouseReleaseEvent(QGraphicsSceneMouseEvent* ev)
{
    if (d->page) {
        const bool accepted = ev->isAccepted();
        d->page->event(ev);
        ev->setAccepted(accepted);
    }

    if (!ev->isAccepted())
        QGraphicsItem::mouseReleaseEvent(ev);
}

/*! \reimp
*/
void QGraphicsWebView::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* ev)
{
    if (d->page) {
        const bool accepted = ev->isAccepted();
        d->page->event(ev);
        ev->setAccepted(accepted);
    }

    if (!ev->isAccepted())
        QGraphicsItem::mouseDoubleClickEvent(ev);
}

/*! \reimp
*/
void QGraphicsWebView::keyPressEvent(QKeyEvent* ev)
{
    if (d->page)
        d->page->event(ev);

    if (!ev->isAccepted())
        QGraphicsItem::keyPressEvent(ev);
}

/*! \reimp
*/
void QGraphicsWebView::keyReleaseEvent(QKeyEvent* ev)
{
    if (d->page)
        d->page->event(ev);

    if (!ev->isAccepted())
        QGraphicsItem::keyReleaseEvent(ev);
}

/*! \reimp
*/
void QGraphicsWebView::focusInEvent(QFocusEvent* ev)
{
    if (d->page)
        d->page->event(ev);
    else
        QGraphicsItem::focusInEvent(ev);
}

/*! \reimp
*/
void QGraphicsWebView::focusOutEvent(QFocusEvent* ev)
{
    if (d->page)
        d->page->event(ev);
    else
        QGraphicsItem::focusOutEvent(ev);
}

/*! \reimp
*/
bool QGraphicsWebView::focusNextPrevChild(bool next)
{
    if (d->page)
        return d->page->focusNextPrevChild(next);

    return QGraphicsWidget::focusNextPrevChild(next);
}

/*! \reimp
*/
void QGraphicsWebView::dragEnterEvent(QGraphicsSceneDragDropEvent* ev)
{
#ifndef QT_NO_DRAGANDDROP
    if (d->page)
        d->page->event(ev);
#else
    Q_UNUSED(ev);
#endif
}

/*! \reimp
*/
void QGraphicsWebView::dragLeaveEvent(QGraphicsSceneDragDropEvent* ev)
{
#ifndef QT_NO_DRAGANDDROP
    if (d->page) {
        const bool accepted = ev->isAccepted();
        d->page->event(ev);
        ev->setAccepted(accepted);
    }

    if (!ev->isAccepted())
        QGraphicsWidget::dragLeaveEvent(ev);
#else
    Q_UNUSED(ev);
#endif
}

/*! \reimp
*/
void QGraphicsWebView::dragMoveEvent(QGraphicsSceneDragDropEvent* ev)
{
#ifndef QT_NO_DRAGANDDROP
    if (d->page) {
        const bool accepted = ev->isAccepted();
        d->page->event(ev);
        ev->setAccepted(accepted);
    }

    if (!ev->isAccepted())
        QGraphicsWidget::dragMoveEvent(ev);
#else
    Q_UNUSED(ev);
#endif
}

/*! \reimp
*/
void QGraphicsWebView::dropEvent(QGraphicsSceneDragDropEvent* ev)
{
#ifndef QT_NO_DRAGANDDROP
    if (d->page) {
        const bool accepted = ev->isAccepted();
        d->page->event(ev);
        ev->setAccepted(accepted);
    }

    if (!ev->isAccepted())
        QGraphicsWidget::dropEvent(ev);
#else
    Q_UNUSED(ev);
#endif
}

#ifndef QT_NO_CONTEXTMENU
/*! \reimp
*/
void QGraphicsWebView::contextMenuEvent(QGraphicsSceneContextMenuEvent* ev)
{
    if (d->page) {
        const bool accepted = ev->isAccepted();
        d->page->event(ev);
        ev->setAccepted(accepted);
    }
}
#endif // QT_NO_CONTEXTMENU

#ifndef QT_NO_WHEELEVENT
/*! \reimp
*/
void QGraphicsWebView::wheelEvent(QGraphicsSceneWheelEvent* ev)
{
    if (d->page) {
        const bool accepted = ev->isAccepted();
        d->page->event(ev);
        ev->setAccepted(accepted);
    }

    if (!ev->isAccepted())
        QGraphicsItem::wheelEvent(ev);
}
#endif // QT_NO_WHEELEVENT

/*! \reimp
*/
void QGraphicsWebView::inputMethodEvent(QInputMethodEvent* ev)
{
    if (d->page)
        d->page->event(ev);

    if (!ev->isAccepted())
        QGraphicsItem::inputMethodEvent(ev);
}

/*!
    \fn void QGraphicsWebView::statusBarMessage(const QString& text)

    This signal is emitted when the statusbar \a text is changed by the page.
*/

/*!
    \fn void QGraphicsWebView::loadProgress(int progress)

    This signal is emitted every time an element in the web page
    completes loading and the overall loading progress advances.

    This signal tracks the progress of all child frames.

    The current value is provided by \a progress and scales from 0 to 100,
    which is the default range of QProgressBar.

    \sa loadStarted(), loadFinished()
*/

/*!
    \fn void QGraphicsWebView::linkClicked(const QUrl &url)

    This signal is emitted whenever the user clicks on a link and the page's linkDelegationPolicy
    property is set to delegate the link handling for the specified \a url.

    \sa QWebPage::linkDelegationPolicy()
*/

#endif // QT_NO_GRAPHICSVIEW

#include "moc_qgraphicswebview.cpp"
