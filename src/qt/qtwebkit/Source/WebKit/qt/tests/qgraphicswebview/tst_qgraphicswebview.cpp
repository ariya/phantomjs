/*
    Copyright (C) 2009 Jakub Wieczorek <faw217@gmail.com>

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

#include "../util.h"
#include <QtTest/QtTest>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QStyleOptionGraphicsItem>
#include <qgraphicswebview.h>
#include <qwebpage.h>
#include <qwebframe.h>

#if defined(ENABLE_WEBGL) && ENABLE_WEBGL
#include <QGLWidget>
#endif

class tst_QGraphicsWebView : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void qgraphicswebview();
    void crashOnViewlessWebPages();
    void microFocusCoordinates();
    void focusInputTypes();
    void crashOnSetScaleBeforeSetUrl();
    void widgetsRenderingThroughCache();
    void windowResizeEvent();
    void horizontalScrollbarTest();

#if !(defined(WTF_USE_QT_MOBILE_THEME) && WTF_USE_QT_MOBILE_THEME)
    void setPalette_data();
    void setPalette();
#endif
    void renderHints();
#if defined(WTF_USE_TILED_BACKING_STORE) && WTF_USE_TILED_BACKING_STORE
    void bug57798();
    void bug56929();
#endif
#if defined(ENABLE_WEBGL) && ENABLE_WEBGL
    void webglSoftwareFallbackVerticalOrientation();
    void webglSoftwareFallbackHorizontalOrientation();

private:
    void compareCanvasToImage(const QUrl&, const QImage&);
#endif
};

void tst_QGraphicsWebView::qgraphicswebview()
{
    QGraphicsWebView item;
    item.url();
    item.title();
    item.icon();
    item.zoomFactor();
    item.history();
    item.settings();
    item.page();
    item.setPage(0);
    item.page();
    item.setUrl(QUrl());
    item.setZoomFactor(0);
    item.load(QUrl());
    item.setHtml(QString());
    item.setContent(QByteArray());
    item.isModified();
}

class WebPage : public QWebPage
{
    Q_OBJECT

public:
    WebPage(QObject* parent = 0): QWebPage(parent)
    {
    }

    QGraphicsWebView* webView;

private Q_SLOTS:
    // Force a webview deletion during the load.
    // It should not cause WebPage to crash due to
    // it accessing invalid pageClient pointer.
    void aborting()
    {
        delete webView;
    }
};

class GraphicsWebView : public QGraphicsWebView
{
    Q_OBJECT

public:
    GraphicsWebView(QGraphicsItem* parent = 0): QGraphicsWebView(parent)
    {
    }

    void fireMouseClick(QPointF point) {
        QGraphicsSceneMouseEvent presEv(QEvent::GraphicsSceneMousePress);
        presEv.setPos(point);
        presEv.setButton(Qt::LeftButton);
        presEv.setButtons(Qt::LeftButton);
        QGraphicsSceneMouseEvent relEv(QEvent::GraphicsSceneMouseRelease);
        relEv.setPos(point);
        relEv.setButton(Qt::LeftButton);
        relEv.setButtons(Qt::LeftButton);
        QGraphicsWebView::sceneEvent(&presEv);
        QGraphicsWebView::sceneEvent(&relEv);
    }
};

void tst_QGraphicsWebView::crashOnViewlessWebPages()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);

    QGraphicsWebView* webView = new QGraphicsWebView;
    WebPage* page = new WebPage;
    webView->setPage(page);
    page->webView = webView;
    scene.addItem(webView);

    view.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view.resize(600, 480);
    webView->resize(view.geometry().size());

    QCoreApplication::processEvents();
    view.show();

    // Resizing the page will resize and layout the empty "about:blank"
    // page, so we first connect the signal afterward.
    connect(page->mainFrame(), SIGNAL(initialLayoutCompleted()), page, SLOT(aborting()));

    page->mainFrame()->load(QUrl("data:text/html,"
                                 "<frameset cols=\"25%,75%\">"
                                     "<frame src=\"data:text/html,foo \">"
                                     "<frame src=\"data:text/html,bar\">"
                                 "</frameset>"));

    QVERIFY(waitForSignal(page, SIGNAL(loadFinished(bool))));
    delete page;
}

void tst_QGraphicsWebView::crashOnSetScaleBeforeSetUrl()
{
    QGraphicsWebView* webView = new QGraphicsWebView;
    webView->setScale(2.0);
    delete webView;
}

void tst_QGraphicsWebView::widgetsRenderingThroughCache()
{
    // Widgets should be rendered the same way with and without
    // intermediate cache (tiling for example).
    // See bug https://bugs.webkit.org/show_bug.cgi?id=47767 where
    // widget are rendered as disabled when caching is using.

    QGraphicsWebView* webView = new QGraphicsWebView;
    webView->setHtml(QLatin1String("<body style=\"background-color: white\"><input type=range></input><input type=checkbox></input><input type=radio></input><input type=file></input></body>"));

    QGraphicsView view;
    // Disable the scrollbars on the graphics view because QtWebKit handles scrolling and scrollbar automatically
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.show();
    QGraphicsScene* scene = new QGraphicsScene(&view);
    view.setScene(scene);
    scene->addItem(webView);
    view.setGeometry(QRect(0, 0, 500, 500));
    QWidget *const widget = &view;
    QTest::qWaitForWindowExposed(widget);

    // 1. Reference without tiling.
    webView->settings()->setAttribute(QWebSettings::TiledBackingStoreEnabled, false);
    QPixmap referencePixmap(view.size());
    widget->render(&referencePixmap);

    // 2. With tiling.
    webView->settings()->setAttribute(QWebSettings::TiledBackingStoreEnabled, true);
    QPixmap viewWithTiling(view.size());
    widget->render(&viewWithTiling);
    QApplication::processEvents();
    widget->render(&viewWithTiling);

    QCOMPARE(referencePixmap.toImage(), viewWithTiling.toImage());
}

#if defined(WTF_USE_TILED_BACKING_STORE) && WTF_USE_TILED_BACKING_STORE
void tst_QGraphicsWebView::bug57798()
{
    // When content size grows from less than viewport size to more than that, tiles may need to be regenerated.

    QGraphicsWebView* webView = new QGraphicsWebView();
    webView->setGeometry(QRectF(0.0, 0.0, 100.0, 100.0));
    QGraphicsView view(new QGraphicsScene());
    view.scene()->setParent(&view);
    view.scene()->addItem(webView);
    webView->settings()->setAttribute(QWebSettings::TiledBackingStoreEnabled, true);
    QStyleOptionGraphicsItem option;
    option.exposedRect = view.sceneRect();
    QImage img(view.width(), view.height(),
    QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&img);
    // This will not paint anything as the tiles are not ready, but will trigger tile creation with size (0, 0).
    webView->paint(&painter, &option);
    QApplication::processEvents();
    QUrl url("qrc:///resources/greendiv.html");
    webView->load(url);
    QVERIFY(waitForSignal(webView, SIGNAL(loadFinished(bool))));
    // This should trigger the recreation of the tiles.
    webView->paint(&painter, &option);
    QApplication::processEvents();
    painter.fillRect(option.exposedRect, Qt::red); // This is here to ensure failure if paint does not paint anything
    webView->paint(&painter, &option);
    QCOMPARE(img.pixel(option.exposedRect.width() / 4, option.exposedRect.height() / 4), qRgba(0, 128, 0, 255));
}

void tst_QGraphicsWebView::bug56929()
{
    // When rendering from tiles sychronous layout should not be triggered
    // and scrollbars should be in sync with the size of the document in the displayed state.

    QGraphicsWebView* webView = new QGraphicsWebView();
    webView->setGeometry(QRectF(0.0, 0.0, 100.0, 100.0));
    QGraphicsView view(new QGraphicsScene());
    view.scene()->setParent(&view);
    view.scene()->addItem(webView);
    webView->settings()->setAttribute(QWebSettings::TiledBackingStoreEnabled, true);
    QUrl url("qrc:///resources/greendiv.html");
    webView->load(url);
    QVERIFY(waitForSignal(webView, SIGNAL(loadFinished(bool))));
    QStyleOptionGraphicsItem option;
    option.exposedRect = webView->geometry();
    QImage img(option.exposedRect.width(), option.exposedRect.height(), QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&img);
    // This will not paint anything as the tiles are not ready, yet.
    webView->paint(&painter, &option);
    QApplication::processEvents();
    webView->paint(&painter, &option);
    QCOMPARE(img.pixel(option.exposedRect.width() - 2, option.exposedRect.height() / 2), qRgba(255, 255, 255, 255));
    painter.fillRect(option.exposedRect, Qt::black);
    QCOMPARE(img.pixel(option.exposedRect.width() - 2, option.exposedRect.height() / 2), qRgba(0, 0, 0, 255));
    webView->page()->mainFrame()->evaluateJavaScript(QString("resizeDiv();"));
    webView->paint(&painter, &option);
    QCOMPARE(img.pixel(option.exposedRect.width() - 2, option.exposedRect.height() / 2), qRgba(255, 255, 255, 255));
}
#endif

void tst_QGraphicsWebView::microFocusCoordinates()
{
    QWebPage* page = new QWebPage;
    QGraphicsWebView* webView = new QGraphicsWebView;
    webView->setPage( page );
    QGraphicsView* view = new QGraphicsView;
    QGraphicsScene* scene = new QGraphicsScene(view);
    view->setScene(scene);
    scene->addItem(webView);
    view->setGeometry(QRect(0,0,500,500));

    page->mainFrame()->setHtml("<html><body>" \
        "<input type='text' id='input1' style='font--family: serif' value='' maxlength='20'/><br>" \
        "<canvas id='canvas1' width='500' height='500'></canvas>" \
        "<input type='password'/><br>" \
        "<canvas id='canvas2' width='500' height='500'></canvas>" \
        "</body></html>");

    page->mainFrame()->setFocus();

    QVariant initialMicroFocus = page->inputMethodQuery(Qt::ImMicroFocus);
    QVERIFY(initialMicroFocus.isValid());

    page->mainFrame()->scroll(0,300);

    QVariant currentMicroFocus = page->inputMethodQuery(Qt::ImMicroFocus);
    QVERIFY(currentMicroFocus.isValid());

    QCOMPARE(initialMicroFocus.toRect().translated(QPoint(0,-300)), currentMicroFocus.toRect());

    delete view;
}

void tst_QGraphicsWebView::focusInputTypes()
{
    QWebPage* page = new QWebPage;
    GraphicsWebView* webView = new GraphicsWebView;
    webView->setPage( page );
    QGraphicsView* view = new QGraphicsView;
    QGraphicsScene* scene = new QGraphicsScene(view);
    view->setScene(scene);
    scene->addItem(webView);
    view->setGeometry(QRect(0,0,500,500));
    QCoreApplication::processEvents();
    QUrl url("qrc:///resources/input_types.html");
    page->mainFrame()->load(url);
    page->mainFrame()->setFocus();

    QVERIFY(waitForSignal(page, SIGNAL(loadFinished(bool))));

    // 'text' type
    webView->fireMouseClick(QPointF(20.0, 10.0));
    QVERIFY(webView->inputMethodHints() == Qt::ImhNone);

    // 'password' field
    webView->fireMouseClick(QPointF(20.0, 60.0));
    QVERIFY(webView->inputMethodHints() & Qt::ImhHiddenText);

    // 'tel' field
    webView->fireMouseClick(QPointF(20.0, 110.0));
    QVERIFY(webView->inputMethodHints() & Qt::ImhDialableCharactersOnly);

    // 'number' field
    webView->fireMouseClick(QPointF(20.0, 160.0));
    QVERIFY(webView->inputMethodHints() & Qt::ImhDigitsOnly);

    // 'email' field
    webView->fireMouseClick(QPointF(20.0, 210.0));
    QVERIFY(webView->inputMethodHints() & Qt::ImhEmailCharactersOnly);

    // 'url' field
    webView->fireMouseClick(QPointF(20.0, 260.0));
    QVERIFY(webView->inputMethodHints() & Qt::ImhUrlCharactersOnly);

    delete webView;
    delete view;
}

#if !(defined(WTF_USE_QT_MOBILE_THEME) && WTF_USE_QT_MOBILE_THEME)
void tst_QGraphicsWebView::setPalette_data()
{
    QTest::addColumn<bool>("active");
    QTest::addColumn<bool>("background");
    QTest::newRow("activeBG") << true << true;
    QTest::newRow("activeFG") << true << false;
    QTest::newRow("inactiveBG") << false << true;
    QTest::newRow("inactiveFG") << false << false;
}

// Render a QGraphicsWebView to a QImage twice, each time with a different palette set,
// verify that images rendered are not the same, confirming WebCore usage of
// custom palette on selections.
void tst_QGraphicsWebView::setPalette()
{
    QString html = "<html><head></head>"
                   "<body>"
                   "Some text here"
                   "</body>"
                   "</html>";

    QFETCH(bool, active);
    QFETCH(bool, background);

    QWidget* activeView = 0;

    // Use controlView to manage active/inactive state of test views by raising
    // or lowering their position in the window stack.
    QGraphicsScene controlScene;
    QGraphicsView controlView(&controlScene);
    QGraphicsWebView controlWebView;
    controlScene.addItem(&controlWebView);
    controlWebView.setHtml(html);
    controlWebView.setGeometry(QRectF(0, 0, 200, 200));

    QGraphicsScene scene1;
    QGraphicsView view1(&scene1);
    view1.setSceneRect(0, 0, 300, 300);
    QGraphicsWebView webView1;
    webView1.setResizesToContents(true);
    scene1.addItem(&webView1);
    webView1.setFocus();

    QPalette palette1;
    QBrush brush1(Qt::red);
    brush1.setStyle(Qt::SolidPattern);
    if (active && background) {
        // Rendered image must have red background on an active QGraphicsWebView.
        palette1.setBrush(QPalette::Active, QPalette::Highlight, brush1);
    } else if (active && !background) {
        // Rendered image must have red foreground on an active QGraphicsWebView.
        palette1.setBrush(QPalette::Active, QPalette::HighlightedText, brush1);
    } else if (!active && background) {
        // Rendered image must have red background on an inactive QGraphicsWebView.
        palette1.setBrush(QPalette::Inactive, QPalette::Highlight, brush1);
    } else if (!active && !background) {
        // Rendered image must have red foreground on an inactive QGraphicsWebView.
        palette1.setBrush(QPalette::Inactive, QPalette::HighlightedText, brush1);
    }

    webView1.setHtml(html);
    view1.resize(webView1.page()->viewportSize());
    webView1.setPalette(palette1);
    view1.show();

    QVERIFY(webView1.palette() == palette1);
    QVERIFY(webView1.page()->palette() == palette1);

    QTest::qWaitForWindowExposed(&view1);

    if (!active) {
        controlView.show();
        QTest::qWaitForWindowExposed(&controlView);
        QApplication::setActiveWindow(&controlView);
        activeView = &controlView;
        controlView.activateWindow();
    } else {
        QApplication::setActiveWindow(&view1);
        view1.activateWindow();
        activeView = &view1;
    }

    QTRY_COMPARE(QApplication::activeWindow(), activeView);

    webView1.page()->triggerAction(QWebPage::SelectAll);

    QImage img1(webView1.page()->viewportSize(), QImage::Format_ARGB32);
    QPainter painter1(&img1);
    webView1.page()->currentFrame()->render(&painter1);
    painter1.end();
    view1.close();
    controlView.close();

    QGraphicsScene scene2;
    QGraphicsView view2(&scene2);
    view2.setSceneRect(0, 0, 300, 300);
    QGraphicsWebView webView2;
    webView2.setResizesToContents(true);
    scene2.addItem(&webView2);
    webView2.setFocus();

    QPalette palette2;
    QBrush brush2(Qt::blue);
    brush2.setStyle(Qt::SolidPattern);
    if (active && background) {
        // Rendered image must have blue background on an active QGraphicsWebView.
        palette2.setBrush(QPalette::Active, QPalette::Highlight, brush2);
    } else if (active && !background) {
        // Rendered image must have blue foreground on an active QGraphicsWebView.
        palette2.setBrush(QPalette::Active, QPalette::HighlightedText, brush2);
    } else if (!active && background) {
        // Rendered image must have blue background on an inactive QGraphicsWebView.
        palette2.setBrush(QPalette::Inactive, QPalette::Highlight, brush2);
    } else if (!active && !background) {
        // Rendered image must have blue foreground on an inactive QGraphicsWebView.
        palette2.setBrush(QPalette::Inactive, QPalette::HighlightedText, brush2);
    }

    webView2.setHtml(html);
    view2.resize(webView2.page()->viewportSize());
    webView2.setPalette(palette2);
    view2.show();

    QTest::qWaitForWindowExposed(&view2);

    if (!active) {
        controlView.show();
        QTest::qWaitForWindowExposed(&controlView);
        QApplication::setActiveWindow(&controlView);
        activeView = &controlView;
        controlView.activateWindow();
    } else {
        QApplication::setActiveWindow(&view2);
        view2.activateWindow();
        activeView = &view2;
    }

    QTRY_COMPARE(QApplication::activeWindow(), activeView);

    webView2.page()->triggerAction(QWebPage::SelectAll);

    QImage img2(webView2.page()->viewportSize(), QImage::Format_ARGB32);
    QPainter painter2(&img2);
    webView2.page()->currentFrame()->render(&painter2);
    painter2.end();

    view2.close();
    controlView.close();

    QVERIFY(img1 != img2);
}
#endif

void tst_QGraphicsWebView::renderHints()
{
    QGraphicsWebView webView;

    // default is only text antialiasing + smooth pixmap transform
    QVERIFY(!(webView.renderHints() & QPainter::Antialiasing));
    QVERIFY(webView.renderHints() & QPainter::TextAntialiasing);
    QVERIFY(webView.renderHints() & QPainter::SmoothPixmapTransform);
    QVERIFY(!(webView.renderHints() & QPainter::HighQualityAntialiasing));

    webView.setRenderHint(QPainter::Antialiasing, true);
    QVERIFY(webView.renderHints() & QPainter::Antialiasing);
    QVERIFY(webView.renderHints() & QPainter::TextAntialiasing);
    QVERIFY(webView.renderHints() & QPainter::SmoothPixmapTransform);
    QVERIFY(!(webView.renderHints() & QPainter::HighQualityAntialiasing));

    webView.setRenderHint(QPainter::Antialiasing, false);
    QVERIFY(!(webView.renderHints() & QPainter::Antialiasing));
    QVERIFY(webView.renderHints() & QPainter::TextAntialiasing);
    QVERIFY(webView.renderHints() & QPainter::SmoothPixmapTransform);
    QVERIFY(!(webView.renderHints() & QPainter::HighQualityAntialiasing));

    webView.setRenderHint(QPainter::SmoothPixmapTransform, true);
    QVERIFY(!(webView.renderHints() & QPainter::Antialiasing));
    QVERIFY(webView.renderHints() & QPainter::TextAntialiasing);
    QVERIFY(webView.renderHints() & QPainter::SmoothPixmapTransform);
    QVERIFY(!(webView.renderHints() & QPainter::HighQualityAntialiasing));

    webView.setRenderHint(QPainter::SmoothPixmapTransform, false);
    QVERIFY(webView.renderHints() & QPainter::TextAntialiasing);
    QVERIFY(!(webView.renderHints() & QPainter::SmoothPixmapTransform));
    QVERIFY(!(webView.renderHints() & QPainter::HighQualityAntialiasing));
}

class GraphicsView : public QGraphicsView {
public:
    GraphicsView();
    QGraphicsWebView* m_webView;
};

#if defined(ENABLE_WEBGL) && ENABLE_WEBGL
bool compareImagesFuzzyPixelCount(const QImage& image1, const QImage& image2, qreal tolerance = 0.05)
{
    if (image1.size() != image2.size())
        return false;

    unsigned diffPixelCount = 0;
    for (int row = 0; row < image1.size().width(); ++row) {
        for (int column = 0; column < image1.size().height(); ++column)
            if (image1.pixel(row, column) != image2.pixel(row, column))
                ++diffPixelCount;
    }

    if (diffPixelCount > (image1.size().width() * image1.size().height()) * tolerance)
        return false;

    return true;
}

GraphicsView::GraphicsView()
{
    QGraphicsScene* const scene = new QGraphicsScene(this);
    setScene(scene);

    m_webView = new QGraphicsWebView;
    scene->addItem(m_webView);

    m_webView->page()->settings()->setAttribute(QWebSettings::WebGLEnabled, true);
    m_webView->setResizesToContents(true);

    setFrameShape(QFrame::NoFrame);
    setViewport(new QGLWidget);
}

void tst_QGraphicsWebView::webglSoftwareFallbackVerticalOrientation()
{
    QSKIP("Hangs on X11 -- https://bugs.webkit.org/show_bug.cgi?id=105820");
    QSize canvasSize(100, 100);
    QImage reference(canvasSize, QImage::Format_ARGB32);
    reference.fill(0xFF00FF00);
    { // Reference.
        QPainter painter(&reference);
        QPolygonF triangleUp;
        triangleUp << QPointF(0, canvasSize.height())
                   << QPointF(canvasSize.width(), canvasSize.height())
                   << QPointF(canvasSize.width() / 2.0, canvasSize.height() / 2.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::red);
        painter.drawPolygon(triangleUp);
    }

    compareCanvasToImage(QUrl(QLatin1String("qrc:///resources/pointing_up.html")), reference);
}

void tst_QGraphicsWebView::webglSoftwareFallbackHorizontalOrientation()
{
    QSKIP("Hangs on X11 -- https://bugs.webkit.org/show_bug.cgi?id=105820");
    QSize canvasSize(100, 100);
    QImage reference(canvasSize, QImage::Format_ARGB32);
    reference.fill(0xFF00FF00);
    { // Reference.
        QPainter painter(&reference);
        QPolygonF triangleUp;
        triangleUp << QPointF(0, 0)
                   << QPointF(0, canvasSize.height())
                   << QPointF(canvasSize.width() / 2.0, canvasSize.height() / 2.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::red);
        painter.drawPolygon(triangleUp);
    }

    compareCanvasToImage(QUrl(QLatin1String("qrc:///resources/pointing_right.html")), reference);
}

void tst_QGraphicsWebView::compareCanvasToImage(const QUrl& url, const QImage& reference)
{
    GraphicsView view;
    view.show();
    QTest::qWaitForWindowExposed(&view);

    QGraphicsWebView* const graphicsWebView = view.m_webView;
    graphicsWebView->load(url);
    QVERIFY(waitForSignal(graphicsWebView, SIGNAL(loadFinished(bool))));
    { // Force a render, to create the accelerated compositing tree.
        QPixmap pixmap(view.size());
        QPainter painter(&pixmap);
        view.render(&painter);
    }

    const QSize imageSize = reference.size();

    QImage target(imageSize, QImage::Format_ARGB32);
    { // Web page content.
        QPainter painter(&target);
        QRectF renderRect(0, 0, imageSize.width(), imageSize.height());
        view.scene()->render(&painter, renderRect, renderRect);
    }
    QVERIFY(compareImagesFuzzyPixelCount(target, reference, 0.01));
}
#endif

class ResizeSpy : public QObject {
    Q_OBJECT
public Q_SLOTS:
    void receiveResize(int width, int height)
    {
        m_size = QSize(width, height);
        emit resized();
    }

    QSize size() const
    {
        return m_size;
    }

Q_SIGNALS:
    void resized();

private:
    QSize m_size;
};

void tst_QGraphicsWebView::windowResizeEvent()
{
    QGraphicsWebView webView;
    ResizeSpy resizeSpy;
    resizeSpy.setProperty("resizeCount", 0);

    QString html = "<html><body><script>"
                   "function onResize() { window.resizeSpy.receiveResize(window.innerWidth, window.innerHeight); }"
                   "window.addEventListener('resize', onResize , false);"
                   "</script></body></html>";

    webView.page()->mainFrame()->setHtml(html);
    webView.page()->mainFrame()->addToJavaScriptWindowObject("resizeSpy",
                                                             &resizeSpy);
    webView.setGeometry(QRect(0, 0, 50, 50));
    QEXPECT_FAIL("", "https://bugs.webkit.org/show_bug.cgi?id=118670", Continue);
    QVERIFY(::waitForSignal(&resizeSpy, SIGNAL(resized()), 1000));
    QCOMPARE(resizeSpy.size(), QSize(50, 50));

    webView.page()->setActualVisibleContentRect(QRect(10, 10, 60, 60));
    webView.setGeometry(QRect(0, 0, 100, 100));
    waitForSignal(&resizeSpy, SIGNAL(resized()), 1000);

    // This will be triggered without the fix on DOMWindow::innerHeight/Width
    QCOMPARE(resizeSpy.size(), QSize(60, 60));
}

void tst_QGraphicsWebView::horizontalScrollbarTest()
{
    QWebPage* page = new QWebPage;
    GraphicsWebView* webView = new GraphicsWebView;
    webView->setPage(page);
    webView->setGeometry(QRect(0, 0, 600, 600));
    QGraphicsView* view = new QGraphicsView;
    QGraphicsScene* scene = new QGraphicsScene(view);
    view->setScene(scene);
    scene->addItem(webView);

    // Turn off scrolling on the containing QGraphicsView, let the
    // QGraphicsWebView handle the scrolling by itself.
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->show();
    QCoreApplication::processEvents();

    QUrl url("qrc:///resources/scrolltest_page.html");
    page->mainFrame()->load(url);
    page->mainFrame()->setFocus();

    QVERIFY(waitForSignal(page, SIGNAL(loadFinished(bool))));

    QVERIFY(webView->page()->mainFrame()->scrollPosition() == QPoint(0, 0));

    // Note: The test below assumes that the layout direction is Qt::LeftToRight.
    webView->fireMouseClick(QPointF(550.0, 590.0));
    QVERIFY(page->mainFrame()->scrollPosition().x() > 0);

    // Note: The test below assumes that the layout direction is Qt::LeftToRight.
    webView->fireMouseClick(QPointF(20.0, 590.0));
    QVERIFY(page->mainFrame()->scrollPosition() == QPoint(0, 0));

    delete webView;
    delete view;
}

QTEST_MAIN(tst_QGraphicsWebView)

#include "tst_qgraphicswebview.moc"
