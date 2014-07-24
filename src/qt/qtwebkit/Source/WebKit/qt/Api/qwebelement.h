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

#ifndef QWEBELEMENT_H
#define QWEBELEMENT_H

#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qrect.h>
#include <QtCore/qvariant.h>
#include <QtCore/qshareddata.h>

#include "qwebkitglobal.h"
namespace WebCore {
    class Element;
    class Node;
}

QT_BEGIN_NAMESPACE
class QPainter;
QT_END_NAMESPACE

class QWebFrame;
class QWebElementCollection;
class QWebElementPrivate;

class QWEBKIT_EXPORT QWebElement {
public:
    QWebElement();
    QWebElement(const QWebElement&);
    QWebElement &operator=(const QWebElement&);
    ~QWebElement();

    bool operator==(const QWebElement& o) const;
    bool operator!=(const QWebElement& o) const;

    bool isNull() const;

    QWebElementCollection findAll(const QString &selectorQuery) const;
    QWebElement findFirst(const QString &selectorQuery) const;

    void setPlainText(const QString& text);
    QString toPlainText() const;

    void setOuterXml(const QString& markup);
    QString toOuterXml() const;

    void setInnerXml(const QString& markup);
    QString toInnerXml() const;

    void setAttribute(const QString& name, const QString& value);
    void setAttributeNS(const QString& namespaceUri, const QString& name, const QString& value);
    QString attribute(const QString& name, const QString& defaultValue = QString()) const;
    QString attributeNS(const QString& namespaceUri, const QString& name, const QString& defaultValue = QString()) const;
    bool hasAttribute(const QString& name) const;
    bool hasAttributeNS(const QString& namespaceUri, const QString& name) const;
    void removeAttribute(const QString& name);
    void removeAttributeNS(const QString& namespaceUri, const QString& name);
    bool hasAttributes() const;
    QStringList attributeNames(const QString& namespaceUri = QString()) const;

    QStringList classes() const;
    bool hasClass(const QString& name) const;
    void addClass(const QString& name);
    void removeClass(const QString& name);
    void toggleClass(const QString& name);

    bool hasFocus() const;
    void setFocus();

    QRect geometry() const;

    QString tagName() const;
    QString prefix() const;
    QString localName() const;
    QString namespaceUri() const;

    QWebElement parent() const;
    QWebElement firstChild() const;
    QWebElement lastChild() const;
    QWebElement nextSibling() const;
    QWebElement previousSibling() const;
    QWebElement document() const;
    QWebFrame *webFrame() const;

    // TODO: Add QWebElementCollection overloads
    // docs need example snippet
    void appendInside(const QString& markup);
    void appendInside(const QWebElement& element);

    // docs need example snippet
    void prependInside(const QString& markup);
    void prependInside(const QWebElement& element);

    // docs need example snippet
    void appendOutside(const QString& markup);
    void appendOutside(const QWebElement& element);

    // docs need example snippet
    void prependOutside(const QString& markup);
    void prependOutside(const QWebElement& element);

    // docs need example snippet
    void encloseContentsWith(const QWebElement& element);
    void encloseContentsWith(const QString& markup);
    void encloseWith(const QString& markup);
    void encloseWith(const QWebElement& element);

    void replace(const QString& markup);
    void replace(const QWebElement& element);

    QWebElement clone() const;
    QWebElement& takeFromDocument();
    void removeFromDocument();
    void removeAllChildren();

    QVariant evaluateJavaScript(const QString& scriptSource);

    enum StyleResolveStrategy {
         InlineStyle,
         CascadedStyle,
         ComputedStyle
    };
    QString styleProperty(const QString& name, StyleResolveStrategy strategy) const;
    void setStyleProperty(const QString& name, const QString& value);

    void render(QPainter* painter);
    void render(QPainter* painter, const QRect& clipRect);

private:
    explicit QWebElement(WebCore::Element*);
    explicit QWebElement(WebCore::Node*);

    static QWebElement enclosingElement(WebCore::Node*);

    friend class DumpRenderTreeSupportQt;
    friend class QWebFrameAdapter;
    friend class QWebElementCollection;
    friend class QWebHitTestResult;
    friend class QWebHitTestResultPrivate;
    friend class QWebPage;
    friend class QWebPagePrivate;
    friend class QtWebElementRuntime;

    QWebElementPrivate* d;
    WebCore::Element* m_element;
};

class QWebElementCollectionPrivate;

class QWEBKIT_EXPORT QWebElementCollection
{
public:
    QWebElementCollection();
    QWebElementCollection(const QWebElement &contextElement, const QString &query);
    QWebElementCollection(const QWebElementCollection &);
    QWebElementCollection &operator=(const QWebElementCollection &);
    ~QWebElementCollection();

    QWebElementCollection operator+(const QWebElementCollection &other) const;
    inline QWebElementCollection &operator+=(const QWebElementCollection &other)
    {
        append(other); return *this;
    }

    void append(const QWebElementCollection &collection);

    int count() const;
    QWebElement at(int i) const;
    inline QWebElement operator[](int i) const { return at(i); }

    inline QWebElement first() const { return at(0); }
    inline QWebElement last() const { return at(count() - 1); }

    QList<QWebElement> toList() const;

    class const_iterator {
       public:
           inline const_iterator(const QWebElementCollection* collection_, int index) : i(index), collection(collection_) {}
           inline const_iterator(const const_iterator& o) : i(o.i), collection(o.collection) {}

           inline const QWebElement operator*() const { return collection->at(i); }

           inline bool operator==(const const_iterator& o) const { return i == o.i && collection == o.collection; }
           inline bool operator!=(const const_iterator& o) const { return i != o.i || collection != o.collection; }
           inline bool operator<(const const_iterator& o) const { return i < o.i; }
           inline bool operator<=(const const_iterator& o) const { return i <= o.i; }
           inline bool operator>(const const_iterator& o) const { return i > o.i; }
           inline bool operator>=(const const_iterator& o) const { return i >= o.i; }

           inline const_iterator& operator++() { ++i; return *this; }
           inline const_iterator operator++(int) { const_iterator n(collection, i); ++i; return n; }
           inline const_iterator& operator--() { i--; return *this; }
           inline const_iterator operator--(int) { const_iterator n(collection, i); i--; return n; }
           inline const_iterator& operator+=(int j) { i += j; return *this; }
           inline const_iterator& operator-=(int j) { i -= j; return *this; }
           inline const_iterator operator+(int j) const { return const_iterator(collection, i + j); }
           inline const_iterator operator-(int j) const { return const_iterator(collection, i - j); }
           inline int operator-(const_iterator j) const { return i - j.i; }
       private:
            int i;
            const QWebElementCollection* const collection;
    };
    friend class const_iterator;

    inline const_iterator begin() const { return constBegin(); }
    inline const_iterator end() const { return constEnd(); }
    inline const_iterator constBegin() const { return const_iterator(this, 0); }
    inline const_iterator constEnd() const { return const_iterator(this, count()); };

    class iterator {
    public:
        inline iterator(const QWebElementCollection* collection_, int index) : i(index), collection(collection_) {}
        inline iterator(const iterator& o) : i(o.i), collection(o.collection) {}

        inline QWebElement operator*() const { return collection->at(i); }

        inline bool operator==(const iterator& o) const { return i == o.i && collection == o.collection; }
        inline bool operator!=(const iterator& o) const { return i != o.i || collection != o.collection; }
        inline bool operator<(const iterator& o) const { return i < o.i; }
        inline bool operator<=(const iterator& o) const { return i <= o.i; }
        inline bool operator>(const iterator& o) const { return i > o.i; }
        inline bool operator>=(const iterator& o) const { return i >= o.i; }

        inline iterator& operator++() { ++i; return *this; }
        inline iterator operator++(int) { iterator n(collection, i); ++i; return n; }
        inline iterator& operator--() { i--; return *this; }
        inline iterator operator--(int) { iterator n(collection, i); i--; return n; }
        inline iterator& operator+=(int j) { i += j; return *this; }
        inline iterator& operator-=(int j) { i -= j; return *this; }
        inline iterator operator+(int j) const { return iterator(collection, i + j); }
        inline iterator operator-(int j) const { return iterator(collection, i - j); }
        inline int operator-(iterator j) const { return i - j.i; }
    private:
        int i;
        const QWebElementCollection* const collection;
    };
    friend class iterator;

    inline iterator begin() { return iterator(this, 0); }
    inline iterator end()  { return iterator(this, count()); }
private:
    QExplicitlySharedDataPointer<QWebElementCollectionPrivate> d;
};

Q_DECLARE_METATYPE(QWebElement)

#endif // QWEBELEMENT_H
