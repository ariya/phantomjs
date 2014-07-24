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


#include "../../util.h"
#include <QGLWidget>
#include <QGraphicsView>
#include <QGraphicsWebView>
#include <QScopedPointer>
#include <QWebFrame>
#include <QtTest/QtTest>

class GraphicsView;

class tst_WebGlPerformance : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();

    void benchSoftwareFallbackRgb16();
    void benchSoftwareFallbackRgb32();
    void benchSoftwareFallbackArgb32();
    void benchSoftwareFallbackArgb32Premultiplied();

private:
    void benchmarkFrameRenderingOnImage(QImage::Format);

    QScopedPointer<GraphicsView> m_view;
};

class GraphicsView : public QGraphicsView {
public:
    GraphicsView();
    QGraphicsWebView* m_webView;

protected:
    void resizeEvent(QResizeEvent*);
};

GraphicsView::GraphicsView()
{
    QGraphicsScene* const scene = new QGraphicsScene(this);
    setScene(scene);

    m_webView = new QGraphicsWebView;
    scene->addItem(m_webView);

    m_webView->page()->settings()->setAttribute(QWebSettings::WebGLEnabled, true);

    resize(800, 600);
    setFrameShape(QFrame::NoFrame);
    setViewport(new QGLWidget);
}

void GraphicsView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    QRectF rect(QPoint(0, 0), event->size());
    m_webView->setGeometry(rect);
    scene()->setSceneRect(rect);
}

void tst_WebGlPerformance::init()
{
    m_view.reset(new GraphicsView);
    m_view->showMaximized();
    QTest::qWaitForWindowShown(m_view.data());
}

void tst_WebGlPerformance::cleanup()
{
    m_view.reset();
}

void tst_WebGlPerformance::benchSoftwareFallbackRgb16()
{
    benchmarkFrameRenderingOnImage(QImage::Format_RGB16);
}

void tst_WebGlPerformance::benchSoftwareFallbackRgb32()
{
    benchmarkFrameRenderingOnImage(QImage::Format_RGB32);
}

void tst_WebGlPerformance::benchSoftwareFallbackArgb32()
{
    benchmarkFrameRenderingOnImage(QImage::Format_ARGB32);
}

void tst_WebGlPerformance::benchSoftwareFallbackArgb32Premultiplied()
{
    benchmarkFrameRenderingOnImage(QImage::Format_ARGB32_Premultiplied);
}

void tst_WebGlPerformance::benchmarkFrameRenderingOnImage(QImage::Format format)
{
    m_view->m_webView->load(QUrl(QLatin1String("qrc:///testcases/10000_triangles.html")));
    const bool pageLoaded = waitForSignal(m_view->m_webView, SIGNAL(loadFinished(bool)));
    Q_ASSERT(pageLoaded);
    Q_UNUSED(pageLoaded);

    QImage target(m_view->size(), format);
    QBENCHMARK {
        QPainter painter(&target);
        m_view->render(&painter);
        painter.end();
    }
}

QTEST_MAIN(tst_WebGlPerformance)

#include "tst_webgl.moc"
