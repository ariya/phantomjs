/*
 * Copyright (C) 2009 Holger Hans Peter Freyther
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
 */

#ifndef QT_NO_BEARERMANAGEMENT
#include <QNetworkConfigurationManager>
#endif

#include <QtTest/QtTest>

#include <qwebelement.h>
#include <qwebframe.h>
#include <qwebview.h>
#include <qpainter.h>

#include "util.h"

class tst_Painting : public QObject
{
    Q_OBJECT

public:

public Q_SLOTS:
    void init();
    void cleanup();

private Q_SLOTS:
    void paint_data();
    void paint();
    void textAreas();

private:
#ifndef QT_NO_BEARERMANAGEMENT
    QNetworkConfigurationManager m_manager;
#endif
    QWebView* m_view;
    QWebPage* m_page;
};

void tst_Painting::init()
{
    m_view = new QWebView;
    m_page = m_view->page();

    QSize viewportSize(1024, 768);
    m_view->setFixedSize(viewportSize);
    m_page->setViewportSize(viewportSize);
}

void tst_Painting::cleanup()
{
    delete m_view;
}

void tst_Painting::paint_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::newRow("amazon") << QUrl("http://www.amazon.com");
}

void tst_Painting::paint()
{
    QFETCH(QUrl, url);

#ifndef QT_NO_BEARERMANAGEMENT
    if (!m_manager.isOnline())
        W_QSKIP("This test requires an active network connection", SkipSingle);
#endif

    m_view->load(url);
    ::waitForSignal(m_view, SIGNAL(loadFinished(bool)), 0);

    /* force a layout */
    QWebFrame* mainFrame = m_page->mainFrame();
    mainFrame->toPlainText();

    QPixmap pixmap(m_page->viewportSize());
    QBENCHMARK {
        QPainter painter(&pixmap);
        mainFrame->render(&painter, QRect(QPoint(0, 0), m_page->viewportSize()));
        painter.end();
    }
}

void tst_Painting::textAreas()
{
    m_view->load(QUrl("data:text/html;<html><body></body></html>"));
    ::waitForSignal(m_view, SIGNAL(loadFinished(bool)), 0);

    QWebElement bodyElement = m_page->mainFrame()->findFirstElement("body");

    int count = 100;
    while (count--) {
        QString markup("<textarea cols='1' rows='1'></textarea>");
        bodyElement.appendInside(markup);
    }

    /* force a layout */
    QWebFrame* mainFrame = m_page->mainFrame();
    mainFrame->toPlainText();

    QPixmap pixmap(mainFrame->contentsSize());
    QBENCHMARK {
        QPainter painter(&pixmap);
        mainFrame->render(&painter, QRect(QPoint(0, 0), mainFrame->contentsSize()));
        painter.end();
    }
}

QTEST_MAIN(tst_Painting)
#include "tst_painting.moc"
