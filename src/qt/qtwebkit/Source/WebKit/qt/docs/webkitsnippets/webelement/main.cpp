/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

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

#include <QApplication>
#include <QUrl>
#include <qwebview.h>
#include <qwebframe.h>
#include <qwebelement.h>

static QWebFrame *frame;

static void traverse()
{
//! [Traversing with QWebElement]
    frame->setHtml("<html><body><p>First Paragraph</p><p>Second Paragraph</p></body></html>");
    QWebElement doc = frame->documentElement();
    QWebElement body = doc.firstChild();
    QWebElement firstParagraph = body.firstChild();
    QWebElement secondParagraph = firstParagraph.nextSibling();
//! [Traversing with QWebElement]
}

static void findButtonAndClick()
{

    frame->setHtml("<form name=\"myform\" action=\"submit_form.asp\" method=\"get\">"
                   "<input type=\"text\" name=\"myfield\">"
                   "<input type=\"submit\" value=\"Submit\">"
                   "</form>");

//! [Calling a DOM element method]

    QWebElement document = frame->documentElement();
    /* Assume that the document has the following structure:

        <form name="myform" action="submit_form.asp" method="get">
            <input type="text" name="myfield">
            <input type="submit" value="Submit">
        </form>

     */

    QWebElement button = document.findFirst("input[type=submit]");
    button.evaluateJavaScript("click()");

//! [Calling a DOM element method]

 }

static void autocomplete1()
{
    QWebElement document = frame->documentElement();

//! [autocomplete1]
    QWebElement firstTextInput = document.findFirst("input[type=text]");
    QString storedText = firstTextInput.attribute("value");
//! [autocomplete1]

}


static void autocomplete2()
{

    QWebElement document = frame->documentElement();
    QString storedText = "text";

//! [autocomplete2]
    QWebElement firstTextInput = document.findFirst("input[type=text]");
    textInput.setAttribute("value", storedText);
//! [autocomplete2]

}


static void findAll()
{
//! [FindAll]
    QWebElement document = frame->documentElement();
    /* Assume the document has the following structure:

       <p class=intro>
         <span>Intro</span>
         <span>Snippets</span>
       </p>
       <p>
         <span>Content</span>
         <span>Here</span>
       </p>
    */

//! [FindAll intro]
    QWebElementCollection allSpans = document.findAll("span");
    QWebElementCollection introSpans = document.findAll("p.intro span");
//! [FindAll intro] //! [FindAll]
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QWebView *view = new QWebView(0);
    frame = view->page()->mainFrame();
    traverse();
    findAll();
    findButtonAndClick();
    autocomplete1();
    autocomplete2();
    return 0;
}
