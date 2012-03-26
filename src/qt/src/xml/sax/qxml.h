/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtXml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QXML_H
#define QXML_H

#include <QtCore/qtextstream.h>
#include <QtCore/qfile.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qlist.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Xml)

class QXmlNamespaceSupport;
class QXmlAttributes;
class QXmlContentHandler;
class QXmlDefaultHandler;
class QXmlDTDHandler;
class QXmlEntityResolver;
class QXmlErrorHandler;
class QXmlLexicalHandler;
class QXmlDeclHandler;
class QXmlInputSource;
class QXmlLocator;
class QXmlNamespaceSupport;
class QXmlParseException;

class QXmlReader;
class QXmlSimpleReader;

class QXmlSimpleReaderPrivate;
class QXmlNamespaceSupportPrivate;
class QXmlAttributesPrivate;
class QXmlInputSourcePrivate;
class QXmlParseExceptionPrivate;
class QXmlLocatorPrivate;
class QXmlDefaultHandlerPrivate;


//
// SAX Namespace Support
//

class Q_XML_EXPORT QXmlNamespaceSupport
{
public:
    QXmlNamespaceSupport();
    ~QXmlNamespaceSupport();

    void setPrefix(const QString&, const QString&);

    QString prefix(const QString&) const;
    QString uri(const QString&) const;
    void splitName(const QString&, QString&, QString&) const;
    void processName(const QString&, bool, QString&, QString&) const;
    QStringList prefixes() const;
    QStringList prefixes(const QString&) const;

    void pushContext();
    void popContext();
    void reset();

private:
    QXmlNamespaceSupportPrivate *d;

    friend class QXmlSimpleReaderPrivate;
    Q_DISABLE_COPY(QXmlNamespaceSupport)
};


//
// SAX Attributes
//

class Q_XML_EXPORT QXmlAttributes
{
public:
    QXmlAttributes() {}
    virtual ~QXmlAttributes() {}

    int index(const QString& qName) const;
    int index(const QLatin1String& qName) const;
    int index(const QString& uri, const QString& localPart) const;
    int length() const;
    int count() const;
    QString localName(int index) const;
    QString qName(int index) const;
    QString uri(int index) const;
    QString type(int index) const;
    QString type(const QString& qName) const;
    QString type(const QString& uri, const QString& localName) const;
    QString value(int index) const;
    QString value(const QString& qName) const;
    QString value(const QLatin1String& qName) const;
    QString value(const QString& uri, const QString& localName) const;

    void clear();
    void append(const QString &qName, const QString &uri, const QString &localPart, const QString &value);

private:
    struct Attribute {
        QString qname, uri, localname, value;
    };
    typedef QList<Attribute> AttributeList;
    AttributeList attList;

    QXmlAttributesPrivate *d;
};

//
// SAX Input Source
//

class Q_XML_EXPORT QXmlInputSource
{
public:
    QXmlInputSource();
    QXmlInputSource(QIODevice *dev);
    virtual ~QXmlInputSource();

    virtual void setData(const QString& dat);
    virtual void setData(const QByteArray& dat);
    virtual void fetchData();
    virtual QString data() const;
    virtual QChar next();
    virtual void reset();

    static const ushort EndOfData;
    static const ushort EndOfDocument;

#ifdef QT3_SUPPORT
    QT3_SUPPORT_CONSTRUCTOR QXmlInputSource(QFile& file);
    QT3_SUPPORT_CONSTRUCTOR QXmlInputSource(QTextStream& stream);
#endif

protected:
    virtual QString fromRawData(const QByteArray &data, bool beginning = false);

private:
    void init();
    QXmlInputSourcePrivate *d;
};

//
// SAX Exception Classes
//

class Q_XML_EXPORT QXmlParseException
{
public:
    explicit QXmlParseException(const QString &name = QString(), int c = -1, int l = -1,
                                const QString &p = QString(), const QString &s = QString());
    QXmlParseException(const QXmlParseException &other);
    ~QXmlParseException();

    int columnNumber() const;
    int lineNumber() const;
    QString publicId() const;
    QString systemId() const;
    QString message() const;

private:
    QScopedPointer<QXmlParseExceptionPrivate> d;
};


//
// XML Reader
//

class Q_XML_EXPORT QXmlReader
{
public:
    virtual ~QXmlReader() {}
    virtual bool feature(const QString& name, bool *ok = 0) const = 0;
    virtual void setFeature(const QString& name, bool value) = 0;
    virtual bool hasFeature(const QString& name) const = 0;
    virtual void* property(const QString& name, bool *ok = 0) const = 0;
    virtual void setProperty(const QString& name, void* value) = 0;
    virtual bool hasProperty(const QString& name) const = 0;
    virtual void setEntityResolver(QXmlEntityResolver* handler) = 0;
    virtual QXmlEntityResolver* entityResolver() const = 0;
    virtual void setDTDHandler(QXmlDTDHandler* handler) = 0;
    virtual QXmlDTDHandler* DTDHandler() const = 0;
    virtual void setContentHandler(QXmlContentHandler* handler) = 0;
    virtual QXmlContentHandler* contentHandler() const = 0;
    virtual void setErrorHandler(QXmlErrorHandler* handler) = 0;
    virtual QXmlErrorHandler* errorHandler() const = 0;
    virtual void setLexicalHandler(QXmlLexicalHandler* handler) = 0;
    virtual QXmlLexicalHandler* lexicalHandler() const = 0;
    virtual void setDeclHandler(QXmlDeclHandler* handler) = 0;
    virtual QXmlDeclHandler* declHandler() const = 0;
    virtual bool parse(const QXmlInputSource& input) = 0;
    virtual bool parse(const QXmlInputSource* input) = 0;
};

class Q_XML_EXPORT QXmlSimpleReader : public QXmlReader
{
public:
    QXmlSimpleReader();
    virtual ~QXmlSimpleReader();

    bool feature(const QString& name, bool *ok = 0) const;
    void setFeature(const QString& name, bool value);
    bool hasFeature(const QString& name) const;

    void* property(const QString& name, bool *ok = 0) const;
    void setProperty(const QString& name, void* value);
    bool hasProperty(const QString& name) const;

    void setEntityResolver(QXmlEntityResolver* handler);
    QXmlEntityResolver* entityResolver() const;
    void setDTDHandler(QXmlDTDHandler* handler);
    QXmlDTDHandler* DTDHandler() const;
    void setContentHandler(QXmlContentHandler* handler);
    QXmlContentHandler* contentHandler() const;
    void setErrorHandler(QXmlErrorHandler* handler);
    QXmlErrorHandler* errorHandler() const;
    void setLexicalHandler(QXmlLexicalHandler* handler);
    QXmlLexicalHandler* lexicalHandler() const;
    void setDeclHandler(QXmlDeclHandler* handler);
    QXmlDeclHandler* declHandler() const;

    bool parse(const QXmlInputSource& input);
    bool parse(const QXmlInputSource* input);
    virtual bool parse(const QXmlInputSource* input, bool incremental);
    virtual bool parseContinue();

private:
    Q_DISABLE_COPY(QXmlSimpleReader)
    Q_DECLARE_PRIVATE(QXmlSimpleReader)
    QScopedPointer<QXmlSimpleReaderPrivate> d_ptr;

    friend class QXmlSimpleReaderLocator;
};

//
// SAX Locator
//

class Q_XML_EXPORT QXmlLocator
{
public:
    QXmlLocator();
    virtual ~QXmlLocator();

    virtual int columnNumber() const = 0;
    virtual int lineNumber() const = 0;
//    QString getPublicId() const
//    QString getSystemId() const
};

//
// SAX handler classes
//

class Q_XML_EXPORT QXmlContentHandler
{
public:
    virtual ~QXmlContentHandler() {}
    virtual void setDocumentLocator(QXmlLocator* locator) = 0;
    virtual bool startDocument() = 0;
    virtual bool endDocument() = 0;
    virtual bool startPrefixMapping(const QString& prefix, const QString& uri) = 0;
    virtual bool endPrefixMapping(const QString& prefix) = 0;
    virtual bool startElement(const QString& namespaceURI, const QString& localName, const QString& qName, const QXmlAttributes& atts) = 0;
    virtual bool endElement(const QString& namespaceURI, const QString& localName, const QString& qName) = 0;
    virtual bool characters(const QString& ch) = 0;
    virtual bool ignorableWhitespace(const QString& ch) = 0;
    virtual bool processingInstruction(const QString& target, const QString& data) = 0;
    virtual bool skippedEntity(const QString& name) = 0;
    virtual QString errorString() const = 0;
};

class Q_XML_EXPORT QXmlErrorHandler
{
public:
    virtual ~QXmlErrorHandler() {}
    virtual bool warning(const QXmlParseException& exception) = 0;
    virtual bool error(const QXmlParseException& exception) = 0;
    virtual bool fatalError(const QXmlParseException& exception) = 0;
    virtual QString errorString() const = 0;
};

class Q_XML_EXPORT QXmlDTDHandler
{
public:
    virtual ~QXmlDTDHandler() {}
    virtual bool notationDecl(const QString& name, const QString& publicId, const QString& systemId) = 0;
    virtual bool unparsedEntityDecl(const QString& name, const QString& publicId, const QString& systemId, const QString& notationName) = 0;
    virtual QString errorString() const = 0;
};

class Q_XML_EXPORT QXmlEntityResolver
{
public:
    virtual ~QXmlEntityResolver() {}
    virtual bool resolveEntity(const QString& publicId, const QString& systemId, QXmlInputSource*& ret) = 0;
    virtual QString errorString() const = 0;
};

class Q_XML_EXPORT QXmlLexicalHandler
{
public:
    virtual ~QXmlLexicalHandler() {}
    virtual bool startDTD(const QString& name, const QString& publicId, const QString& systemId) = 0;
    virtual bool endDTD() = 0;
    virtual bool startEntity(const QString& name) = 0;
    virtual bool endEntity(const QString& name) = 0;
    virtual bool startCDATA() = 0;
    virtual bool endCDATA() = 0;
    virtual bool comment(const QString& ch) = 0;
    virtual QString errorString() const = 0;
};

class Q_XML_EXPORT QXmlDeclHandler
{
public:
    virtual ~QXmlDeclHandler() {}
    virtual bool attributeDecl(const QString& eName, const QString& aName, const QString& type, const QString& valueDefault, const QString& value) = 0;
    virtual bool internalEntityDecl(const QString& name, const QString& value) = 0;
    virtual bool externalEntityDecl(const QString& name, const QString& publicId, const QString& systemId) = 0;
    virtual QString errorString() const = 0;
    // ### Qt 5: Conform to SAX by adding elementDecl
};


class Q_XML_EXPORT QXmlDefaultHandler : public QXmlContentHandler, public QXmlErrorHandler, public QXmlDTDHandler, public QXmlEntityResolver, public QXmlLexicalHandler, public QXmlDeclHandler
{
public:
    QXmlDefaultHandler() { }
    virtual ~QXmlDefaultHandler() { }

    void setDocumentLocator(QXmlLocator* locator);
    bool startDocument();
    bool endDocument();
    bool startPrefixMapping(const QString& prefix, const QString& uri);
    bool endPrefixMapping(const QString& prefix);
    bool startElement(const QString& namespaceURI, const QString& localName, const QString& qName, const QXmlAttributes& atts);
    bool endElement(const QString& namespaceURI, const QString& localName, const QString& qName);
    bool characters(const QString& ch);
    bool ignorableWhitespace(const QString& ch);
    bool processingInstruction(const QString& target, const QString& data);
    bool skippedEntity(const QString& name);

    bool warning(const QXmlParseException& exception);
    bool error(const QXmlParseException& exception);
    bool fatalError(const QXmlParseException& exception);

    bool notationDecl(const QString& name, const QString& publicId, const QString& systemId);
    bool unparsedEntityDecl(const QString& name, const QString& publicId, const QString& systemId, const QString& notationName);

    bool resolveEntity(const QString& publicId, const QString& systemId, QXmlInputSource*& ret);

    bool startDTD(const QString& name, const QString& publicId, const QString& systemId);
    bool endDTD();
    bool startEntity(const QString& name);
    bool endEntity(const QString& name);
    bool startCDATA();
    bool endCDATA();
    bool comment(const QString& ch);

    bool attributeDecl(const QString& eName, const QString& aName, const QString& type, const QString& valueDefault, const QString& value);
    bool internalEntityDecl(const QString& name, const QString& value);
    bool externalEntityDecl(const QString& name, const QString& publicId, const QString& systemId);

    QString errorString() const;

private:
    QXmlDefaultHandlerPrivate *d;
    Q_DISABLE_COPY(QXmlDefaultHandler)
};

// inlines

inline int QXmlAttributes::count() const
{ return length(); }

QT_END_NAMESPACE

QT_END_HEADER

#endif // QXML_H
