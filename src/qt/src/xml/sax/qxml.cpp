/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qxml.h"
#include "qtextcodec.h"
#include "qbuffer.h"
#include "qregexp.h"
#include "qmap.h"
#include "qstack.h"
#include <qdebug.h>


#ifdef Q_CC_BOR // borland 6 finds bogus warnings when building this file in uic3
#    pragma warn -8080
#endif

//#define QT_QXML_DEBUG

// Error strings for the XML reader
#define XMLERR_OK                         QT_TRANSLATE_NOOP("QXml", "no error occurred")
#define XMLERR_ERRORBYCONSUMER            QT_TRANSLATE_NOOP("QXml", "error triggered by consumer")
#define XMLERR_UNEXPECTEDEOF              QT_TRANSLATE_NOOP("QXml", "unexpected end of file")
#define XMLERR_MORETHANONEDOCTYPE         QT_TRANSLATE_NOOP("QXml", "more than one document type definition")
#define XMLERR_ERRORPARSINGELEMENT        QT_TRANSLATE_NOOP("QXml", "error occurred while parsing element")
#define XMLERR_TAGMISMATCH                QT_TRANSLATE_NOOP("QXml", "tag mismatch")
#define XMLERR_ERRORPARSINGCONTENT        QT_TRANSLATE_NOOP("QXml", "error occurred while parsing content")
#define XMLERR_UNEXPECTEDCHARACTER        QT_TRANSLATE_NOOP("QXml", "unexpected character")
#define XMLERR_INVALIDNAMEFORPI           QT_TRANSLATE_NOOP("QXml", "invalid name for processing instruction")
#define XMLERR_VERSIONEXPECTED            QT_TRANSLATE_NOOP("QXml", "version expected while reading the XML declaration")
#define XMLERR_WRONGVALUEFORSDECL         QT_TRANSLATE_NOOP("QXml", "wrong value for standalone declaration")
#define XMLERR_EDECLORSDDECLEXPECTED      QT_TRANSLATE_NOOP("QXml", "encoding declaration or standalone declaration expected while reading the XML declaration")
#define XMLERR_SDDECLEXPECTED             QT_TRANSLATE_NOOP("QXml", "standalone declaration expected while reading the XML declaration")
#define XMLERR_ERRORPARSINGDOCTYPE        QT_TRANSLATE_NOOP("QXml", "error occurred while parsing document type definition")
#define XMLERR_LETTEREXPECTED             QT_TRANSLATE_NOOP("QXml", "letter is expected")
#define XMLERR_ERRORPARSINGCOMMENT        QT_TRANSLATE_NOOP("QXml", "error occurred while parsing comment")
#define XMLERR_ERRORPARSINGREFERENCE      QT_TRANSLATE_NOOP("QXml", "error occurred while parsing reference")
#define XMLERR_INTERNALGENERALENTITYINDTD QT_TRANSLATE_NOOP("QXml", "internal general entity reference not allowed in DTD")
#define XMLERR_EXTERNALGENERALENTITYINAV  QT_TRANSLATE_NOOP("QXml", "external parsed general entity reference not allowed in attribute value")
#define XMLERR_EXTERNALGENERALENTITYINDTD QT_TRANSLATE_NOOP("QXml", "external parsed general entity reference not allowed in DTD")
#define XMLERR_UNPARSEDENTITYREFERENCE    QT_TRANSLATE_NOOP("QXml", "unparsed entity reference in wrong context")
#define XMLERR_RECURSIVEENTITIES          QT_TRANSLATE_NOOP("QXml", "recursive entities")
#define XMLERR_ERRORINTEXTDECL            QT_TRANSLATE_NOOP("QXml", "error in the text declaration of an external entity")

QT_BEGIN_NAMESPACE

// the constants for the lookup table
static const signed char cltWS      =  0; // white space
static const signed char cltPer     =  1; // %
static const signed char cltAmp     =  2; // &
static const signed char cltGt      =  3; // >
static const signed char cltLt      =  4; // <
static const signed char cltSlash   =  5; // /
static const signed char cltQm      =  6; // ?
static const signed char cltEm      =  7; // !
static const signed char cltDash    =  8; // -
static const signed char cltCB      =  9; // ]
static const signed char cltOB      = 10; // [
static const signed char cltEq      = 11; // =
static const signed char cltDq      = 12; // "
static const signed char cltSq      = 13; // '
static const signed char cltUnknown = 14;

// Hack for letting QDom know where the skipped entity occurred
// ### Qt5: the use of this variable means the code isn't reentrant.
bool qt_xml_skipped_entity_in_content;

// character lookup table
static const signed char charLookupTable[256]={
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0x00 - 0x07
    cltUnknown, // 0x08
    cltWS,      // 0x09 \t
    cltWS,      // 0x0A \n
    cltUnknown, // 0x0B
    cltUnknown, // 0x0C
    cltWS,      // 0x0D \r
    cltUnknown, // 0x0E
    cltUnknown, // 0x0F
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0x17 - 0x16
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0x18 - 0x1F
    cltWS,      // 0x20 Space
    cltEm,      // 0x21 !
    cltDq,      // 0x22 "
    cltUnknown, // 0x23
    cltUnknown, // 0x24
    cltPer,     // 0x25 %
    cltAmp,     // 0x26 &
    cltSq,      // 0x27 '
    cltUnknown, // 0x28
    cltUnknown, // 0x29
    cltUnknown, // 0x2A
    cltUnknown, // 0x2B
    cltUnknown, // 0x2C
    cltDash,    // 0x2D -
    cltUnknown, // 0x2E
    cltSlash,   // 0x2F /
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0x30 - 0x37
    cltUnknown, // 0x38
    cltUnknown, // 0x39
    cltUnknown, // 0x3A
    cltUnknown, // 0x3B
    cltLt,      // 0x3C <
    cltEq,      // 0x3D =
    cltGt,      // 0x3E >
    cltQm,      // 0x3F ?
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0x40 - 0x47
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0x48 - 0x4F
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0x50 - 0x57
    cltUnknown, // 0x58
    cltUnknown, // 0x59
    cltUnknown, // 0x5A
    cltOB,      // 0x5B [
    cltUnknown, // 0x5C
    cltCB,      // 0x5D]
    cltUnknown, // 0x5E
    cltUnknown, // 0x5F
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0x60 - 0x67
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0x68 - 0x6F
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0x70 - 0x77
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0x78 - 0x7F
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0x80 - 0x87
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0x88 - 0x8F
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0x90 - 0x97
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0x98 - 0x9F
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0xA0 - 0xA7
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0xA8 - 0xAF
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0xB0 - 0xB7
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0xB8 - 0xBF
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0xC0 - 0xC7
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0xC8 - 0xCF
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0xD0 - 0xD7
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0xD8 - 0xDF
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0xE0 - 0xE7
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0xE8 - 0xEF
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, // 0xF0 - 0xF7
    cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown, cltUnknown  // 0xF8 - 0xFF
};

//
// local helper functions
//

/*
  This function strips the TextDecl [77] ("<?xml ...?>") from the string \a
  str. The stripped version is stored in \a str. If this function finds an
  invalid TextDecl, it returns false, otherwise true.

  This function is used for external entities since those can include an
  TextDecl that must be stripped before inserting the entity.
*/
static bool stripTextDecl(QString& str)
{
    QString textDeclStart(QLatin1String("<?xml"));
    if (str.startsWith(textDeclStart)) {
        QRegExp textDecl(QString::fromLatin1(
            "^<\\?xml\\s+"
            "(version\\s*=\\s*((['\"])[-a-zA-Z0-9_.:]+\\3))?"
            "\\s*"
            "(encoding\\s*=\\s*((['\"])[A-Za-z][-a-zA-Z0-9_.]*\\6))?"
            "\\s*\\?>"
       ));
        QString strTmp = str.replace(textDecl, QLatin1String(""));
        if (strTmp.length() != str.length())
            return false; // external entity has wrong TextDecl
        str = strTmp;
    }
    return true;
}


class QXmlAttributesPrivate
{
};

/* \class QXmlInputSourcePrivate
    \internal

  There's a slight misdesign in this class that can
  be worth to keep in mind: the `str' member is
  a buffer which QXmlInputSource::next() returns from,
  and which is populated from the input device or input
  stream. However, when the input is a QString(the user called
  QXmlInputSource::setData()), `str' has two roles: it's the
  buffer, but also the source. This /seems/ to be no problem
  because in the case of having no device or stream, the QString
  is read in one go.
 */
class QXmlInputSourcePrivate
{
public:
    QIODevice *inputDevice;
    QTextStream *inputStream;

    QString str;
    const QChar *unicode;
    int pos;
    int length;
    bool nextReturnedEndOfData;
#ifndef QT_NO_TEXTCODEC
    QTextDecoder *encMapper;
#endif

    QByteArray encodingDeclBytes;
    QString encodingDeclChars;
    bool lookingForEncodingDecl;
};
class QXmlParseExceptionPrivate
{
public:
    QXmlParseExceptionPrivate()
        : column(-1), line(-1)
    {
    }
    QXmlParseExceptionPrivate(const QXmlParseExceptionPrivate &other)
        : msg(other.msg), column(other.column), line(other.line),
          pub(other.pub), sys(other.sys)
    {
    }

    QString msg;
    int column;
    int line;
    QString pub;
    QString sys;

};

class QXmlLocatorPrivate
{
};

class QXmlDefaultHandlerPrivate
{
};

class QXmlSimpleReaderPrivate
{
public:
    ~QXmlSimpleReaderPrivate();
private:
    // functions
    QXmlSimpleReaderPrivate(QXmlSimpleReader *reader);
    void initIncrementalParsing();

    // used to determine if elements are correctly nested
    QStack<QString> tags;

    // used by parseReference() and parsePEReference()
    enum EntityRecognitionContext { InContent, InAttributeValue, InEntityValue, InDTD };

    // used for entity declarations
    struct ExternParameterEntity
    {
        ExternParameterEntity() {}
        ExternParameterEntity(const QString &p, const QString &s)
            : publicId(p), systemId(s) {}
        QString publicId;
        QString systemId;

        Q_DUMMY_COMPARISON_OPERATOR(ExternParameterEntity)
    };
    struct ExternEntity
    {
        ExternEntity() {}
        ExternEntity(const QString &p, const QString &s, const QString &n)
            : publicId(p), systemId(s), notation(n) {}
        QString publicId;
        QString systemId;
        QString notation;
        Q_DUMMY_COMPARISON_OPERATOR(ExternEntity)
    };
    QMap<QString,ExternParameterEntity> externParameterEntities;
    QMap<QString,QString> parameterEntities;
    QMap<QString,ExternEntity> externEntities;
    QMap<QString,QString> entities;

    // used for parsing of entity references
    struct XmlRef {
        XmlRef()
            : index(0) {}
        XmlRef(const QString &_name, const QString &_value)
            : name(_name), value(_value), index(0) {}
        bool isEmpty() const { return index == value.length(); }
        QChar next() { return value.at(index++); }
        QString name;
        QString value;
        int index;
    };
    QStack<XmlRef> xmlRefStack;

    // used for standalone declaration
    enum Standalone { Yes, No, Unknown };

    QString doctype; // only used for the doctype
    QString xmlVersion; // only used to store the version information
    QString encoding; // only used to store the encoding
    Standalone standalone; // used to store the value of the standalone declaration

    QString publicId; // used by parseExternalID() to store the public ID
    QString systemId; // used by parseExternalID() to store the system ID

    // Since publicId/systemId is used as temporary variables by parseExternalID(), it
    // might overwrite the PUBLIC/SYSTEM for the document we're parsing. In effect, we would
    // possibly send off an QXmlParseException that has the PUBLIC/SYSTEM of a entity declaration
    // instead of those of the current document.
    // Hence we have these two variables for storing the document's data.
    QString thisPublicId;
    QString thisSystemId;

    QString attDeclEName; // use by parseAttlistDecl()
    QString attDeclAName; // use by parseAttlistDecl()

    // flags for some features support
    bool useNamespaces;
    bool useNamespacePrefixes;
    bool reportWhitespaceCharData;
    bool reportEntities;

    // used to build the attribute list
    QXmlAttributes attList;

    // used in QXmlSimpleReader::parseContent() to decide whether character
    // data was read
    bool contentCharDataRead;

    // helper classes
    QScopedPointer<QXmlLocator> locator;
    QXmlNamespaceSupport namespaceSupport;

    // error string
    QString error;

    // arguments for parse functions (this is needed to allow incremental
    // parsing)
    bool parsePI_xmldecl;
    bool parseName_useRef;
    bool parseReference_charDataRead;
    EntityRecognitionContext parseReference_context;
    bool parseExternalID_allowPublicID;
    EntityRecognitionContext parsePEReference_context;
    QString parseString_s;

    // for incremental parsing
    struct ParseState {
        typedef bool (QXmlSimpleReaderPrivate::*ParseFunction)();
        ParseFunction function;
        int state;
    };
    QStack<ParseState> *parseStack;

    // used in parseProlog()
    bool xmldecl_possible;
    bool doctype_read;

    // used in parseDoctype()
    bool startDTDwasReported;

    // used in parseString()
    signed char Done;


    // variables
    QXmlContentHandler *contentHnd;
    QXmlErrorHandler   *errorHnd;
    QXmlDTDHandler     *dtdHnd;
    QXmlEntityResolver *entityRes;
    QXmlLexicalHandler *lexicalHnd;
    QXmlDeclHandler    *declHnd;

    QXmlInputSource *inputSource;

    QChar c; // the character at reading position
    int   lineNr; // number of line
    int   columnNr; // position in line

    QChar   nameArray[256]; // only used for names
    QString nameValue; // only used for names
    int     nameArrayPos;
    int     nameValueLen;
    QChar   refArray[256]; // only used for references
    QString refValue; // only used for references
    int     refArrayPos;
    int     refValueLen;
    QChar   stringArray[256]; // used for any other strings that are parsed
    QString stringValue; // used for any other strings that are parsed
    int     stringArrayPos;
    int     stringValueLen;
    QString emptyStr;

    const QString &string();
    void stringClear();
    void stringAddC(QChar);
    inline void stringAddC() { stringAddC(c); }
    const QString &name();
    void nameClear();
    void nameAddC(QChar);
    inline void nameAddC() { nameAddC(c); }
    const QString &ref();
    void refClear();
    void refAddC(QChar);
    inline void refAddC() { refAddC(c); }

    // private functions
    bool eat_ws();
    bool next_eat_ws();

    void QT_FASTCALL next();
    bool atEnd();

    void init(const QXmlInputSource* i);
    void initData();

    bool entityExist(const QString&) const;

    bool parseBeginOrContinue(int state, bool incremental);

    bool parseProlog();
    bool parseElement();
    bool processElementEmptyTag();
    bool processElementETagBegin2();
    bool processElementAttribute();
    bool parseMisc();
    bool parseContent();

    bool parsePI();
    bool parseDoctype();
    bool parseComment();

    bool parseName();
    bool parseNmtoken();
    bool parseAttribute();
    bool parseReference();
    bool processReference();

    bool parseExternalID();
    bool parsePEReference();
    bool parseMarkupdecl();
    bool parseAttlistDecl();
    bool parseAttType();
    bool parseAttValue();
    bool parseElementDecl();
    bool parseNotationDecl();
    bool parseChoiceSeq();
    bool parseEntityDecl();
    bool parseEntityValue();

    bool parseString();

    bool insertXmlRef(const QString&, const QString&, bool);

    bool reportEndEntities();
    void reportParseError(const QString& error);

    typedef bool (QXmlSimpleReaderPrivate::*ParseFunction) ();
    void unexpectedEof(ParseFunction where, int state);
    void parseFailed(ParseFunction where, int state);
    void pushParseState(ParseFunction function, int state);

    Q_DECLARE_PUBLIC(QXmlSimpleReader)
    QXmlSimpleReader *q_ptr;

    friend class QXmlSimpleReaderLocator;
};

/*!
    \class QXmlParseException
    \reentrant
    \brief The QXmlParseException class is used to report errors with
    the QXmlErrorHandler interface.

    \inmodule QtXml
    \ingroup xml-tools

    The XML subsystem constructs an instance of this class when it
    detects an error. You can retrieve the place where the error
    occurred using systemId(), publicId(), lineNumber() and
    columnNumber(), along with the error message(). The possible error
    messages are:


    \list
        \o "no error occurred"
        \o "error triggered by consumer"
        \o "unexpected end of file"
        \o "more than one document type definition"
        \o "error occurred while parsing element"
        \o "tag mismatch"
        \o "error occurred while parsing content"
        \o "unexpected character"
        \o "invalid name for processing instruction"
        \o "version expected while reading the XML declaration"
        \o "wrong value for standalone declaration"
        \o "encoding declaration or standalone declaration expected while reading the XML declaration"
        \o "standalone declaration expected while reading the XML declaration"
        \o "error occurred while parsing document type definition"
        \o "letter is expected"
        \o "error occurred while parsing comment"
        \o "error occurred while parsing reference"
        \o "internal general entity reference not allowed in DTD"
        \o "external parsed general entity reference not allowed in attribute value"
        \o "external parsed general entity reference not allowed in DTD"
        \o "unparsed entity reference n wrong context"
        \o "recursive entities"
        \o "error in the text declaration of an external entity"
    \endlist

    Note that, if you want to display these error messages to your
    application's users, they will be displayed in English unless
    they are explicitly translated.

    \sa QXmlErrorHandler, QXmlReader
*/

/*!
    Constructs a parse exception with the error string \a name for
    column \a c and line \a l for the public identifier \a p and the
    system identifier \a s.
*/

QXmlParseException::QXmlParseException(const QString& name, int c, int l,
                                       const QString& p, const QString& s)
    : d(new QXmlParseExceptionPrivate)
{
    d->msg = name;
    d->column = c;
    d->line = l;
    d->pub = p;
    d->sys = s;
}

/*!
    Creates a copy of \a other.
*/
QXmlParseException::QXmlParseException(const QXmlParseException& other) :
     d(new QXmlParseExceptionPrivate(*other.d))
{

}

/*!
    Destroys the QXmlParseException.
*/
QXmlParseException::~QXmlParseException()
{
}

/*!
    Returns the error message.
*/
QString QXmlParseException::message() const
{
    return d->msg;
}
/*!
    Returns the column number where the error occurred.
*/
int QXmlParseException::columnNumber() const
{
    return d->column;
}
/*!
    Returns the line number where the error occurred.
*/
int QXmlParseException::lineNumber() const
{
    return d->line;
}
/*!
    Returns the public identifier where the error occurred.
*/
QString QXmlParseException::publicId() const
{
    return d->pub;
}
/*!
    Returns the system identifier where the error occurred.
*/
QString QXmlParseException::systemId() const
{
    return d->sys;
}


/*!
    \class QXmlLocator
    \reentrant
    \brief The QXmlLocator class provides the XML handler classes with
    information about the parsing position within a file.

    \inmodule QtXml
    \ingroup xml-tools

    The reader reports a QXmlLocator to the content handler before it
    starts to parse the document. This is done with the
    QXmlContentHandler::setDocumentLocator() function. The handler
    classes can now use this locator to get the position (lineNumber()
    and columnNumber()) that the reader has reached.
*/

/*!
    Constructor.
*/
QXmlLocator::QXmlLocator()
{
}

/*!
    Destructor.
*/
QXmlLocator::~QXmlLocator()
{
}

/*!
    \fn int QXmlLocator::columnNumber() const

    Returns the column number (starting at 1) or -1 if there is no
    column number available.
*/

/*!
    \fn int QXmlLocator::lineNumber() const

    Returns the line number (starting at 1) or -1 if there is no line
    number available.
*/

class QXmlSimpleReaderLocator : public QXmlLocator
{
public:
    QXmlSimpleReaderLocator(QXmlSimpleReader* parent)
    {
        reader = parent;
    }
    ~QXmlSimpleReaderLocator()
    {
    }

    int columnNumber() const
    {
        return (reader->d_ptr->columnNr == -1 ? -1 : reader->d_ptr->columnNr + 1);
    }
    int lineNumber() const
    {
        return (reader->d_ptr->lineNr == -1 ? -1 : reader->d_ptr->lineNr + 1);
    }
//    QString getPublicId()
//    QString getSystemId()

private:
    QXmlSimpleReader *reader;
};

/*********************************************
 *
 * QXmlNamespaceSupport
 *
 *********************************************/

typedef QMap<QString, QString> NamespaceMap;

class QXmlNamespaceSupportPrivate
{
public:
    QXmlNamespaceSupportPrivate()
    {
        ns.insert(QLatin1String("xml"), QLatin1String("http://www.w3.org/XML/1998/namespace")); // the XML namespace
    }

    ~QXmlNamespaceSupportPrivate()
    {
    }

    QStack<NamespaceMap> nsStack;
    NamespaceMap ns;
};

/*!
    \class QXmlNamespaceSupport
    \since 4.4
    \reentrant
    \brief The QXmlNamespaceSupport class is a helper class for XML
    readers which want to include namespace support.

    \inmodule QtXml
    \ingroup xml-tools

    You can set the prefix for the current namespace with setPrefix(),
    and get the list of current prefixes (or those for a given URI)
    with prefixes(). The namespace URI is available from uri(). Use
    pushContext() to start a new namespace context, and popContext()
    to return to the previous namespace context. Use splitName() or
    processName() to split a name into its prefix and local name.

    \sa {Namespace Support via Features}
*/

/*!
    Constructs a QXmlNamespaceSupport.
*/
QXmlNamespaceSupport::QXmlNamespaceSupport()
{
    d = new QXmlNamespaceSupportPrivate;
}

/*!
    Destroys a QXmlNamespaceSupport.
*/
QXmlNamespaceSupport::~QXmlNamespaceSupport()
{
    delete d;
}

/*!
    This function declares a prefix \a pre in the current namespace
    context to be the namespace URI \a uri. The prefix remains in
    force until this context is popped, unless it is shadowed in a
    descendant context.

    Note that there is an asymmetry in this library. prefix() does not
    return the default "" prefix, even if you have declared one; to
    check for a default prefix, you must look it up explicitly using
    uri(). This asymmetry exists to make it easier to look up prefixes
    for attribute names, where the default prefix is not allowed.
*/
void QXmlNamespaceSupport::setPrefix(const QString& pre, const QString& uri)
{
    if(pre.isNull()) {
        d->ns.insert(QLatin1String(""), uri);
    } else {
        d->ns.insert(pre, uri);
    }
}

/*!
    Returns one of the prefixes mapped to the namespace URI \a uri.

    If more than one prefix is currently mapped to the same URI, this
    function makes an arbitrary selection; if you want all of the
    prefixes, use prefixes() instead.

    Note: to check for a default prefix, use the uri() function with
    an argument of "".
*/
QString QXmlNamespaceSupport::prefix(const QString& uri) const
{
    NamespaceMap::const_iterator itc, it = d->ns.constBegin();
    while ((itc=it) != d->ns.constEnd()) {
        ++it;
        if (*itc == uri && !itc.key().isEmpty())
            return itc.key();
    }
    return QLatin1String("");
}

/*!
    Looks up the prefix \a prefix in the current context and returns
    the currently-mapped namespace URI. Use the empty string ("") for
    the default namespace.
*/
QString QXmlNamespaceSupport::uri(const QString& prefix) const
{
    return d->ns[prefix];
}

/*!
    Splits the name \a qname at the ':' and returns the prefix in \a
    prefix and the local name in \a localname.

    \sa processName()
*/
void QXmlNamespaceSupport::splitName(const QString& qname, QString& prefix,
                                     QString& localname) const
{
    int pos = qname.indexOf(QLatin1Char(':'));
    if (pos == -1)
        pos = qname.size();

    prefix = qname.left(pos);
    localname = qname.mid(pos+1);
}

/*!
    Processes a raw XML 1.0 name in the current context by removing
    the prefix and looking it up among the prefixes currently
    declared.

    \a qname is the raw XML 1.0 name to be processed. \a isAttribute
    is true if the name is an attribute name.

    This function stores the namespace URI in \a nsuri (which will be
    set to an empty string if the raw name has an undeclared prefix),
    and stores the local name (without prefix) in \a localname (which
    will be set to an empty string if no namespace is in use).

    Note that attribute names are processed differently than element
    names: an unprefixed element name gets the default namespace (if
    any), while an unprefixed attribute name does not.
*/
void QXmlNamespaceSupport::processName(const QString& qname,
        bool isAttribute,
        QString& nsuri, QString& localname) const
{
    int len = qname.size();
    const QChar *data = qname.constData();
    for (int pos = 0; pos < len; ++pos) {
        if (data[pos] == QLatin1Char(':')) {
            nsuri = uri(qname.left(pos));
            localname = qname.mid(pos + 1);
            return;
        }
    }

    // there was no ':'
    nsuri.clear();
    // attributes don't take default namespace
    if (!isAttribute && !d->ns.isEmpty()) {
	/*
	    We want to access d->ns.value(""), but as an optimization
	    we use the fact that "" compares less than any other
	    string, so it's either first in the map or not there.
	*/
        NamespaceMap::const_iterator first = d->ns.constBegin();
        if (first.key().isEmpty())
            nsuri = first.value(); // get default namespace
    }
    localname = qname;
}

/*!
    Returns a list of all the prefixes currently declared.

    If there is a default prefix, this function does not return it in
    the list; check for the default prefix using uri() with an
    argument of "".
*/
QStringList QXmlNamespaceSupport::prefixes() const
{
    QStringList list;

    NamespaceMap::const_iterator itc, it = d->ns.constBegin();
    while ((itc=it) != d->ns.constEnd()) {
        ++it;
        if (!itc.key().isEmpty())
            list.append(itc.key());
    }
    return list;
}

/*!
    \overload

    Returns a list of all prefixes currently declared for the
    namespace URI \a uri.

    The "xml:" prefix is included. If you only want one prefix that is
    mapped to the namespace URI, and you don't care which one you get,
    use the prefix() function instead.

    Note: The empty (default) prefix is never included in this list;
    to check for the presence of a default namespace, call uri() with
    "" as the argument.
*/
QStringList QXmlNamespaceSupport::prefixes(const QString& uri) const
{
    QStringList list;

    NamespaceMap::const_iterator itc, it = d->ns.constBegin();
    while ((itc=it) != d->ns.constEnd()) {
        ++it;
        if (*itc == uri && !itc.key().isEmpty())
            list.append(itc.key());
    }
    return list;
}

/*!
    Starts a new namespace context.

    Normally, you should push a new context at the beginning of each
    XML element: the new context automatically inherits the
    declarations of its parent context, and it also keeps track of
    which declarations were made within this context.

    \sa popContext()
*/
void QXmlNamespaceSupport::pushContext()
{
    d->nsStack.push(d->ns);
}

/*!
    Reverts to the previous namespace context.

    Normally, you should pop the context at the end of each XML
    element. After popping the context, all namespace prefix mappings
    that were previously in force are restored.

    \sa pushContext()
*/
void QXmlNamespaceSupport::popContext()
{
    d->ns.clear();
    if(!d->nsStack.isEmpty())
        d->ns = d->nsStack.pop();
}

/*!
    Resets this namespace support object ready for reuse.
*/
void QXmlNamespaceSupport::reset()
{
    QXmlNamespaceSupportPrivate *newD = new QXmlNamespaceSupportPrivate;
    delete d;
    d = newD;
}



/*********************************************
 *
 * QXmlAttributes
 *
 *********************************************/

/*!
    \class QXmlAttributes
    \reentrant
    \brief The QXmlAttributes class provides XML attributes.

    \inmodule QtXml
    \ingroup xml-tools

    If attributes are reported by QXmlContentHandler::startElement()
    this class is used to pass the attribute values.

    Use index() to locate the position of an attribute in the list,
    count() to retrieve the number of attributes, and clear() to
    remove the attributes. New attributes can be added with append().
    Use type() to get an attribute's type and value() to get its
    value. The attribute's name is available from localName() or
    qName(), and its namespace URI from uri().

*/

/*!
    \fn QXmlAttributes::QXmlAttributes()

    Constructs an empty attribute list.
*/

/*!
    \fn QXmlAttributes::~QXmlAttributes()

    Destroys the attributes object.
*/

/*!
    Looks up the index of an attribute by the qualified name \a qName.

    Returns the index of the attribute or -1 if it wasn't found.

    \sa {Namespace Support via Features}
*/
int QXmlAttributes::index(const QString& qName) const
{
    for (int i = 0; i < attList.size(); ++i) {
        if (attList.at(i).qname == qName)
            return i;
    }
    return -1;
}

/*! \overload
  */
int QXmlAttributes::index(const QLatin1String& qName) const
{
    for (int i = 0; i < attList.size(); ++i) {
        if (attList.at(i).qname == qName)
            return i;
    }
    return -1;
}

/*!
    \overload

    Looks up the index of an attribute by a namespace name.

    \a uri specifies the namespace URI, or an empty string if the name
    has no namespace URI. \a localPart specifies the attribute's local
    name.

    Returns the index of the attribute, or -1 if it wasn't found.

    \sa {Namespace Support via Features}
*/
int QXmlAttributes::index(const QString& uri, const QString& localPart) const
{
    for (int i = 0; i < attList.size(); ++i) {
        const Attribute &att = attList.at(i);
        if (att.uri == uri && att.localname == localPart)
            return i;
    }
    return -1;
}

/*!
    Returns the number of attributes in the list.

    \sa count()
*/
int QXmlAttributes::length() const
{
    return attList.count();
}

/*!
    \fn int QXmlAttributes::count() const

    Returns the number of attributes in the list. This function is
    equivalent to length().
*/

/*!
    Looks up an attribute's local name for the attribute at position
    \a index. If no namespace processing is done, the local name is
    an empty string.

    \sa {Namespace Support via Features}
*/
QString QXmlAttributes::localName(int index) const
{
    return attList.at(index).localname;
}

/*!
    Looks up an attribute's XML 1.0 qualified name for the attribute
    at position \a index.

    \sa {Namespace Support via Features}
*/
QString QXmlAttributes::qName(int index) const
{
    return attList.at(index).qname;
}

/*!
    Looks up an attribute's namespace URI for the attribute at
    position \a index. If no namespace processing is done or if the
    attribute has no namespace, the namespace URI is an empty string.

    \sa {Namespace Support via Features}
*/
QString QXmlAttributes::uri(int index) const
{
    return attList.at(index).uri;
}

/*!
    Looks up an attribute's type for the attribute at position \a
    index.

    Currently only "CDATA" is returned.
*/
QString QXmlAttributes::type(int) const
{
    return QLatin1String("CDATA");
}

/*!
    \overload

    Looks up an attribute's type for the qualified name \a qName.

    Currently only "CDATA" is returned.
*/
QString QXmlAttributes::type(const QString&) const
{
    return QLatin1String("CDATA");
}

/*!
    \overload

    Looks up an attribute's type by namespace name.

    \a uri specifies the namespace URI and \a localName specifies the
    local name. If the name has no namespace URI, use an empty string
    for \a uri.

    Currently only "CDATA" is returned.
*/
QString QXmlAttributes::type(const QString&, const QString&) const
{
    return QLatin1String("CDATA");
}

/*!
    Returns an attribute's value for the attribute at position \a
    index. The index must be a valid position
    (i.e., 0 <= \a index < count()).
*/
QString QXmlAttributes::value(int index) const
{
    return attList.at(index).value;
}

/*!
    \overload

    Returns an attribute's value for the qualified name \a qName, or an
    empty string if no attribute exists for the name given.

    \sa {Namespace Support via Features}
*/
QString QXmlAttributes::value(const QString& qName) const
{
    int i = index(qName);
    if (i == -1)
        return QString();
    return attList.at(i).value;
}

/*!
    \overload

    Returns an attribute's value for the qualified name \a qName, or an
    empty string if no attribute exists for the name given.

    \sa {Namespace Support via Features}
*/
QString QXmlAttributes::value(const QLatin1String& qName) const
{
    int i = index(qName);
    if (i == -1)
        return QString();
    return attList.at(i).value;
}

/*!
    \overload

    Returns an attribute's value by namespace name.

    \a uri specifies the namespace URI, or an empty string if the name
    has no namespace URI. \a localName specifies the attribute's local
    name.
*/
QString QXmlAttributes::value(const QString& uri, const QString& localName) const
{
    int i = index(uri, localName);
    if (i == -1)
        return QString();
    return attList.at(i).value;
}

/*!
    Clears the list of attributes.

    \sa append()
*/
void QXmlAttributes::clear()
{
    attList.clear();
}

/*!
    Appends a new attribute entry to the list of attributes. The
    qualified name of the attribute is \a qName, the namespace URI is
    \a uri and the local name is \a localPart. The value of the
    attribute is \a value.

    \sa qName() uri() localName() value()
*/
void QXmlAttributes::append(const QString &qName, const QString &uri, const QString &localPart, const QString &value)
{
    Attribute att;
    att.qname = qName;
    att.uri = uri;
    att.localname = localPart;
    att.value = value;

    attList.append(att);
}


/*********************************************
 *
 * QXmlInputSource
 *
 *********************************************/

/*!
    \class QXmlInputSource
    \reentrant
    \brief The QXmlInputSource class provides the input data for the
    QXmlReader subclasses.

    \inmodule QtXml
    \ingroup xml-tools

    All subclasses of QXmlReader read the input XML document from this
    class.

    This class recognizes the encoding of the data by reading the
    encoding declaration in the XML file if it finds one, and reading
    the data using the corresponding encoding. If it does not find an
    encoding declaration, then it assumes that the data is either in
    UTF-8 or UTF-16, depending on whether it can find a byte-order
    mark.

    There are two ways to populate the input source with data: you can
    construct it with a QIODevice* so that the input source reads the
    data from that device. Or you can set the data explicitly with one
    of the setData() functions.

    Usually you either construct a QXmlInputSource that works on a
    QIODevice* or you construct an empty QXmlInputSource and set the
    data with setData(). There are only rare occasions where you would
    want to mix both methods.

    The QXmlReader subclasses use the next() function to read the
    input character by character. If you want to start from the
    beginning again, use reset().

    The functions data() and fetchData() are useful if you want to do
    something with the data other than parsing, e.g. displaying the
    raw XML file. The benefit of using the QXmlInputClass in such
    cases is that it tries to use the correct encoding.

    \sa QXmlReader QXmlSimpleReader
*/

// the following two are guaranteed not to be a character
const ushort QXmlInputSource::EndOfData = 0xfffe;
const ushort QXmlInputSource::EndOfDocument = 0xffff;

/*
    Common part of the constructors.
*/
void QXmlInputSource::init()
{
    d = new QXmlInputSourcePrivate;

    QT_TRY {
        d->inputDevice = 0;
        d->inputStream = 0;

        setData(QString());
#ifndef QT_NO_TEXTCODEC
        d->encMapper = 0;
#endif
        d->nextReturnedEndOfData = true; // first call to next() will call fetchData()

        d->encodingDeclBytes.clear();
        d->encodingDeclChars.clear();
        d->lookingForEncodingDecl = true;
    } QT_CATCH(...) {
        delete(d);
        QT_RETHROW;
    }
}

/*!
    Constructs an input source which contains no data.

    \sa setData()
*/
QXmlInputSource::QXmlInputSource()
{
    init();
}

/*!
    Constructs an input source and gets the data from device \a dev.
    If \a dev is not open, it is opened in read-only mode. If \a dev
    is 0 or it is not possible to read from the device, the input
    source will contain no data.

    \sa setData() fetchData() QIODevice
*/
QXmlInputSource::QXmlInputSource(QIODevice *dev)
{
    init();
    d->inputDevice = dev;
    if (dev->isOpen())
        d->inputDevice->setTextModeEnabled(false);
}

#ifdef QT3_SUPPORT
/*!
    Use the QXmlInputSource(QIODevice *) constructor instead, with
    the device used by \a stream.

    \sa QTextStream::device()
*/
QXmlInputSource::QXmlInputSource(QTextStream& stream)
{
    init();
    d->inputStream = &stream;
}

/*!
    Use QXmlInputSource(&\a file) instead.
*/
QXmlInputSource::QXmlInputSource(QFile& file)
{
    init();
    d->inputDevice = &file;
}
#endif

/*!
    Destructor.
*/
QXmlInputSource::~QXmlInputSource()
{
    // ### Qt 5: close the input device. See task 153111
#ifndef QT_NO_TEXTCODEC
    delete d->encMapper;
#endif
    delete d;
}

/*!
Returns the next character of the input source. If this function
reaches the end of available data, it returns
QXmlInputSource::EndOfData. If you call next() after that, it
tries to fetch more data by calling fetchData(). If the
fetchData() call results in new data, this function returns the
first character of that data; otherwise it returns
QXmlInputSource::EndOfDocument.

Readers, such as QXmlSimpleReader, will assume that the end of
the XML document has been reached if the this function returns
QXmlInputSource::EndOfDocument, and will check that the
supplied input is well-formed. Therefore, when reimplementing
this function, it is important to ensure that this behavior is
duplicated.

\sa reset() fetchData() QXmlSimpleReader::parse() QXmlSimpleReader::parseContinue()
*/
QChar QXmlInputSource::next()
{
    if (d->pos >= d->length) {
        if (d->nextReturnedEndOfData) {
            d->nextReturnedEndOfData = false;
            fetchData();
            if (d->pos >= d->length) {
                return EndOfDocument;
            }
            return next();
        }
        d->nextReturnedEndOfData = true;
        return EndOfData;
    }

    // QXmlInputSource has no way to signal encoding errors. The best we can do
    // is return EndOfDocument. We do *not* return EndOfData, because the reader
    // will then just call this function again to get the next char.
    QChar c = d->unicode[d->pos++];
    if (c.unicode() == EndOfData)
        c = EndOfDocument;
    return c;
}

/*!
    This function sets the position used by next() to the beginning of
    the data returned by data(). This is useful if you want to use the
    input source for more than one parse.

    \note In the case that the underlying data source is a QIODevice,
    the current position in the device is not automatically set to the
    start of input. Call QIODevice::seek(0) on the device to do this.

    \sa next()
*/
void QXmlInputSource::reset()
{
    d->nextReturnedEndOfData = false;
    d->pos = 0;
}

/*!
    Returns the data the input source contains or an empty string if the
    input source does not contain any data.

    \sa setData() QXmlInputSource() fetchData()
*/
QString QXmlInputSource::data() const
{
    if (d->nextReturnedEndOfData) {
        QXmlInputSource *that = const_cast<QXmlInputSource*>(this);
        that->d->nextReturnedEndOfData = false;
        that->fetchData();
    }
    return d->str;
}

/*!
    Sets the data of the input source to \a dat.

    If the input source already contains data, this function deletes
    that data first.

    \sa data()
*/
void QXmlInputSource::setData(const QString& dat)
{
    d->str = dat;
    d->unicode = dat.unicode();
    d->pos = 0;
    d->length = d->str.length();
    d->nextReturnedEndOfData = false;
}

/*!
    \overload

    The data \a dat is passed through the correct text-codec, before
    it is set.
*/
void QXmlInputSource::setData(const QByteArray& dat)
{
    setData(fromRawData(dat));
}

/*!
    This function reads more data from the device that was set during
    construction. If the input source already contained data, this
    function deletes that data first.

    This object contains no data after a call to this function if the
    object was constructed without a device to read data from or if
    this function was not able to get more data from the device.

    There are two occasions where a fetch is done implicitly by
    another function call: during construction (so that the object
    starts out with some initial data where available), and during a
    call to next() (if the data had run out).

    You don't normally need to use this function if you use next().

    \sa data() next() QXmlInputSource()
*/

void QXmlInputSource::fetchData()
{
    enum
    {
        BufferSize = 1024
    };

    QByteArray rawData;

    if (d->inputDevice || d->inputStream) {
        QIODevice *device = d->inputDevice ? d->inputDevice : d->inputStream->device();

        if (!device) {
            if (d->inputStream && d->inputStream->string()) {
                QString *s = d->inputStream->string();
                rawData = QByteArray((const char *) s->constData(), s->size() * sizeof(QChar));
            }
        } else if (device->isOpen() || device->open(QIODevice::ReadOnly)) {
            rawData.resize(BufferSize);
            qint64 size = device->read(rawData.data(), BufferSize);

            if (size != -1) {
                // We don't want to give fromRawData() less than four bytes if we can avoid it.
                while (size < 4) {
                    if (!device->waitForReadyRead(-1))
                        break;
                    int ret = device->read(rawData.data() + size, BufferSize - size);
                    if (ret <= 0)
                        break;
                    size += ret;
                }
            }

            rawData.resize(qMax(qint64(0), size));
        }

        /* We do this inside the "if (d->inputDevice ..." scope
         * because if we're not using a stream or device, that is,
         * the user set a QString manually, we don't want to set
         * d->str. */
        setData(fromRawData(rawData));
    }
}

#ifndef QT_NO_TEXTCODEC
static QString extractEncodingDecl(const QString &text, bool *needMoreText)
{
    *needMoreText = false;

    int l = text.length();
    QString snip = QString::fromLatin1("<?xml").left(l);
    if (l > 0 && !text.startsWith(snip))
        return QString();

    int endPos = text.indexOf(QLatin1Char('>'));
    if (endPos == -1) {
        *needMoreText = l < 255; // we won't look forever
        return QString();
    }

    int pos = text.indexOf(QLatin1String("encoding"));
    if (pos == -1 || pos >= endPos)
        return QString();

    while (pos < endPos) {
        ushort uc = text.at(pos).unicode();
        if (uc == '\'' || uc == '"')
            break;
        ++pos;
    }

    if (pos == endPos)
        return QString();

    QString encoding;
    ++pos;
    while (pos < endPos) {
        ushort uc = text.at(pos).unicode();
        if (uc == '\'' || uc == '"')
            break;
        encoding.append(uc);
        ++pos;
    }

    return encoding;
}
#endif // QT_NO_TEXTCODEC

/*!
    This function reads the XML file from \a data and tries to
    recognize the encoding. It converts the raw data \a data into a
    QString and returns it. It tries its best to get the correct
    encoding for the XML file.

    If \a beginning is true, this function assumes that the data
    starts at the beginning of a new XML document and looks for an
    encoding declaration. If \a beginning is false, it converts the
    raw data using the encoding determined from prior calls.
*/
QString QXmlInputSource::fromRawData(const QByteArray &data, bool beginning)
{
#ifdef QT_NO_TEXTCODEC
    Q_UNUSED(beginning);
    return QString::fromAscii(data.constData(), data.size());
#else
    if (data.size() == 0)
        return QString();
    if (beginning) {
        delete d->encMapper;
        d->encMapper = 0;
    }

    int mib = 106; // UTF-8

    // This is the initial UTF codec we will read the encoding declaration with
    if (d->encMapper == 0) {
        d->encodingDeclBytes.clear();
        d->encodingDeclChars.clear();
        d->lookingForEncodingDecl = true;

        // look for byte order mark and read the first 5 characters
        if (data.size() >= 4) {
            uchar ch1 = data.at(0);
            uchar ch2 = data.at(1);
            uchar ch3 = data.at(2);
            uchar ch4 = data.at(3);

            if ((ch1 == 0 && ch2 == 0 && ch3 == 0xfe && ch4 == 0xff) ||
                (ch1 == 0xff && ch2 == 0xfe && ch3 == 0 && ch4 == 0))
                mib = 1017; // UTF-32 with byte order mark
            else if (ch1 == 0x3c && ch2 == 0x00 && ch3 == 0x00 && ch4 == 0x00)
                mib = 1019; // UTF-32LE
            else if (ch1 == 0x00 && ch2 == 0x00 && ch3 == 0x00 && ch4 == 0x3c)
                mib = 1018; // UTF-32BE
        }
        if (mib == 106 && data.size() >= 2) {
            uchar ch1 = data.at(0);
            uchar ch2 = data.at(1);

            if ((ch1 == 0xfe && ch2 == 0xff) || (ch1 == 0xff && ch2 == 0xfe))
                mib = 1015; // UTF-16 with byte order mark
            else if (ch1 == 0x3c && ch2 == 0x00)
                mib = 1014; // UTF-16LE
            else if (ch1 == 0x00 && ch2 == 0x3c)
                mib = 1013; // UTF-16BE
        }

        QTextCodec *codec = QTextCodec::codecForMib(mib);
        Q_ASSERT(codec);

        d->encMapper = codec->makeDecoder();
    }

    QString input = d->encMapper->toUnicode(data, data.size());

    if (d->lookingForEncodingDecl) {
        d->encodingDeclChars += input;

        bool needMoreText;
        QString encoding = extractEncodingDecl(d->encodingDeclChars, &needMoreText);

        if (!encoding.isEmpty()) {
            if (QTextCodec *codec = QTextCodec::codecForName(encoding.toLatin1())) {
                /* If the encoding is the same, we don't have to do toUnicode() all over again. */
                if(codec->mibEnum() != mib) {
                    delete d->encMapper;
                    d->encMapper = codec->makeDecoder();

                    /* The variable input can potentially be large, so we deallocate
                     * it before calling toUnicode() in order to avoid having two
                     * large QStrings in memory simultaneously. */
                    input.clear();

                    // prime the decoder with the data so far
                    d->encMapper->toUnicode(d->encodingDeclBytes, d->encodingDeclBytes.size());
                    // now feed it the new data
                    input = d->encMapper->toUnicode(data, data.size());
                }
            }
        }

        d->encodingDeclBytes += data;
        d->lookingForEncodingDecl = needMoreText;
    }

    return input;
#endif
}


/*********************************************
 *
 * QXmlDefaultHandler
 *
 *********************************************/

/*!
    \class QXmlContentHandler
    \reentrant
    \brief The QXmlContentHandler class provides an interface to
    report the logical content of XML data.

    \inmodule QtXml
    \ingroup xml-tools

    If the application needs to be informed of basic parsing events,
    it can implement this interface and activate it using
    QXmlReader::setContentHandler(). The reader can then report basic
    document-related events like the start and end of elements and
    character data through this interface.

    The order of events in this interface is very important, and
    mirrors the order of information in the document itself. For
    example, all of an element's content (character data, processing
    instructions, and sub-elements) appears, in order, between the
    startElement() event and the corresponding endElement() event.

    The class QXmlDefaultHandler provides a default implementation for
    this interface; subclassing from the QXmlDefaultHandler class is
    very convenient if you only want to be informed of some parsing
    events.

    The startDocument() function is called at the start of the
    document, and endDocument() is called at the end. Before parsing
    begins setDocumentLocator() is called. For each element
    startElement() is called, with endElement() being called at the
    end of each element. The characters() function is called with
    chunks of character data; ignorableWhitespace() is called with
    chunks of whitespace and processingInstruction() is called with
    processing instructions. If an entity is skipped skippedEntity()
    is called. At the beginning of prefix-URI scopes
    startPrefixMapping() is called.

    \sa QXmlDTDHandler, QXmlDeclHandler, QXmlEntityResolver, QXmlErrorHandler,
        QXmlLexicalHandler, {Introduction to SAX2}
*/

/*!
    \fn QXmlContentHandler::~QXmlContentHandler()

    Destroys the content handler.
*/

/*!
    \fn void QXmlContentHandler::setDocumentLocator(QXmlLocator* locator)

    The reader calls this function before it starts parsing the
    document. The argument \a locator is a pointer to a QXmlLocator
    which allows the application to get the parsing position within
    the document.

    Do not destroy the \a locator; it is destroyed when the reader is
    destroyed. (Do not use the \a locator after the reader is
    destroyed).
*/

/*!
    \fn bool QXmlContentHandler::startDocument()

    The reader calls this function when it starts parsing the
    document. The reader calls this function just once, after the call
    to setDocumentLocator(), and before any other functions in this
    class or in the QXmlDTDHandler class are called.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.

    \sa endDocument()
*/

/*!
    \fn bool QXmlContentHandler::endDocument()

    The reader calls this function after it has finished parsing. It
    is called just once, and is the last handler function called. It
    is called after the reader has read all input or has abandoned
    parsing because of a fatal error.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.

    \sa startDocument()
*/

/*!
    \fn bool QXmlContentHandler::startPrefixMapping(const QString& prefix, const QString& uri)

    The reader calls this function to signal the begin of a prefix-URI
    namespace mapping scope. This information is not necessary for
    normal namespace processing since the reader automatically
    replaces prefixes for element and attribute names.

    Note that startPrefixMapping() and endPrefixMapping() calls are
    not guaranteed to be properly nested relative to each other: all
    startPrefixMapping() events occur before the corresponding
    startElement() event, and all endPrefixMapping() events occur
    after the corresponding endElement() event, but their order is not
    otherwise guaranteed.

    The argument \a prefix is the namespace prefix being declared and
    the argument \a uri is the namespace URI the prefix is mapped to.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.

    \sa endPrefixMapping(), {Namespace Support via Features}
*/

/*!
    \fn bool QXmlContentHandler::endPrefixMapping(const QString& prefix)

    The reader calls this function to signal the end of a prefix
    mapping for the prefix \a prefix.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.

    \sa startPrefixMapping(), {Namespace Support via Features}
*/

/*!
    \fn bool QXmlContentHandler::startElement(const QString& namespaceURI, const QString& localName, const QString& qName, const QXmlAttributes& atts)

    The reader calls this function when it has parsed a start element
    tag.

    There is a corresponding endElement() call when the corresponding
    end element tag is read. The startElement() and endElement() calls
    are always nested correctly. Empty element tags (e.g. \c{<x/>})
    cause a startElement() call to be immediately followed by an
    endElement() call.

    The attribute list provided only contains attributes with explicit
    values. The attribute list contains attributes used for namespace
    declaration (i.e. attributes starting with xmlns) only if the
    namespace-prefix property of the reader is true.

    The argument \a namespaceURI is the namespace URI, or
    an empty string if the element has no namespace URI or if no
    namespace processing is done. \a localName is the local name
    (without prefix), or an empty string if no namespace processing is
    done, \a qName is the qualified name (with prefix) and \a atts are
    the attributes attached to the element. If there are no
    attributes, \a atts is an empty attributes object.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.

    \sa endElement(), {Namespace Support via Features}
*/

/*!
    \fn bool QXmlContentHandler::endElement(const QString& namespaceURI, const QString& localName, const QString& qName)

    The reader calls this function when it has parsed an end element
    tag with the qualified name \a qName, the local name \a localName
    and the namespace URI \a namespaceURI.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.

    \sa startElement(), {Namespace Support via Features}
*/

/*!
    \fn bool QXmlContentHandler::characters(const QString& ch)

    The reader calls this function when it has parsed a chunk of
    character data (either normal character data or character data
    inside a CDATA section; if you need to distinguish between those
    two types you must use QXmlLexicalHandler::startCDATA() and
    QXmlLexicalHandler::endCDATA()). The character data is reported in
    \a ch.

    Some readers report whitespace in element content using the
    ignorableWhitespace() function rather than using this one.

    A reader may report the character data of an element in more than
    one chunk; e.g. a reader might want to report "a\<b" in three
    characters() events ("a ", "\<" and " b").

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.
*/

/*!
    \fn bool QXmlContentHandler::ignorableWhitespace(const QString& ch)

    Some readers may use this function to report each chunk of
    whitespace in element content. The whitespace is reported in \a ch.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.
*/

/*!
    \fn bool QXmlContentHandler::processingInstruction(const QString& target, const QString& data)

    The reader calls this function when it has parsed a processing
    instruction.

    \a target is the target name of the processing instruction and \a
    data is the data in the processing instruction.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.
*/

/*!
    \fn bool QXmlContentHandler::skippedEntity(const QString& name)

    Some readers may skip entities if they have not seen the
    declarations (e.g. because they are in an external DTD). If they
    do so they report that they skipped the entity called \a name by
    calling this function.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.
*/

/*!
    \fn QString QXmlContentHandler::errorString() const

    The reader calls this function to get an error string, e.g. if any
    of the handler functions returns false.
*/


/*!
    \class QXmlErrorHandler
    \reentrant
    \brief The QXmlErrorHandler class provides an interface to report
    errors in XML data.

    \inmodule QtXml
    \ingroup xml-tools

    If you want your application to report errors to the user or to
    perform customized error handling, you should subclass this class.

    You can set the error handler with QXmlReader::setErrorHandler().

    Errors can be reported using warning(), error() and fatalError(),
    with the error text being reported with errorString().

    \sa QXmlDTDHandler, QXmlDeclHandler, QXmlContentHandler, QXmlEntityResolver,
        QXmlLexicalHandler, {Introduction to SAX2}
*/

/*!
    \fn QXmlErrorHandler::~QXmlErrorHandler()

    Destroys the error handler.
*/

/*!
    \fn bool QXmlErrorHandler::warning(const QXmlParseException& exception)

    A reader might use this function to report a warning. Warnings are
    conditions that are not errors or fatal errors as defined by the
    XML 1.0 specification. Details of the warning are stored in \a
    exception.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.
*/

/*!
    \fn bool QXmlErrorHandler::error(const QXmlParseException& exception)

    A reader might use this function to report a recoverable error. A
    recoverable error corresponds to the definiton of "error" in
    section 1.2 of the XML 1.0 specification. Details of the error are
    stored in \a exception.

    The reader must continue to provide normal parsing events after
    invoking this function.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.
*/

/*!
\fn bool QXmlErrorHandler::fatalError(const QXmlParseException& exception)

A reader must use this function to report a non-recoverable error.
Details of the error are stored in \a exception.

If this function returns true the reader might try to go on
parsing and reporting further errors, but no regular parsing
events are reported.
*/

/*!
    \fn QString QXmlErrorHandler::errorString() const

    The reader calls this function to get an error string if any of
    the handler functions returns false.
*/


/*!
    \class QXmlDTDHandler
    \reentrant
    \brief The QXmlDTDHandler class provides an interface to report
    DTD content of XML data.

    \inmodule QtXml
    \ingroup xml-tools

    If an application needs information about notations and unparsed
    entities, it can implement this interface and register an instance
    with QXmlReader::setDTDHandler().

    Note that this interface includes only those DTD events that the
    XML recommendation requires processors to report, i.e. notation
    and unparsed entity declarations using notationDecl() and
    unparsedEntityDecl() respectively.

    \sa QXmlDeclHandler, QXmlContentHandler, QXmlEntityResolver, QXmlErrorHandler,
        QXmlLexicalHandler, {Introduction to SAX2}
*/

/*!
    \fn QXmlDTDHandler::~QXmlDTDHandler()

    Destroys the DTD handler.
*/

/*!
    \fn bool QXmlDTDHandler::notationDecl(const QString& name, const QString& publicId, const QString& systemId)

    The reader calls this function when it has parsed a notation
    declaration.

    The argument \a name is the notation name, \a publicId is the
    notation's public identifier and \a systemId is the notation's
    system identifier.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.
*/

/*!
    \fn bool QXmlDTDHandler::unparsedEntityDecl(const QString& name, const QString& publicId, const QString& systemId, const QString& notationName)

    The reader calls this function when it finds an unparsed entity
    declaration.

    The argument \a name is the unparsed entity's name, \a publicId is
    the entity's public identifier, \a systemId is the entity's system
    identifier and \a notationName is the name of the associated
    notation.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.
*/

/*!
    \fn QString QXmlDTDHandler::errorString() const

    The reader calls this function to get an error string if any of
    the handler functions returns false.
*/


/*!
    \class QXmlEntityResolver
    \reentrant
    \brief The QXmlEntityResolver class provides an interface to
    resolve external entities contained in XML data.

    \inmodule QtXml
    \ingroup xml-tools

    If an application needs to implement customized handling for
    external entities, it must implement this interface, i.e.
    resolveEntity(), and register it with
    QXmlReader::setEntityResolver().

    \sa QXmlDTDHandler, QXmlDeclHandler, QXmlContentHandler, QXmlErrorHandler,
        QXmlLexicalHandler, {Introduction to SAX2}
*/

/*!
    \fn QXmlEntityResolver::~QXmlEntityResolver()

    Destroys the entity resolver.
*/

/*!
    \fn bool QXmlEntityResolver::resolveEntity(const QString& publicId, const QString& systemId, QXmlInputSource*& ret)

    The reader calls this function before it opens any external
    entity, except the top-level document entity. The application may
    request the reader to resolve the entity itself (\a ret is 0) or
    to use an entirely different input source (\a ret points to the
    input source).

    The reader deletes the input source \a ret when it no longer needs
    it, so you should allocate it on the heap with \c new.

    The argument \a publicId is the public identifier of the external
    entity, \a systemId is the system identifier of the external
    entity and \a ret is the return value of this function. If \a ret
    is 0 the reader should resolve the entity itself, if it is
    non-zero it must point to an input source which the reader uses
    instead.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.
*/

/*!
    \fn QString QXmlEntityResolver::errorString() const

    The reader calls this function to get an error string if any of
    the handler functions returns false.
*/


/*!
    \class QXmlLexicalHandler
    \reentrant
    \brief The QXmlLexicalHandler class provides an interface to
    report the lexical content of XML data.

    \inmodule QtXml
    \ingroup xml-tools

    The events in the lexical handler apply to the entire document,
    not just to the document element, and all lexical handler events
    appear between the content handler's startDocument and endDocument
    events.

    You can set the lexical handler with
    QXmlReader::setLexicalHandler().

    This interface's design is based on the SAX2 extension
    LexicalHandler.

    The interface provides the startDTD(), endDTD(), startEntity(),
    endEntity(), startCDATA(), endCDATA() and comment() functions.

    \sa QXmlDTDHandler, QXmlDeclHandler, QXmlContentHandler, QXmlEntityResolver,
        QXmlErrorHandler, {Introduction to SAX2}
*/

/*!
    \fn QXmlLexicalHandler::~QXmlLexicalHandler()

    Destroys the lexical handler.
*/

/*!
    \fn bool QXmlLexicalHandler::startDTD(const QString& name, const QString& publicId, const QString& systemId)

    The reader calls this function to report the start of a DTD
    declaration, if any. It reports the name of the document type in
    \a name, the public identifier in \a publicId and the system
    identifier in \a systemId.

    If the public identifier is missing, \a publicId is set to
    an empty string. If the system identifier is missing, \a systemId is
    set to an empty string. Note that it is not valid XML to have a
    public identifier but no system identifier; in such cases a parse
    error will occur.

    All declarations reported through QXmlDTDHandler or
    QXmlDeclHandler appear between the startDTD() and endDTD() calls.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.

    \sa endDTD()
*/

/*!
    \fn bool QXmlLexicalHandler::endDTD()

    The reader calls this function to report the end of a DTD
    declaration, if any.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.

    \sa startDTD()
*/

/*!
    \fn bool QXmlLexicalHandler::startEntity(const QString& name)

    The reader calls this function to report the start of an entity
    called \a name.

    Note that if the entity is unknown, the reader reports it through
    QXmlContentHandler::skippedEntity() and not through this
    function.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.

    \sa endEntity(), QXmlSimpleReader::setFeature()
*/

/*!
    \fn bool QXmlLexicalHandler::endEntity(const QString& name)

    The reader calls this function to report the end of an entity
    called \a name.

    For every startEntity() call, there is a corresponding endEntity()
    call. The calls to startEntity() and endEntity() are properly
    nested.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.

    \sa startEntity(), QXmlContentHandler::skippedEntity(), QXmlSimpleReader::setFeature()
*/

/*!
    \fn bool QXmlLexicalHandler::startCDATA()

    The reader calls this function to report the start of a CDATA
    section. The content of the CDATA section is reported through the
    QXmlContentHandler::characters() function. This function is
    intended only to report the boundary.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.

    \sa endCDATA()
*/

/*!
    \fn bool QXmlLexicalHandler::endCDATA()

    The reader calls this function to report the end of a CDATA
    section.

    If this function returns false the reader stops parsing and reports
    an error. The reader uses the function errorString() to get the error
    message.

    \sa startCDATA(), QXmlContentHandler::characters()
*/

/*!
    \fn bool QXmlLexicalHandler::comment(const QString& ch)

    The reader calls this function to report an XML comment anywhere
    in the document. It reports the text of the comment in \a ch.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.
*/

/*!
    \fn QString QXmlLexicalHandler::errorString() const

    The reader calls this function to get an error string if any of
    the handler functions returns false.
*/


/*!
    \class QXmlDeclHandler
    \reentrant
    \brief The QXmlDeclHandler class provides an interface to report declaration
    content of XML data.

    \inmodule QtXml
    \ingroup xml-tools

    You can set the declaration handler with
    QXmlReader::setDeclHandler().

    This interface is based on the SAX2 extension DeclHandler.

    The interface provides attributeDecl(), internalEntityDecl() and
    externalEntityDecl() functions.

    \sa QXmlDTDHandler, QXmlContentHandler, QXmlEntityResolver, QXmlErrorHandler,
        QXmlLexicalHandler, {Introduction to SAX2}
*/

/*!
    \fn QXmlDeclHandler::~QXmlDeclHandler()

    Destroys the declaration handler.
*/

/*!
    \fn bool QXmlDeclHandler::attributeDecl(const QString& eName, const QString& aName, const QString& type, const QString& valueDefault, const QString& value)

    The reader calls this function to report an attribute type
    declaration. Only the effective (first) declaration for an
    attribute is reported.

    The reader passes the name of the associated element in \a eName
    and the name of the attribute in \a aName. It passes a string that
    represents the attribute type in \a type and a string that
    represents the attribute default in \a valueDefault. This string
    is one of "#IMPLIED", "#REQUIRED", "#FIXED" or an empty string (if
    none of the others applies). The reader passes the attribute's
    default value in \a value. If no default value is specified in the
    XML file, \a value is an empty string.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.
*/

/*!
    \fn bool QXmlDeclHandler::internalEntityDecl(const QString& name, const QString& value)

    The reader calls this function to report an internal entity
    declaration. Only the effective (first) declaration is reported.

    The reader passes the name of the entity in \a name and the value
    of the entity in \a value.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.
*/

/*!
    \fn bool QXmlDeclHandler::externalEntityDecl(const QString& name, const QString& publicId, const QString& systemId)

    The reader calls this function to report a parsed external entity
    declaration. Only the effective (first) declaration for each
    entity is reported.

    The reader passes the name of the entity in \a name, the public
    identifier in \a publicId and the system identifier in \a
    systemId. If there is no public identifier specified, it passes
    an empty string in \a publicId.

    If this function returns false the reader stops parsing and
    reports an error. The reader uses the function errorString() to
    get the error message.
*/

/*!
    \fn QString QXmlDeclHandler::errorString() const

    The reader calls this function to get an error string if any of
    the handler functions returns false.
*/


/*!
    \class QXmlDefaultHandler
    \reentrant
    \brief The QXmlDefaultHandler class provides a default implementation of all
    the XML handler classes.

    \inmodule QtXml
    \ingroup xml-tools

    This class gathers together the features of
    the specialized handler classes, making it a convenient
    starting point when implementing custom handlers for
    subclasses of QXmlReader, particularly QXmlSimpleReader.
    The virtual functions from each of the base classes are
    reimplemented in this class, providing sensible default behavior
    for many common cases. By subclassing this class, and
    overriding these functions, you can concentrate
    on implementing the parts of the handler relevant to your
    application.

    The XML reader must be told which handler to use for different
    kinds of events during parsing. This means that, although
    QXmlDefaultHandler provides default implementations of functions
    inherited from all its base classes, we can still use specialized
    handlers for particular kinds of events.

    For example, QXmlDefaultHandler subclasses both
    QXmlContentHandler and QXmlErrorHandler, so by subclassing
    it we can use the same handler for both of the following
    reader functions:

    \snippet doc/src/snippets/xml/rsslisting/rsslisting.cpp 0

    Since the reader will inform the handler of parsing errors, it is
    necessary to reimplement QXmlErrorHandler::fatalError() if, for
    example, we want to stop parsing when such an error occurs:

    \snippet doc/src/snippets/xml/rsslisting/handler.cpp 0

    The above function returns false, which tells the reader to stop
    parsing. To continue to use the same reader,
    it is necessary to create a new handler instance, and set up the
    reader to use it in the manner described above.

    It is useful to examine some of the functions inherited by
    QXmlDefaultHandler, and consider why they might be
    reimplemented in a custom handler.
    Custom handlers will typically reimplement
    QXmlContentHandler::startDocument() to prepare the handler for
    new content. Document elements and the text within them can be
    processed by reimplementing QXmlContentHandler::startElement(),
    QXmlContentHandler::endElement(), and
    QXmlContentHandler::characters().
    You may want to reimplement QXmlContentHandler::endDocument()
    to perform some finalization or validation on the content once the
    document has been read completely.

    \sa QXmlDTDHandler, QXmlDeclHandler, QXmlContentHandler, QXmlEntityResolver,
        QXmlErrorHandler, QXmlLexicalHandler, {Introduction to SAX2}
*/

/*!
    \fn QXmlDefaultHandler::QXmlDefaultHandler()

    Constructs a handler for use with subclasses of QXmlReader.
*/
/*!
    \fn QXmlDefaultHandler::~QXmlDefaultHandler()

    Destroys the handler.
*/

/*!
    \reimp

    This reimplementation does nothing.
*/
void QXmlDefaultHandler::setDocumentLocator(QXmlLocator*)
{
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::startDocument()
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::endDocument()
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::startPrefixMapping(const QString&, const QString&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::endPrefixMapping(const QString&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::startElement(const QString&, const QString&,
        const QString&, const QXmlAttributes&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::endElement(const QString&, const QString&,
        const QString&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::characters(const QString&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::ignorableWhitespace(const QString&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::processingInstruction(const QString&,
        const QString&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::skippedEntity(const QString&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::warning(const QXmlParseException&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::error(const QXmlParseException&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::fatalError(const QXmlParseException&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::notationDecl(const QString&, const QString&,
        const QString&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::unparsedEntityDecl(const QString&, const QString&,
        const QString&, const QString&)
{
    return true;
}

/*!
    \reimp

    Sets \a ret to 0, so that the reader uses the system identifier
    provided in the XML document.
*/
bool QXmlDefaultHandler::resolveEntity(const QString&, const QString&,
        QXmlInputSource*& ret)
{
    ret = 0;
    return true;
}

/*!
    \reimp

    Returns the default error string.
*/
QString QXmlDefaultHandler::errorString() const
{
    return QString::fromLatin1(XMLERR_ERRORBYCONSUMER);
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::startDTD(const QString&, const QString&, const QString&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::endDTD()
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::startEntity(const QString&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::endEntity(const QString&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::startCDATA()
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::endCDATA()
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::comment(const QString&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::attributeDecl(const QString&, const QString&, const QString&, const QString&, const QString&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::internalEntityDecl(const QString&, const QString&)
{
    return true;
}

/*!
    \reimp

    This reimplementation does nothing.
*/
bool QXmlDefaultHandler::externalEntityDecl(const QString&, const QString&, const QString&)
{
    return true;
}


/*********************************************
 *
 * QXmlSimpleReaderPrivate
 *
 *********************************************/

inline bool QXmlSimpleReaderPrivate::atEnd()
{
    return (c.unicode()|0x0001) == 0xffff;
}

inline void QXmlSimpleReaderPrivate::stringClear()
{
    stringValueLen = 0; stringArrayPos = 0;
}
inline void QXmlSimpleReaderPrivate::nameClear()
{
    nameValueLen = 0; nameArrayPos = 0;
}

inline void QXmlSimpleReaderPrivate::refClear()
{
    refValueLen = 0; refArrayPos = 0;
}

QXmlSimpleReaderPrivate::QXmlSimpleReaderPrivate(QXmlSimpleReader *reader)
{
    q_ptr = reader;
    parseStack = 0;

    locator.reset(new QXmlSimpleReaderLocator(reader));
    entityRes  = 0;
    dtdHnd     = 0;
    contentHnd = 0;
    errorHnd   = 0;
    lexicalHnd = 0;
    declHnd    = 0;

    // default feature settings
    useNamespaces = true;
    useNamespacePrefixes = false;
    reportWhitespaceCharData = true;
    reportEntities = false;
}

QXmlSimpleReaderPrivate::~QXmlSimpleReaderPrivate()
{
    delete parseStack;
}

void QXmlSimpleReaderPrivate::initIncrementalParsing()
{
    if(parseStack)
        parseStack->clear();
    else
        parseStack = new QStack<ParseState>;
}

/*********************************************
 *
 * QXmlSimpleReader
 *
 *********************************************/

/*!
    \class QXmlReader
    \reentrant
    \brief The QXmlReader class provides an interface for XML readers (i.e.
    parsers).

    \inmodule QtXml
    \ingroup xml-tools

    This abstract class provides an interface for all of Qt's XML
    readers. Currently there is only one implementation of a reader
    included in Qt's XML module: QXmlSimpleReader. In future releases
    there might be more readers with different properties available
    (e.g. a validating parser).

    The design of the XML classes follows the \link
    http://www.saxproject.org/ SAX2 Java interface\endlink, with
    the names adapted to fit Qt naming conventions. It should be very
    easy for anybody who has worked with SAX2 to get started with the
    Qt XML classes.

    All readers use the class QXmlInputSource to read the input
    document. Since you are normally interested in particular content
    in the XML document, the reader reports the content through
    special handler classes (QXmlDTDHandler, QXmlDeclHandler,
    QXmlContentHandler, QXmlEntityResolver, QXmlErrorHandler and
    QXmlLexicalHandler), which you must subclass, if you want to
    process the contents.

    Since the handler classes only describe interfaces you must
    implement all the functions. We provide the QXmlDefaultHandler
    class to make this easier: it implements a default behavior (do
    nothing) for all functions, so you can subclass it and just
    implement the functions you are interested in.

    Features and properties of the reader can be set with setFeature()
    and setProperty() respectively. You can set the reader to use your
    own subclasses with setEntityResolver(), setDTDHandler(),
    setContentHandler(), setErrorHandler(), setLexicalHandler() and
    setDeclHandler(). The parse itself is started with a call to
    parse().

    \sa QXmlSimpleReader
*/

/*!
    \fn QXmlReader::~QXmlReader()

    Destroys the reader.
*/

/*!
    \fn bool QXmlReader::feature(const QString& name, bool *ok) const

    If the reader has the feature called \a name, the feature's value
    is returned. If no such feature exists the return value is
    undefined.

    If \a ok is not 0: \c{*}\a{ok}  is set to true if the reader has the
    feature called \a name; otherwise \c{*}\a{ok} is set to false.

    \sa setFeature(), hasFeature()
*/

/*!
    \fn void QXmlReader::setFeature(const QString& name, bool value)

    Sets the feature called \a name to the given \a value. If the
    reader doesn't have the feature nothing happens.

    \sa feature(), hasFeature()
*/

/*!
    \fn bool QXmlReader::hasFeature(const QString& name) const

    Returns \c true if the reader has the feature called \a name;
    otherwise returns false.

    \sa feature(), setFeature()
*/

/*!
    \fn void* QXmlReader::property(const QString& name, bool *ok) const

    If the reader has the property \a name, this function returns the
    value of the property; otherwise the return value is undefined.

    If \a ok is not 0: if the reader has the \a name property
    \c{*}\a{ok} is set to true; otherwise \c{*}\a{ok} is set to false.

    \sa setProperty(), hasProperty()
*/

/*!
    \fn void QXmlReader::setProperty(const QString& name, void* value)

    Sets the property \a name to \a value. If the reader doesn't have
    the property nothing happens.

    \sa property(), hasProperty()
*/

/*!
    \fn bool QXmlReader::hasProperty(const QString& name) const

    Returns true if the reader has the property \a name; otherwise
    returns false.

    \sa property(), setProperty()
*/

/*!
    \fn void QXmlReader::setEntityResolver(QXmlEntityResolver* handler)

    Sets the entity resolver to \a handler.

    \sa entityResolver()
*/

/*!
    \fn QXmlEntityResolver* QXmlReader::entityResolver() const

    Returns the entity resolver or 0 if none was set.

    \sa setEntityResolver()
*/

/*!
    \fn void QXmlReader::setDTDHandler(QXmlDTDHandler* handler)

    Sets the DTD handler to \a handler.

    \sa DTDHandler()
*/

/*!
    \fn QXmlDTDHandler* QXmlReader::DTDHandler() const

    Returns the DTD handler or 0 if none was set.

    \sa setDTDHandler()
*/

/*!
    \fn void QXmlReader::setContentHandler(QXmlContentHandler* handler)

    Sets the content handler to \a handler.

    \sa contentHandler()
*/

/*!
    \fn QXmlContentHandler* QXmlReader::contentHandler() const

    Returns the content handler or 0 if none was set.

    \sa setContentHandler()
*/

/*!
    \fn void QXmlReader::setErrorHandler(QXmlErrorHandler* handler)

    Sets the error handler to \a handler. Clears the error handler if
    \a handler is 0.

    \sa errorHandler()
*/

/*!
    \fn QXmlErrorHandler* QXmlReader::errorHandler() const

    Returns the error handler or 0 if none is set.

    \sa setErrorHandler()
*/

/*!
    \fn void QXmlReader::setLexicalHandler(QXmlLexicalHandler* handler)

    Sets the lexical handler to \a handler.

    \sa lexicalHandler()
*/

/*!
    \fn QXmlLexicalHandler* QXmlReader::lexicalHandler() const

    Returns the lexical handler or 0 if none was set.

    \sa setLexicalHandler()
*/

/*!
    \fn void QXmlReader::setDeclHandler(QXmlDeclHandler* handler)

    Sets the declaration handler to \a handler.

    \sa declHandler()
*/

/*!
    \fn QXmlDeclHandler* QXmlReader::declHandler() const

    Returns the declaration handler or 0 if none was set.

    \sa setDeclHandler()
*/

/*!
  \fn bool QXmlReader::parse(const QXmlInputSource &input)

  \obsolete

  Parses the given \a input.
*/

/*!
    \fn bool QXmlReader::parse(const QXmlInputSource *input)

    Reads an XML document from \a input and parses it. Returns true if
    the parsing was successful; otherwise returns false.
*/


/*!
    \class QXmlSimpleReader
    \nonreentrant
    \brief The QXmlSimpleReader class provides an implementation of a
    simple XML parser.

    \inmodule QtXml
    \ingroup xml-tools


    This XML reader is suitable for a wide range of applications. It
    is able to parse well-formed XML and can report the namespaces of
    elements to a content handler; however, it does not parse any
    external entities. For historical reasons, Attribute Value
    Normalization and End-of-Line Handling as described in the XML 1.0
    specification is not performed.

    The easiest pattern of use for this class is to create a reader
    instance, define an input source, specify the handlers to be used
    by the reader, and parse the data.

    For example, we could use a QFile to supply the input. Here, we
    create a reader, and define an input source to be used by the
    reader:

    \snippet doc/src/snippets/xml/simpleparse/main.cpp 0

    A handler lets us perform actions when the reader encounters
    certain types of content, or if errors in the input are found. The
    reader must be told which handler to use for each type of
    event. For many common applications, we can create a custom
    handler by subclassing QXmlDefaultHandler, and use this to handle
    both error and content events:

    \snippet doc/src/snippets/xml/simpleparse/main.cpp 1

    If you don't set at least the content and error handlers, the
    parser will fall back on its default behavior---and will do
    nothing.

    The most convenient way to handle the input is to read it in a
    single pass using the parse() function with an argument that
    specifies the input source:

    \snippet doc/src/snippets/xml/simpleparse/main.cpp 2

    If you can't parse the entire input in one go (for example, it is
    huge, or is being delivered over a network connection), data can
    be fed to the parser in pieces. This is achieved by telling
    parse() to work incrementally, and making subsequent calls to the
    parseContinue() function, until all the data has been processed.

    A common way to perform incremental parsing is to connect the \c
    readyRead() signal of a \l{QNetworkReply} {network reply} a slot,
    and handle the incoming data there. See QNetworkAccessManager.
    
    Aspects of the parsing behavior can be adapted using setFeature()
    and setProperty().
    
    \snippet doc/src/snippets/code/src_xml_sax_qxml.cpp 0

    QXmlSimpleReader is not reentrant. If you want to use the class
    in threaded code, lock the code using QXmlSimpleReader with a
    locking mechanism, such as a QMutex.
*/

static inline bool is_S(QChar ch)
{
    ushort uc = ch.unicode();
    return (uc == ' ' || uc == '\t' || uc == '\n' || uc == '\r');
}

enum NameChar { NameBeginning, NameNotBeginning, NotName };

static const char Begi = (char)NameBeginning;
static const char NtBg = (char)NameNotBeginning;
static const char NotN = (char)NotName;

static const char nameCharTable[128] =
{
// 0x00
    NotN, NotN, NotN, NotN, NotN, NotN, NotN, NotN,
    NotN, NotN, NotN, NotN, NotN, NotN, NotN, NotN,
// 0x10
    NotN, NotN, NotN, NotN, NotN, NotN, NotN, NotN,
    NotN, NotN, NotN, NotN, NotN, NotN, NotN, NotN,
// 0x20 (0x2D is '-', 0x2E is '.')
    NotN, NotN, NotN, NotN, NotN, NotN, NotN, NotN,
    NotN, NotN, NotN, NotN, NotN, NtBg, NtBg, NotN,
// 0x30 (0x30..0x39 are '0'..'9', 0x3A is ':')
    NtBg, NtBg, NtBg, NtBg, NtBg, NtBg, NtBg, NtBg,
    NtBg, NtBg, Begi, NotN, NotN, NotN, NotN, NotN,
// 0x40 (0x41..0x5A are 'A'..'Z')
    NotN, Begi, Begi, Begi, Begi, Begi, Begi, Begi,
    Begi, Begi, Begi, Begi, Begi, Begi, Begi, Begi,
// 0x50 (0x5F is '_')
    Begi, Begi, Begi, Begi, Begi, Begi, Begi, Begi,
    Begi, Begi, Begi, NotN, NotN, NotN, NotN, Begi,
// 0x60 (0x61..0x7A are 'a'..'z')
    NotN, Begi, Begi, Begi, Begi, Begi, Begi, Begi,
    Begi, Begi, Begi, Begi, Begi, Begi, Begi, Begi,
// 0x70
    Begi, Begi, Begi, Begi, Begi, Begi, Begi, Begi,
    Begi, Begi, Begi, NotN, NotN, NotN, NotN, NotN
};

static inline NameChar fastDetermineNameChar(QChar ch)
{
    ushort uc = ch.unicode();
    if (!(uc & ~0x7f)) // uc < 128
        return (NameChar)nameCharTable[uc];

    QChar::Category cat = ch.category();
    // ### some these categories might be slightly wrong
    if ((cat >= QChar::Letter_Uppercase && cat <= QChar::Letter_Other)
        || cat == QChar::Number_Letter)
        return NameBeginning;
    if ((cat >= QChar::Number_DecimalDigit && cat <= QChar::Number_Other)
                || (cat >= QChar::Mark_NonSpacing && cat <= QChar::Mark_Enclosing))
        return NameNotBeginning;
    return NotName;
}

static NameChar determineNameChar(QChar ch)
{
    return fastDetermineNameChar(ch);
}

/*!
    Constructs a simple XML reader.

*/
QXmlSimpleReader::QXmlSimpleReader()
    : d_ptr(new QXmlSimpleReaderPrivate(this))
{
}

/*!
    Destroys the simple XML reader.
*/
QXmlSimpleReader::~QXmlSimpleReader()
{
}

/*!
    \reimp
*/
bool QXmlSimpleReader::feature(const QString& name, bool *ok) const
{
    const QXmlSimpleReaderPrivate *d = d_func();

    // Qt5 ###: Change these strings to qt.nokia.com
    if (ok != 0)
        *ok = true;
    if (name == QLatin1String("http://xml.org/sax/features/namespaces")) {
        return d->useNamespaces;
    } else if (name == QLatin1String("http://xml.org/sax/features/namespace-prefixes")) {
        return d->useNamespacePrefixes;
    } else if (name == QLatin1String("http://trolltech.com/xml/features/report-whitespace-only-CharData")) { // Shouldn't change in Qt 4
        return d->reportWhitespaceCharData;
    } else if (name == QLatin1String("http://trolltech.com/xml/features/report-start-end-entity")) { // Shouldn't change in Qt 4
        return d->reportEntities;
    } else {
        qWarning("Unknown feature %s", name.toLatin1().data());
        if (ok != 0)
            *ok = false;
    }
    return false;
}

/*!
    Turns on the feature \a name if \a enable is true; otherwise turns it off.

    The \a name parameter must be one of the following strings:
    \table
    \header \i Feature \i Default \i Notes
    \row \i \e http://xml.org/sax/features/namespaces
         \i true
         \i If enabled, namespaces are reported to the content handler.
    \row \i \e http://xml.org/sax/features/namespace-prefixes
         \i false
         \i If enabled, the original prefixed names
            and attributes used for namespace declarations are
            reported.
    \row \i \e http://trolltech.com/xml/features/report-whitespace-only-CharData
         \i true
         \i If enabled, CharData that consist of
            only whitespace characters are reported
            using QXmlContentHandler::characters(). If disabled, whitespace is silently
            discarded.
    \row \i \e http://trolltech.com/xml/features/report-start-end-entity
         \i false
         \i If enabled, the parser reports
            QXmlContentHandler::startEntity() and
            QXmlContentHandler::endEntity() events, so character data
            might be reported in chunks.
            If disabled, the parser does not report these events, but
            silently substitutes the entities, and reports the character
            data in one chunk.
    \endtable

    \sa feature(), hasFeature(), {SAX2 Features}
*/
void QXmlSimpleReader::setFeature(const QString& name, bool enable)
{
    Q_D(QXmlSimpleReader);
    // Qt5 ###: Change these strings to qt.nokia.com
    if (name == QLatin1String("http://xml.org/sax/features/namespaces")) {
        d->useNamespaces = enable;
    } else if (name == QLatin1String("http://xml.org/sax/features/namespace-prefixes")) {
        d->useNamespacePrefixes = enable;
    } else if (name == QLatin1String("http://trolltech.com/xml/features/report-whitespace-only-CharData")) { // Shouldn't change in Qt 4
        d->reportWhitespaceCharData = enable;
    } else if (name == QLatin1String("http://trolltech.com/xml/features/report-start-end-entity")) { // Shouldn't change in Qt 4
        d->reportEntities = enable;
    } else {
        qWarning("Unknown feature %s", name.toLatin1().data());
    }
}

/*! \reimp
*/
bool QXmlSimpleReader::hasFeature(const QString& name) const
{
    // Qt5 ###: Change these strings to qt.nokia.com
    if (name == QLatin1String("http://xml.org/sax/features/namespaces")
        || name == QLatin1String("http://xml.org/sax/features/namespace-prefixes")
        || name == QLatin1String("http://trolltech.com/xml/features/report-whitespace-only-CharData") // Shouldn't change in Qt 4
        || name == QLatin1String("http://trolltech.com/xml/features/report-start-end-entity")) { // Shouldn't change in Qt 4
        return true;
    } else {
        return false;
    }
}

/*! \reimp
*/
void* QXmlSimpleReader::property(const QString&, bool *ok) const
{
    if (ok != 0)
        *ok = false;
    return 0;
}

/*! \reimp
*/
void QXmlSimpleReader::setProperty(const QString&, void*)
{
}

/*!
    \reimp
*/
bool QXmlSimpleReader::hasProperty(const QString&) const
{
    return false;
}

/*!
    \reimp
*/
void QXmlSimpleReader::setEntityResolver(QXmlEntityResolver* handler)
{
    Q_D(QXmlSimpleReader);
    d->entityRes = handler;
}

/*!
    \reimp
*/
QXmlEntityResolver* QXmlSimpleReader::entityResolver() const
{
    const QXmlSimpleReaderPrivate *d = d_func();
    return d->entityRes;
}

/*!
    \reimp
*/
void QXmlSimpleReader::setDTDHandler(QXmlDTDHandler* handler)
{
    Q_D(QXmlSimpleReader);
    d->dtdHnd = handler;
}

/*!
    \reimp
*/
QXmlDTDHandler* QXmlSimpleReader::DTDHandler() const
{
    const QXmlSimpleReaderPrivate *d = d_func();
    return d->dtdHnd;
}

/*!
    \reimp
*/
void QXmlSimpleReader::setContentHandler(QXmlContentHandler* handler)
{
    Q_D(QXmlSimpleReader);
    d->contentHnd = handler;
}

/*!
    \reimp
*/
QXmlContentHandler* QXmlSimpleReader::contentHandler() const
{
    const QXmlSimpleReaderPrivate *d = d_func();
    return d->contentHnd;
}

/*!
    \reimp
*/
void QXmlSimpleReader::setErrorHandler(QXmlErrorHandler* handler)
{
    Q_D(QXmlSimpleReader);
    d->errorHnd = handler;
}

/*!
    \reimp
*/
QXmlErrorHandler* QXmlSimpleReader::errorHandler() const
{
    const QXmlSimpleReaderPrivate *d = d_func();
    return d->errorHnd;
}

/*!
    \reimp
*/
void QXmlSimpleReader::setLexicalHandler(QXmlLexicalHandler* handler)
{
    Q_D(QXmlSimpleReader);
    d->lexicalHnd = handler;
}

/*!
    \reimp
*/
QXmlLexicalHandler* QXmlSimpleReader::lexicalHandler() const
{
    const QXmlSimpleReaderPrivate *d = d_func();
    return d->lexicalHnd;
}

/*!
    \reimp
*/
void QXmlSimpleReader::setDeclHandler(QXmlDeclHandler* handler)
{
    Q_D(QXmlSimpleReader);
    d->declHnd = handler;
}

/*!
    \reimp
*/
QXmlDeclHandler* QXmlSimpleReader::declHandler() const
{
    const QXmlSimpleReaderPrivate *d = d_func();
    return d->declHnd;
}



/*!
    \reimp
*/
bool QXmlSimpleReader::parse(const QXmlInputSource& input)
{
    return parse(&input, false);
}

/*!
    Reads an XML document from \a input and parses it in one pass (non-incrementally).
    Returns true if the parsing was successful; otherwise returns false.
*/
bool QXmlSimpleReader::parse(const QXmlInputSource* input)
{
    return parse(input, false);
}

/*!
    Reads an XML document from \a input and parses it. Returns true
    if the parsing is completed successfully; otherwise returns false,
    indicating that an error occurred.

    If \a incremental is false, this function will return false if the XML
    file is not read completely. The parsing cannot be continued in this
    case.

    If \a incremental is true, the parser does not return false if
    it reaches the end of the \a input before reaching the end
    of the XML file. Instead, it stores the state of the parser so that
    parsing can be continued later when more data is available.
    In such a case, you can use the function parseContinue() to
    continue with parsing. This class stores a pointer to the input
    source \a input and the parseContinue() function tries to read from
    that input source. Therefore, you should not delete the input
    source \a input until you no longer need to call parseContinue().

    If this function is called with \a incremental set to true
    while an incremental parse is in progress, a new parsing
    session will be started, and the previous session will be lost.

    \sa parseContinue(), QTcpSocket
*/
bool QXmlSimpleReader::parse(const QXmlInputSource *input, bool incremental)
{
    Q_D(QXmlSimpleReader);

    if (incremental) {
        d->initIncrementalParsing();
    } else {
        delete d->parseStack;
        d->parseStack = 0;
    }
    d->init(input);

    // call the handler
    if (d->contentHnd) {
        d->contentHnd->setDocumentLocator(d->locator.data());
        if (!d->contentHnd->startDocument()) {
            d->reportParseError(d->contentHnd->errorString());
            d->tags.clear();
            return false;
        }
    }
    qt_xml_skipped_entity_in_content = false;
    return d->parseBeginOrContinue(0, incremental);
}

/*!
    Continues incremental parsing, taking input from the
    QXmlInputSource that was specified with the most recent
    call to parse(). To use this function, you \e must have called
    parse() with the incremental argument set to true.

    Returns false if a parsing error occurs; otherwise returns true,
    even if the end of the XML file has not been reached. You can
    continue parsing at a later stage by calling this function again
    when there is more data available to parse.

    Calling this function when there is no data available in the input
    source indicates to the reader that the end of the XML file has
    been reached. If the input supplied up to this point was
    not well-formed then a parsing error occurs, and false is returned.
    If the input supplied was well-formed, true is returned.
    It is important to end the input in this way because it allows you
    to reuse the reader to parse other XML files.

    Calling this function after the end of file has been reached, but
    without available data will cause false to be returned whether the
    previous input was well-formed or not.

    \sa parse(), QXmlInputSource::data(), QXmlInputSource::next()
*/
bool QXmlSimpleReader::parseContinue()
{
    Q_D(QXmlSimpleReader);
    if (d->parseStack == 0 || d->parseStack->isEmpty())
        return false;
    d->initData();
    int state = d->parseStack->pop().state;
    return d->parseBeginOrContinue(state, true);
}

/*
  Common part of parse() and parseContinue()
*/
bool QXmlSimpleReaderPrivate::parseBeginOrContinue(int state, bool incremental)
{
    bool atEndOrig = atEnd();

    if (state==0) {
        if (!parseProlog()) {
            if (incremental && error.isNull()) {
                pushParseState(0, 0);
                return true;
            } else {
                tags.clear();
                return false;
            }
        }
        state = 1;
    }
    if (state==1) {
        if (!parseElement()) {
            if (incremental && error.isNull()) {
                pushParseState(0, 1);
                return true;
            } else {
                tags.clear();
                return false;
            }
        }
        state = 2;
    }
    // parse Misc*
    while (!atEnd()) {
        if (!parseMisc()) {
            if (incremental && error.isNull()) {
                pushParseState(0, 2);
                return true;
            } else {
                tags.clear();
                return false;
            }
        }
    }
    if (!atEndOrig && incremental) {
        // we parsed something at all, so be prepared to come back later
        pushParseState(0, 2);
        return true;
    }
    // is stack empty?
    if (!tags.isEmpty() && !error.isNull()) {
        reportParseError(QLatin1String(XMLERR_UNEXPECTEDEOF));
        tags.clear();
        return false;
    }
    // call the handler
    if (contentHnd) {
        delete parseStack;
        parseStack = 0;
        if (!contentHnd->endDocument()) {
            reportParseError(contentHnd->errorString());
            return false;
        }
    }
    return true;
}

//
// The following private parse functions have another semantics for the return
// value: They return true iff parsing has finished successfully (i.e. the end
// of the XML file must be reached!). If one of these functions return false,
// there is only an error when d->error.isNULL() is also false.
//

/*
  For the incremental parsing, it is very important that the parse...()
  functions have a certain structure. Since it might be hard to understand how
  they work, here is a description of the layout of these functions:

    bool QXmlSimpleReader::parse...()
    {
(1)        const signed char Init             = 0;
        ...

(2)        const signed char Inp...           = 0;
        ...

(3)        static const signed char table[3][2] = {
        ...
        };
        signed char state;
        signed char input;

(4)        if (d->parseStack == 0 || d->parseStack->isEmpty()) {
(4a)        ...
        } else {
(4b)        ...
        }

        for (; ;) {
(5)            switch (state) {
            ...
            }

(6)
(6a)            if (atEnd()) {
                unexpectedEof(&QXmlSimpleReader::parseNmtoken, state);
                return false;
            }
(6b)            if (determineNameChar(c) != NotName) {
            ...
            }
(7)            state = table[state][input];

(8)            switch (state) {
            ...
            }
        }
    }

  Explanation:
  ad 1: constants for the states (used in the transition table)
  ad 2: constants for the input (used in the transition table)
  ad 3: the transition table for the state machine
  ad 4: test if we are in a parseContinue() step
        a) if no, do inititalizations
        b) if yes, restore the state and call parse functions recursively
  ad 5: Do some actions according to the state; from the logical execution
        order, this code belongs after 8 (see there for an explanation)
  ad 6: Check the character that is at the actual "cursor" position:
        a) If we reached the EOF, report either error or push the state (in the
           case of incremental parsing).
        b) Otherwise, set the input character constant for the transition
           table.
  ad 7: Get the new state according to the input that was read.
  ad 8: Do some actions according to the state. The last line in every case
        statement reads new data (i.e. it move the cursor). This can also be
        done by calling another parse...() function. If you need processing for
        this state after that, you have to put it into the switch statement 5.
        This ensures that you have a well defined re-entry point, when you ran
        out of data.
*/

/*
  Parses the prolog [22].
*/

bool QXmlSimpleReaderPrivate::parseProlog()
{
    const signed char Init             = 0;
    const signed char EatWS            = 1; // eat white spaces
    const signed char Lt               = 2; // '<' read
    const signed char Em               = 3; // '!' read
    const signed char DocType          = 4; // read doctype
    const signed char Comment          = 5; // read comment
    const signed char CommentR         = 6; // same as Comment, but already reported
    const signed char PInstr           = 7; // read PI
    const signed char PInstrR          = 8; // same as PInstr, but already reported
    const signed char Done             = 9;

    const signed char InpWs            = 0;
    const signed char InpLt            = 1; // <
    const signed char InpQm            = 2; // ?
    const signed char InpEm            = 3; // !
    const signed char InpD             = 4; // D
    const signed char InpDash          = 5; // -
    const signed char InpUnknown       = 6;

    static const signed char table[9][7] = {
     /*  InpWs   InpLt  InpQm  InpEm  InpD      InpDash  InpUnknown */
        { EatWS,  Lt,    -1,    -1,    -1,       -1,       -1      }, // Init
        { -1,     Lt,    -1,    -1,    -1,       -1,       -1      }, // EatWS
        { -1,     -1,    PInstr,Em,    Done,     -1,       Done    }, // Lt
        { -1,     -1,    -1,    -1,    DocType,  Comment,  -1      }, // Em
        { EatWS,  Lt,    -1,    -1,    -1,       -1,       -1      }, // DocType
        { EatWS,  Lt,    -1,    -1,    -1,       -1,       -1      }, // Comment
        { EatWS,  Lt,    -1,    -1,    -1,       -1,       -1      }, // CommentR
        { EatWS,  Lt,    -1,    -1,    -1,       -1,       -1      }, // PInstr
        { EatWS,  Lt,    -1,    -1,    -1,       -1,       -1      }  // PInstrR
    };
    signed char state;
    signed char input;

    if (parseStack == 0 || parseStack->isEmpty()) {
        xmldecl_possible = true;
        doctype_read = false;
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseProlog (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseProlog, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case DocType:
                if (doctype_read) {
                    reportParseError(QLatin1String(XMLERR_MORETHANONEDOCTYPE));
                    return false;
                } else {
                    doctype_read = false;
                }
                break;
            case Comment:
                if (lexicalHnd) {
                    if (!lexicalHnd->comment(string())) {
                        reportParseError(lexicalHnd->errorString());
                        return false;
                    }
                }
                state = CommentR;
                break;
            case PInstr:
                // call the handler
                if (contentHnd) {
                    if (xmldecl_possible && !xmlVersion.isEmpty()) {
                        QString value(QLatin1String("version='"));
                        value += xmlVersion;
                        value += QLatin1Char('\'');
                        if (!encoding.isEmpty()) {
                            value += QLatin1String(" encoding='");
                            value += encoding;
                            value += QLatin1Char('\'');
                        }
                        if (standalone == QXmlSimpleReaderPrivate::Yes) {
                            value += QLatin1String(" standalone='yes'");
                        } else if (standalone == QXmlSimpleReaderPrivate::No) {
                            value += QLatin1String(" standalone='no'");
                        }
                        if (!contentHnd->processingInstruction(QLatin1String("xml"), value)) {
                            reportParseError(contentHnd->errorString());
                            return false;
                        }
                    } else {
                        if (!contentHnd->processingInstruction(name(), string())) {
                            reportParseError(contentHnd->errorString());
                            return false;
                        }
                    }
                }
                // XML declaration only on first position possible
                xmldecl_possible = false;
                state = PInstrR;
                break;
            case Done:
                return true;
            case -1:
                reportParseError(QLatin1String(XMLERR_ERRORPARSINGELEMENT));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseProlog, state);
            return false;
        }
        if (is_S(c)) {
            input = InpWs;
        } else if (c == QLatin1Char('<')) {
            input = InpLt;
        } else if (c == QLatin1Char('?')) {
            input = InpQm;
        } else if (c == QLatin1Char('!')) {
            input = InpEm;
        } else if (c == QLatin1Char('D')) {
            input = InpD;
        } else if (c == QLatin1Char('-')) {
            input = InpDash;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case EatWS:
                // XML declaration only on first position possible
                xmldecl_possible = false;
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseProlog, state);
                    return false;
                }
                break;
            case Lt:
                next();
                break;
            case Em:
                // XML declaration only on first position possible
                xmldecl_possible = false;
                next();
                break;
            case DocType:
                if (!parseDoctype()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseProlog, state);
                    return false;
                }
                break;
            case Comment:
            case CommentR:
                if (!parseComment()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseProlog, state);
                    return false;
                }
                break;
            case PInstr:
            case PInstrR:
                parsePI_xmldecl = xmldecl_possible;
                if (!parsePI()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseProlog, state);
                    return false;
                }
                break;
        }
    }
    return false;
}

/*
  Parse an element [39].

  Precondition: the opening '<' is already read.
*/
bool QXmlSimpleReaderPrivate::parseElement()
{
    const int Init             =  0;
    const int ReadName         =  1;
    const int Ws1              =  2;
    const int STagEnd          =  3;
    const int STagEnd2         =  4;
    const int ETagBegin        =  5;
    const int ETagBegin2       =  6;
    const int Ws2              =  7;
    const int EmptyTag         =  8;
    const int Attrib           =  9;
    const int AttribPro        = 10; // like Attrib, but processAttribute was already called
    const int Ws3              = 11;
    const int Done             = 12;

    const int InpWs            = 0; // whitespace
    const int InpNameBe        = 1; // NameBeginning
    const int InpGt            = 2; // >
    const int InpSlash         = 3; // /
    const int InpUnknown       = 4;

    static const int table[12][5] = {
     /*  InpWs      InpNameBe    InpGt        InpSlash     InpUnknown */
        { -1,        ReadName,    -1,          -1,          -1        }, // Init
        { Ws1,       Attrib,      STagEnd,     EmptyTag,    -1        }, // ReadName
        { -1,        Attrib,      STagEnd,     EmptyTag,    -1        }, // Ws1
        { STagEnd2,  STagEnd2,    STagEnd2,    STagEnd2,    STagEnd2  }, // STagEnd
        { -1,        -1,          -1,          ETagBegin,   -1        }, // STagEnd2
        { -1,        ETagBegin2,  -1,          -1,          -1        }, // ETagBegin
        { Ws2,       -1,          Done,        -1,          -1        }, // ETagBegin2
        { -1,        -1,          Done,        -1,          -1        }, // Ws2
        { -1,        -1,          Done,        -1,          -1        }, // EmptyTag
        { Ws3,       Attrib,      STagEnd,     EmptyTag,    -1        }, // Attrib
        { Ws3,       Attrib,      STagEnd,     EmptyTag,    -1        }, // AttribPro
        { -1,        Attrib,      STagEnd,     EmptyTag,    -1        }  // Ws3
    };
    int state;
    int input;

    if (parseStack == 0 || parseStack->isEmpty()) {
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseElement (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseElement, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case ReadName:
                // store it on the stack
                tags.push(name());
                // empty the attributes
                attList.clear();
                if (useNamespaces)
                    namespaceSupport.pushContext();
                break;
            case ETagBegin2:
                if (!processElementETagBegin2())
                    return false;
                break;
            case Attrib:
                if (!processElementAttribute())
                    return false;
                state = AttribPro;
                break;
            case Done:
                return true;
            case -1:
                reportParseError(QLatin1String(XMLERR_ERRORPARSINGELEMENT));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseElement, state);
            return false;
        }
        if (fastDetermineNameChar(c) == NameBeginning) {
            input = InpNameBe;
        } else if (c == QLatin1Char('>')) {
            input = InpGt;
        } else if (is_S(c)) {
            input = InpWs;
        } else if (c == QLatin1Char('/')) {
            input = InpSlash;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case ReadName:
                parseName_useRef = false;
                if (!parseName()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElement, state);
                    return false;
                }
                break;
            case Ws1:
            case Ws2:
            case Ws3:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElement, state);
                    return false;
                }
                break;
            case STagEnd:
                // call the handler
                if (contentHnd) {
                    const QString &tagsTop = tags.top();
                    if (useNamespaces) {
                        QString uri, lname;
                        namespaceSupport.processName(tagsTop, false, uri, lname);
                        if (!contentHnd->startElement(uri, lname, tagsTop, attList)) {
                            reportParseError(contentHnd->errorString());
                            return false;
                        }
                    } else {
                        if (!contentHnd->startElement(QString(), QString(), tagsTop, attList)) {
                            reportParseError(contentHnd->errorString());
                            return false;
                        }
                    }
                }
                next();
                break;
            case STagEnd2:
                if (!parseContent()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElement, state);
                    return false;
                }
                break;
            case ETagBegin:
                next();
                break;
            case ETagBegin2:
                // get the name of the tag
                parseName_useRef = false;
                if (!parseName()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElement, state);
                    return false;
                }
                break;
            case EmptyTag:
                if  (tags.isEmpty()) {
                    reportParseError(QLatin1String(XMLERR_TAGMISMATCH));
                    return false;
                }
                if (!processElementEmptyTag())
                    return false;
                next();
                break;
            case Attrib:
            case AttribPro:
                // get name and value of attribute
                if (!parseAttribute()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElement, state);
                    return false;
                }
                break;
            case Done:
                next();
                break;
        }
    }
    return false;
}

/*
  Helper to break down the size of the code in the case statement.
  Return false on error, otherwise true.
*/
bool QXmlSimpleReaderPrivate::processElementEmptyTag()
{
    QString uri, lname;
    // pop the stack and call the handler
    if (contentHnd) {
        if (useNamespaces) {
            // report startElement first...
            namespaceSupport.processName(tags.top(), false, uri, lname);
            if (!contentHnd->startElement(uri, lname, tags.top(), attList)) {
                reportParseError(contentHnd->errorString());
                return false;
            }
            // ... followed by endElement...
            if (!contentHnd->endElement(uri, lname, tags.pop())) {
                reportParseError(contentHnd->errorString());
                return false;
            }
            // ... followed by endPrefixMapping
            QStringList prefixesBefore, prefixesAfter;
            if (contentHnd) {
                prefixesBefore = namespaceSupport.prefixes();
            }
            namespaceSupport.popContext();
            // call the handler for prefix mapping
            prefixesAfter = namespaceSupport.prefixes();
            for (QStringList::Iterator it = prefixesBefore.begin(); it != prefixesBefore.end(); ++it) {
                if (!prefixesAfter.contains(*it)) {
                    if (!contentHnd->endPrefixMapping(*it)) {
                        reportParseError(contentHnd->errorString());
                        return false;
                    }
                }
            }
        } else {
            // report startElement first...
            if (!contentHnd->startElement(QString(), QString(), tags.top(), attList)) {
                reportParseError(contentHnd->errorString());
                return false;
            }
            // ... followed by endElement
            if (!contentHnd->endElement(QString(), QString(), tags.pop())) {
                reportParseError(contentHnd->errorString());
                return false;
            }
        }
    } else {
        tags.pop_back();
        namespaceSupport.popContext();
    }
    return true;
}
/*
  Helper to break down the size of the code in the case statement.
  Return false on error, otherwise true.
*/
bool QXmlSimpleReaderPrivate::processElementETagBegin2()
{
    const QString &name = QXmlSimpleReaderPrivate::name();

    // pop the stack and compare it with the name
    if (tags.pop() != name) {
        reportParseError(QLatin1String(XMLERR_TAGMISMATCH));
        return false;
    }
    // call the handler
    if (contentHnd) {
        QString uri, lname;

        if (useNamespaces)
            namespaceSupport.processName(name, false, uri, lname);
        if (!contentHnd->endElement(uri, lname, name)) {
            reportParseError(contentHnd->errorString());
            return false;
        }
    }
    if (useNamespaces) {
        NamespaceMap prefixesBefore, prefixesAfter;
        if (contentHnd)
            prefixesBefore = namespaceSupport.d->ns;

        namespaceSupport.popContext();
        // call the handler for prefix mapping
        if (contentHnd) {
            prefixesAfter = namespaceSupport.d->ns;
            if (prefixesBefore.size() != prefixesAfter.size()) {
                for (NamespaceMap::const_iterator it = prefixesBefore.constBegin(); it != prefixesBefore.constEnd(); ++it) {
                    if (!it.key().isEmpty() && !prefixesAfter.contains(it.key())) {
                        if (!contentHnd->endPrefixMapping(it.key())) {
                            reportParseError(contentHnd->errorString());
                            return false;
                        }
                    }
                }
            }
        }
    }
    return true;
}
/*
  Helper to break down the size of the code in the case statement.
  Return false on error, otherwise true.
*/
bool QXmlSimpleReaderPrivate::processElementAttribute()
{
    QString uri, lname, prefix;
    const QString &name = QXmlSimpleReaderPrivate::name();
    const QString &string = QXmlSimpleReaderPrivate::string();

    // add the attribute to the list
    if (useNamespaces) {
        // is it a namespace declaration?
        namespaceSupport.splitName(name, prefix, lname);
        if (prefix == QLatin1String("xmlns")) {
            // namespace declaration
            namespaceSupport.setPrefix(lname, string);
            if (useNamespacePrefixes) {
                // according to http://www.w3.org/2000/xmlns/, the "prefix"
                // xmlns maps to the namespace name
                // http://www.w3.org/2000/xmlns/
                attList.append(name, QLatin1String("http://www.w3.org/2000/xmlns/"), lname, string);
            }
            // call the handler for prefix mapping
            if (contentHnd) {
                if (!contentHnd->startPrefixMapping(lname, string)) {
                    reportParseError(contentHnd->errorString());
                    return false;
                }
            }
        } else {
            // no namespace delcaration
            namespaceSupport.processName(name, true, uri, lname);
            attList.append(name, uri, lname, string);
        }
    } else {
        // no namespace support
        attList.append(name, uri, lname, string);
    }
    return true;
}

/*
  Parse a content [43].

  A content is only used between tags. If a end tag is found the < is already
  read and the head stand on the '/' of the end tag '</name>'.
*/
bool QXmlSimpleReaderPrivate::parseContent()
{
    const signed char Init             =  0;
    const signed char ChD              =  1; // CharData
    const signed char ChD1             =  2; // CharData help state
    const signed char ChD2             =  3; // CharData help state
    const signed char Ref              =  4; // Reference
    const signed char Lt               =  5; // '<' read
    const signed char PInstr           =  6; // PI
    const signed char PInstrR          =  7; // same as PInstr, but already reported
    const signed char Elem             =  8; // Element
    const signed char Em               =  9; // '!' read
    const signed char Com              = 10; // Comment
    const signed char ComR             = 11; // same as Com, but already reported
    const signed char CDS              = 12; // CDSect
    const signed char CDS1             = 13; // read a CDSect
    const signed char CDS2             = 14; // read a CDSect (help state)
    const signed char CDS3             = 15; // read a CDSect (help state)
    const signed char Done             = 16; // finished reading content

    const signed char InpLt            = 0; // <
    const signed char InpGt            = 1; // >
    const signed char InpSlash         = 2; // /
    const signed char InpQMark         = 3; // ?
    const signed char InpEMark         = 4; // !
    const signed char InpAmp           = 5; // &
    const signed char InpDash          = 6; // -
    const signed char InpOpenB         = 7; // [
    const signed char InpCloseB        = 8; //]
    const signed char InpUnknown       = 9;

    static const signed char mapCLT2FSMChar[] = {
        InpUnknown, // white space
        InpUnknown, // %
        InpAmp,     // &
        InpGt,      // >
        InpLt,      // <
        InpSlash,   // /
        InpQMark,   // ?
        InpEMark,   // !
        InpDash,    // -
        InpCloseB,  //]
        InpOpenB,   // [
        InpUnknown, // =
        InpUnknown, // "
        InpUnknown, // '
        InpUnknown  // unknown
    };

    static const signed char table[16][10] = {
     /*  InpLt  InpGt  InpSlash  InpQMark  InpEMark  InpAmp  InpDash  InpOpenB  InpCloseB  InpUnknown */
        { Lt,    ChD,   ChD,      ChD,      ChD,      Ref,    ChD,     ChD,      ChD1,      ChD  }, // Init
        { Lt,    ChD,   ChD,      ChD,      ChD,      Ref,    ChD,     ChD,      ChD1,      ChD  }, // ChD
        { Lt,    ChD,   ChD,      ChD,      ChD,      Ref,    ChD,     ChD,      ChD2,      ChD  }, // ChD1
        { Lt,    -1,    ChD,      ChD,      ChD,      Ref,    ChD,     ChD,      ChD2,      ChD  }, // ChD2
        { Lt,    ChD,   ChD,      ChD,      ChD,      Ref,    ChD,     ChD,      ChD,       ChD  }, // Ref (same as Init)
        { -1,    -1,    Done,     PInstr,   Em,       -1,     -1,      -1,       -1,        Elem }, // Lt
        { Lt,    ChD,   ChD,      ChD,      ChD,      Ref,    ChD,     ChD,      ChD,       ChD  }, // PInstr (same as Init)
        { Lt,    ChD,   ChD,      ChD,      ChD,      Ref,    ChD,     ChD,      ChD,       ChD  }, // PInstrR
        { Lt,    ChD,   ChD,      ChD,      ChD,      Ref,    ChD,     ChD,      ChD,       ChD  }, // Elem (same as Init)
        { -1,    -1,    -1,       -1,       -1,       -1,     Com,     CDS,      -1,        -1   }, // Em
        { Lt,    ChD,   ChD,      ChD,      ChD,      Ref,    ChD,     ChD,      ChD,       ChD  }, // Com (same as Init)
        { Lt,    ChD,   ChD,      ChD,      ChD,      Ref,    ChD,     ChD,      ChD,       ChD  }, // ComR
        { CDS1,  CDS1,  CDS1,     CDS1,     CDS1,     CDS1,   CDS1,    CDS1,     CDS2,      CDS1 }, // CDS
        { CDS1,  CDS1,  CDS1,     CDS1,     CDS1,     CDS1,   CDS1,    CDS1,     CDS2,      CDS1 }, // CDS1
        { CDS1,  CDS1,  CDS1,     CDS1,     CDS1,     CDS1,   CDS1,    CDS1,     CDS3,      CDS1 }, // CDS2
        { CDS1,  Init,  CDS1,     CDS1,     CDS1,     CDS1,   CDS1,    CDS1,     CDS3,      CDS1 }  // CDS3
    };
    signed char state;
    signed char input;

    if (parseStack == 0 || parseStack->isEmpty()) {
        contentCharDataRead = false;
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseContent (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseContent, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case Ref:
                if (!contentCharDataRead)
                    contentCharDataRead = parseReference_charDataRead;
                break;
            case PInstr:
                if (contentHnd) {
                    if (!contentHnd->processingInstruction(name(),string())) {
                        reportParseError(contentHnd->errorString());
                        return false;
                    }
                }
                state = PInstrR;
                break;
            case Com:
                if (lexicalHnd) {
                    if (!lexicalHnd->comment(string())) {
                        reportParseError(lexicalHnd->errorString());
                        return false;
                    }
                }
                state = ComR;
                break;
            case CDS:
                stringClear();
                break;
            case CDS2:
                if (!atEnd() && c != QLatin1Char(']'))
                    stringAddC(QLatin1Char(']'));
                break;
            case CDS3:
                // test if this skipping was legal
                if (!atEnd()) {
                    if (c == QLatin1Char('>')) {
                        // the end of the CDSect
                        if (lexicalHnd) {
                            if (!lexicalHnd->startCDATA()) {
                                reportParseError(lexicalHnd->errorString());
                                return false;
                            }
                        }
                        if (contentHnd) {
                            if (!contentHnd->characters(string())) {
                                reportParseError(contentHnd->errorString());
                                return false;
                            }
                        }
                        if (lexicalHnd) {
                            if (!lexicalHnd->endCDATA()) {
                                reportParseError(lexicalHnd->errorString());
                                return false;
                            }
                        }
                    } else if (c == QLatin1Char(']')) {
                        // three or more ']'
                        stringAddC(QLatin1Char(']'));
                    } else {
                        // after ']]' comes another character
                        stringAddC(QLatin1Char(']'));
                        stringAddC(QLatin1Char(']'));
                    }
                }
                break;
            case Done:
                // call the handler for CharData
                if (contentHnd) {
                    if (contentCharDataRead) {
                        if (reportWhitespaceCharData || !string().simplified().isEmpty()) {
                            if (!contentHnd->characters(string())) {
                                reportParseError(contentHnd->errorString());
                                return false;
                            }
                        }
                    }
                }
                // Done
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_ERRORPARSINGCONTENT));
                return false;
        }

        // get input (use lookup-table instead of nested ifs for performance
        // reasons)
        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseContent, state);
            return false;
        }
        if (c.row()) {
            input = InpUnknown;
        } else {
            input = mapCLT2FSMChar[charLookupTable[c.cell()]];
        }
        state = table[state][input];

        switch (state) {
            case Init:
                // skip the ending '>' of a CDATASection
                next();
                break;
            case ChD:
                // on first call: clear string
                if (!contentCharDataRead) {
                    contentCharDataRead = true;
                    stringClear();
                }
                stringAddC();
                if (reportEntities) {
                    if (!reportEndEntities())
                        return false;
                }
                next();
                break;
            case ChD1:
                // on first call: clear string
                if (!contentCharDataRead) {
                    contentCharDataRead = true;
                    stringClear();
                }
                stringAddC();
                if (reportEntities) {
                    if (!reportEndEntities())
                        return false;
                }
                next();
                break;
            case ChD2:
                stringAddC();
                if (reportEntities) {
                    if (!reportEndEntities())
                        return false;
                }
                next();
                break;
            case Ref:
                if (!contentCharDataRead) {
                    // reference may be CharData; so clear string to be safe
                    stringClear();
                    parseReference_context = InContent;
                    if (!parseReference()) {
                        parseFailed(&QXmlSimpleReaderPrivate::parseContent, state);
                        return false;
                    }
                } else {
                    if (reportEntities) {
                        // report character data in chunks
                        if (contentHnd) {
                            if (reportWhitespaceCharData || !string().simplified().isEmpty()) {
                                if (!contentHnd->characters(string())) {
                                    reportParseError(contentHnd->errorString());
                                    return false;
                                }
                            }
                        }
                        stringClear();
                    }
                    parseReference_context = InContent;
                    if (!parseReference()) {
                        parseFailed(&QXmlSimpleReaderPrivate::parseContent, state);
                        return false;
                    }
                }
                break;
            case Lt:
                // call the handler for CharData
                if (contentHnd) {
                    if (contentCharDataRead) {
                        if (reportWhitespaceCharData || !string().simplified().isEmpty()) {
                            if (!contentHnd->characters(string())) {
                                reportParseError(contentHnd->errorString());
                                return false;
                            }
                        }
                    }
                }
                contentCharDataRead = false;
                next();
                break;
            case PInstr:
            case PInstrR:
                parsePI_xmldecl = false;
                if (!parsePI()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseContent, state);
                    return false;
                }
                break;
            case Elem:
                if (!parseElement()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseContent, state);
                    return false;
                }
                break;
            case Em:
                next();
                break;
            case Com:
            case ComR:
                if (!parseComment()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseContent, state);
                    return false;
                }
                break;
            case CDS:
                parseString_s = QLatin1String("[CDATA[");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseContent, state);
                    return false;
                }
                break;
            case CDS1:
                stringAddC();
                next();
                break;
            case CDS2:
                // skip ']'
                next();
                break;
            case CDS3:
                // skip ']'...
                next();
                break;
        }
    }
    return false;
}

bool QXmlSimpleReaderPrivate::reportEndEntities()
{
    int count = (int)xmlRefStack.count();
    while (count != 0 && xmlRefStack.top().isEmpty()) {
        if (contentHnd) {
            if (reportWhitespaceCharData || !string().simplified().isEmpty()) {
                if (!contentHnd->characters(string())) {
                    reportParseError(contentHnd->errorString());
                    return false;
                }
            }
        }
        stringClear();
        if (lexicalHnd) {
            if (!lexicalHnd->endEntity(xmlRefStack.top().name)) {
                reportParseError(lexicalHnd->errorString());
                return false;
            }
        }
        xmlRefStack.pop_back();
        count--;
    }
    return true;
}

/*
  Parse Misc [27].
*/
bool QXmlSimpleReaderPrivate::parseMisc()
{
    const signed char Init             = 0;
    const signed char Lt               = 1; // '<' was read
    const signed char Comment          = 2; // read comment
    const signed char eatWS            = 3; // eat whitespaces
    const signed char PInstr           = 4; // read PI
    const signed char Comment2         = 5; // read comment

    const signed char InpWs            = 0; // S
    const signed char InpLt            = 1; // <
    const signed char InpQm            = 2; // ?
    const signed char InpEm            = 3; // !
    const signed char InpUnknown       = 4;

    static const signed char table[3][5] = {
     /*  InpWs   InpLt  InpQm  InpEm     InpUnknown */
        { eatWS,  Lt,    -1,    -1,       -1        }, // Init
        { -1,     -1,    PInstr,Comment,  -1        }, // Lt
        { -1,     -1,    -1,    -1,       Comment2  }  // Comment
    };
    signed char state;
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseMisc (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseMisc, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case eatWS:
                return true;
            case PInstr:
                if (contentHnd) {
                    if (!contentHnd->processingInstruction(name(),string())) {
                        reportParseError(contentHnd->errorString());
                        return false;
                    }
                }
                return true;
            case Comment2:
                if (lexicalHnd) {
                    if (!lexicalHnd->comment(string())) {
                        reportParseError(lexicalHnd->errorString());
                        return false;
                    }
                }
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_UNEXPECTEDCHARACTER));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseMisc, state);
            return false;
        }
        if (is_S(c)) {
            input = InpWs;
        } else if (c == QLatin1Char('<')) {
            input = InpLt;
        } else if (c == QLatin1Char('?')) {
            input = InpQm;
        } else if (c == QLatin1Char('!')) {
            input = InpEm;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case eatWS:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseMisc, state);
                    return false;
                }
                break;
            case Lt:
                next();
                break;
            case PInstr:
                parsePI_xmldecl = false;
                if (!parsePI()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseMisc, state);
                    return false;
                }
                break;
            case Comment:
                next();
                break;
            case Comment2:
                if (!parseComment()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseMisc, state);
                    return false;
                }
                break;
        }
    }
    return false;
}

/*
  Parse a processing instruction [16].

  If xmldec is true, it tries to parse a PI or a XML declaration [23].

  Precondition: the beginning '<' of the PI is already read and the head stand
  on the '?' of '<?'.

  If this funktion was successful, the head-position is on the first
  character after the PI.
*/
bool QXmlSimpleReaderPrivate::parsePI()
{
    const signed char Init             =  0;
    const signed char QmI              =  1; // ? was read
    const signed char Name             =  2; // read Name
    const signed char XMLDecl          =  3; // read XMLDecl
    const signed char Ws1              =  4; // eat ws after "xml" of XMLDecl
    const signed char PInstr           =  5; // read PI
    const signed char Ws2              =  6; // eat ws after Name of PI
    const signed char Version          =  7; // read versionInfo
    const signed char Ws3              =  8; // eat ws after versionInfo
    const signed char EorSD            =  9; // read EDecl or SDDecl
    const signed char Ws4              = 10; // eat ws after EDecl or SDDecl
    const signed char SD               = 11; // read SDDecl
    const signed char Ws5              = 12; // eat ws after SDDecl
    const signed char ADone            = 13; // almost done
    const signed char Char             = 14; // Char was read
    const signed char Qm               = 15; // Qm was read
    const signed char Done             = 16; // finished reading content

    const signed char InpWs            = 0; // whitespace
    const signed char InpNameBe        = 1; // NameBeginning
    const signed char InpGt            = 2; // >
    const signed char InpQm            = 3; // ?
    const signed char InpUnknown       = 4;

    static const signed char table[16][5] = {
     /*  InpWs,  InpNameBe  InpGt  InpQm   InpUnknown  */
        { -1,     -1,        -1,    QmI,    -1     }, // Init
        { -1,     Name,      -1,    -1,     -1     }, // QmI
        { -1,     -1,        -1,    -1,     -1     }, // Name (this state is left not through input)
        { Ws1,    -1,        -1,    -1,     -1     }, // XMLDecl
        { -1,     Version,   -1,    -1,     -1     }, // Ws1
        { Ws2,    -1,        -1,    Qm,     -1     }, // PInstr
        { Char,   Char,      Char,  Qm,     Char   }, // Ws2
        { Ws3,    -1,        -1,    ADone,  -1     }, // Version
        { -1,     EorSD,     -1,    ADone,  -1     }, // Ws3
        { Ws4,    -1,        -1,    ADone,  -1     }, // EorSD
        { -1,     SD,        -1,    ADone,  -1     }, // Ws4
        { Ws5,    -1,        -1,    ADone,  -1     }, // SD
        { -1,     -1,        -1,    ADone,  -1     }, // Ws5
        { -1,     -1,        Done,  -1,     -1     }, // ADone
        { Char,   Char,      Char,  Qm,     Char   }, // Char
        { Char,   Char,      Done,  Qm,     Char   }, // Qm
    };
    signed char state;
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parsePI (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parsePI, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case Name:
                // test what name was read and determine the next state
                // (not very beautiful, I admit)
                if (name().toLower() == QLatin1String("xml")) {
                    if (parsePI_xmldecl && name() == QLatin1String("xml")) {
                        state = XMLDecl;
                    } else {
                        reportParseError(QLatin1String(XMLERR_INVALIDNAMEFORPI));
                        return false;
                    }
                } else {
                    state = PInstr;
                    stringClear();
                }
                break;
            case Version:
                // get version (syntax like an attribute)
                if (name() != QLatin1String("version")) {
                    reportParseError(QLatin1String(XMLERR_VERSIONEXPECTED));
                    return false;
                }
                xmlVersion = string();
                break;
            case EorSD:
                // get the EDecl or SDDecl (syntax like an attribute)
                if (name() == QLatin1String("standalone")) {
                    if (string()== QLatin1String("yes")) {
                        standalone = QXmlSimpleReaderPrivate::Yes;
                    } else if (string() == QLatin1String("no")) {
                        standalone = QXmlSimpleReaderPrivate::No;
                    } else {
                        reportParseError(QLatin1String(XMLERR_WRONGVALUEFORSDECL));
                        return false;
                    }
                } else if (name() == QLatin1String("encoding")) {
                    encoding = string();
                } else {
                    reportParseError(QLatin1String(XMLERR_EDECLORSDDECLEXPECTED));
                    return false;
                }
                break;
            case SD:
                if (name() != QLatin1String("standalone")) {
                    reportParseError(QLatin1String(XMLERR_SDDECLEXPECTED));
                    return false;
                }
                if (string() == QLatin1String("yes")) {
                    standalone = QXmlSimpleReaderPrivate::Yes;
                } else if (string() == QLatin1String("no")) {
                    standalone = QXmlSimpleReaderPrivate::No;
                } else {
                    reportParseError(QLatin1String(XMLERR_WRONGVALUEFORSDECL));
                    return false;
                }
                break;
            case Qm:
                // test if the skipping was legal
                if (!atEnd() && c != QLatin1Char('>'))
                    stringAddC(QLatin1Char('?'));
                break;
            case Done:
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_UNEXPECTEDCHARACTER));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parsePI, state);
            return false;
        }
        if (is_S(c)) {
            input = InpWs;
        } else if (determineNameChar(c) == NameBeginning) {
            input = InpNameBe;
        } else if (c == QLatin1Char('>')) {
            input = InpGt;
        } else if (c == QLatin1Char('?')) {
            input = InpQm;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case QmI:
                next();
                break;
            case Name:
                parseName_useRef = false;
                if (!parseName()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parsePI, state);
                    return false;
                }
                break;
            case Ws1:
            case Ws2:
            case Ws3:
            case Ws4:
            case Ws5:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parsePI, state);
                    return false;
                }
                break;
            case Version:
                if (!parseAttribute()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parsePI, state);
                    return false;
                }
                break;
            case EorSD:
                if (!parseAttribute()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parsePI, state);
                    return false;
                }
                break;
            case SD:
                // get the SDDecl (syntax like an attribute)
                if (standalone != QXmlSimpleReaderPrivate::Unknown) {
                    // already parsed the standalone declaration
                    reportParseError(QLatin1String(XMLERR_UNEXPECTEDCHARACTER));
                    return false;
                }
                if (!parseAttribute()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parsePI, state);
                    return false;
                }
                break;
            case ADone:
                next();
                break;
            case Char:
                stringAddC();
                next();
                break;
            case Qm:
                // skip the '?'
                next();
                break;
            case Done:
                next();
                break;
        }
    }
    return false;
}

/*
  Parse a document type definition (doctypedecl [28]).

  Precondition: the beginning '<!' of the doctype is already read the head
  stands on the 'D' of '<!DOCTYPE'.

  If this function was successful, the head-position is on the first
  character after the document type definition.
*/
bool QXmlSimpleReaderPrivate::parseDoctype()
{
    const signed char Init             =  0;
    const signed char Doctype          =  1; // read the doctype
    const signed char Ws1              =  2; // eat_ws
    const signed char Doctype2         =  3; // read the doctype, part 2
    const signed char Ws2              =  4; // eat_ws
    const signed char Sys              =  5; // read SYSTEM or PUBLIC
    const signed char Ws3              =  6; // eat_ws
    const signed char MP               =  7; // markupdecl or PEReference
    const signed char MPR              =  8; // same as MP, but already reported
    const signed char PER              =  9; // PERReference
    const signed char Mup              = 10; // markupdecl
    const signed char Ws4              = 11; // eat_ws
    const signed char MPE              = 12; // end of markupdecl or PEReference
    const signed char Done             = 13;

    const signed char InpWs            = 0;
    const signed char InpD             = 1; // 'D'
    const signed char InpS             = 2; // 'S' or 'P'
    const signed char InpOB            = 3; // [
    const signed char InpCB            = 4; //]
    const signed char InpPer           = 5; // %
    const signed char InpGt            = 6; // >
    const signed char InpUnknown       = 7;

    static const signed char table[13][8] = {
     /*  InpWs,  InpD       InpS       InpOB  InpCB  InpPer InpGt  InpUnknown */
        { -1,     Doctype,   -1,        -1,    -1,    -1,    -1,    -1        }, // Init
        { Ws1,    -1,        -1,        -1,    -1,    -1,    -1,    -1        }, // Doctype
        { -1,     Doctype2,  Doctype2,  -1,    -1,    -1,    -1,    Doctype2  }, // Ws1
        { Ws2,    -1,        Sys,       MP,    -1,    -1,    Done,  -1        }, // Doctype2
        { -1,     -1,        Sys,       MP,    -1,    -1,    Done,  -1        }, // Ws2
        { Ws3,    -1,        -1,        MP,    -1,    -1,    Done,  -1        }, // Sys
        { -1,     -1,        -1,        MP,    -1,    -1,    Done,  -1        }, // Ws3
        { -1,     -1,        -1,        -1,    MPE,   PER,   -1,    Mup       }, // MP
        { -1,     -1,        -1,        -1,    MPE,   PER,   -1,    Mup       }, // MPR
        { Ws4,    -1,        -1,        -1,    MPE,   PER,   -1,    Mup       }, // PER
        { Ws4,    -1,        -1,        -1,    MPE,   PER,   -1,    Mup       }, // Mup
        { -1,     -1,        -1,        -1,    MPE,   PER,   -1,    Mup       }, // Ws4
        { -1,     -1,        -1,        -1,    -1,    -1,    Done,  -1        }  // MPE
    };
    signed char state;
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        startDTDwasReported = false;
        systemId.clear();
        publicId.clear();
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseDoctype (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseDoctype, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case Doctype2:
                doctype = name();
                break;
            case MP:
                if (!startDTDwasReported && lexicalHnd ) {
                    startDTDwasReported = true;
                    if (!lexicalHnd->startDTD(doctype, publicId, systemId)) {
                        reportParseError(lexicalHnd->errorString());
                        return false;
                    }
                }
                state = MPR;
                break;
            case Done:
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_ERRORPARSINGDOCTYPE));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseDoctype, state);
            return false;
        }
        if (is_S(c)) {
            input = InpWs;
        } else if (c == QLatin1Char('D')) {
            input = InpD;
        } else if (c == QLatin1Char('S')) {
            input = InpS;
        } else if (c == QLatin1Char('P')) {
            input = InpS;
        } else if (c == QLatin1Char('[')) {
            input = InpOB;
        } else if (c == QLatin1Char(']')) {
            input = InpCB;
        } else if (c == QLatin1Char('%')) {
            input = InpPer;
        } else if (c == QLatin1Char('>')) {
            input = InpGt;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case Doctype:
                parseString_s = QLatin1String("DOCTYPE");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseDoctype, state);
                    return false;
                }
                break;
            case Ws1:
            case Ws2:
            case Ws3:
            case Ws4:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseDoctype, state);
                    return false;
                }
                break;
            case Doctype2:
                parseName_useRef = false;
                if (!parseName()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseDoctype, state);
                    return false;
                }
                break;
            case Sys:
                parseExternalID_allowPublicID = false;
                if (!parseExternalID()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseDoctype, state);
                    return false;
                }
                thisPublicId = publicId;
                thisSystemId = systemId;
                break;
            case MP:
            case MPR:
                if (!next_eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseDoctype, state);
                    return false;
                }
                break;
            case PER:
                parsePEReference_context = InDTD;
                if (!parsePEReference()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseDoctype, state);
                    return false;
                }
                break;
            case Mup:
                if (!parseMarkupdecl()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseDoctype, state);
                    return false;
                }
                break;
            case MPE:
                if (!next_eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseDoctype, state);
                    return false;
                }
                break;
            case Done:
                if (lexicalHnd) {
                    if (!startDTDwasReported) {
                        startDTDwasReported = true;
                        if (!lexicalHnd->startDTD(doctype, publicId, systemId)) {
                            reportParseError(lexicalHnd->errorString());
                            return false;
                        }
                    }
                    if (!lexicalHnd->endDTD()) {
                        reportParseError(lexicalHnd->errorString());
                        return false;
                    }
                }
                next();
                break;
        }
    }
    return false;
}

/*
  Parse a ExternalID [75].

  If allowPublicID is true parse ExternalID [75] or PublicID [83].
*/
bool QXmlSimpleReaderPrivate::parseExternalID()
{
    const signed char Init             =  0;
    const signed char Sys              =  1; // parse 'SYSTEM'
    const signed char SysWS            =  2; // parse the whitespace after 'SYSTEM'
    const signed char SysSQ            =  3; // parse SystemLiteral with '
    const signed char SysSQ2           =  4; // parse SystemLiteral with '
    const signed char SysDQ            =  5; // parse SystemLiteral with "
    const signed char SysDQ2           =  6; // parse SystemLiteral with "
    const signed char Pub              =  7; // parse 'PUBLIC'
    const signed char PubWS            =  8; // parse the whitespace after 'PUBLIC'
    const signed char PubSQ            =  9; // parse PubidLiteral with '
    const signed char PubSQ2           = 10; // parse PubidLiteral with '
    const signed char PubDQ            = 11; // parse PubidLiteral with "
    const signed char PubDQ2           = 12; // parse PubidLiteral with "
    const signed char PubE             = 13; // finished parsing the PubidLiteral
    const signed char PubWS2           = 14; // parse the whitespace after the PubidLiteral
    const signed char PDone            = 15; // done if allowPublicID is true
    const signed char Done             = 16;

    const signed char InpSQ            = 0; // '
    const signed char InpDQ            = 1; // "
    const signed char InpS             = 2; // S
    const signed char InpP             = 3; // P
    const signed char InpWs            = 4; // white space
    const signed char InpUnknown       = 5;

    static const signed char table[15][6] = {
     /*  InpSQ    InpDQ    InpS     InpP     InpWs     InpUnknown */
        { -1,      -1,      Sys,     Pub,     -1,       -1      }, // Init
        { -1,      -1,      -1,      -1,      SysWS,    -1      }, // Sys
        { SysSQ,   SysDQ,   -1,      -1,      -1,       -1      }, // SysWS
        { Done,    SysSQ2,  SysSQ2,  SysSQ2,  SysSQ2,   SysSQ2  }, // SysSQ
        { Done,    SysSQ2,  SysSQ2,  SysSQ2,  SysSQ2,   SysSQ2  }, // SysSQ2
        { SysDQ2,  Done,    SysDQ2,  SysDQ2,  SysDQ2,   SysDQ2  }, // SysDQ
        { SysDQ2,  Done,    SysDQ2,  SysDQ2,  SysDQ2,   SysDQ2  }, // SysDQ2
        { -1,      -1,      -1,      -1,      PubWS,    -1      }, // Pub
        { PubSQ,   PubDQ,   -1,      -1,      -1,       -1      }, // PubWS
        { PubE,    -1,      PubSQ2,  PubSQ2,  PubSQ2,   PubSQ2  }, // PubSQ
        { PubE,    -1,      PubSQ2,  PubSQ2,  PubSQ2,   PubSQ2  }, // PubSQ2
        { -1,      PubE,    PubDQ2,  PubDQ2,  PubDQ2,   PubDQ2  }, // PubDQ
        { -1,      PubE,    PubDQ2,  PubDQ2,  PubDQ2,   PubDQ2  }, // PubDQ2
        { PDone,   PDone,   PDone,   PDone,   PubWS2,   PDone   }, // PubE
        { SysSQ,   SysDQ,   PDone,   PDone,   PDone,    PDone   }  // PubWS2
    };
    signed char state;
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        systemId.clear();
        publicId.clear();
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseExternalID (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseExternalID, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case PDone:
                if (parseExternalID_allowPublicID) {
                    publicId = string();
                    return true;
                } else {
                    reportParseError(QLatin1String(XMLERR_UNEXPECTEDCHARACTER));
                    return false;
                }
            case Done:
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_UNEXPECTEDCHARACTER));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseExternalID, state);
            return false;
        }
        if (is_S(c)) {
            input = InpWs;
        } else if (c == QLatin1Char('\'')) {
            input = InpSQ;
        } else if (c == QLatin1Char('"')) {
            input = InpDQ;
        } else if (c == QLatin1Char('S')) {
            input = InpS;
        } else if (c == QLatin1Char('P')) {
            input = InpP;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case Sys:
                parseString_s = QLatin1String("SYSTEM");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseExternalID, state);
                    return false;
                }
                break;
            case SysWS:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseExternalID, state);
                    return false;
                }
                break;
            case SysSQ:
            case SysDQ:
                stringClear();
                next();
                break;
            case SysSQ2:
            case SysDQ2:
                stringAddC();
                next();
                break;
            case Pub:
                parseString_s = QLatin1String("PUBLIC");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseExternalID, state);
                    return false;
                }
                break;
            case PubWS:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseExternalID, state);
                    return false;
                }
                break;
            case PubSQ:
            case PubDQ:
                stringClear();
                next();
                break;
            case PubSQ2:
            case PubDQ2:
                stringAddC();
                next();
                break;
            case PubE:
                next();
                break;
            case PubWS2:
                publicId = string();
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseExternalID, state);
                    return false;
                }
                break;
            case Done:
                systemId = string();
                next();
                break;
        }
    }
    return false;
}

/*
  Parse a markupdecl [29].
*/
bool QXmlSimpleReaderPrivate::parseMarkupdecl()
{
    const signed char Init             = 0;
    const signed char Lt               = 1; // < was read
    const signed char Em               = 2; // ! was read
    const signed char CE               = 3; // E was read
    const signed char Qm               = 4; // ? was read
    const signed char Dash             = 5; // - was read
    const signed char CA               = 6; // A was read
    const signed char CEL              = 7; // EL was read
    const signed char CEN              = 8; // EN was read
    const signed char CN               = 9; // N was read
    const signed char Done             = 10;

    const signed char InpLt            = 0; // <
    const signed char InpQm            = 1; // ?
    const signed char InpEm            = 2; // !
    const signed char InpDash          = 3; // -
    const signed char InpA             = 4; // A
    const signed char InpE             = 5; // E
    const signed char InpL             = 6; // L
    const signed char InpN             = 7; // N
    const signed char InpUnknown       = 8;

    static const signed char table[4][9] = {
     /*  InpLt  InpQm  InpEm  InpDash  InpA   InpE   InpL   InpN   InpUnknown */
        { Lt,    -1,    -1,    -1,      -1,    -1,    -1,    -1,    -1     }, // Init
        { -1,    Qm,    Em,    -1,      -1,    -1,    -1,    -1,    -1     }, // Lt
        { -1,    -1,    -1,    Dash,    CA,    CE,    -1,    CN,    -1     }, // Em
        { -1,    -1,    -1,    -1,      -1,    -1,    CEL,   CEN,   -1     }  // CE
    };
    signed char state;
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseMarkupdecl (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseMarkupdecl, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case Qm:
                if (contentHnd) {
                    if (!contentHnd->processingInstruction(name(),string())) {
                        reportParseError(contentHnd->errorString());
                        return false;
                    }
                }
                return true;
            case Dash:
                if (lexicalHnd) {
                    if (!lexicalHnd->comment(string())) {
                        reportParseError(lexicalHnd->errorString());
                        return false;
                    }
                }
                return true;
            case CA:
                return true;
            case CEL:
                return true;
            case CEN:
                return true;
            case CN:
                return true;
            case Done:
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_LETTEREXPECTED));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseMarkupdecl, state);
            return false;
        }
        if        (c == QLatin1Char('<')) {
            input = InpLt;
        } else if (c == QLatin1Char('?')) {
            input = InpQm;
        } else if (c == QLatin1Char('!')) {
            input = InpEm;
        } else if (c == QLatin1Char('-')) {
            input = InpDash;
        } else if (c == QLatin1Char('A')) {
            input = InpA;
        } else if (c == QLatin1Char('E')) {
            input = InpE;
        } else if (c == QLatin1Char('L')) {
            input = InpL;
        } else if (c == QLatin1Char('N')) {
            input = InpN;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case Lt:
                next();
                break;
            case Em:
                next();
                break;
            case CE:
                next();
                break;
            case Qm:
                parsePI_xmldecl = false;
                if (!parsePI()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseMarkupdecl, state);
                    return false;
                }
                break;
            case Dash:
                if (!parseComment()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseMarkupdecl, state);
                    return false;
                }
                break;
            case CA:
                if (!parseAttlistDecl()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseMarkupdecl, state);
                    return false;
                }
                break;
            case CEL:
                if (!parseElementDecl()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseMarkupdecl, state);
                    return false;
                }
                break;
            case CEN:
                if (!parseEntityDecl()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseMarkupdecl, state);
                    return false;
                }
                break;
            case CN:
                if (!parseNotationDecl()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseMarkupdecl, state);
                    return false;
                }
                break;
        }
    }
    return false;
}

/*
  Parse a PEReference [69]
*/
bool QXmlSimpleReaderPrivate::parsePEReference()
{
    const signed char Init             = 0;
    const signed char Next             = 1;
    const signed char Name             = 2;
    const signed char NameR            = 3; // same as Name, but already reported
    const signed char Done             = 4;

    const signed char InpSemi          = 0; // ;
    const signed char InpPer           = 1; // %
    const signed char InpUnknown       = 2;

    static const signed char table[4][3] = {
     /*  InpSemi  InpPer  InpUnknown */
        { -1,      Next,   -1    }, // Init
        { -1,      -1,     Name  }, // Next
        { Done,    -1,     -1    }, // Name
        { Done,    -1,     -1    }  // NameR
    };
    signed char state;
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parsePEReference (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parsePEReference, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case Name:
                {
                    bool skipIt = true;
                    QString xmlRefString;

                    QMap<QString,QString>::Iterator it;
                    it = parameterEntities.find(ref());
                    if (it != parameterEntities.end()) {
                        skipIt = false;
                        xmlRefString = *it;
                    } else if (entityRes) {
                        QMap<QString,QXmlSimpleReaderPrivate::ExternParameterEntity>::Iterator it2;
                        it2 = externParameterEntities.find(ref());
                        QXmlInputSource *ret = 0;
                        if (it2 != externParameterEntities.end()) {
                            if (!entityRes->resolveEntity((*it2).publicId, (*it2).systemId, ret)) {
                                delete ret;
                                reportParseError(entityRes->errorString());
                                return false;
                            }
                            if (ret) {
                                xmlRefString = ret->data();
                                delete ret;
                                if (!stripTextDecl(xmlRefString)) {
                                    reportParseError(QLatin1String(XMLERR_ERRORINTEXTDECL));
                                    return false;
                                }
                                skipIt = false;
                            }
                        }
                    }

                    if (skipIt) {
                        if (contentHnd) {
                            if (!contentHnd->skippedEntity(QLatin1Char('%') + ref())) {
                                reportParseError(contentHnd->errorString());
                                return false;
                            }
                        }
                    } else {
                        if (parsePEReference_context == InEntityValue) {
                            // Included in literal
                            if (!insertXmlRef(xmlRefString, ref(), true))
                                return false;
                        } else if (parsePEReference_context == InDTD) {
                            // Included as PE
                            if (!insertXmlRef(QLatin1Char(' ') + xmlRefString + QLatin1Char(' '), ref(), false))
                                return false;
                        }
                    }
                }
                state = NameR;
                break;
            case Done:
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_LETTEREXPECTED));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parsePEReference, state);
            return false;
        }
        if        (c == QLatin1Char(';')) {
            input = InpSemi;
        } else if (c == QLatin1Char('%')) {
            input = InpPer;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case Next:
                next();
                break;
            case Name:
            case NameR:
                parseName_useRef = true;
                if (!parseName()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parsePEReference, state);
                    return false;
                }
                break;
            case Done:
                next();
                break;
        }
    }
    return false;
}

/*
  Parse a AttlistDecl [52].

  Precondition: the beginning '<!' is already read and the head
  stands on the 'A' of '<!ATTLIST'
*/
bool QXmlSimpleReaderPrivate::parseAttlistDecl()
{
    const signed char Init             =  0;
    const signed char Attlist          =  1; // parse the string "ATTLIST"
    const signed char Ws               =  2; // whitespace read
    const signed char Name             =  3; // parse name
    const signed char Ws1              =  4; // whitespace read
    const signed char Attdef           =  5; // parse the AttDef
    const signed char Ws2              =  6; // whitespace read
    const signed char Atttype          =  7; // parse the AttType
    const signed char Ws3              =  8; // whitespace read
    const signed char DDecH            =  9; // DefaultDecl with #
    const signed char DefReq           = 10; // parse the string "REQUIRED"
    const signed char DefImp           = 11; // parse the string "IMPLIED"
    const signed char DefFix           = 12; // parse the string "FIXED"
    const signed char Attval           = 13; // parse the AttValue
    const signed char Ws4              = 14; // whitespace read
    const signed char Done             = 15;

    const signed char InpWs            = 0; // white space
    const signed char InpGt            = 1; // >
    const signed char InpHash          = 2; // #
    const signed char InpA             = 3; // A
    const signed char InpI             = 4; // I
    const signed char InpF             = 5; // F
    const signed char InpR             = 6; // R
    const signed char InpUnknown       = 7;

    static const signed char table[15][8] = {
     /*  InpWs    InpGt    InpHash  InpA      InpI     InpF     InpR     InpUnknown */
        { -1,      -1,      -1,      Attlist,  -1,      -1,      -1,      -1      }, // Init
        { Ws,      -1,      -1,      -1,       -1,      -1,      -1,      -1      }, // Attlist
        { -1,      -1,      -1,      Name,     Name,    Name,    Name,    Name    }, // Ws
        { Ws1,     Done,    Attdef,  Attdef,   Attdef,  Attdef,  Attdef,  Attdef  }, // Name
        { -1,      Done,    Attdef,  Attdef,   Attdef,  Attdef,  Attdef,  Attdef  }, // Ws1
        { Ws2,     -1,      -1,      -1,       -1,      -1,      -1,      -1      }, // Attdef
        { -1,      Atttype, Atttype, Atttype,  Atttype, Atttype, Atttype, Atttype }, // Ws2
        { Ws3,     -1,      -1,      -1,       -1,      -1,      -1,      -1      }, // Attype
        { -1,      Attval,  DDecH,   Attval,   Attval,  Attval,  Attval,  Attval  }, // Ws3
        { -1,      -1,      -1,      -1,       DefImp,  DefFix,  DefReq,  -1      }, // DDecH
        { Ws4,     Ws4,     -1,      -1,       -1,      -1,      -1,      -1      }, // DefReq
        { Ws4,     Ws4,     -1,      -1,       -1,      -1,      -1,      -1      }, // DefImp
        { Ws3,     -1,      -1,      -1,       -1,      -1,      -1,      -1      }, // DefFix
        { Ws4,     Ws4,     -1,      -1,       -1,      -1,      -1,      -1      }, // Attval
        { -1,      Done,    Attdef,  Attdef,   Attdef,  Attdef,  Attdef,  Attdef  }  // Ws4
    };
    signed char state;
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseAttlistDecl (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseAttlistDecl, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case Name:
                attDeclEName = name();
                break;
            case Attdef:
                attDeclAName = name();
                break;
            case Done:
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_LETTEREXPECTED));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseAttlistDecl, state);
            return false;
        }
        if (is_S(c)) {
            input = InpWs;
        } else if (c == QLatin1Char('>')) {
            input = InpGt;
        } else if (c == QLatin1Char('#')) {
            input = InpHash;
        } else if (c == QLatin1Char('A')) {
            input = InpA;
        } else if (c == QLatin1Char('I')) {
            input = InpI;
        } else if (c == QLatin1Char('F')) {
            input = InpF;
        } else if (c == QLatin1Char('R')) {
            input = InpR;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case Attlist:
                parseString_s = QLatin1String("ATTLIST");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttlistDecl, state);
                    return false;
                }
                break;
            case Ws:
            case Ws1:
            case Ws2:
            case Ws3:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttlistDecl, state);
                    return false;
                }
                break;
            case Name:
                parseName_useRef = false;
                if (!parseName()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttlistDecl, state);
                    return false;
                }
                break;
            case Attdef:
                parseName_useRef = false;
                if (!parseName()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttlistDecl, state);
                    return false;
                }
                break;
            case Atttype:
                if (!parseAttType()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttlistDecl, state);
                    return false;
                }
                break;
            case DDecH:
                next();
                break;
            case DefReq:
                parseString_s = QLatin1String("REQUIRED");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttlistDecl, state);
                    return false;
                }
                break;
            case DefImp:
                parseString_s = QLatin1String("IMPLIED");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttlistDecl, state);
                    return false;
                }
                break;
            case DefFix:
                parseString_s = QLatin1String("FIXED");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttlistDecl, state);
                    return false;
                }
                break;
            case Attval:
                if (!parseAttValue()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttlistDecl, state);
                    return false;
                }
                break;
            case Ws4:
                if (declHnd) {
                    // ### not all values are computed yet...
                    if (!declHnd->attributeDecl(attDeclEName, attDeclAName, QLatin1String(""), QLatin1String(""), QLatin1String(""))) {
                        reportParseError(declHnd->errorString());
                        return false;
                    }
                }
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttlistDecl, state);
                    return false;
                }
                break;
            case Done:
                next();
                break;
        }
    }
    return false;
}

/*
  Parse a AttType [54]
*/
bool QXmlSimpleReaderPrivate::parseAttType()
{
    const signed char Init             =  0;
    const signed char ST               =  1; // StringType
    const signed char TTI              =  2; // TokenizedType starting with 'I'
    const signed char TTI2             =  3; // TokenizedType helpstate
    const signed char TTI3             =  4; // TokenizedType helpstate
    const signed char TTE              =  5; // TokenizedType starting with 'E'
    const signed char TTEY             =  6; // TokenizedType starting with 'ENTITY'
    const signed char TTEI             =  7; // TokenizedType starting with 'ENTITI'
    const signed char N                =  8; // N read (TokenizedType or Notation)
    const signed char TTNM             =  9; // TokenizedType starting with 'NM'
    const signed char TTNM2            = 10; // TokenizedType helpstate
    const signed char NO               = 11; // Notation
    const signed char NO2              = 12; // Notation helpstate
    const signed char NO3              = 13; // Notation helpstate
    const signed char NOName           = 14; // Notation, read name
    const signed char NO4              = 15; // Notation helpstate
    const signed char EN               = 16; // Enumeration
    const signed char ENNmt            = 17; // Enumeration, read Nmtoken
    const signed char EN2              = 18; // Enumeration helpstate
    const signed char ADone            = 19; // almost done (make next and accept)
    const signed char Done             = 20;

    const signed char InpWs            =  0; // whitespace
    const signed char InpOp            =  1; // (
    const signed char InpCp            =  2; //)
    const signed char InpPipe          =  3; // |
    const signed char InpC             =  4; // C
    const signed char InpE             =  5; // E
    const signed char InpI             =  6; // I
    const signed char InpM             =  7; // M
    const signed char InpN             =  8; // N
    const signed char InpO             =  9; // O
    const signed char InpR             = 10; // R
    const signed char InpS             = 11; // S
    const signed char InpY             = 12; // Y
    const signed char InpUnknown       = 13;

    static const signed char table[19][14] = {
     /*  InpWs    InpOp    InpCp    InpPipe  InpC     InpE     InpI     InpM     InpN     InpO     InpR     InpS     InpY     InpUnknown */
        { -1,      EN,      -1,      -1,      ST,      TTE,     TTI,     -1,      N,       -1,      -1,      -1,      -1,      -1     }, // Init
        { Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done   }, // ST
        { Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    TTI2,    Done,    Done,    Done   }, // TTI
        { Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    TTI3,    Done,    Done   }, // TTI2
        { Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done   }, // TTI3
        { -1,      -1,      -1,      -1,      -1,      -1,      TTEI,    -1,      -1,      -1,      -1,      -1,      TTEY,    -1     }, // TTE
        { Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done   }, // TTEY
        { Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done   }, // TTEI
        { -1,      -1,      -1,      -1,      -1,      -1,      -1,      TTNM,    -1,      NO,      -1,      -1,      -1,      -1     }, // N
        { Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    TTNM2,   Done,    Done   }, // TTNM
        { Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done,    Done   }, // TTNM2
        { NO2,     -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1     }, // NO
        { -1,      NO3,     -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1     }, // NO2
        { NOName,  NOName,  NOName,  NOName,  NOName,  NOName,  NOName,  NOName,  NOName,  NOName,  NOName,  NOName,  NOName,  NOName }, // NO3
        { NO4,     -1,      ADone,   NO3,     -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1     }, // NOName
        { -1,      -1,      ADone,   NO3,     -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1     }, // NO4
        { -1,      -1,      ENNmt,   -1,      ENNmt,   ENNmt,   ENNmt,   ENNmt,   ENNmt,   ENNmt,   ENNmt,   ENNmt,   ENNmt,   ENNmt  }, // EN
        { EN2,     -1,      ADone,   EN,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1     }, // ENNmt
        { -1,      -1,      ADone,   EN,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1     }  // EN2
    };
    signed char state;
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseAttType (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseAttType, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case ADone:
                return true;
            case Done:
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_LETTEREXPECTED));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseAttType, state);
            return false;
        }
        if (is_S(c)) {
            input = InpWs;
        } else if (c == QLatin1Char('(')) {
            input = InpOp;
        } else if (c == QLatin1Char(')')) {
            input = InpCp;
        } else if (c == QLatin1Char('|')) {
            input = InpPipe;
        } else if (c == QLatin1Char('C')) {
            input = InpC;
        } else if (c == QLatin1Char('E')) {
            input = InpE;
        } else if (c == QLatin1Char('I')) {
            input = InpI;
        } else if (c == QLatin1Char('M')) {
            input = InpM;
        } else if (c == QLatin1Char('N')) {
            input = InpN;
        } else if (c == QLatin1Char('O')) {
            input = InpO;
        } else if (c == QLatin1Char('R')) {
            input = InpR;
        } else if (c == QLatin1Char('S')) {
            input = InpS;
        } else if (c == QLatin1Char('Y')) {
            input = InpY;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case ST:
                parseString_s = QLatin1String("CDATA");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttType, state);
                    return false;
                }
                break;
            case TTI:
                parseString_s = QLatin1String("ID");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttType, state);
                    return false;
                }
                break;
            case TTI2:
                parseString_s = QLatin1String("REF");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttType, state);
                    return false;
                }
                break;
            case TTI3:
                next(); // S
                break;
            case TTE:
                parseString_s = QLatin1String("ENTIT");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttType, state);
                    return false;
                }
                break;
            case TTEY:
                next(); // Y
                break;
            case TTEI:
                parseString_s = QLatin1String("IES");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttType, state);
                    return false;
                }
                break;
            case N:
                next(); // N
                break;
            case TTNM:
                parseString_s = QLatin1String("MTOKEN");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttType, state);
                    return false;
                }
                break;
            case TTNM2:
                next(); // S
                break;
            case NO:
                parseString_s = QLatin1String("OTATION");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttType, state);
                    return false;
                }
                break;
            case NO2:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttType, state);
                    return false;
                }
                break;
            case NO3:
                if (!next_eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttType, state);
                    return false;
                }
                break;
            case NOName:
                parseName_useRef = false;
                if (!parseName()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttType, state);
                    return false;
                }
                break;
            case NO4:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttType, state);
                    return false;
                }
                break;
            case EN:
                if (!next_eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttType, state);
                    return false;
                }
                break;
            case ENNmt:
                if (!parseNmtoken()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttType, state);
                    return false;
                }
                break;
            case EN2:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttType, state);
                    return false;
                }
                break;
            case ADone:
                next();
                break;
        }
    }
    return false;
}

/*
  Parse a AttValue [10]

  Precondition: the head stands on the beginning " or '

  If this function was successful, the head stands on the first
  character after the closing " or ' and the value of the attribute
  is in string().
*/
bool QXmlSimpleReaderPrivate::parseAttValue()
{
    const signed char Init             = 0;
    const signed char Dq               = 1; // double quotes were read
    const signed char DqRef            = 2; // read references in double quotes
    const signed char DqC              = 3; // signed character read in double quotes
    const signed char Sq               = 4; // single quotes were read
    const signed char SqRef            = 5; // read references in single quotes
    const signed char SqC              = 6; // signed character read in single quotes
    const signed char Done             = 7;

    const signed char InpDq            = 0; // "
    const signed char InpSq            = 1; // '
    const signed char InpAmp           = 2; // &
    const signed char InpLt            = 3; // <
    const signed char InpUnknown       = 4;

    static const signed char table[7][5] = {
     /*  InpDq  InpSq  InpAmp  InpLt InpUnknown */
        { Dq,    Sq,    -1,     -1,   -1    }, // Init
        { Done,  DqC,   DqRef,  -1,   DqC   }, // Dq
        { Done,  DqC,   DqRef,  -1,   DqC   }, // DqRef
        { Done,  DqC,   DqRef,  -1,   DqC   }, // DqC
        { SqC,   Done,  SqRef,  -1,   SqC   }, // Sq
        { SqC,   Done,  SqRef,  -1,   SqC   }, // SqRef
        { SqC,   Done,  SqRef,  -1,   SqC   }  // SqRef
    };
    signed char state;
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseAttValue (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseAttValue, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case Done:
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_UNEXPECTEDCHARACTER));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseAttValue, state);
            return false;
        }
        if        (c == QLatin1Char('"')) {
            input = InpDq;
        } else if (c == QLatin1Char('\'')) {
            input = InpSq;
        } else if (c == QLatin1Char('&')) {
            input = InpAmp;
        } else if (c == QLatin1Char('<')) {
            input = InpLt;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case Dq:
            case Sq:
                stringClear();
                next();
                break;
            case DqRef:
            case SqRef:
                parseReference_context = InAttributeValue;
                if (!parseReference()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttValue, state);
                    return false;
                }
                break;
            case DqC:
            case SqC:
                stringAddC();
                next();
                break;
            case Done:
                next();
                break;
        }
    }
    return false;
}

/*
  Parse a elementdecl [45].

  Precondition: the beginning '<!E' is already read and the head
  stands on the 'L' of '<!ELEMENT'
*/
bool QXmlSimpleReaderPrivate::parseElementDecl()
{
    const signed char Init             =  0;
    const signed char Elem             =  1; // parse the beginning string
    const signed char Ws1              =  2; // whitespace required
    const signed char Nam              =  3; // parse Name
    const signed char Ws2              =  4; // whitespace required
    const signed char Empty            =  5; // read EMPTY
    const signed char Any              =  6; // read ANY
    const signed char Cont             =  7; // read contentspec (except ANY or EMPTY)
    const signed char Mix              =  8; // read Mixed
    const signed char Mix2             =  9; //
    const signed char Mix3             = 10; //
    const signed char MixN1            = 11; //
    const signed char MixN2            = 12; //
    const signed char MixN3            = 13; //
    const signed char MixN4            = 14; //
    const signed char Cp               = 15; // parse cp
    const signed char Cp2              = 16; //
    const signed char WsD              = 17; // eat whitespace before Done
    const signed char Done             = 18;

    const signed char InpWs            =  0;
    const signed char InpGt            =  1; // >
    const signed char InpPipe          =  2; // |
    const signed char InpOp            =  3; // (
    const signed char InpCp            =  4; //)
    const signed char InpHash          =  5; // #
    const signed char InpQm            =  6; // ?
    const signed char InpAst           =  7; // *
    const signed char InpPlus          =  8; // +
    const signed char InpA             =  9; // A
    const signed char InpE             = 10; // E
    const signed char InpL             = 11; // L
    const signed char InpUnknown       = 12;

    static const signed char table[18][13] = {
     /*  InpWs   InpGt  InpPipe  InpOp  InpCp   InpHash  InpQm  InpAst  InpPlus  InpA    InpE    InpL    InpUnknown */
        { -1,     -1,    -1,      -1,    -1,     -1,      -1,    -1,     -1,      -1,     -1,     Elem,   -1     }, // Init
        { Ws1,    -1,    -1,      -1,    -1,     -1,      -1,    -1,     -1,      -1,     -1,     -1,     -1     }, // Elem
        { -1,     -1,    -1,      -1,    -1,     -1,      -1,    -1,     -1,      Nam,    Nam,    Nam,    Nam    }, // Ws1
        { Ws2,    -1,    -1,      -1,    -1,     -1,      -1,    -1,     -1,      -1,     -1,     -1,     -1     }, // Nam
        { -1,     -1,    -1,      Cont,  -1,     -1,      -1,    -1,     -1,      Any,    Empty,  -1,     -1     }, // Ws2
        { WsD,    Done,  -1,      -1,    -1,     -1,      -1,    -1,     -1,      -1,     -1,     -1,     -1     }, // Empty
        { WsD,    Done,  -1,      -1,    -1,     -1,      -1,    -1,     -1,      -1,     -1,     -1,     -1     }, // Any
        { -1,     -1,    -1,      Cp,    Cp,     Mix,     -1,    -1,     -1,      Cp,     Cp,     Cp,     Cp     }, // Cont
        { Mix2,   -1,    MixN1,   -1,    Mix3,   -1,      -1,    -1,     -1,      -1,     -1,     -1,     -1     }, // Mix
        { -1,     -1,    MixN1,   -1,    Mix3,   -1,      -1,    -1,     -1,      -1,     -1,     -1,     -1     }, // Mix2
        { WsD,    Done,  -1,      -1,    -1,     -1,      -1,    WsD,    -1,      -1,     -1,     -1,     -1     }, // Mix3
        { -1,     -1,    -1,      -1,    -1,     -1,      -1,    -1,     -1,      MixN2,  MixN2,  MixN2,  MixN2  }, // MixN1
        { MixN3,  -1,    MixN1,   -1,    MixN4,  -1,      -1,    -1,     -1,      -1,     -1,     -1,     -1     }, // MixN2
        { -1,     -1,    MixN1,   -1,    MixN4,  -1,      -1,    -1,     -1,      -1,     -1,     -1,     -1     }, // MixN3
        { -1,     -1,    -1,      -1,    -1,     -1,      -1,    WsD,    -1,      -1,     -1,     -1,     -1     }, // MixN4
        { WsD,    Done,  -1,      -1,    -1,     -1,      Cp2,   Cp2,    Cp2,     -1,     -1,     -1,     -1     }, // Cp
        { WsD,    Done,  -1,      -1,    -1,     -1,      -1,    -1,     -1,      -1,     -1,     -1,     -1     }, // Cp2
        { -1,     Done,  -1,      -1,    -1,     -1,      -1,    -1,     -1,      -1,     -1,     -1,     -1     }  // WsD
    };
    signed char state;
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseElementDecl (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseElementDecl, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case Done:
                return true;
            case -1:
                reportParseError(QLatin1String(XMLERR_UNEXPECTEDCHARACTER));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseElementDecl, state);
            return false;
        }
        if (is_S(c)) {
            input = InpWs;
        } else if (c == QLatin1Char('>')) {
            input = InpGt;
        } else if (c == QLatin1Char('|')) {
            input = InpPipe;
        } else if (c == QLatin1Char('(')) {
            input = InpOp;
        } else if (c == QLatin1Char(')')) {
            input = InpCp;
        } else if (c == QLatin1Char('#')) {
            input = InpHash;
        } else if (c == QLatin1Char('?')) {
            input = InpQm;
        } else if (c == QLatin1Char('*')) {
            input = InpAst;
        } else if (c == QLatin1Char('+')) {
            input = InpPlus;
        } else if (c == QLatin1Char('A')) {
            input = InpA;
        } else if (c == QLatin1Char('E')) {
            input = InpE;
        } else if (c == QLatin1Char('L')) {
            input = InpL;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case Elem:
                parseString_s = QLatin1String("LEMENT");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElementDecl, state);
                    return false;
                }
                break;
            case Ws1:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElementDecl, state);
                    return false;
                }
                break;
            case Nam:
                parseName_useRef = false;
                if (!parseName()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElementDecl, state);
                    return false;
                }
                break;
            case Ws2:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElementDecl, state);
                    return false;
                }
                break;
            case Empty:
                parseString_s = QLatin1String("EMPTY");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElementDecl, state);
                    return false;
                }
                break;
            case Any:
                parseString_s = QLatin1String("ANY");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElementDecl, state);
                    return false;
                }
                break;
            case Cont:
                if (!next_eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElementDecl, state);
                    return false;
                }
                break;
            case Mix:
                parseString_s = QLatin1String("#PCDATA");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElementDecl, state);
                    return false;
                }
                break;
            case Mix2:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElementDecl, state);
                    return false;
                }
                break;
            case Mix3:
                next();
                break;
            case MixN1:
                if (!next_eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElementDecl, state);
                    return false;
                }
                break;
            case MixN2:
                parseName_useRef = false;
                if (!parseName()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElementDecl, state);
                    return false;
                }
                break;
            case MixN3:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElementDecl, state);
                    return false;
                }
                break;
            case MixN4:
                next();
                break;
            case Cp:
                if (!parseChoiceSeq()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElementDecl, state);
                    return false;
                }
                break;
            case Cp2:
                next();
                break;
            case WsD:
                if (!next_eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseElementDecl, state);
                    return false;
                }
                break;
            case Done:
                next();
                break;
        }
    }
    return false;
}

/*
  Parse a NotationDecl [82].

  Precondition: the beginning '<!' is already read and the head
  stands on the 'N' of '<!NOTATION'
*/
bool QXmlSimpleReaderPrivate::parseNotationDecl()
{
    const signed char Init             = 0;
    const signed char Not              = 1; // read NOTATION
    const signed char Ws1              = 2; // eat whitespaces
    const signed char Nam              = 3; // read Name
    const signed char Ws2              = 4; // eat whitespaces
    const signed char ExtID            = 5; // parse ExternalID
    const signed char ExtIDR           = 6; // same as ExtID, but already reported
    const signed char Ws3              = 7; // eat whitespaces
    const signed char Done             = 8;

    const signed char InpWs            = 0;
    const signed char InpGt            = 1; // >
    const signed char InpN             = 2; // N
    const signed char InpUnknown       = 3;

    static const signed char table[8][4] = {
     /*  InpWs   InpGt  InpN    InpUnknown */
        { -1,     -1,    Not,    -1     }, // Init
        { Ws1,    -1,    -1,     -1     }, // Not
        { -1,     -1,    Nam,    Nam    }, // Ws1
        { Ws2,    Done,  -1,     -1     }, // Nam
        { -1,     Done,  ExtID,  ExtID  }, // Ws2
        { Ws3,    Done,  -1,     -1     }, // ExtID
        { Ws3,    Done,  -1,     -1     }, // ExtIDR
        { -1,     Done,  -1,     -1     }  // Ws3
    };
    signed char state;
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseNotationDecl (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseNotationDecl, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case ExtID:
                // call the handler
                if (dtdHnd) {
                    if (!dtdHnd->notationDecl(name(), publicId, systemId)) {
                        reportParseError(dtdHnd->errorString());
                        return false;
                    }
                }
                state = ExtIDR;
                break;
            case Done:
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_UNEXPECTEDCHARACTER));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseNotationDecl, state);
            return false;
        }
        if (is_S(c)) {
            input = InpWs;
        } else if (c == QLatin1Char('>')) {
            input = InpGt;
        } else if (c == QLatin1Char('N')) {
            input = InpN;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case Not:
                parseString_s = QLatin1String("NOTATION");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseNotationDecl, state);
                    return false;
                }
                break;
            case Ws1:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseNotationDecl, state);
                    return false;
                }
                break;
            case Nam:
                parseName_useRef = false;
                if (!parseName()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseNotationDecl, state);
                    return false;
                }
                break;
            case Ws2:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseNotationDecl, state);
                    return false;
                }
                break;
            case ExtID:
            case ExtIDR:
                parseExternalID_allowPublicID = true;
                if (!parseExternalID()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseNotationDecl, state);
                    return false;
                }
                break;
            case Ws3:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseNotationDecl, state);
                    return false;
                }
                break;
            case Done:
                next();
                break;
        }
    }
    return false;
}

/*
  Parse choice [49] or seq [50].

  Precondition: the beginning '('S? is already read and the head
  stands on the first non-whitespace character after it.
*/
bool QXmlSimpleReaderPrivate::parseChoiceSeq()
{
    const signed char Init             = 0;
    const signed char Ws1              = 1; // eat whitespace
    const signed char CoS              = 2; // choice or set
    const signed char Ws2              = 3; // eat whitespace
    const signed char More             = 4; // more cp to read
    const signed char Name             = 5; // read name
    const signed char Done             = 6; //

    const signed char InpWs            = 0; // S
    const signed char InpOp            = 1; // (
    const signed char InpCp            = 2; //)
    const signed char InpQm            = 3; // ?
    const signed char InpAst           = 4; // *
    const signed char InpPlus          = 5; // +
    const signed char InpPipe          = 6; // |
    const signed char InpComm          = 7; // ,
    const signed char InpUnknown       = 8;

    static const signed char table[6][9] = {
     /*  InpWs   InpOp  InpCp  InpQm  InpAst  InpPlus  InpPipe  InpComm  InpUnknown */
        { -1,     Ws1,   -1,    -1,    -1,     -1,      -1,      -1,      Name  }, // Init
        { -1,     CoS,   -1,    -1,    -1,     -1,      -1,      -1,      CoS   }, // Ws1
        { Ws2,    -1,    Done,  Ws2,   Ws2,    Ws2,     More,    More,    -1    }, // CS
        { -1,     -1,    Done,  -1,    -1,     -1,      More,    More,    -1    }, // Ws2
        { -1,     Ws1,   -1,    -1,    -1,     -1,      -1,      -1,      Name  }, // More (same as Init)
        { Ws2,    -1,    Done,  Ws2,   Ws2,    Ws2,     More,    More,    -1    }  // Name (same as CS)
    };
    signed char state;
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseChoiceSeq (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseChoiceSeq, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case Done:
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_UNEXPECTEDCHARACTER));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseChoiceSeq, state);
            return false;
        }
        if (is_S(c)) {
            input = InpWs;
        } else if (c == QLatin1Char('(')) {
            input = InpOp;
        } else if (c == QLatin1Char(')')) {
            input = InpCp;
        } else if (c == QLatin1Char('?')) {
            input = InpQm;
        } else if (c == QLatin1Char('*')) {
            input = InpAst;
        } else if (c == QLatin1Char('+')) {
            input = InpPlus;
        } else if (c == QLatin1Char('|')) {
            input = InpPipe;
        } else if (c == QLatin1Char(',')) {
            input = InpComm;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case Ws1:
                if (!next_eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseChoiceSeq, state);
                    return false;
                }
                break;
            case CoS:
                if (!parseChoiceSeq()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseChoiceSeq, state);
                    return false;
                }
                break;
            case Ws2:
                if (!next_eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseChoiceSeq, state);
                    return false;
                }
                break;
            case More:
                if (!next_eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseChoiceSeq, state);
                    return false;
                }
                break;
            case Name:
                parseName_useRef = false;
                if (!parseName()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseChoiceSeq, state);
                    return false;
                }
                break;
            case Done:
                next();
                break;
        }
    }
    return false;
}

/*
  Parse a EntityDecl [70].

  Precondition: the beginning '<!E' is already read and the head
  stand on the 'N' of '<!ENTITY'
*/
bool QXmlSimpleReaderPrivate::parseEntityDecl()
{
    const signed char Init             =  0;
    const signed char Ent              =  1; // parse "ENTITY"
    const signed char Ws1              =  2; // white space read
    const signed char Name             =  3; // parse name
    const signed char Ws2              =  4; // white space read
    const signed char EValue           =  5; // parse entity value
    const signed char EValueR          =  6; // same as EValue, but already reported
    const signed char ExtID            =  7; // parse ExternalID
    const signed char Ws3              =  8; // white space read
    const signed char Ndata            =  9; // parse "NDATA"
    const signed char Ws4              = 10; // white space read
    const signed char NNam             = 11; // parse name
    const signed char NNamR            = 12; // same as NNam, but already reported
    const signed char PEDec            = 13; // parse PEDecl
    const signed char Ws6              = 14; // white space read
    const signed char PENam            = 15; // parse name
    const signed char Ws7              = 16; // white space read
    const signed char PEVal            = 17; // parse entity value
    const signed char PEValR           = 18; // same as PEVal, but already reported
    const signed char PEEID            = 19; // parse ExternalID
    const signed char PEEIDR           = 20; // same as PEEID, but already reported
    const signed char WsE              = 21; // white space read
    const signed char Done             = 22;
    const signed char EDDone           = 23; // done, but also report an external, unparsed entity decl

    const signed char InpWs            = 0; // white space
    const signed char InpPer           = 1; // %
    const signed char InpQuot          = 2; // " or '
    const signed char InpGt            = 3; // >
    const signed char InpN             = 4; // N
    const signed char InpUnknown       = 5;

    static const signed char table[22][6] = {
     /*  InpWs  InpPer  InpQuot  InpGt  InpN    InpUnknown */
        { -1,    -1,     -1,      -1,    Ent,    -1      }, // Init
        { Ws1,   -1,     -1,      -1,    -1,     -1      }, // Ent
        { -1,    PEDec,  -1,      -1,    Name,   Name    }, // Ws1
        { Ws2,   -1,     -1,      -1,    -1,     -1      }, // Name
        { -1,    -1,     EValue,  -1,    -1,     ExtID   }, // Ws2
        { WsE,   -1,     -1,      Done,  -1,     -1      }, // EValue
        { WsE,   -1,     -1,      Done,  -1,     -1      }, // EValueR
        { Ws3,   -1,     -1,      EDDone,-1,     -1      }, // ExtID
        { -1,    -1,     -1,      EDDone,Ndata,  -1      }, // Ws3
        { Ws4,   -1,     -1,      -1,    -1,     -1      }, // Ndata
        { -1,    -1,     -1,      -1,    NNam,   NNam    }, // Ws4
        { WsE,   -1,     -1,      Done,  -1,     -1      }, // NNam
        { WsE,   -1,     -1,      Done,  -1,     -1      }, // NNamR
        { Ws6,   -1,     -1,      -1,    -1,     -1      }, // PEDec
        { -1,    -1,     -1,      -1,    PENam,  PENam   }, // Ws6
        { Ws7,   -1,     -1,      -1,    -1,     -1      }, // PENam
        { -1,    -1,     PEVal,   -1,    -1,     PEEID   }, // Ws7
        { WsE,   -1,     -1,      Done,  -1,     -1      }, // PEVal
        { WsE,   -1,     -1,      Done,  -1,     -1      }, // PEValR
        { WsE,   -1,     -1,      Done,  -1,     -1      }, // PEEID
        { WsE,   -1,     -1,      Done,  -1,     -1      }, // PEEIDR
        { -1,    -1,     -1,      Done,  -1,     -1      }  // WsE
    };
    signed char state;
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseEntityDecl (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case EValue:
                if ( !entityExist(name())) {
                    entities.insert(name(), string());
                    if (declHnd) {
                        if (!declHnd->internalEntityDecl(name(), string())) {
                            reportParseError(declHnd->errorString());
                            return false;
                        }
                    }
                }
                state = EValueR;
                break;
            case NNam:
                if ( !entityExist(name())) {
                    externEntities.insert(name(), QXmlSimpleReaderPrivate::ExternEntity(publicId, systemId, ref()));
                    if (dtdHnd) {
                        if (!dtdHnd->unparsedEntityDecl(name(), publicId, systemId, ref())) {
                            reportParseError(declHnd->errorString());
                            return false;
                        }
                    }
                }
                state = NNamR;
                break;
            case PEVal:
                if ( !entityExist(name())) {
                    parameterEntities.insert(name(), string());
                    if (declHnd) {
                        if (!declHnd->internalEntityDecl(QLatin1Char('%') + name(), string())) {
                            reportParseError(declHnd->errorString());
                            return false;
                        }
                    }
                }
                state = PEValR;
                break;
            case PEEID:
                if ( !entityExist(name())) {
                    externParameterEntities.insert(name(), QXmlSimpleReaderPrivate::ExternParameterEntity(publicId, systemId));
                    if (declHnd) {
                        if (!declHnd->externalEntityDecl(QLatin1Char('%') + name(), publicId, systemId)) {
                            reportParseError(declHnd->errorString());
                            return false;
                        }
                    }
                }
                state = PEEIDR;
                break;
            case EDDone:
                if ( !entityExist(name())) {
                    externEntities.insert(name(), QXmlSimpleReaderPrivate::ExternEntity(publicId, systemId, QString()));
                    if (declHnd) {
                        if (!declHnd->externalEntityDecl(name(), publicId, systemId)) {
                            reportParseError(declHnd->errorString());
                            return false;
                        }
                    }
                }
                return true;
            case Done:
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_LETTEREXPECTED));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
            return false;
        }
        if (is_S(c)) {
            input = InpWs;
        } else if (c == QLatin1Char('%')) {
            input = InpPer;
        } else if (c == QLatin1Char('"') || c == QLatin1Char('\'')) {
            input = InpQuot;
        } else if (c == QLatin1Char('>')) {
            input = InpGt;
        } else if (c == QLatin1Char('N')) {
            input = InpN;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case Ent:
                parseString_s = QLatin1String("NTITY");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
                    return false;
                }
                break;
            case Ws1:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
                    return false;
                }
                break;
            case Name:
                parseName_useRef = false;
                if (!parseName()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
                    return false;
                }
                break;
            case Ws2:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
                    return false;
                }
                break;
            case EValue:
            case EValueR:
                if (!parseEntityValue()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
                    return false;
                }
                break;
            case ExtID:
                parseExternalID_allowPublicID = false;
                if (!parseExternalID()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
                    return false;
                }
                break;
            case Ws3:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
                    return false;
                }
                break;
            case Ndata:
                parseString_s = QLatin1String("NDATA");
                if (!parseString()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
                    return false;
                }
                break;
            case Ws4:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
                    return false;
                }
                break;
            case NNam:
            case NNamR:
                parseName_useRef = true;
                if (!parseName()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
                    return false;
                }
                break;
            case PEDec:
                next();
                break;
            case Ws6:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
                    return false;
                }
                break;
            case PENam:
                parseName_useRef = false;
                if (!parseName()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
                    return false;
                }
                break;
            case Ws7:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
                    return false;
                }
                break;
            case PEVal:
            case PEValR:
                if (!parseEntityValue()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
                    return false;
                }
                break;
            case PEEID:
            case PEEIDR:
                parseExternalID_allowPublicID = false;
                if (!parseExternalID()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
                    return false;
                }
                break;
            case WsE:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityDecl, state);
                    return false;
                }
                break;
            case EDDone:
                next();
                break;
            case Done:
                next();
                break;
        }
    }
    return false;
}

/*
  Parse a EntityValue [9]
*/
bool QXmlSimpleReaderPrivate::parseEntityValue()
{
    const signed char Init             = 0;
    const signed char Dq               = 1; // EntityValue is double quoted
    const signed char DqC              = 2; // signed character
    const signed char DqPER            = 3; // PERefence
    const signed char DqRef            = 4; // Reference
    const signed char Sq               = 5; // EntityValue is double quoted
    const signed char SqC              = 6; // signed character
    const signed char SqPER            = 7; // PERefence
    const signed char SqRef            = 8; // Reference
    const signed char Done             = 9;

    const signed char InpDq            = 0; // "
    const signed char InpSq            = 1; // '
    const signed char InpAmp           = 2; // &
    const signed char InpPer           = 3; // %
    const signed char InpUnknown       = 4;

    static const signed char table[9][5] = {
     /*  InpDq  InpSq  InpAmp  InpPer  InpUnknown */
        { Dq,    Sq,    -1,     -1,     -1    }, // Init
        { Done,  DqC,   DqRef,  DqPER,  DqC   }, // Dq
        { Done,  DqC,   DqRef,  DqPER,  DqC   }, // DqC
        { Done,  DqC,   DqRef,  DqPER,  DqC   }, // DqPER
        { Done,  DqC,   DqRef,  DqPER,  DqC   }, // DqRef
        { SqC,   Done,  SqRef,  SqPER,  SqC   }, // Sq
        { SqC,   Done,  SqRef,  SqPER,  SqC   }, // SqC
        { SqC,   Done,  SqRef,  SqPER,  SqC   }, // SqPER
        { SqC,   Done,  SqRef,  SqPER,  SqC   }  // SqRef
    };
    signed char state;
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseEntityValue (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseEntityValue, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case Done:
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_LETTEREXPECTED));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseEntityValue, state);
            return false;
        }
        if        (c == QLatin1Char('"')) {
            input = InpDq;
        } else if (c == QLatin1Char('\'')) {
            input = InpSq;
        } else if (c == QLatin1Char('&')) {
            input = InpAmp;
        } else if (c == QLatin1Char('%')) {
            input = InpPer;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case Dq:
            case Sq:
                stringClear();
                next();
                break;
            case DqC:
            case SqC:
                stringAddC();
                next();
                break;
            case DqPER:
            case SqPER:
                parsePEReference_context = InEntityValue;
                if (!parsePEReference()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityValue, state);
                    return false;
                }
                break;
            case DqRef:
            case SqRef:
                parseReference_context = InEntityValue;
                if (!parseReference()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseEntityValue, state);
                    return false;
                }
                break;
            case Done:
                next();
                break;
        }
    }
    return false;
}

/*
  Parse a comment [15].

  Precondition: the beginning '<!' of the comment is already read and the head
  stands on the first '-' of '<!--'.

  If this funktion was successful, the head-position is on the first
  character after the comment.
*/
bool QXmlSimpleReaderPrivate::parseComment()
{
    const signed char Init             = 0;
    const signed char Dash1            = 1; // the first dash was read
    const signed char Dash2            = 2; // the second dash was read
    const signed char Com              = 3; // read comment
    const signed char Com2             = 4; // read comment (help state)
    const signed char ComE             = 5; // finished reading comment
    const signed char Done             = 6;

    const signed char InpDash          = 0; // -
    const signed char InpGt            = 1; // >
    const signed char InpUnknown       = 2;

    static const signed char table[6][3] = {
     /*  InpDash  InpGt  InpUnknown */
        { Dash1,   -1,    -1  }, // Init
        { Dash2,   -1,    -1  }, // Dash1
        { Com2,    Com,   Com }, // Dash2
        { Com2,    Com,   Com }, // Com
        { ComE,    Com,   Com }, // Com2
        { -1,      Done,  -1  }  // ComE
    };
    signed char state;
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseComment (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseComment, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case Dash2:
                stringClear();
                break;
            case Com2:
                // if next character is not a dash than don't skip it
                if (!atEnd() && c != QLatin1Char('-'))
                    stringAddC(QLatin1Char('-'));
                break;
            case Done:
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_ERRORPARSINGCOMMENT));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseComment, state);
            return false;
        }
        if (c == QLatin1Char('-')) {
            input = InpDash;
        } else if (c == QLatin1Char('>')) {
            input = InpGt;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case Dash1:
                next();
                break;
            case Dash2:
                next();
                break;
            case Com:
                stringAddC();
                next();
                break;
            case Com2:
                next();
                break;
            case ComE:
                next();
                break;
            case Done:
                next();
                break;
        }
    }
    return false;
}

/*
    Parse an Attribute [41].

    Precondition: the head stands on the first character of the name
    of the attribute (i.e. all whitespaces are already parsed).

    The head stand on the next character after the end quotes. The
    variable name contains the name of the attribute and the variable
    string contains the value of the attribute.
*/
bool QXmlSimpleReaderPrivate::parseAttribute()
{
    const int Init             = 0;
    const int PName            = 1; // parse name
    const int Ws               = 2; // eat ws
    const int Eq               = 3; // the '=' was read
    const int Quotes           = 4; // " or ' were read

    const int InpNameBe        = 0;
    const int InpEq            = 1; // =
    const int InpDq            = 2; // "
    const int InpSq            = 3; // '
    const int InpUnknown       = 4;

    static const int table[4][5] = {
     /*  InpNameBe  InpEq  InpDq    InpSq    InpUnknown */
        { PName,     -1,    -1,      -1,      -1    }, // Init
        { -1,        Eq,    -1,      -1,      Ws    }, // PName
        { -1,        Eq,    -1,      -1,      -1    }, // Ws
        { -1,        -1,    Quotes,  Quotes,  -1    }  // Eq
    };
    int state;
    int input;

    if (parseStack==0 || parseStack->isEmpty()) {
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseAttribute (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseAttribute, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case Quotes:
                // Done
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_UNEXPECTEDCHARACTER));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseAttribute, state);
            return false;
        }
        if (determineNameChar(c) == NameBeginning) {
            input = InpNameBe;
        } else if (c == QLatin1Char('=')) {
            input = InpEq;
        } else if (c == QLatin1Char('"')) {
            input = InpDq;
        } else if (c == QLatin1Char('\'')) {
            input = InpSq;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case PName:
                parseName_useRef = false;
                if (!parseName()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttribute, state);
                    return false;
                }
                break;
            case Ws:
                if (!eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttribute, state);
                    return false;
                }
                break;
            case Eq:
                if (!next_eat_ws()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttribute, state);
                    return false;
                }
                break;
            case Quotes:
                if (!parseAttValue()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseAttribute, state);
                    return false;
                }
                break;
        }
    }
    return false;
}

/*
  Parse a Name [5] and store the name in name or ref (if useRef is true).
*/
bool QXmlSimpleReaderPrivate::parseName()
{
    const int Init             = 0;
    const int Name1            = 1; // parse first character of the name
    const int Name             = 2; // parse name
    const int Done             = 3;

    static const int table[3][3] = {
     /*  InpNameBe  InpNameCh  InpUnknown */
        { Name1,     -1,        -1    }, // Init
        { Name,      Name,      Done  }, // Name1
        { Name,      Name,      Done  }  // Name
    };
    int state;

    if (parseStack==0 || parseStack->isEmpty()) {
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseName (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseName, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case Done:
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_LETTEREXPECTED));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseName, state);
            return false;
        }

        // we can safely do the (int) cast thanks to the Q_ASSERTs earlier in this function
        state = table[state][(int)fastDetermineNameChar(c)];

        switch (state) {
            case Name1:
                if (parseName_useRef) {
                    refClear();
                    refAddC();
                } else {
                    nameClear();
                    nameAddC();
                }
                next();
                break;
            case Name:
                if (parseName_useRef) {
                    refAddC();
                } else {
                    nameAddC();
                }
                next();
                break;
        }
    }
    return false;
}

/*
  Parse a Nmtoken [7] and store the name in name.
*/
bool QXmlSimpleReaderPrivate::parseNmtoken()
{
    const signed char Init             = 0;
    const signed char NameF            = 1;
    const signed char Name             = 2;
    const signed char Done             = 3;

    const signed char InpNameCh        = 0; // NameChar without InpNameBe
    const signed char InpUnknown       = 1;

    static const signed char table[3][2] = {
     /*  InpNameCh  InpUnknown */
        { NameF,     -1    }, // Init
        { Name,      Done  }, // NameF
        { Name,      Done  }  // Name
    };
    signed char state;
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseNmtoken (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseNmtoken, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case Done:
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_LETTEREXPECTED));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseNmtoken, state);
            return false;
        }
        if (determineNameChar(c) == NotName) {
            input = InpUnknown;
        } else {
            input = InpNameCh;
        }
        state = table[state][input];

        switch (state) {
            case NameF:
                nameClear();
                nameAddC();
                next();
                break;
            case Name:
                nameAddC();
                next();
                break;
        }
    }
    return false;
}

/*
  Parse a Reference [67].

  parseReference_charDataRead is set to true if the reference must not be
  parsed. The character(s) which the reference mapped to are appended to
  string. The head stands on the first character after the reference.

  parseReference_charDataRead is set to false if the reference must be parsed.
  The charachter(s) which the reference mapped to are inserted at the reference
  position. The head stands on the first character of the replacement).
*/
bool QXmlSimpleReaderPrivate::parseReference()
{
    // temporary variables (only used in very local context, so they don't
    // interfere with incremental parsing)
    uint tmp;
    bool ok;

    const signed char Init             =  0;
    const signed char SRef             =  1; // start of a reference
    const signed char ChRef            =  2; // parse CharRef
    const signed char ChDec            =  3; // parse CharRef decimal
    const signed char ChHexS           =  4; // start CharRef hexadecimal
    const signed char ChHex            =  5; // parse CharRef hexadecimal
    const signed char Name             =  6; // parse name
    const signed char DoneD            =  7; // done CharRef decimal
    const signed char DoneH            =  8; // done CharRef hexadecimal
    const signed char DoneN            =  9; // done EntityRef

    const signed char InpAmp           = 0; // &
    const signed char InpSemi          = 1; // ;
    const signed char InpHash          = 2; // #
    const signed char InpX             = 3; // x
    const signed char InpNum           = 4; // 0-9
    const signed char InpHex           = 5; // a-f A-F
    const signed char InpUnknown       = 6;

    static const signed char table[8][7] = {
     /*  InpAmp  InpSemi  InpHash  InpX     InpNum  InpHex  InpUnknown */
        { SRef,   -1,      -1,      -1,      -1,     -1,     -1    }, // Init
        { -1,     -1,      ChRef,   Name,    Name,   Name,   Name  }, // SRef
        { -1,     -1,      -1,      ChHexS,  ChDec,  -1,     -1    }, // ChRef
        { -1,     DoneD,   -1,      -1,      ChDec,  -1,     -1    }, // ChDec
        { -1,     -1,      -1,      -1,      ChHex,  ChHex,  -1    }, // ChHexS
        { -1,     DoneH,   -1,      -1,      ChHex,  ChHex,  -1    }, // ChHex
        { -1,     DoneN,   -1,      -1,      -1,     -1,     -1    }  // Name
    };
    signed char state;
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        parseReference_charDataRead = false;
        state = Init;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseReference (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseReference, state);
                return false;
            }
        }
    }

    for (;;) {
        switch (state) {
            case DoneD:
                return true;
            case DoneH:
                return true;
            case DoneN:
                return true;
            case -1:
                // Error
                reportParseError(QLatin1String(XMLERR_ERRORPARSINGREFERENCE));
                return false;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseReference, state);
            return false;
        }
        if        (c.row()) {
            input = InpUnknown;
        } else if (c.cell() == '&') {
            input = InpAmp;
        } else if (c.cell() == ';') {
            input = InpSemi;
        } else if (c.cell() == '#') {
            input = InpHash;
        } else if (c.cell() == 'x') {
            input = InpX;
        } else if ('0' <= c.cell() && c.cell() <= '9') {
            input = InpNum;
        } else if ('a' <= c.cell() && c.cell() <= 'f') {
            input = InpHex;
        } else if ('A' <= c.cell() && c.cell() <= 'F') {
            input = InpHex;
        } else {
            input = InpUnknown;
        }
        state = table[state][input];

        switch (state) {
            case SRef:
                refClear();
                next();
                break;
            case ChRef:
                next();
                break;
            case ChDec:
                refAddC();
                next();
                break;
            case ChHexS:
                next();
                break;
            case ChHex:
                refAddC();
                next();
                break;
            case Name:
                // read the name into the ref
                parseName_useRef = true;
                if (!parseName()) {
                    parseFailed(&QXmlSimpleReaderPrivate::parseReference, state);
                    return false;
                }
                break;
            case DoneD:
                tmp = ref().toUInt(&ok, 10);
                if (ok) {
                    stringAddC(QChar(tmp));
                } else {
                    reportParseError(QLatin1String(XMLERR_ERRORPARSINGREFERENCE));
                    return false;
                }
                parseReference_charDataRead = true;
                next();
                break;
            case DoneH:
                tmp = ref().toUInt(&ok, 16);
                if (ok) {
                    stringAddC(QChar(tmp));
                } else {
                    reportParseError(QLatin1String(XMLERR_ERRORPARSINGREFERENCE));
                    return false;
                }
                parseReference_charDataRead = true;
                next();
                break;
            case DoneN:
                if (!processReference())
                    return false;
                next();
                break;
        }
    }
    return false;
}

/*
  Helper function for parseReference()
*/
bool QXmlSimpleReaderPrivate::processReference()
{
    QString reference = ref();
    if (reference == QLatin1String("amp")) {
        if (parseReference_context == InEntityValue) {
            // Bypassed
            stringAddC(QLatin1Char('&')); stringAddC(QLatin1Char('a')); stringAddC(QLatin1Char('m')); stringAddC(QLatin1Char('p')); stringAddC(QLatin1Char(';'));
        } else {
            // Included or Included in literal
            stringAddC(QLatin1Char('&'));
        }
        parseReference_charDataRead = true;
    } else if (reference == QLatin1String("lt")) {
        if (parseReference_context == InEntityValue) {
            // Bypassed
            stringAddC(QLatin1Char('&')); stringAddC(QLatin1Char('l')); stringAddC(QLatin1Char('t')); stringAddC(QLatin1Char(';'));
        } else {
            // Included or Included in literal
            stringAddC(QLatin1Char('<'));
        }
        parseReference_charDataRead = true;
    } else if (reference == QLatin1String("gt")) {
        if (parseReference_context == InEntityValue) {
            // Bypassed
            stringAddC(QLatin1Char('&')); stringAddC(QLatin1Char('g')); stringAddC(QLatin1Char('t')); stringAddC(QLatin1Char(';'));
        } else {
            // Included or Included in literal
            stringAddC(QLatin1Char('>'));
        }
        parseReference_charDataRead = true;
    } else if (reference == QLatin1String("apos")) {
        if (parseReference_context == InEntityValue) {
            // Bypassed
            stringAddC(QLatin1Char('&')); stringAddC(QLatin1Char('a')); stringAddC(QLatin1Char('p')); stringAddC(QLatin1Char('o')); stringAddC(QLatin1Char('s')); stringAddC(QLatin1Char(';'));
        } else {
            // Included or Included in literal
            stringAddC(QLatin1Char('\''));
        }
        parseReference_charDataRead = true;
    } else if (reference == QLatin1String("quot")) {
        if (parseReference_context == InEntityValue) {
            // Bypassed
            stringAddC(QLatin1Char('&')); stringAddC(QLatin1Char('q')); stringAddC(QLatin1Char('u')); stringAddC(QLatin1Char('o')); stringAddC(QLatin1Char('t')); stringAddC(QLatin1Char(';'));
        } else {
            // Included or Included in literal
            stringAddC(QLatin1Char('"'));
        }
        parseReference_charDataRead = true;
    } else {
        QMap<QString,QString>::Iterator it;
        it = entities.find(reference);
        if (it != entities.end()) {
            // "Internal General"
            switch (parseReference_context) {
                case InContent:
                    // Included
                    if (!insertXmlRef(*it, reference, false))
                        return false;
                    parseReference_charDataRead = false;
                    break;
                case InAttributeValue:
                    // Included in literal
                    if (!insertXmlRef(*it, reference, true))
                        return false;
                    parseReference_charDataRead = false;
                    break;
                case InEntityValue:
                    {
                        // Bypassed
                        stringAddC(QLatin1Char('&'));
                        for (int i=0; i<(int)reference.length(); i++) {
                            stringAddC(reference[i]);
                        }
                        stringAddC(QLatin1Char(';'));
                        parseReference_charDataRead = true;
                    }
                    break;
                case InDTD:
                    // Forbidden
                    parseReference_charDataRead = false;
                    reportParseError(QLatin1String(XMLERR_INTERNALGENERALENTITYINDTD));
                    return false;
            }
        } else {
            QMap<QString,QXmlSimpleReaderPrivate::ExternEntity>::Iterator itExtern;
            itExtern = externEntities.find(reference);
            if (itExtern == externEntities.end()) {
                // entity not declared
                // ### check this case for conformance
                if (parseReference_context == InEntityValue) {
                    // Bypassed
                    stringAddC(QLatin1Char('&'));
                    for (int i=0; i<(int)reference.length(); i++) {
                        stringAddC(reference[i]);
                    }
                    stringAddC(QLatin1Char(';'));
                    parseReference_charDataRead = true;
                } else {
                    // if we have some char data read, report it now
                    if (parseReference_context == InContent) {
                        if (contentCharDataRead) {
                            if (reportWhitespaceCharData || !string().simplified().isEmpty()) {
                                if (contentHnd != 0 && !contentHnd->characters(string())) {
                                    reportParseError(contentHnd->errorString());
                                    return false;
                                }
                            }
                            stringClear();
                            contentCharDataRead = false;
                        }
                    }

                    if (contentHnd) {
                        qt_xml_skipped_entity_in_content = parseReference_context == InContent;
                        if (!contentHnd->skippedEntity(reference)) {
                            qt_xml_skipped_entity_in_content = false;
                            reportParseError(contentHnd->errorString());
                            return false; // error
                        }
                        qt_xml_skipped_entity_in_content = false;
                    }
                }
            } else if ((*itExtern).notation.isNull()) {
                // "External Parsed General"
                switch (parseReference_context) {
                    case InContent:
                        {
                            // Included if validating
                            bool skipIt = true;
                            if (entityRes) {
                                QXmlInputSource *ret = 0;
                                if (!entityRes->resolveEntity((*itExtern).publicId, (*itExtern).systemId, ret)) {
                                    delete ret;
                                    reportParseError(entityRes->errorString());
                                    return false;
                                }
                                if (ret) {
                                    QString xmlRefString;
                                    QString buffer = ret->data();
                                    while (buffer.length()>0){
                                        xmlRefString += buffer;
                                        ret->fetchData();
                                        buffer = ret->data();
                                    }
                                    delete ret;
                                    if (!stripTextDecl(xmlRefString)) {
                                        reportParseError(QLatin1String(XMLERR_ERRORINTEXTDECL));
                                        return false;
                                    }
                                    if (!insertXmlRef(xmlRefString, reference, false))
                                        return false;
                                    skipIt = false;
                                }
                            }
                            if (skipIt && contentHnd) {
                                qt_xml_skipped_entity_in_content = true;
                                if (!contentHnd->skippedEntity(reference)) {
                                    qt_xml_skipped_entity_in_content = false;
                                    reportParseError(contentHnd->errorString());
                                    return false; // error
                                }
                                qt_xml_skipped_entity_in_content = false;
                            }
                            parseReference_charDataRead = false;
                        } break;
                    case InAttributeValue:
                        // Forbidden
                        parseReference_charDataRead = false;
                        reportParseError(QLatin1String(XMLERR_EXTERNALGENERALENTITYINAV));
                        return false;
                    case InEntityValue:
                        {
                            // Bypassed
                            stringAddC(QLatin1Char('&'));
                            for (int i=0; i<(int)reference.length(); i++) {
                                stringAddC(reference[i]);
                            }
                            stringAddC(QLatin1Char(';'));
                            parseReference_charDataRead = true;
                        }
                        break;
                    case InDTD:
                        // Forbidden
                        parseReference_charDataRead = false;
                        reportParseError(QLatin1String(XMLERR_EXTERNALGENERALENTITYINDTD));
                        return false;
                }
            } else {
                // "Unparsed"
                // ### notify for "Occurs as Attribute Value" missing (but this is no refence, anyway)
                // Forbidden
                parseReference_charDataRead = false;
                reportParseError(QLatin1String(XMLERR_UNPARSEDENTITYREFERENCE));
                return false; // error
            }
        }
    }
    return true; // no error
}


/*
  Parses over a simple string.

  After the string was successfully parsed, the head is on the first
  character after the string.
*/
bool QXmlSimpleReaderPrivate::parseString()
{
    const signed char InpCharExpected  = 0; // the character that was expected
    const signed char InpUnknown       = 1;

    signed char state; // state in this function is the position in the string s
    signed char input;

    if (parseStack==0 || parseStack->isEmpty()) {
        Done = parseString_s.length();
        state = 0;
    } else {
        state = parseStack->pop().state;
#if defined(QT_QXML_DEBUG)
        qDebug("QXmlSimpleReader: parseString (cont) in state %d", state);
#endif
        if (!parseStack->isEmpty()) {
            ParseFunction function = parseStack->top().function;
            if (function == &QXmlSimpleReaderPrivate::eat_ws) {
                parseStack->pop();
#if defined(QT_QXML_DEBUG)
                qDebug("QXmlSimpleReader: eat_ws (cont)");
#endif
            }
            if (!(this->*function)()) {
                parseFailed(&QXmlSimpleReaderPrivate::parseString, state);
                return false;
            }
        }
    }

    for (;;) {
        if (state == Done) {
            return true;
        }

        if (atEnd()) {
            unexpectedEof(&QXmlSimpleReaderPrivate::parseString, state);
            return false;
        }
        if (c == parseString_s[(int)state]) {
            input = InpCharExpected;
        } else {
            input = InpUnknown;
        }
        if (input == InpCharExpected) {
            state++;
        } else {
            // Error
            reportParseError(QLatin1String(XMLERR_UNEXPECTEDCHARACTER));
            return false;
        }

        next();
    }
    return false;
}

/*
  This private function inserts and reports an entity substitution. The
  substituted string is \a data and the name of the entity reference is \a
  name. If \a inLiteral is true, the entity is IncludedInLiteral (i.e., " and '
  must be quoted. Otherwise they are not quoted.

  This function returns false on error.
*/
bool QXmlSimpleReaderPrivate::insertXmlRef(const QString &data, const QString &name, bool inLiteral)
{
    if (inLiteral) {
        QString tmp = data;
        xmlRefStack.push(XmlRef(name, tmp.replace(QLatin1Char('\"'),
                            QLatin1String("&quot;")).replace(QLatin1Char('\''), QLatin1String("&apos;"))));
    } else {
        xmlRefStack.push(XmlRef(name, data));
    }
    int n = qMax(parameterEntities.count(), entities.count());
    if (xmlRefStack.count() > n+1) {
        // recursive entities
        reportParseError(QLatin1String(XMLERR_RECURSIVEENTITIES));
        return false;
    }
    if (reportEntities && lexicalHnd) {
        if (!lexicalHnd->startEntity(name)) {
            reportParseError(lexicalHnd->errorString());
            return false;
        }
    }
    return true;
}

/*
  This private function moves the cursor to the next character.
*/
void QXmlSimpleReaderPrivate::next()
{
    int count = xmlRefStack.size();
    while (count != 0) {
        if (xmlRefStack.top().isEmpty()) {
            xmlRefStack.pop_back();
            count--;
        } else {
            c = xmlRefStack.top().next();
            return;
        }
    }

    // the following could be written nicer, but since it is a time-critical
    // function, rather optimize for speed
    ushort uc = c.unicode();
    c = inputSource->next();
    // If we are not incremental parsing, we just skip over EndOfData chars to give the
    // parser an uninterrupted stream of document chars.
    if (c == QXmlInputSource::EndOfData && parseStack == 0)
        c = inputSource->next();
    if (uc == '\n') {
        lineNr++;
        columnNr = -1;
    } else if (uc == '\r') {
        if (c != QLatin1Char('\n')) {
            lineNr++;
            columnNr = -1;
        }
    }
    ++columnNr;
}

/*
  This private function moves the cursor to the next non-whitespace character.
  This function does not move the cursor if the actual cursor position is a
  non-whitespace charcter.

  Returns false when you use incremental parsing and this function reaches EOF
  with reading only whitespace characters. In this case it also poplulates the
  parseStack with useful information. In all other cases, this function returns
  true.
*/
bool QXmlSimpleReaderPrivate::eat_ws()
{
    while (!atEnd()) {
        if (!is_S(c)) {
            return true;
        }
        next();
    }
    if (parseStack != 0) {
        unexpectedEof(&QXmlSimpleReaderPrivate::eat_ws, 0);
        return false;
    }
    return true;
}

bool QXmlSimpleReaderPrivate::next_eat_ws()
{
    next();
    return eat_ws();
}


/*
  This private function initializes the reader. \a i is the input source to
  read the data from.
*/
void QXmlSimpleReaderPrivate::init(const QXmlInputSource *i)
{
    lineNr = 0;
    columnNr = -1;
    inputSource = const_cast<QXmlInputSource *>(i);
    initData();

    externParameterEntities.clear();
    parameterEntities.clear();
    externEntities.clear();
    entities.clear();

    tags.clear();

    doctype.clear();
    xmlVersion.clear();
    encoding.clear();
    standalone = QXmlSimpleReaderPrivate::Unknown;
    error.clear();
}

/*
  This private function initializes the XML data related variables. Especially,
  it reads the data from the input source.
*/
void QXmlSimpleReaderPrivate::initData()
{
    c = QXmlInputSource::EndOfData;
    xmlRefStack.clear();
    next();
}

/*
  Returns true if a entity with the name \a e exists,
  otherwise returns false.
*/
bool QXmlSimpleReaderPrivate::entityExist(const QString& e) const
{
    if ( parameterEntities.find(e) == parameterEntities.end() &&
          externParameterEntities.find(e) == externParameterEntities.end() &&
          externEntities.find(e) == externEntities.end() &&
          entities.find(e) == entities.end()) {
        return false;
    } else {
        return true;
    }
}

void QXmlSimpleReaderPrivate::reportParseError(const QString& error)
{
    this->error = error;
    if (errorHnd) {
        if (this->error.isNull()) {
            const QXmlParseException ex(QLatin1String(XMLERR_OK), columnNr+1, lineNr+1,
                                        thisPublicId, thisSystemId);
            errorHnd->fatalError(ex);
        } else {
            const QXmlParseException ex(this->error, columnNr+1, lineNr+1,
                                        thisPublicId, thisSystemId);
            errorHnd->fatalError(ex);
        }
    }
}

/*
  This private function is called when a parsing function encounters an
  unexpected EOF. It decides what to do (depending on incremental parsing or
  not). \a where is a pointer to the function where the error occurred and \a
  state is the parsing state in this function.
*/
void QXmlSimpleReaderPrivate::unexpectedEof(ParseFunction where, int state)
{
    if (parseStack == 0) {
        reportParseError(QLatin1String(XMLERR_UNEXPECTEDEOF));
    } else {
        if (c == QXmlInputSource::EndOfDocument) {
            reportParseError(QLatin1String(XMLERR_UNEXPECTEDEOF));
        } else {
            pushParseState(where, state);
        }
    }
}

/*
  This private function is called when a parse...() function returned false. It
  determines if there was an error or if incremental parsing simply went out of
  data and does the right thing for the case. \a where is a pointer to the
  function where the error occurred and \a state is the parsing state in this
  function.
*/
void QXmlSimpleReaderPrivate::parseFailed(ParseFunction where, int state)
{
    if (parseStack!=0 && error.isNull()) {
        pushParseState(where, state);
    }
}

/*
  This private function pushes the function pointer \a function and state \a
  state to the parse stack. This is used when you are doing an incremental
  parsing and reach the end of file too early.

  Only call this function when d->parseStack!=0.
*/
void QXmlSimpleReaderPrivate::pushParseState(ParseFunction function, int state)
{
    QXmlSimpleReaderPrivate::ParseState ps;
    ps.function = function;
    ps.state = state;
    parseStack->push(ps);
}

inline static void updateValue(QString &value, const QChar *array, int &arrayPos, int &valueLen)
{
    value.resize(valueLen + arrayPos);
    memcpy(value.data() + valueLen, array, arrayPos * sizeof(QChar));
    valueLen += arrayPos;
    arrayPos = 0;
}

// use buffers instead of QString::operator+= when single characters are read
const QString& QXmlSimpleReaderPrivate::string()
{
    updateValue(stringValue, stringArray, stringArrayPos, stringValueLen);
    return stringValue;
}
const QString& QXmlSimpleReaderPrivate::name()
{
    updateValue(nameValue, nameArray, nameArrayPos, nameValueLen);
    return nameValue;
}
const QString& QXmlSimpleReaderPrivate::ref()
{
    updateValue(refValue, refArray, refArrayPos, refValueLen);
    return refValue;
}

void QXmlSimpleReaderPrivate::stringAddC(QChar ch)
{
    if (stringArrayPos == 256)
        updateValue(stringValue, stringArray, stringArrayPos, stringValueLen);
    stringArray[stringArrayPos++] = ch;
}
void QXmlSimpleReaderPrivate::nameAddC(QChar ch)
{
    if (nameArrayPos == 256)
        updateValue(nameValue, nameArray, nameArrayPos, nameValueLen);
    nameArray[nameArrayPos++] = ch;
}
void QXmlSimpleReaderPrivate::refAddC(QChar ch)
{
    if (refArrayPos == 256)
        updateValue(refValue, refArray, refArrayPos, refValueLen);
    refArray[refArrayPos++] = ch;
}
QT_END_NAMESPACE
