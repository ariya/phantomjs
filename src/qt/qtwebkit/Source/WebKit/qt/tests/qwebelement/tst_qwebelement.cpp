/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)

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
#include <qwidget.h>
#include <qwebview.h>
#include <qwebframe.h>
#include <qwebelement.h>
#include <util.h>
//TESTED_CLASS=
//TESTED_FILES=

class tst_QWebElement : public QObject
{
    Q_OBJECT

public:
    tst_QWebElement();
    virtual ~tst_QWebElement();

public Q_SLOTS:
    void init();
    void cleanup();

private Q_SLOTS:
    void textHtml();
    void simpleCollection();
    void attributes();
    void attributesNS();
    void listAttributes();
    void classes();
    void namespaceURI();
    void iteration();
    void nonConstIterator();
    void constIterator();
    void foreachManipulation();
    void emptyCollection();
    void appendCollection();
    void evaluateJavaScript();
    void documentElement();
    void frame();
    void style();
    void computedStyle();
    void appendAndPrepend();
    void insertBeforeAndAfter();
    void remove();
    void clear();
    void replaceWith();
    void encloseWith();
    void encloseContentsWith();
    void nullSelect();
    void firstChildNextSibling();
    void lastChildPreviousSibling();
    void hasSetFocus();
    void render();
    void addElementToHead();

private:
    QWebView* m_view;
    QWebPage* m_page;
    QWebFrame* m_mainFrame;
};

tst_QWebElement::tst_QWebElement()
{
}

tst_QWebElement::~tst_QWebElement()
{
}

void tst_QWebElement::init()
{
    m_view = new QWebView();
    m_page = m_view->page();
    m_mainFrame = m_page->mainFrame();
}

void tst_QWebElement::cleanup()
{
    delete m_view;
}

void tst_QWebElement::textHtml()
{
    QString html = "<head></head><body><p>test</p></body>";
    m_mainFrame->setHtml(html);
    QWebElement body = m_mainFrame->documentElement();
    QVERIFY(!body.isNull());

    QCOMPARE(body.toPlainText(), QString("test"));
    QCOMPARE(body.toPlainText(), m_mainFrame->toPlainText());

    QCOMPARE(body.toInnerXml(), html);
}

void tst_QWebElement::simpleCollection()
{
    QString html = "<body><p>first para</p><p>second para</p></body>";
    m_mainFrame->setHtml(html);
    QWebElement body = m_mainFrame->documentElement();

    QWebElementCollection list = body.findAll("p");
    QCOMPARE(list.count(), 2);
    QCOMPARE(list.at(0).toPlainText(), QString("first para"));
    QCOMPARE(list.at(1).toPlainText(), QString("second para"));
}

void tst_QWebElement::attributes()
{
    m_mainFrame->setHtml("<body><p>Test");
    QWebElement body = m_mainFrame->documentElement();

    QVERIFY(!body.hasAttribute("title"));
    QVERIFY(!body.hasAttributes());

    body.setAttribute("title", "test title");

    QVERIFY(body.hasAttributes());
    QVERIFY(body.hasAttribute("title"));

    QCOMPARE(body.attribute("title"), QString("test title"));

    body.removeAttribute("title");

    QVERIFY(!body.hasAttribute("title"));
    QVERIFY(!body.hasAttributes());

    QCOMPARE(body.attribute("does-not-exist", "testvalue"), QString("testvalue"));
}

void tst_QWebElement::attributesNS()
{
    QString content = "<html xmlns=\"http://www.w3.org/1999/xhtml\" "
                      "xmlns:svg=\"http://www.w3.org/2000/svg\">"
                      "<body><svg:svg id=\"foobar\" width=\"400px\" height=\"300px\">"
                      "</svg:svg></body></html>";

    m_mainFrame->setContent(content.toUtf8(), "application/xhtml+xml");

    QWebElement svg = m_mainFrame->findFirstElement("svg");
    QVERIFY(!svg.isNull());

    QVERIFY(!svg.hasAttributeNS("http://www.w3.org/2000/svg", "foobar"));
    QCOMPARE(svg.attributeNS("http://www.w3.org/2000/svg", "foobar", "defaultblah"), QString("defaultblah"));
    svg.setAttributeNS("http://www.w3.org/2000/svg", "svg:foobar", "true");
    QVERIFY(svg.hasAttributeNS("http://www.w3.org/2000/svg", "foobar"));
    QCOMPARE(svg.attributeNS("http://www.w3.org/2000/svg", "foobar", "defaultblah"), QString("true"));
}

void tst_QWebElement::listAttributes()
{
    QString content = "<html xmlns=\"http://www.w3.org/1999/xhtml\" "
                      "xmlns:svg=\"http://www.w3.org/2000/svg\">"
                      "<body><svg:svg foo=\"\" svg:bar=\"\">"
                      "</svg:svg></body></html>";

    m_mainFrame->setContent(content.toUtf8(), "application/xhtml+xml");

    QWebElement svg = m_mainFrame->findFirstElement("svg");
    QVERIFY(!svg.isNull());

    QVERIFY(svg.attributeNames().contains("foo"));
    QVERIFY(svg.attributeNames("http://www.w3.org/2000/svg").contains("bar"));

    svg.setAttributeNS("http://www.w3.org/2000/svg", "svg:foobar", "true");
    QVERIFY(svg.attributeNames().contains("foo"));
    QStringList attributes = svg.attributeNames("http://www.w3.org/2000/svg");
    QCOMPARE(attributes.size(), 2);
    QVERIFY(attributes.contains("bar"));
    QVERIFY(attributes.contains("foobar"));
}

void tst_QWebElement::classes()
{
    m_mainFrame->setHtml("<body><p class=\"a b c d a c\">Test");

    QWebElement body = m_mainFrame->documentElement();
    QCOMPARE(body.classes().count(), 0);

    QWebElement p = m_mainFrame->documentElement().findAll("p").at(0);
    QStringList classes = p.classes();
    QCOMPARE(classes.count(), 4);
    QCOMPARE(classes[0], QLatin1String("a"));
    QCOMPARE(classes[1], QLatin1String("b"));
    QCOMPARE(classes[2], QLatin1String("c"));
    QCOMPARE(classes[3], QLatin1String("d"));
    QVERIFY(p.hasClass("a"));
    QVERIFY(p.hasClass("b"));
    QVERIFY(p.hasClass("c"));
    QVERIFY(p.hasClass("d"));
    QVERIFY(!p.hasClass("e"));

    p.addClass("f");
    QVERIFY(p.hasClass("f"));
    p.addClass("a");
    QCOMPARE(p.classes().count(), 5);
    QVERIFY(p.hasClass("a"));
    QVERIFY(p.hasClass("b"));
    QVERIFY(p.hasClass("c"));
    QVERIFY(p.hasClass("d"));

    p.toggleClass("a");
    QVERIFY(!p.hasClass("a"));
    QVERIFY(p.hasClass("b"));
    QVERIFY(p.hasClass("c"));
    QVERIFY(p.hasClass("d"));
    QVERIFY(p.hasClass("f"));
    QCOMPARE(p.classes().count(), 4);
    p.toggleClass("f");
    QVERIFY(!p.hasClass("f"));
    QCOMPARE(p.classes().count(), 3);
    p.toggleClass("a");
    p.toggleClass("f");
    QVERIFY(p.hasClass("a"));
    QVERIFY(p.hasClass("f"));
    QCOMPARE(p.classes().count(), 5);

    p.removeClass("f");
    QVERIFY(!p.hasClass("f"));
    QCOMPARE(p.classes().count(), 4);
    p.removeClass("d");
    QVERIFY(!p.hasClass("d"));
    QCOMPARE(p.classes().count(), 3);
    p.removeClass("not-exist");
    QCOMPARE(p.classes().count(), 3);
    p.removeClass("c");
    QVERIFY(!p.hasClass("c"));
    QCOMPARE(p.classes().count(), 2);
    p.removeClass("b");
    QVERIFY(!p.hasClass("b"));
    QCOMPARE(p.classes().count(), 1);
    p.removeClass("a");
    QVERIFY(!p.hasClass("a"));
    QCOMPARE(p.classes().count(), 0);
    p.removeClass("foobar");
    QCOMPARE(p.classes().count(), 0);
}

void tst_QWebElement::namespaceURI()
{
    QString content = "<html xmlns=\"http://www.w3.org/1999/xhtml\" "
                      "xmlns:svg=\"http://www.w3.org/2000/svg\">"
                      "<body><svg:svg id=\"foobar\" width=\"400px\" height=\"300px\">"
                      "</svg:svg></body></html>";

    m_mainFrame->setContent(content.toUtf8(), "application/xhtml+xml");
    QWebElement body = m_mainFrame->documentElement();
    QCOMPARE(body.namespaceUri(), QLatin1String("http://www.w3.org/1999/xhtml"));

    QWebElement svg = body.findAll("*#foobar").at(0);
    QCOMPARE(svg.prefix(), QLatin1String("svg"));
    QCOMPARE(svg.localName(), QLatin1String("svg"));
    QCOMPARE(svg.tagName(), QLatin1String("svg:svg"));
    QCOMPARE(svg.namespaceUri(), QLatin1String("http://www.w3.org/2000/svg"));

}

void tst_QWebElement::iteration()
{
    QString html = "<body><p>first para</p><p>second para</p></body>";
    m_mainFrame->setHtml(html);
    QWebElement body = m_mainFrame->documentElement();

   QWebElementCollection paras = body.findAll("p");
    QList<QWebElement> referenceList = paras.toList();

    QList<QWebElement> foreachList;
    foreach(QWebElement p, paras) {
       foreachList.append(p);
    }
    QVERIFY(foreachList.count() == 2);
    QCOMPARE(foreachList.count(), referenceList.count());
    QCOMPARE(foreachList.at(0), referenceList.at(0));
    QCOMPARE(foreachList.at(1), referenceList.at(1));

    QList<QWebElement> forLoopList;
    for (int i = 0; i < paras.count(); ++i) {
        forLoopList.append(paras.at(i));
    }
    QVERIFY(foreachList.count() == 2);
    QCOMPARE(foreachList.count(), referenceList.count());
    QCOMPARE(foreachList.at(0), referenceList.at(0));
    QCOMPARE(foreachList.at(1), referenceList.at(1));

    for (int i = 0; i < paras.count(); ++i) {
        QCOMPARE(paras.at(i), paras[i]);
    }

    QCOMPARE(paras.at(0), paras.first());
    QCOMPARE(paras.at(1), paras.last());
}

void tst_QWebElement::nonConstIterator()
{
    QString html = "<body><p>first para</p><p>second para</p></body>";
    m_mainFrame->setHtml(html);
    QWebElement body = m_mainFrame->documentElement();
    QWebElementCollection paras = body.findAll("p");

    QWebElementCollection::iterator it = paras.begin();
    QCOMPARE(*it, paras.at(0));
    ++it;
    (*it).encloseWith("<div>");
    QCOMPARE(*it, paras.at(1));
    ++it;
    QCOMPARE(it,  paras.end());
}

void tst_QWebElement::constIterator()
{
    QString html = "<body><p>first para</p><p>second para</p></body>";
    m_mainFrame->setHtml(html);
    QWebElement body = m_mainFrame->documentElement();
    const QWebElementCollection paras = body.findAll("p");

    QWebElementCollection::const_iterator it = paras.begin();
    QCOMPARE(*it, paras.at(0));
    ++it;
    QCOMPARE(*it, paras.at(1));
    ++it;
    QCOMPARE(it,  paras.end());
}

void tst_QWebElement::foreachManipulation()
{
    QString html = "<body><p>first para</p><p>second para</p></body>";
    m_mainFrame->setHtml(html);
    QWebElement body = m_mainFrame->documentElement();

    foreach(QWebElement p, body.findAll("p")) {
        p.setInnerXml("<div>foo</div><div>bar</div>");
    }

    QCOMPARE(body.findAll("div").count(), 4);
}

void tst_QWebElement::emptyCollection()
{
    QWebElementCollection emptyCollection;
    QCOMPARE(emptyCollection.count(), 0);
}

void tst_QWebElement::appendCollection()
{
    QString html = "<body><span class='a'>aaa</span><p>first para</p><div>foo</div>"
        "<span class='b'>bbb</span><p>second para</p><div>bar</div></body>";
    m_mainFrame->setHtml(html);
    QWebElement body = m_mainFrame->documentElement();

    QWebElementCollection collection = body.findAll("p");
    QCOMPARE(collection.count(), 2);

    collection.append(body.findAll("div"));
    QCOMPARE(collection.count(), 4);

    collection += body.findAll("span.a");
    QCOMPARE(collection.count(), 5);

    QWebElementCollection all = collection + body.findAll("span.b");
    QCOMPARE(all.count(), 6);
    QCOMPARE(collection.count(), 5);

     all += collection;
    QCOMPARE(all.count(), 11);

    QCOMPARE(collection.count(), 5);
    QWebElementCollection test;
    test.append(collection);
    QCOMPARE(test.count(), 5);
    test.append(QWebElementCollection());
    QCOMPARE(test.count(), 5);
}

void tst_QWebElement::evaluateJavaScript()
{
    QVariant result;
    m_mainFrame->setHtml("<body><p>test");
    QWebElement para = m_mainFrame->findFirstElement("p");

    result = para.evaluateJavaScript("this.tagName");
    QVERIFY(result.isValid());
    QVERIFY(result.type() == QVariant::String);
    QCOMPARE(result.toString(), QLatin1String("P"));

    result = para.evaluateJavaScript("this.hasAttributes()");
    QVERIFY(result.isValid());
    QVERIFY(result.type() == QVariant::Bool);
    QVERIFY(!result.toBool());

    para.evaluateJavaScript("this.setAttribute('align', 'left');");
    QCOMPARE(para.attribute("align"), QLatin1String("left"));

    result = para.evaluateJavaScript("this.hasAttributes()");
    QVERIFY(result.isValid());
    QVERIFY(result.type() == QVariant::Bool);
    QVERIFY(result.toBool());
}

void tst_QWebElement::documentElement()
{
    m_mainFrame->setHtml("<body><p>Test");

    QWebElement para = m_mainFrame->documentElement().findAll("p").at(0);
    QVERIFY(para.parent().parent() == m_mainFrame->documentElement());
    QVERIFY(para.document() == m_mainFrame->documentElement());
}

void tst_QWebElement::frame()
{
    m_mainFrame->setHtml("<body><p>test");

    QWebElement doc = m_mainFrame->documentElement();
    QVERIFY(doc.webFrame() == m_mainFrame);

    m_mainFrame->load(QUrl("data:text/html,<frameset cols=\"25%,75%\"><frame src=\"data:text/html,"
                           "<p>frame1\">"
                           "<frame src=\"data:text/html,<p>frame2\"></frameset>"));

    waitForSignal(m_page, SIGNAL(loadFinished(bool)));

    QCOMPARE(m_mainFrame->childFrames().count(), 2);

    QWebFrame* firstFrame = m_mainFrame->childFrames().at(0);
    QWebFrame* secondFrame = m_mainFrame->childFrames().at(1);

    QCOMPARE(firstFrame->toPlainText(), QString("frame1"));
    QCOMPARE(secondFrame->toPlainText(), QString("frame2"));

    QWebElement firstPara = firstFrame->documentElement().findAll("p").at(0);
    QWebElement secondPara = secondFrame->documentElement().findAll("p").at(0);

    QVERIFY(firstPara.webFrame() == firstFrame);
    QVERIFY(secondPara.webFrame() == secondFrame);
}

void tst_QWebElement::style()
{
    QString html = "<head>"
        "<style type='text/css'>"
            "p { color: green !important }"
            "#idP { color: red }"
            ".classP { color : yellow ! important }"
        "</style>"
    "</head>"
    "<body>"
        "<p id='idP' class='classP' style='color: blue;'>some text</p>"
    "</body>";

    m_mainFrame->setHtml(html);

    QWebElement p = m_mainFrame->documentElement().findAll("p").at(0);
    QCOMPARE(p.styleProperty("color", QWebElement::InlineStyle), QLatin1String("blue"));
    QVERIFY(p.styleProperty("cursor", QWebElement::InlineStyle).isEmpty());

    p.setStyleProperty("color", "red");
    p.setStyleProperty("cursor", "auto");

    QCOMPARE(p.styleProperty("color", QWebElement::InlineStyle), QLatin1String("red"));
    QCOMPARE(p.styleProperty("color", QWebElement::CascadedStyle), QLatin1String("yellow"));
    QCOMPARE(p.styleProperty("cursor", QWebElement::InlineStyle), QLatin1String("auto"));

    p.setStyleProperty("color", "green !important");
    QCOMPARE(p.styleProperty("color", QWebElement::InlineStyle), QLatin1String("green"));
    QCOMPARE(p.styleProperty("color", QWebElement::CascadedStyle), QLatin1String("green"));

    p.setStyleProperty("color", "blue");
    // A current important InlineStyle shouldn't be overwritten by a non-important one.
    QCOMPARE(p.styleProperty("color", QWebElement::InlineStyle), QLatin1String("green"));
    QCOMPARE(p.styleProperty("color", QWebElement::CascadedStyle), QLatin1String("green"));

    p.setStyleProperty("color", "blue !important");
    QCOMPARE(p.styleProperty("color", QWebElement::InlineStyle), QLatin1String("blue"));
    QCOMPARE(p.styleProperty("color", QWebElement::CascadedStyle), QLatin1String("blue"));

    QString html2 = "<head>"
        "<style type='text/css'>"
            "p { color: green }"
            "#idP { color: red }"
            ".classP { color: yellow }"
        "</style>"
    "</head>"
    "<body>"
        "<p id='idP' class='classP' style='color: blue;'>some text</p>"
    "</body>";

    m_mainFrame->setHtml(html2);
    p = m_mainFrame->documentElement().findAll("p").at(0);

    QCOMPARE(p.styleProperty("color", QWebElement::InlineStyle), QLatin1String("blue"));
    QCOMPARE(p.styleProperty("color", QWebElement::CascadedStyle), QLatin1String("blue"));

    QString html3 = "<head>"
        "<style type='text/css'>"
            "p { color: green !important }"
            "#idP { color: red !important}"
            ".classP { color: yellow !important}"
        "</style>"
    "</head>"
    "<body>"
        "<p id='idP' class='classP' style='color: blue !important;'>some text</p>"
    "</body>";

    m_mainFrame->setHtml(html3);
    p = m_mainFrame->documentElement().findAll("p").at(0);

    QCOMPARE(p.styleProperty("color", QWebElement::InlineStyle), QLatin1String("blue"));
    QCOMPARE(p.styleProperty("color", QWebElement::CascadedStyle), QLatin1String("blue"));

    QString html5 = "<head>"
        "<style type='text/css'>"
            "p { color: green }"
            "#idP { color: red }"
            ".classP { color: yellow }"
        "</style>"
    "</head>"
    "<body>"
        "<p id='idP' class='classP'>some text</p>"
    "</body>";

    m_mainFrame->setHtml(html5);
    p = m_mainFrame->documentElement().findAll("p").at(0);

    QCOMPARE(p.styleProperty("color", QWebElement::InlineStyle), QLatin1String(""));
    QCOMPARE(p.styleProperty("color", QWebElement::CascadedStyle), QLatin1String("red"));

    QString html6 = "<head>"
        "<link rel='stylesheet' href='qrc:/style.css' type='text/css' />"
        "<style type='text/css'>"
            "p { color: green }"
            "#idP { color: red }"
            ".classP { color: yellow ! important}"
        "</style>"
    "</head>"
    "<body>"
        "<p id='idP' class='classP' style='color: blue;'>some text</p>"
    "</body>";

    // in few seconds, the CSS should be completey loaded
    m_mainFrame->setHtml(html6);
    waitForSignal(m_page, SIGNAL(loadFinished(bool)), 200);

    p = m_mainFrame->documentElement().findAll("p").at(0);
    QCOMPARE(p.styleProperty("color", QWebElement::InlineStyle), QLatin1String("blue"));
    QCOMPARE(p.styleProperty("color", QWebElement::CascadedStyle), QLatin1String("black"));

    QString html7 = "<head>"
        "<style type='text/css'>"
            "@import url(qrc:/style2.css);"
        "</style>"
        "<link rel='stylesheet' href='qrc:/style.css' type='text/css' />"
    "</head>"
    "<body>"
        "<p id='idP' style='color: blue;'>some text</p>"
    "</body>";

    // in few seconds, the style should be completey loaded
    m_mainFrame->setHtml(html7);
    waitForSignal(m_page, SIGNAL(loadFinished(bool)), 200);

    p = m_mainFrame->documentElement().findAll("p").at(0);
    QCOMPARE(p.styleProperty("color", QWebElement::CascadedStyle), QLatin1String("black"));

    QString html8 = "<body><p>some text</p></body>";

    m_mainFrame->setHtml(html8);
    p = m_mainFrame->documentElement().findAll("p").at(0);

    QCOMPARE(p.styleProperty("color", QWebElement::InlineStyle), QLatin1String(""));
    QCOMPARE(p.styleProperty("color", QWebElement::CascadedStyle), QLatin1String(""));
}

void tst_QWebElement::computedStyle()
{
    QString html = "<body><p>some text</p></body>";
    m_mainFrame->setHtml(html);

    QWebElement p = m_mainFrame->documentElement().findAll("p").at(0);
    QCOMPARE(p.styleProperty("cursor", QWebElement::ComputedStyle), QLatin1String("auto"));
    QVERIFY(!p.styleProperty("cursor", QWebElement::ComputedStyle).isEmpty());
    QVERIFY(p.styleProperty("cursor", QWebElement::InlineStyle).isEmpty());

    p.setStyleProperty("cursor", "text");
    p.setStyleProperty("color", "red");

    QCOMPARE(p.styleProperty("cursor", QWebElement::ComputedStyle), QLatin1String("text"));
    QCOMPARE(p.styleProperty("color", QWebElement::ComputedStyle), QLatin1String("rgb(255, 0, 0)"));
    QCOMPARE(p.styleProperty("color", QWebElement::InlineStyle), QLatin1String("red"));
}

void tst_QWebElement::appendAndPrepend()
{
    QString html = "<body>"
        "<p>"
            "foo"
        "</p>"
        "<p>"
            "bar"
        "</p>"
    "</body>";

    m_mainFrame->setHtml(html);
    QWebElement body = m_mainFrame->documentElement().findFirst("body");

    QCOMPARE(body.findAll("p").count(), 2);
    body.appendInside(body.findFirst("p"));
    QCOMPARE(body.findAll("p").count(), 2);
    QCOMPARE(body.findFirst("p").toPlainText(), QString("bar"));
    QCOMPARE(body.findAll("p").last().toPlainText(), QString("foo"));

    body.appendInside(body.findFirst("p").clone());
    QCOMPARE(body.findAll("p").count(), 3);
    QCOMPARE(body.findFirst("p").toPlainText(), QString("bar"));
    QCOMPARE(body.findAll("p").last().toPlainText(), QString("bar"));

    body.prependInside(body.findAll("p").at(1).clone());
    QCOMPARE(body.findAll("p").count(), 4);
    QCOMPARE(body.findFirst("p").toPlainText(), QString("foo"));

    body.findFirst("p").appendInside("<div>booyakasha</div>");
    QCOMPARE(body.findAll("p div").count(), 1);
    QCOMPARE(body.findFirst("p div").toPlainText(), QString("booyakasha"));

    body.findFirst("div").prependInside("<code>yepp</code>");
    QCOMPARE(body.findAll("p div code").count(), 1);
    QCOMPARE(body.findFirst("p div code").toPlainText(), QString("yepp"));

    // Inserting HTML into an img tag is not allowed, but appending/prepending outside is.
    body.findFirst("div").appendInside("<img src=\"test.png\">");
    QCOMPARE(body.findAll("p div img").count(), 1);

    QWebElement img = body.findFirst("img");
    QVERIFY(!img.isNull());
    img.appendInside("<p id=\"fail1\"></p>");
    QCOMPARE(body.findAll("p#fail1").count(), 0);

    img.appendOutside("<p id=\"success1\"></p>");
    QCOMPARE(body.findAll("p#success1").count(), 1);

    img.prependInside("<p id=\"fail2\"></p>");
    QCOMPARE(body.findAll("p#fail2").count(), 0);

    img.prependOutside("<p id=\"success2\"></p>");
    QCOMPARE(body.findAll("p#success2").count(), 1);


}

void tst_QWebElement::insertBeforeAndAfter()
{
    QString html = "<body>"
        "<p>"
            "foo"
        "</p>"
        "<div>"
            "yeah"
        "</div>"
        "<p>"
            "bar"
        "</p>"
    "</body>";

    m_mainFrame->setHtml(html);
    QWebElement body = m_mainFrame->documentElement().findFirst("body");
    QWebElement div = body.findFirst("div");

    QCOMPARE(body.findAll("p").count(), 2);
    QCOMPARE(body.findAll("div").count(), 1);

    div.prependOutside(body.findAll("p").last().clone());
    QCOMPARE(body.findAll("p").count(), 3);
    QCOMPARE(body.findAll("p").at(0).toPlainText(), QString("foo"));
    QCOMPARE(body.findAll("p").at(1).toPlainText(), QString("bar"));
    QCOMPARE(body.findAll("p").at(2).toPlainText(), QString("bar"));

    div.appendOutside(body.findFirst("p").clone());
    QCOMPARE(body.findAll("p").count(), 4);
    QCOMPARE(body.findAll("p").at(0).toPlainText(), QString("foo"));
    QCOMPARE(body.findAll("p").at(1).toPlainText(), QString("bar"));
    QCOMPARE(body.findAll("p").at(2).toPlainText(), QString("foo"));
    QCOMPARE(body.findAll("p").at(3).toPlainText(), QString("bar"));

    div.prependOutside("<span>hey</span>");
    QCOMPARE(body.findAll("span").count(), 1);

    div.appendOutside("<span>there</span>");
    QCOMPARE(body.findAll("span").count(), 2);
    QCOMPARE(body.findAll("span").at(0).toPlainText(), QString("hey"));
    QCOMPARE(body.findAll("span").at(1).toPlainText(), QString("there"));
}

void tst_QWebElement::remove()
{
    QString html = "<body>"
        "<p>"
            "foo"
        "</p>"
        "<div>"
            "<p>yeah</p>"
        "</div>"
        "<p>"
            "bar"
        "</p>"
    "</body>";

    m_mainFrame->setHtml(html);
    QWebElement body = m_mainFrame->documentElement().findFirst("body");

    QCOMPARE(body.findAll("div").count(), 1);
    QCOMPARE(body.findAll("p").count(), 3);

    QWebElement div = body.findFirst("div");
    div.takeFromDocument();

    QCOMPARE(div.isNull(), false);
    QCOMPARE(body.findAll("div").count(), 0);
    QCOMPARE(body.findAll("p").count(), 2);

    body.appendInside(div);

    QCOMPARE(body.findAll("div").count(), 1);
    QCOMPARE(body.findAll("p").count(), 3);
}

void tst_QWebElement::clear()
{
    QString html = "<body>"
        "<p>"
            "foo"
        "</p>"
        "<div>"
            "<p>yeah</p>"
        "</div>"
        "<p>"
            "bar"
        "</p>"
    "</body>";

    m_mainFrame->setHtml(html);
    QWebElement body = m_mainFrame->documentElement().findFirst("body");

    QCOMPARE(body.findAll("div").count(), 1);
    QCOMPARE(body.findAll("p").count(), 3);
    body.findFirst("div").removeAllChildren();
    QCOMPARE(body.findAll("div").count(), 1);
    QCOMPARE(body.findAll("p").count(), 2);
}


void tst_QWebElement::replaceWith()
{
    QString html = "<body>"
        "<p>"
            "foo"
        "</p>"
        "<div>"
            "yeah"
        "</div>"
        "<p>"
            "<span>haba</span>"
        "</p>"
    "</body>";

    m_mainFrame->setHtml(html);
    QWebElement body = m_mainFrame->documentElement().findFirst("body");

    QCOMPARE(body.findAll("div").count(), 1);
    QCOMPARE(body.findAll("span").count(), 1);
    body.findFirst("div").replace(body.findFirst("span").clone());
    QCOMPARE(body.findAll("div").count(), 0);
    QCOMPARE(body.findAll("span").count(), 2);
    QCOMPARE(body.findAll("p").count(), 2);

    body.findFirst("span").replace("<p><code>wow</code></p>");
    QCOMPARE(body.findAll("p").count(), 3);
    QCOMPARE(body.findAll("p code").count(), 1);
    QCOMPARE(body.findFirst("p code").toPlainText(), QString("wow"));
}

void tst_QWebElement::encloseContentsWith()
{
    QString html = "<body>"
        "<div>"
            "<i>"
                "yeah"
            "</i>"
            "<i>"
                "hello"
            "</i>"
        "</div>"
        "<p>"
            "<span>foo</span>"
            "<span>bar</span>"
        "</p>"
        "<u></u>"
        "<b></b>"
        "<em>hey</em>"
    "</body>";

    m_mainFrame->setHtml(html);
    QWebElement body = m_mainFrame->documentElement().findFirst("body");

    body.findFirst("p").encloseContentsWith(body.findFirst("b"));
    QCOMPARE(body.findAll("p b span").count(), 2);
    QCOMPARE(body.findFirst("p b span").toPlainText(), QString("foo"));

    body.findFirst("u").encloseContentsWith("<i></i>");
    QCOMPARE(body.findAll("u i").count(), 1);
    QCOMPARE(body.findFirst("u i").toPlainText(), QString());

    body.findFirst("div").encloseContentsWith("<span></span>");
    QCOMPARE(body.findAll("div span i").count(), 2);
    QCOMPARE(body.findFirst("div span i").toPlainText(), QString("yeah"));

    QString snippet = ""
        "<table>"
            "<tbody>"
                "<tr>"
                    "<td></td>"
                    "<td></td>"
                "</tr>"
                "<tr>"
                    "<td></td>"
                    "<td></td>"
                "<tr>"
            "</tbody>"
        "</table>";

    body.findFirst("em").encloseContentsWith(snippet);
    QCOMPARE(body.findFirst("em table tbody tr td").toPlainText(), QString("hey"));
}

void tst_QWebElement::encloseWith()
{
    QString html = "<body>"
        "<p>"
            "foo"
        "</p>"
        "<div>"
            "yeah"
        "</div>"
        "<p>"
            "<span>bar</span>"
        "</p>"
        "<em>hey</em>"
        "<h1>hello</h1>"
    "</body>";

    m_mainFrame->setHtml(html);
    QWebElement body = m_mainFrame->documentElement().findFirst("body");

    body.findFirst("p").encloseWith("<br>");
    QCOMPARE(body.findAll("br").count(), 0);

    QCOMPARE(body.findAll("div").count(), 1);
    body.findFirst("div").encloseWith(body.findFirst("span").clone());
    QCOMPARE(body.findAll("div").count(), 1);
    QCOMPARE(body.findAll("span").count(), 2);
    QCOMPARE(body.findAll("p").count(), 2);

    body.findFirst("div").encloseWith("<code></code>");
    QCOMPARE(body.findAll("code").count(), 1);
    QCOMPARE(body.findAll("code div").count(), 1);
    QCOMPARE(body.findFirst("code div").toPlainText(), QString("yeah"));

    QString snippet = ""
        "<table>"
            "<tbody>"
                "<tr>"
                    "<td></td>"
                    "<td></td>"
                "</tr>"
                "<tr>"
                    "<td></td>"
                    "<td></td>"
                "<tr>"
            "</tbody>"
        "</table>";

    body.findFirst("em").encloseWith(snippet);
    QCOMPARE(body.findFirst("table tbody tr td em").toPlainText(), QString("hey"));

    // Enclosing the contents of an img tag is not allowed, but enclosing the img tag itself is.
    body.findFirst("td").appendInside("<img src=\"test.png\">");
    QCOMPARE(body.findAll("img").count(), 1);

    QWebElement img = body.findFirst("img");
    QVERIFY(!img.isNull());
    img.encloseWith("<p id=\"success\"></p>");
    QCOMPARE(body.findAll("p#success").count(), 1);

    img.encloseContentsWith("<p id=\"fail\"></p>");
    QCOMPARE(body.findAll("p#fail").count(), 0);

}

void tst_QWebElement::nullSelect()
{
    m_mainFrame->setHtml("<body><p>Test");

    QWebElementCollection collection = m_mainFrame->findAllElements("invalid{syn(tax;;%#$f223e>>");
    QVERIFY(collection.count() == 0);
}

void tst_QWebElement::firstChildNextSibling()
{
    m_mainFrame->setHtml("<body><!--comment--><p>Test</p><!--another comment--><table>");

    QWebElement body = m_mainFrame->findFirstElement("body");
    QVERIFY(!body.isNull());
    QWebElement p = body.firstChild();
    QVERIFY(!p.isNull());
    QCOMPARE(p.tagName(), QString("P"));
    QWebElement table = p.nextSibling();
    QVERIFY(!table.isNull());
    QCOMPARE(table.tagName(), QString("TABLE"));
    QVERIFY(table.nextSibling().isNull());
}

void tst_QWebElement::lastChildPreviousSibling()
{
    m_mainFrame->setHtml("<body><!--comment--><p>Test</p><!--another comment--><table>");

    QWebElement body = m_mainFrame->findFirstElement("body");
    QVERIFY(!body.isNull());
    QWebElement table = body.lastChild();
    QVERIFY(!table.isNull());
    QCOMPARE(table.tagName(), QString("TABLE"));
    QWebElement p = table.previousSibling();
    QVERIFY(!p.isNull());
    QCOMPARE(p.tagName(), QString("P"));
    QVERIFY(p.previousSibling().isNull());
}

void tst_QWebElement::hasSetFocus()
{
    m_mainFrame->setHtml("<html><body>" \
                            "<input type='text' id='input1'/>" \
                            "<br>"\
                            "<input type='text' id='input2'/>" \
                            "</body></html>");

    QWebElementCollection inputs = m_mainFrame->documentElement().findAll("input");
    QWebElement input1 = inputs.at(0);
    input1.setFocus();
    QVERIFY(input1.hasFocus());

    QWebElement input2 = inputs.at(1);
    input2.setFocus();
    QVERIFY(!input1.hasFocus());
    QVERIFY(input2.hasFocus());
}

void tst_QWebElement::render()
{
    QString html( "<html>"
                    "<head><style>"
                       "body, iframe { margin: 0px; border: none; background: white; }"
                    "</style></head>"
                    "<body><table width='300px' height='300px' border='1'>"
                           "<tr>"
                               "<td>test"
                               "</td>"
                               "<td><img src='qrc:///image.png'>"
                               "</td>"
                           "</tr>"
                          "</table>"
                    "</body>"
                 "</html>"
                );

    QWebPage page;
    QSignalSpy loadSpy(&page, SIGNAL(loadFinished(bool)));
    page.mainFrame()->setHtml(html);

    waitForSignal(&page, SIGNAL(loadFinished(bool)));
    QCOMPARE(loadSpy.count(), 1);

    QSize size = page.mainFrame()->contentsSize();
    page.setViewportSize(size);

    QWebElementCollection imgs = page.mainFrame()->findAllElements("img");
    QCOMPARE(imgs.count(), 1);

    QImage resource(":/image.png");
    QRect imageRect(0, 0, resource.width(), resource.height());

    QImage testImage(resource.width(), resource.height(), QImage::Format_ARGB32);
    QPainter painter0(&testImage);
    painter0.fillRect(imageRect, Qt::white);
    // render() uses pixmaps internally, and pixmaps might have bit depths
    // other than 32, giving different pixel values due to rounding.
    QPixmap pix = QPixmap::fromImage(resource);
    painter0.drawPixmap(0, 0, pix);
    painter0.end();

    QImage image1(resource.width(), resource.height(), QImage::Format_ARGB32);
    QPainter painter1(&image1);
    painter1.fillRect(imageRect, Qt::white);
    imgs[0].render(&painter1);
    painter1.end();

    QEXPECT_FAIL("", "https://bugs.webkit.org/show_bug.cgi?id=65243", Continue);
    QVERIFY(image1 == testImage);

    // render image 2nd time to make sure that cached rendering works fine
    QImage image2(resource.width(), resource.height(), QImage::Format_ARGB32);
    QPainter painter2(&image2);
    painter2.fillRect(imageRect, Qt::white);
    imgs[0].render(&painter2);
    painter2.end();

    QEXPECT_FAIL("", "https://bugs.webkit.org/show_bug.cgi?id=65243", Continue);
    QVERIFY(image2 == testImage);

    // compare table rendered through QWebElement::render to whole page table rendering
    QRect tableRect(0, 0, 300, 300);
    QWebElementCollection tables = page.mainFrame()->findAllElements("table");
    QCOMPARE(tables.count(), 1);

    QImage image3(300, 300, QImage::Format_ARGB32);
    QPainter painter3(&image3);
    painter3.fillRect(tableRect, Qt::white);
    tables[0].render(&painter3);
    painter3.end();

    QImage image4(300, 300, QImage::Format_ARGB32);
    QPainter painter4(&image4);
    page.mainFrame()->render(&painter4, tableRect);
    painter4.end();

    QVERIFY(image3 == image4);

    // Chunked render test reuses page rendered in image4 in previous test
    const int chunkHeight = tableRect.height();
    const int chunkWidth = tableRect.width() / 3;
    QImage chunk(chunkWidth, chunkHeight, QImage::Format_ARGB32);
    QRect chunkRect(0, 0, chunkWidth, chunkHeight);
    for (int x = 0; x < tableRect.width(); x += chunkWidth) {
        QPainter painter(&chunk);
        painter.fillRect(chunkRect, Qt::white);
        QRect chunkPaintRect(x, 0, chunkWidth, chunkHeight);
        tables[0].render(&painter, chunkPaintRect);
        painter.end();

        QVERIFY(chunk == image4.copy(chunkPaintRect));
    }
}

void tst_QWebElement::addElementToHead()
{
    m_mainFrame->setHtml("<html><head></head><body></body></html>");
    QWebElement head = m_mainFrame->findFirstElement("head");
    QVERIFY(!head.isNull());
    QString append = "<script type=\"text/javascript\">var t = 0;</script>";
    head.appendInside(append);
    QEXPECT_FAIL("", "https://bugs.webkit.org/show_bug.cgi?id=102234", Continue);
    QCOMPARE(head.toInnerXml(), append);
}

QTEST_MAIN(tst_QWebElement)
#include "tst_qwebelement.moc"
