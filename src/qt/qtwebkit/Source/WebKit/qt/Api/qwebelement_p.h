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

#ifndef qwebelement_p_h
#define qwebelement_p_h

class QWebElement;

namespace WebCore {
class Element;
}

// this is here as a proxy, so we'd have a class to friend in QWebElement,
// as getting/setting a WebCore in QWebElement is private
class QtWebElementRuntime {
public:
    static void initialize();
    static QWebElement create(WebCore::Element*);
    static WebCore::Element* get(const QWebElement&);
};

#endif // qwebelement_p_h
