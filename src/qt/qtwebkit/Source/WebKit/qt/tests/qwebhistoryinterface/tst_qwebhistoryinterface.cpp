/*
    Copyright (C) 2008 Holger Hans Peter Freyther

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


#include <QtTest/QtTest>

#include <qwebpage.h>
#include <qwebview.h>
#include <qwebframe.h>
#include <qwebelement.h>
#include <qwebhistoryinterface.h>
#include <QDebug>

class tst_QWebHistoryInterface : public QObject
{
    Q_OBJECT

public:
    tst_QWebHistoryInterface();
    virtual ~tst_QWebHistoryInterface();

public Q_SLOTS:
    void init();
    void cleanup();

private Q_SLOTS:
    void visitedLinks();

private:


private:
    QWebView* m_view;
    QWebPage* m_page;
};

tst_QWebHistoryInterface::tst_QWebHistoryInterface()
{
}

tst_QWebHistoryInterface::~tst_QWebHistoryInterface()
{
}

void tst_QWebHistoryInterface::init()
{
    m_view = new QWebView();
    m_page = m_view->page();
}

void tst_QWebHistoryInterface::cleanup()
{
    delete m_view;
}

class FakeHistoryImplementation : public QWebHistoryInterface {
public:
    void addHistoryEntry(const QString&) {}
    bool historyContains(const QString& url) const {
        return url == QLatin1String("http://www.trolltech.com/");
    }
};


/*
 * Test that visited links are properly colored. http://www.trolltech.com is marked
 * as visited, so the below website should have exactly one element in the a:visited
 * state.
 */
void tst_QWebHistoryInterface::visitedLinks()
{
    QWebHistoryInterface::setDefaultInterface(new FakeHistoryImplementation);
    m_view->setHtml("<html><style>:link{color:green}:visited{color:red}</style><body><a href='http://www.trolltech.com' id='vlink'>Trolltech</a></body></html>");
    QWebElement anchor = m_view->page()->mainFrame()->findFirstElement("a[id=vlink]");
    QString linkColor = anchor.styleProperty("color", QWebElement::ComputedStyle);
    QCOMPARE(linkColor, QString::fromLatin1("rgb(255, 0, 0)"));
}

QTEST_MAIN(tst_QWebHistoryInterface)
#include "tst_qwebhistoryinterface.moc"
