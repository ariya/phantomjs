/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QXMLSTREAM_H
#define QXMLSTREAM_H

#include <QtCore/qiodevice.h>

#ifndef QT_NO_XMLSTREAM

#include <QtCore/qstring.h>
#include <QtCore/qvector.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

// QXmlStream* was originally in the QtXml module
// since we've moved it to QtCore in Qt 4.4.0, we need to
// keep binary compatibility
//
// The list of supported platforms is in:
//   http://qt.nokia.com/doc/supported_platforms.html
//
// These platforms do not support symbol moving nor duplication
// (because duplicate symbols cause warnings when linking):
//   Apple MacOS X (Mach-O executable format)
//       special case: 64-bit on Mac wasn't supported before 4.5.0
//   IBM AIX (XCOFF executable format)
//
// These platforms do not support symbol moving but allow it to be duplicated:
//   Microsoft Windows (COFF PE executable format)
//      special case: Windows CE wasn't supported before 4.4.0
//
// These platforms support symbol moving:
//   HP HP-UX (PA-RISC2.0 shared executables)
//   HP HP-UXi (ELF executable format)
//   FreeBSD (ELF executable format)
//   Linux (ELF executable format)
//   SGI IRIX (ELF executable format)
//   Sun Solaris (ELF executable format)
//
// Other platforms are supported through community contributions only.
// We are taking the optimist scenario here to avoid creating more
// symbols to be supported.

#if defined(Q_OS_MAC32) || defined(Q_OS_AIX)
# if !defined QT_BUILD_XML_LIB
#  define Q_XMLSTREAM_RENAME_SYMBOLS
# endif
#endif

#if defined QT_BUILD_XML_LIB
# define Q_XMLSTREAM_EXPORT     Q_XML_EXPORT
#else
# define Q_XMLSTREAM_EXPORT     Q_CORE_EXPORT
#endif

#if defined Q_XMLSTREAM_RENAME_SYMBOLS
// don't worry, we'll undef and change to typedef at the bottom of the file
# define QXmlStreamAttribute QCoreXmlStreamAttribute
# define QXmlStreamAttributes QCoreXmlStreamAttributes
# define QXmlStreamEntityDeclaration QCoreXmlStreamEntityDeclaration
# define QXmlStreamEntityDeclarations QCoreXmlStreamEntityDeclarations
# define QXmlStreamEntityResolver QCoreXmlStreamEntityResolver
# define QXmlStreamNamespaceDeclaration QCoreXmlStreamNamespaceDeclaration
# define QXmlStreamNamespaceDeclarations QCoreXmlStreamNamespaceDeclarations
# define QXmlStreamNotationDeclaration QCoreXmlStreamNotationDeclaration
# define QXmlStreamNotationDeclarations QCoreXmlStreamNotationDeclarations
# define QXmlStreamReader QCoreXmlStreamReader
# define QXmlStreamStringRef QCoreXmlStreamStringRef
# define QXmlStreamWriter QCoreXmlStreamWriter
#endif

class Q_XMLSTREAM_EXPORT QXmlStreamStringRef {
    QString m_string;
    int m_position, m_size;
public:
    inline QXmlStreamStringRef():m_position(0), m_size(0){}
    inline QXmlStreamStringRef(const QStringRef &aString)
        :m_string(aString.string()?*aString.string():QString()), m_position(aString.position()), m_size(aString.size()){}
    inline QXmlStreamStringRef(const QString &aString):m_string(aString), m_position(0), m_size(aString.size()){}
    inline ~QXmlStreamStringRef(){}
    inline void clear() { m_string.clear(); m_position = m_size = 0; }
    inline operator QStringRef() const { return QStringRef(&m_string, m_position, m_size); }
    inline const QString *string() const { return &m_string; }
    inline int position() const { return m_position; }
    inline int size() const { return m_size; }
};


class QXmlStreamReaderPrivate;
class QXmlStreamAttributes;
class Q_XMLSTREAM_EXPORT QXmlStreamAttribute {
    QXmlStreamStringRef m_name, m_namespaceUri, m_qualifiedName, m_value;
    void *reserved;
    uint m_isDefault : 1;
    friend class QXmlStreamReaderPrivate;
    friend class QXmlStreamAttributes;
public:
    QXmlStreamAttribute();
    QXmlStreamAttribute(const QString &qualifiedName, const QString &value);
    QXmlStreamAttribute(const QString &namespaceUri, const QString &name, const QString &value);
    QXmlStreamAttribute(const QXmlStreamAttribute &);
    QXmlStreamAttribute& operator=(const QXmlStreamAttribute &);
    ~QXmlStreamAttribute();
    inline QStringRef namespaceUri() const { return m_namespaceUri; }
    inline QStringRef name() const { return m_name; }
    inline QStringRef qualifiedName() const { return m_qualifiedName; }
    inline QStringRef prefix() const {
        return QStringRef(m_qualifiedName.string(),
                          m_qualifiedName.position(),
                          qMax(0, m_qualifiedName.size() - m_name.size() - 1));
    }
    inline QStringRef value() const { return m_value; }
    inline bool isDefault() const { return m_isDefault; }
    inline bool operator==(const QXmlStreamAttribute &other) const {
        return (value() == other.value()
                && (namespaceUri().isNull() ? (qualifiedName() == other.qualifiedName())
                    : (namespaceUri() == other.namespaceUri() && name() == other.name())));
    }
    inline bool operator!=(const QXmlStreamAttribute &other) const
        { return !operator==(other); }
};

Q_DECLARE_TYPEINFO(QXmlStreamAttribute, Q_MOVABLE_TYPE);

class Q_XMLSTREAM_EXPORT QXmlStreamAttributes : public QVector<QXmlStreamAttribute>
{
public:
    inline QXmlStreamAttributes() {}
    QStringRef value(const QString &namespaceUri, const QString &name) const;
    QStringRef value(const QString &namespaceUri, const QLatin1String &name) const;
    QStringRef value(const QLatin1String &namespaceUri, const QLatin1String &name) const;
    QStringRef value(const QString &qualifiedName) const;
    QStringRef value(const QLatin1String &qualifiedName) const;
    void append(const QString &namespaceUri, const QString &name, const QString &value);
    void append(const QString &qualifiedName, const QString &value);

    inline bool hasAttribute(const QString &qualifiedName) const
    {
        return !value(qualifiedName).isNull();
    }

    inline bool hasAttribute(const QLatin1String &qualifiedName) const
    {
        return !value(qualifiedName).isNull();
    }

    inline bool hasAttribute(const QString &namespaceUri, const QString &name) const
    {
        return !value(namespaceUri, name).isNull();
    }

#if !defined(Q_NO_USING_KEYWORD)
    using QVector<QXmlStreamAttribute>::append;
#else
    inline void append(const QXmlStreamAttribute &attribute)
        { QVector<QXmlStreamAttribute>::append(attribute); }
#endif
};

class Q_XMLSTREAM_EXPORT QXmlStreamNamespaceDeclaration {
    QXmlStreamStringRef m_prefix, m_namespaceUri;
    void *reserved;

    friend class QXmlStreamReaderPrivate;
public:
    QXmlStreamNamespaceDeclaration();
    QXmlStreamNamespaceDeclaration(const QXmlStreamNamespaceDeclaration &);
    QXmlStreamNamespaceDeclaration(const QString &prefix, const QString &namespaceUri);
    ~QXmlStreamNamespaceDeclaration();
    QXmlStreamNamespaceDeclaration& operator=(const QXmlStreamNamespaceDeclaration &);
    inline QStringRef prefix() const { return m_prefix; }
    inline QStringRef namespaceUri() const { return m_namespaceUri; }
    inline bool operator==(const QXmlStreamNamespaceDeclaration &other) const {
        return (prefix() == other.prefix() && namespaceUri() == other.namespaceUri());
    }
    inline bool operator!=(const QXmlStreamNamespaceDeclaration &other) const
        { return !operator==(other); }
};

Q_DECLARE_TYPEINFO(QXmlStreamNamespaceDeclaration, Q_MOVABLE_TYPE);
typedef QVector<QXmlStreamNamespaceDeclaration> QXmlStreamNamespaceDeclarations;

class Q_XMLSTREAM_EXPORT QXmlStreamNotationDeclaration {
    QXmlStreamStringRef m_name, m_systemId, m_publicId;
    void *reserved;

    friend class QXmlStreamReaderPrivate;
public:
    QXmlStreamNotationDeclaration();
    ~QXmlStreamNotationDeclaration();
    QXmlStreamNotationDeclaration(const QXmlStreamNotationDeclaration &);
    QXmlStreamNotationDeclaration& operator=(const QXmlStreamNotationDeclaration &);
    inline QStringRef name() const { return m_name; }
    inline QStringRef systemId() const { return m_systemId; }
    inline QStringRef publicId() const { return m_publicId; }
    inline bool operator==(const QXmlStreamNotationDeclaration &other) const {
        return (name() == other.name() && systemId() == other.systemId()
                && publicId() == other.publicId());
    }
    inline bool operator!=(const QXmlStreamNotationDeclaration &other) const
        { return !operator==(other); }
};

Q_DECLARE_TYPEINFO(QXmlStreamNotationDeclaration, Q_MOVABLE_TYPE);
typedef QVector<QXmlStreamNotationDeclaration> QXmlStreamNotationDeclarations;

class Q_XMLSTREAM_EXPORT QXmlStreamEntityDeclaration {
    QXmlStreamStringRef m_name, m_notationName, m_systemId, m_publicId, m_value;
    void *reserved;

    friend class QXmlStreamReaderPrivate;
public:
    QXmlStreamEntityDeclaration();
    ~QXmlStreamEntityDeclaration();
    QXmlStreamEntityDeclaration(const QXmlStreamEntityDeclaration &);
    QXmlStreamEntityDeclaration& operator=(const QXmlStreamEntityDeclaration &);
    inline QStringRef name() const { return m_name; }
    inline QStringRef notationName() const { return m_notationName; }
    inline QStringRef systemId() const { return m_systemId; }
    inline QStringRef publicId() const { return m_publicId; }
    inline QStringRef value() const { return m_value; }
    inline bool operator==(const QXmlStreamEntityDeclaration &other) const {
        return (name() == other.name()
                && notationName() == other.notationName()
                && systemId() == other.systemId()
                && publicId() == other.publicId()
                && value() == other.value());
    }
    inline bool operator!=(const QXmlStreamEntityDeclaration &other) const
        { return !operator==(other); }
};

Q_DECLARE_TYPEINFO(QXmlStreamEntityDeclaration, Q_MOVABLE_TYPE);
typedef QVector<QXmlStreamEntityDeclaration> QXmlStreamEntityDeclarations;


class Q_XMLSTREAM_EXPORT QXmlStreamEntityResolver
{
public:
    virtual ~QXmlStreamEntityResolver();
    virtual QString resolveEntity(const QString& publicId, const QString& systemId);
    virtual QString resolveUndeclaredEntity(const QString &name);
};

#ifndef QT_NO_XMLSTREAMREADER
class Q_XMLSTREAM_EXPORT QXmlStreamReader {
    QDOC_PROPERTY(bool namespaceProcessing READ namespaceProcessing WRITE setNamespaceProcessing)
public:
    enum TokenType {
        NoToken = 0,
        Invalid,
        StartDocument,
        EndDocument,
        StartElement,
        EndElement,
        Characters,
        Comment,
        DTD,
        EntityReference,
        ProcessingInstruction
    };


    QXmlStreamReader();
    QXmlStreamReader(QIODevice *device);
    QXmlStreamReader(const QByteArray &data);
    QXmlStreamReader(const QString &data);
    QXmlStreamReader(const char * data);
    ~QXmlStreamReader();

    void setDevice(QIODevice *device);
    QIODevice *device() const;
    void addData(const QByteArray &data);
    void addData(const QString &data);
    void addData(const char *data);
    void clear();


    bool atEnd() const;
    TokenType readNext();

    bool readNextStartElement();
    void skipCurrentElement();

    TokenType tokenType() const;
    QString tokenString() const;

    void setNamespaceProcessing(bool);
    bool namespaceProcessing() const;

    inline bool isStartDocument() const { return tokenType() == StartDocument; }
    inline bool isEndDocument() const { return tokenType() == EndDocument; }
    inline bool isStartElement() const { return tokenType() == StartElement; }
    inline bool isEndElement() const { return tokenType() == EndElement; }
    inline bool isCharacters() const { return tokenType() == Characters; }
    bool isWhitespace() const;
    bool isCDATA() const;
    inline bool isComment() const { return tokenType() == Comment; }
    inline bool isDTD() const { return tokenType() == DTD; }
    inline bool isEntityReference() const { return tokenType() == EntityReference; }
    inline bool isProcessingInstruction() const { return tokenType() == ProcessingInstruction; }

    bool isStandaloneDocument() const;
    QStringRef documentVersion() const;
    QStringRef documentEncoding() const;

    qint64 lineNumber() const;
    qint64 columnNumber() const;
    qint64 characterOffset() const;

    QXmlStreamAttributes attributes() const;

    enum ReadElementTextBehaviour {
        ErrorOnUnexpectedElement,
        IncludeChildElements,
        SkipChildElements
    };
    QString readElementText(ReadElementTextBehaviour behaviour);
    QString readElementText();

    QStringRef name() const;
    QStringRef namespaceUri() const;
    QStringRef qualifiedName() const;
    QStringRef prefix() const;

    QStringRef processingInstructionTarget() const;
    QStringRef processingInstructionData() const;

    QStringRef text() const;

    QXmlStreamNamespaceDeclarations namespaceDeclarations() const;
    void addExtraNamespaceDeclaration(const QXmlStreamNamespaceDeclaration &extraNamespaceDeclaraction);
    void addExtraNamespaceDeclarations(const QXmlStreamNamespaceDeclarations &extraNamespaceDeclaractions);
    QXmlStreamNotationDeclarations notationDeclarations() const;
    QXmlStreamEntityDeclarations entityDeclarations() const;
    QStringRef dtdName() const;
    QStringRef dtdPublicId() const;
    QStringRef dtdSystemId() const;


    enum Error {
        NoError,
        UnexpectedElementError,
        CustomError,
        NotWellFormedError,
        PrematureEndOfDocumentError
    };
    void raiseError(const QString& message = QString());
    QString errorString() const;
    Error error() const;

    inline bool hasError() const
    {
        return error() != NoError;
    }

    void setEntityResolver(QXmlStreamEntityResolver *resolver);
    QXmlStreamEntityResolver *entityResolver() const;

private:
    Q_DISABLE_COPY(QXmlStreamReader)
    Q_DECLARE_PRIVATE(QXmlStreamReader)
    QScopedPointer<QXmlStreamReaderPrivate> d_ptr;

};
#endif // QT_NO_XMLSTREAMREADER

#ifndef QT_NO_XMLSTREAMWRITER

class QXmlStreamWriterPrivate;

class Q_XMLSTREAM_EXPORT QXmlStreamWriter
{
    QDOC_PROPERTY(bool autoFormatting READ autoFormatting WRITE setAutoFormatting)
    QDOC_PROPERTY(int autoFormattingIndent READ autoFormattingIndent WRITE setAutoFormattingIndent)
public:
    QXmlStreamWriter();
    QXmlStreamWriter(QIODevice *device);
    QXmlStreamWriter(QByteArray *array);
    QXmlStreamWriter(QString *string);
    ~QXmlStreamWriter();

    void setDevice(QIODevice *device);
    QIODevice *device() const;

#ifndef QT_NO_TEXTCODEC
    void setCodec(QTextCodec *codec);
    void setCodec(const char *codecName);
    QTextCodec *codec() const;
#endif

    void setAutoFormatting(bool);
    bool autoFormatting() const;

    void setAutoFormattingIndent(int spacesOrTabs);
    int autoFormattingIndent() const;

    void writeAttribute(const QString &qualifiedName, const QString &value);
    void writeAttribute(const QString &namespaceUri, const QString &name, const QString &value);
    void writeAttribute(const QXmlStreamAttribute& attribute);
    void writeAttributes(const QXmlStreamAttributes& attributes);

    void writeCDATA(const QString &text);
    void writeCharacters(const QString &text);
    void writeComment(const QString &text);

    void writeDTD(const QString &dtd);

    void writeEmptyElement(const QString &qualifiedName);
    void writeEmptyElement(const QString &namespaceUri, const QString &name);

    void writeTextElement(const QString &qualifiedName, const QString &text);
    void writeTextElement(const QString &namespaceUri, const QString &name, const QString &text);

    void writeEndDocument();
    void writeEndElement();

    void writeEntityReference(const QString &name);
    void writeNamespace(const QString &namespaceUri, const QString &prefix = QString());
    void writeDefaultNamespace(const QString &namespaceUri);
    void writeProcessingInstruction(const QString &target, const QString &data = QString());

    void writeStartDocument();
    void writeStartDocument(const QString &version);
    void writeStartDocument(const QString &version, bool standalone);
    void writeStartElement(const QString &qualifiedName);
    void writeStartElement(const QString &namespaceUri, const QString &name);

#ifndef QT_NO_XMLSTREAMREADER
    void writeCurrentToken(const QXmlStreamReader &reader);
#endif

    bool hasError() const;

private:
    Q_DISABLE_COPY(QXmlStreamWriter)
    Q_DECLARE_PRIVATE(QXmlStreamWriter)
    QScopedPointer<QXmlStreamWriterPrivate> d_ptr;
};
#endif // QT_NO_XMLSTREAMWRITER

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_XMLSTREAM
#endif // QXMLSTREAM_H
