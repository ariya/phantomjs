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

#include "qwebelement.h"
#include "qwebframe_p.h"
#include "qwebpage.h"
#include "qwebpage_p.h"
#include "qwebscriptworld.h"
#include "qwebscriptworld_p.h"
#include "qwebsecurityorigin.h"
#include "DOMWrapperWorld.h"
#include <QMultiMap>
#include <qdebug.h>
#include <qevent.h>
#include <qfileinfo.h>
#include <qpainter.h>
#if HAVE(QTPRINTSUPPORT)
#include "QtPrintContext.h"
#include <qprinter.h>
#endif
#include <qnetworkrequest.h>
#include <qregion.h>

#include "qwebframe_printingaddons_p.h"

using namespace WebCore;

// from text/qfont.cpp
QT_BEGIN_NAMESPACE
extern Q_GUI_EXPORT int qt_defaultDpi();
QT_END_NAMESPACE

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
    pageAdapter = newPage->handle();
    emit q->pageChanged();
}

void QWebFramePrivate::emitUrlChanged()
{
    url = coreFrameUrl();
    emit q->urlChanged(url);
}

void QWebFramePrivate::didStartProvisionalLoad()
{
    emit q->provisionalLoad();
}

void QWebFramePrivate::didClearWindowObject()
{
    emit q->javaScriptWindowObjectCleared();
}

bool QWebFramePrivate::handleProgressFinished(QPoint *localPos)
{
    QWidget *view = q->page()->view();
    if (!view || !localPos)
        return false;
    *localPos = view->mapFromGlobal(QCursor::pos());
    return view->hasFocus() && view->rect().contains(*localPos);
}

void QWebFramePrivate::emitInitialLayoutCompleted()
{
    emit q->initialLayoutCompleted();
}

void QWebFramePrivate::emitIconChanged()
{
    emit q->iconChanged();
}

void QWebFramePrivate::emitLoadStarted(bool originatingLoad)
{
    if (page && originatingLoad)
        emit page->loadStarted();
    emit q->loadStarted();
}

void QWebFramePrivate::emitLoadFinished(bool originatingLoad, bool ok)
{
    if (page && originatingLoad)
        emit page->loadFinished(ok);
    emit q->loadFinished(ok);
}

QWebFrameAdapter* QWebFramePrivate::createChildFrame(QWebFrameData* frameData)
{
    QWebFrame* newFrame = new QWebFrame(/*parent frame*/q, frameData);
    return newFrame->d;
}

QWebFrame *QWebFramePrivate::apiHandle()
{
    return q;
}

QObject *QWebFramePrivate::handle()
{
    return q;
}

void QWebFramePrivate::contentsSizeDidChange(const QSize &size)
{
    emit q->contentsSizeChanged(size);
}

int QWebFramePrivate::scrollBarPolicy(Qt::Orientation orientation) const
{
    return (int) q->scrollBarPolicy(orientation);
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

QWebFrame::QWebFrame(QWebPage *parentPage)
    : QObject(parentPage)
    , d(new QWebFramePrivate)
{
    d->page = parentPage;
    d->q = this;
    d->init(/*page adapter*/ parentPage->handle());

#if ENABLE(ORIENTATION_EVENTS) && HAVE(QTSENSORS)
    connect(&d->m_orientation, SIGNAL(readingChanged()), this, SLOT(_q_orientationChanged()));
    d->m_orientation.start();
#endif
}

QWebFrame::QWebFrame(QWebFrame* parent, QWebFrameData* frameData)
    : QObject(parent)
    , d(new QWebFramePrivate)
{
    d->page = parent->d->page;
    d->q = this;
    d->init(parent->d->pageAdapter, frameData);
#if ENABLE(ORIENTATION_EVENTS) && HAVE(QTSENSORS)
    connect(&d->m_orientation, SIGNAL(readingChanged()), this, SLOT(_q_orientationChanged()));
    d->m_orientation.start();
#endif
}

QWebFrame::~QWebFrame()
{
    delete d;
}

/*!
    \fn void QWebFrame::addToJavaScriptWindowObject(const QString &name, QObject *object, ValueOwnership own)

    Make \a object available under \a name from within the frame's JavaScript
    context. The \a object will be inserted as a child of the frame's window
    object.

    Qt properties will be exposed as JavaScript properties and slots as
    JavaScript methods.
    The interaction between C++ and JavaScript is explained in the documentation of the \l{The Qt WebKit Bridge}{Qt WebKit bridge}.

    If you want to ensure that your QObjects remain accessible after loading a
    new URL, you should add them in a slot connected to the
    javaScriptWindowObjectCleared() signal.

    If Javascript is not enabled for this page, then this method does nothing.

    The ownership of \a object is specified using \a own.
*/
void QWebFrame::addToJavaScriptWindowObject(const QString &name, QObject *object, ValueOwnership ownership)
{
    d->addToJavaScriptWindowObject(name, object, static_cast<QWebFrameAdapter::ValueOwnership>(ownership));
}

/*!
    Returns the frame's content as HTML, enclosed in HTML and BODY tags.

    \sa setHtml(), toPlainText()
*/
QString QWebFrame::toHtml() const
{
    return d->toHtml();
}

/*!
    Returns the content of this frame converted to plain text, completely
    stripped of all HTML formatting.

    \sa toHtml()
*/
QString QWebFrame::toPlainText() const
{
    return d->toPlainText();
}

/*!
    \property QWebFrame::title
    \brief the title of the frame as defined by the HTML &lt;title&gt; element

    \sa titleChanged()
*/

QString QWebFrame::title() const
{
    return d->title();
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
    \header \li Key
            \li Value
    \row    \li "description"
            \li "This document is a tutorial about Qt development"
    \row    \li "keywords"
            \li "Qt, WebKit, Programming"
    \endtable

    This function returns a multi map to support multiple meta tags with the same attribute name.
*/
QMultiMap<QString, QString> QWebFrame::metaData() const
{
    return d->metaData();
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
    d->clearCoreFrame();
    const QUrl absolute = QWebFrameAdapter::ensureAbsoluteUrl(url);
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
    return d->lastRequestedUrl();
}
/*!
    \since 4.6
    \property QWebFrame::baseUrl
    \brief the base URL of the frame, can be used to resolve relative URLs
    \since 4.6
*/

QUrl QWebFrame::baseUrl() const
{
    return d->baseUrl();
}

/*!
    \property QWebFrame::icon
    \brief the icon associated with this frame

    \sa iconChanged(), QWebSettings::iconForUrl()
*/

QIcon QWebFrame::icon() const
{
    return QWebSettings::iconForUrl(d->coreFrameUrl());
}

/*!
  The name of this frame as defined by the parent frame.
*/
QString QWebFrame::frameName() const
{
    return d->uniqueName();
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
void QWebFrame::load(const QNetworkRequest &req, QNetworkAccessManager::Operation operation, const QByteArray &body)
{
    d->load(req, operation, body);
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
    d->setHtml(html, baseUrl);
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
    d->setContent(data, mimeType, baseUrl);
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
    QList<QObject*> objects = d->childFrames();
    QList<QWebFrame*> rc;
    rc.reserve(objects.size());
    Q_FOREACH(QObject* object, objects) {
        if (QWebFrame* frame = qobject_cast<QWebFrame*>(object))
            rc.append(frame);
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
    d->setScrollBarPolicy(orientation, policy);
}

/*!
  Sets the current \a value for the scrollbar with orientation \a orientation.

  The scrollbar forces the \a value to be within the legal range: minimum <= value <= maximum.

  Changing the value also updates the thumb position.

  \sa scrollBarMinimum(), scrollBarMaximum()
*/
void QWebFrame::setScrollBarValue(Qt::Orientation orientation, int value)
{
    d->setScrollBarValue(orientation, value);
}

/*!
  Returns the current value for the scrollbar with orientation \a orientation, or 0
  if no scrollbar is found for \a orientation.

  \sa scrollBarMinimum(), scrollBarMaximum()
*/
int QWebFrame::scrollBarValue(Qt::Orientation orientation) const
{
    return d->scrollBarValue(orientation);
}

/*!
  Returns the maximum value for the scrollbar with orientation \a orientation, or 0
  if no scrollbar is found for \a orientation.

  \sa scrollBarMinimum()
*/
int QWebFrame::scrollBarMaximum(Qt::Orientation orientation) const
{
    return d->scrollBarMaximum(orientation);
}

/*!
  Returns the minimum value for the scrollbar with orientation \a orientation.

  The minimum value is always 0.

  \sa scrollBarMaximum()
*/
int QWebFrame::scrollBarMinimum(Qt::Orientation orientation) const
{
    Q_UNUSED(orientation);
    return 0;
}

/*!
  \since 4.6
  Returns the geometry for the scrollbar with orientation \a orientation.

  If the scrollbar does not exist an empty rect is returned.
*/
QRect QWebFrame::scrollBarGeometry(Qt::Orientation orientation) const
{
    return d->scrollBarGeometry(orientation);
}

/*!
  \since 4.5
  Scrolls the frame \a dx pixels to the right and \a dy pixels downward. Both
  \a dx and \a dy may be negative.

  \sa QWebFrame::scrollPosition
*/

void QWebFrame::scroll(int dx, int dy)
{
    d->scrollBy(dx, dy);
}

/*!
  \property QWebFrame::scrollPosition
  \since 4.5
  \brief the position the frame is currently scrolled to.
*/

QPoint QWebFrame::scrollPosition() const
{
    return d->scrollPosition();
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
    d->scrollToAnchor(anchor);
}

/*!
  \since 4.6
  Render the \a layer of the frame using \a painter clipping to \a clip.

  \sa print()
*/

void QWebFrame::render(QPainter* painter, RenderLayers layer, const QRegion& clip)
{
    if (!clip.isEmpty())
        d->renderRelativeCoords(painter, layer, clip);
    else if (d->hasView())
        d->renderRelativeCoords(painter, layer, QRegion(d->frameRect()));
}

/*!
  Render the frame into \a painter clipping to \a clip.
*/
void QWebFrame::render(QPainter* painter, const QRegion& clip)
{
    render(painter, AllLayers, clip);
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
    d->setTextSizeMultiplier(factor);
}

/*!
    Returns the value of the multiplier used to scale the text in a Web frame.
*/
qreal QWebFrame::textSizeMultiplier() const
{
    return d->zoomFactor();
}

/*!
    \property QWebFrame::zoomFactor
    \since 4.5
    \brief the zoom factor for the frame
*/

void QWebFrame::setZoomFactor(qreal factor)
{
    d->setZoomFactor(factor);
}

qreal QWebFrame::zoomFactor() const
{
    return d->zoomFactor();
}

/*!
    \property QWebFrame::focus
    \since 4.6

    Returns true if this frame has keyboard input focus; otherwise, returns false.
*/
bool QWebFrame::hasFocus() const
{
    return d->hasFocus();
}

/*!
    \since 4.6

    Gives keyboard input focus to this frame.
*/
void QWebFrame::setFocus()
{
    d->setFocus();
}

/*!
    Returns the position of the frame relative to it's parent frame.
*/
QPoint QWebFrame::pos() const
{
    if (!d->hasView())
        return QPoint();

    return d->frameRect().topLeft();
}

/*!
    Return the geometry of the frame relative to it's parent frame.
*/
QRect QWebFrame::geometry() const
{
    return d->frameRect();
}

/*!
    \property QWebFrame::contentsSize
    \brief the size of the contents in this frame

    \sa contentsSizeChanged()
*/
QSize QWebFrame::contentsSize() const
{
    return d->contentsSize();
}

/*!
    \since 4.6

    Returns the document element of this frame.

    The document element provides access to the entire structured
    content of the frame.
*/
QWebElement QWebFrame::documentElement() const
{
    return d->documentElement();
}

/*!
    \since 4.6
    Returns a new list of elements matching the given CSS selector \a selectorQuery.
    If there are no matching elements, an empty list is returned.

    \l{Standard CSS selector} syntax is
    used for the query.

    This method is equivalent to Document::querySelectorAll in the \l{DOM Selectors API}.

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

    \l{Standard CSS selector} syntax is used for the query.

    This method is equivalent to Document::querySelector in the \l{DOM Selectors API}.

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
    QWebHitTestResultPrivate* result = d->hitTestContent(pos);

    if (!result)
        return QWebHitTestResult();

    return QWebHitTestResult(result);
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
    print(printer, 0);
}

void QWebFrame::print(QPrinter *printer, PrintCallback *callback) const
{
#if HAVE(QTPRINTSUPPORT)
    QPainter painter;

    HeaderFooter headerFooter(this, printer, callback);

    if (!painter.begin(printer))
        return;

    const qreal zoomFactorX = (qreal)printer->logicalDpiX() / qt_defaultDpi();
    const qreal zoomFactorY = (qreal)printer->logicalDpiY() / qt_defaultDpi();

    QRect qprinterRect = printer->pageRect();

    QRect pageRect(0, 0, int(qprinterRect.width() / zoomFactorX), int(qprinterRect.height() / zoomFactorY));

    QtPrintContext printContext(&painter, pageRect, d);

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

    if (!fromPage && !toPage) {
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

    for (int i = 0; i < docCopies; ++i) {
        int page = fromPage;
        while (true) {
            for (int j = 0; j < pageCopies; ++j) {
                if (printer->printerState() == QPrinter::Aborted
                    || printer->printerState() == QPrinter::Error) {
                    return;
                }
                if (headerFooter.isValid()) {
                    // print header/footer
                    int logicalPage, logicalPages;
                    d->frame->getPagination(page, printContext.pageCount(), logicalPage, logicalPages);
                    headerFooter.paintHeader(printContext.graphicsContext(), pageRect, logicalPage, logicalPages);
                    headerFooter.paintFooter(printContext.graphicsContext(), pageRect, logicalPage, logicalPages);
                }
                printContext.spoolPage(page - 1, pageRect.width());
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

        if (i < docCopies - 1)
            printer->newPage();
    }
#endif // HAVE(PRINTSUPPORT)
}
#endif // QT_NO_PRINTER

/*!
    Evaluates the JavaScript defined by \a scriptSource using this frame as context
    and returns the result of the last executed statement.

    \sa addToJavaScriptWindowObject(), javaScriptWindowObjectCleared()
*/
QVariant QWebFrame::evaluateJavaScript(const QString& scriptSource, const QString& location)
{
    return d->evaluateJavaScript(scriptSource, location);
}

/*!
    \since 4.5

    Returns the frame's security origin.
*/
QWebSecurityOrigin QWebFrame::securityOrigin() const
{
    return d->securityOrigin();
}

QWebFrame *QWebFramePrivate::kit(const QWebFrameAdapter* frameAdapter)
{
    return static_cast<const QWebFramePrivate*>(frameAdapter)->q;
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
    Returns the position where the hit test occured in the coordinates of frame containing the element hit.

    \sa frame()
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
    return qobject_cast<QWebFrame*>(d->linkTargetFrame.data());
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
    Returns the url of the video or audio element.
    \since 5.2
*/
QUrl QWebHitTestResult::mediaUrl() const
{
    if (!d)
        return QUrl();
    return d->mediaUrl;
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
    if (!d)
        return QWebElement();

    return d->elementForInnerNode();
}

/*!
    Returns the frame of the element hit.
*/
QWebFrame *QWebHitTestResult::frame() const
{
    if (!d)
        return 0;
    return qobject_cast<QWebFrame*>(d->frame.data());
}

/*!
 * \internal
 */
QWebFrameAdapter *QWebFrame::handle() const
{
    return d;
}

#include "moc_qwebframe.cpp"
