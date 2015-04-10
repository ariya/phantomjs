/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

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

#include <QImage>
#include <QMatrix4x4>
#include <QOpenGLContext>
#include <QSize>
#include <QWindow>
#include <QtTest/QtTest>

#include <WebKit2/WKContext.h>
#include <WebKit2/WKPageGroup.h>
#include <WebKit2/WKPageLoadTypes.h>
#include <WebKit2/WKPreferences.h>
#include <WebKit2/WKPreferencesPrivate.h>
#include <WebKit2/WKStringQt.h>
#include <WebKit2/WKURL.h>
#include <WebKit2/qrawwebview_p.h>

static WKContextRef webContext()
{
    static WKContextRef result = WKContextCreate();
    return result;
}

static WKPageGroupRef createWebPageGroup(const QString& name)
{
    WKPageGroupRef pageGroup =WKPageGroupCreateWithIdentifier(WKStringCreateWithQString(name));
    WKPreferencesRef preferences = WKPageGroupGetPreferences(pageGroup);
    WKPreferencesSetAcceleratedCompositingEnabled(preferences, true);
    WKPreferencesSetFrameFlatteningEnabled(preferences, true);

    return pageGroup;
}

static WKPageGroupRef webPageGroup(const QString& name)
{
    static WKPageGroupRef result = createWebPageGroup(name);
    return result;
}

class WebView : public QObject, public QRawWebViewClient {
    Q_OBJECT
public:
    WebView(const QSize& size, bool transparent = false)
    {
        m_webView = new QRawWebView(webContext(), webPageGroup(QString()), this);
        m_webView->setTransparentBackground(transparent);
        m_webView->create();

        WKPageLoaderClient loaderClient;
        memset(&loaderClient, 0, sizeof(WKPageLoaderClient));
        loaderClient.version = kWKPageLoaderClientCurrentVersion;
        loaderClient.clientInfo = this;
        loaderClient.didLayout = WebView::didLayout;

        WKPageSetPageLoaderClient(m_webView->pageRef(), &loaderClient);
        WKPageListenForLayoutMilestones(m_webView->pageRef(), kWKDidFirstVisuallyNonEmptyLayout);
        WKPageSetUseFixedLayout(m_webView->pageRef(), true);

        m_webView->setSize(size);
        m_webView->setFocused(true);
        m_webView->setVisible(true);
        m_webView->setActive(true);
    }

    ~WebView() { delete m_webView; }

    void load(const QString& html)
    {
        m_frameLoaded = false;
        WKPageLoadURL(m_webView->pageRef(), WKURLCreateWithUTF8CString(html.toLocal8Bit().data()));
        QVERIFY(::waitForSignal(this, SIGNAL(loaded()), 5000));
    }

    void setDrawBackground(bool value) { m_webView->setDrawBackground(value); }
    void setTransparentBackground(bool value) { m_webView->setTransparentBackground(value); }

    virtual void viewNeedsDisplay(const QRect&)
    {
        m_webView->paint(QMatrix4x4(), 1, 0);
    }

    virtual void viewRequestedScroll(const QPoint&) { }
    virtual void viewProcessCrashed() { }
    virtual void viewProcessRelaunched() { }
    virtual void viewContentSizeChanged(const QSize&) { }
    virtual void viewRequestedCursorOverride(const QCursor&) { }
    virtual void doneWithKeyEvent(const QKeyEvent*, bool wasHandled) { }
    virtual void doneWithTouchEvent(const QTouchEvent*, bool wasHandled) { }

    void frameLoaded()
    {
        m_frameLoaded = true;
        WKPageForceRepaint(m_webView->pageRef(), this, finishForceRepaint);
    }

    void onRepaintDone()
    {
        emit loaded();
    }

    static void finishForceRepaint(WKErrorRef, void* context)
    {
        static_cast<WebView*>(context)->onRepaintDone();
    }

    static void didLayout(WKPageRef page, WKLayoutMilestones milestones, WKTypeRef userData, const void *clientInfo)
    {
        static_cast<WebView*>(const_cast<void*>(clientInfo))->frameLoaded();
    }

Q_SIGNALS:
    void loaded();

private:
    QRawWebView* m_webView;
    bool m_frameLoaded;
};

static bool compareImages(const QImage& i1, const QImage& i2, int count)
{
    if (i1.size() != i2.size())
        return false;
    for (int x = 0; x < count; ++x) {
        for (int y = 0; y < count; ++y) {
            QPoint point(x * i1.width() / count, y * i1.height() / count);
            if (i1.pixel(point) != i2.pixel(point))
                return false;
        }
    }

    return true;
}

class tst_qrawwebview : public QObject {
    Q_OBJECT
public:
    tst_qrawwebview()
        : m_resourceDir(QString::fromLatin1(TESTS_SOURCE_DIR "/html/resources"))
        , m_baseUrl(QUrl::fromLocalFile(TESTS_SOURCE_DIR "/html").toString())
    {
        addQtWebProcessToPath();
    }

private Q_SLOTS:
    void paint() { run(&tst_qrawwebview::doPaint, m_resourceDir + "/qwkview_paint.png"); }
    void noBackground1() { run(&tst_qrawwebview::doNoBackground1, m_resourceDir + "/qwkview_noBackground1.png"); }
    void noBackground2() { run(&tst_qrawwebview::doNoBackground2, m_resourceDir + "/qwkview_noBackground1.png"); }
    void noBackground3() { run(&tst_qrawwebview::doNoBackground3, m_resourceDir + "/qwkview_noBackground3.png"); }

private:
    const QString m_resourceDir;
    const QString m_baseUrl;

    void doPaint(const QSize& size);
    void doNoBackground1(const QSize& size);
    void doNoBackground2(const QSize& size);
    void doNoBackground3(const QSize& size);

    typedef void (tst_qrawwebview::*PaintMethod)(const QSize& size);
    void run(PaintMethod, const QString& expectation);
};

void tst_qrawwebview::doPaint(const QSize& size)
{
    WebView view(size);
    view.load(m_baseUrl + "/redsquare.html");
}

void tst_qrawwebview::doNoBackground1(const QSize& size)
{
    WebView view(size, true);
    view.load(m_baseUrl + "/redsquare.html");
    view.load(m_baseUrl + "/bluesquare.html");
}

void tst_qrawwebview::doNoBackground2(const QSize& size)
{
    WebView view1(size, true);
    view1.load(m_baseUrl + "/redsquare.html");

    WebView view2(size, true);
    view2.load(m_baseUrl + "/bluesquare.html");
}

void tst_qrawwebview::doNoBackground3(const QSize& size)
{
    WebView view1(size, false);
    view1.load(m_baseUrl + "/redsquare.html");

    WebView view2(size, true);
    view2.load(m_baseUrl + "/bluesquare.html");
}

void tst_qrawwebview::run(PaintMethod method, const QString& expectation)
{
    QWindow window;
    window.setSurfaceType(QSurface::OpenGLSurface);
    window.setGeometry(0, 0, 200, 200);
    window.create();

    QOpenGLContext context;
    context.create();
    context.makeCurrent(&window);

    glViewport(0, 0, window.size().width(), window.size().height());
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    (this->*method)(window.size());

    QImage image(window.size(), QImage::Format_ARGB32_Premultiplied);
    glReadPixels(0, 0, window.size().width(), window.size().height(), GL_RGBA, GL_UNSIGNED_BYTE, image.bits());

    QVERIFY(compareImages(QImage(expectation), image.rgbSwapped(), 5));
}

QTEST_MAIN(tst_qrawwebview)

#include "tst_qrawwebview.moc"
