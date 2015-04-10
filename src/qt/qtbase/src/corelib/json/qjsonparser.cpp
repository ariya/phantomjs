/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013 Intel Corporation
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QT_BOOTSTRAPPED
#include <qcoreapplication.h>
#endif
#include <qdebug.h>
#include "qjsonparser_p.h"
#include "qjson_p.h"
#include "private/qutfcodec_p.h"

//#define PARSER_DEBUG
#ifdef PARSER_DEBUG
static int indent = 0;
#define BEGIN qDebug() << QByteArray(4*indent++, ' ').constData() << "pos=" << current
#define END --indent
#define DEBUG qDebug() << QByteArray(4*indent, ' ').constData()
#else
#define BEGIN if (1) ; else qDebug()
#define END do {} while (0)
#define DEBUG if (1) ; else qDebug()
#endif

static const int nestingLimit = 1024;

QT_BEGIN_NAMESPACE

// error strings for the JSON parser
#define JSONERR_OK          QT_TRANSLATE_NOOP("QJsonParseError", "no error occurred")
#define JSONERR_UNTERM_OBJ  QT_TRANSLATE_NOOP("QJsonParseError", "unterminated object")
#define JSONERR_MISS_NSEP   QT_TRANSLATE_NOOP("QJsonParseError", "missing name separator")
#define JSONERR_UNTERM_AR   QT_TRANSLATE_NOOP("QJsonParseError", "unterminated array")
#define JSONERR_MISS_VSEP   QT_TRANSLATE_NOOP("QJsonParseError", "missing value separator")
#define JSONERR_ILLEGAL_VAL QT_TRANSLATE_NOOP("QJsonParseError", "illegal value")
#define JSONERR_END_OF_NUM  QT_TRANSLATE_NOOP("QJsonParseError", "invalid termination by number")
#define JSONERR_ILLEGAL_NUM QT_TRANSLATE_NOOP("QJsonParseError", "illegal number")
#define JSONERR_STR_ESC_SEQ QT_TRANSLATE_NOOP("QJsonParseError", "invalid escape sequence")
#define JSONERR_STR_UTF8    QT_TRANSLATE_NOOP("QJsonParseError", "invalid UTF8 string")
#define JSONERR_UTERM_STR   QT_TRANSLATE_NOOP("QJsonParseError", "unterminated string")
#define JSONERR_MISS_OBJ    QT_TRANSLATE_NOOP("QJsonParseError", "object is missing after a comma")
#define JSONERR_DEEP_NEST   QT_TRANSLATE_NOOP("QJsonParseError", "too deeply nested document")
#define JSONERR_DOC_LARGE   QT_TRANSLATE_NOOP("QJsonParseError", "too large document")

/*!
    \class QJsonParseError
    \inmodule QtCore
    \ingroup json
    \reentrant
    \since 5.0

    \brief The QJsonParseError class is used to report errors during JSON parsing.

    \sa {JSON Support in Qt}, {JSON Save Game Example}
*/

/*!
    \enum QJsonParseError::ParseError

    This enum describes the type of error that occurred during the parsing of a JSON document.

    \value NoError                  No error occurred
    \value UnterminatedObject       An object is not correctly terminated with a closing curly bracket
    \value MissingNameSeparator     A comma separating different items is missing
    \value UnterminatedArray        The array is not correctly terminated with a closing square bracket
    \value MissingValueSeparator    A colon separating keys from values inside objects is missing
    \value IllegalValue             The value is illegal
    \value TerminationByNumber      The input stream ended while parsing a number
    \value IllegalNumber            The number is not well formed
    \value IllegalEscapeSequence    An illegal escape sequence occurred in the input
    \value IllegalUTF8String        An illegal UTF8 sequence occurred in the input
    \value UnterminatedString       A string wasn't terminated with a quote
    \value MissingObject            An object was expected but couldn't be found
    \value DeepNesting              The JSON document is too deeply nested for the parser to parse it
    \value DocumentTooLarge         The JSON document is too large for the parser to parse it
*/

/*!
    \variable QJsonParseError::error

    Contains the type of the parse error. Is equal to QJsonParseError::NoError if the document
    was parsed correctly.

    \sa ParseError, errorString()
*/


/*!
    \variable QJsonParseError::offset

    Contains the offset in the input string where the parse error occurred.

    \sa error, errorString()
*/

/*!
  Returns the human-readable message appropriate to the reported JSON parsing error.

  \sa error
 */
QString QJsonParseError::errorString() const
{
    const char *sz = "";
    switch (error) {
    case NoError:
        sz = JSONERR_OK;
        break;
    case UnterminatedObject:
        sz = JSONERR_UNTERM_OBJ;
        break;
    case MissingNameSeparator:
        sz = JSONERR_MISS_NSEP;
        break;
    case UnterminatedArray:
        sz = JSONERR_UNTERM_AR;
        break;
    case MissingValueSeparator:
        sz = JSONERR_MISS_VSEP;
        break;
    case IllegalValue:
        sz = JSONERR_ILLEGAL_VAL;
        break;
    case TerminationByNumber:
        sz = JSONERR_END_OF_NUM;
        break;
    case IllegalNumber:
        sz = JSONERR_ILLEGAL_NUM;
        break;
    case IllegalEscapeSequence:
        sz = JSONERR_STR_ESC_SEQ;
        break;
    case IllegalUTF8String:
        sz = JSONERR_STR_UTF8;
        break;
    case UnterminatedString:
        sz = JSONERR_UTERM_STR;
        break;
    case MissingObject:
        sz = JSONERR_MISS_OBJ;
        break;
    case DeepNesting:
        sz = JSONERR_DEEP_NEST;
        break;
    case DocumentTooLarge:
        sz = JSONERR_DOC_LARGE;
        break;
    }
#ifndef QT_BOOTSTRAPPED
    return QCoreApplication::translate("QJsonParseError", sz);
#else
    return QLatin1String(sz);
#endif
}

using namespace QJsonPrivate;

Parser::Parser(const char *json, int length)
    : head(json), json(json), data(0), dataLength(0), current(0), nestingLevel(0), lastError(QJsonParseError::NoError)
{
    end = json + length;
}



/*

begin-array     = ws %x5B ws  ; [ left square bracket

begin-object    = ws %x7B ws  ; { left curly bracket

end-array       = ws %x5D ws  ; ] right square bracket

end-object      = ws %x7D ws  ; } right curly bracket

name-separator  = ws %x3A ws  ; : colon

value-separator = ws %x2C ws  ; , comma

Insignificant whitespace is allowed before or after any of the six
structural characters.

ws = *(
          %x20 /              ; Space
          %x09 /              ; Horizontal tab
          %x0A /              ; Line feed or New line
          %x0D                ; Carriage return
      )

*/

enum {
    Space = 0x20,
    Tab = 0x09,
    LineFeed = 0x0a,
    Return = 0x0d,
    BeginArray = 0x5b,
    BeginObject = 0x7b,
    EndArray = 0x5d,
    EndObject = 0x7d,
    NameSeparator = 0x3a,
    ValueSeparator = 0x2c,
    Quote = 0x22
};

void Parser::eatBOM()
{
    // eat UTF-8 byte order mark
    uchar utf8bom[3] = { 0xef, 0xbb, 0xbf };
    if (end - json > 3 &&
        (uchar)json[0] == utf8bom[0] &&
        (uchar)json[1] == utf8bom[1] &&
        (uchar)json[2] == utf8bom[2])
        json += 3;
}

bool Parser::eatSpace()
{
    while (json < end) {
        if (*json > Space)
            break;
        if (*json != Space &&
            *json != Tab &&
            *json != LineFeed &&
            *json != Return)
            break;
        ++json;
    }
    return (json < end);
}

char Parser::nextToken()
{
    if (!eatSpace())
        return 0;
    char token = *json++;
    switch (token) {
    case BeginArray:
    case BeginObject:
    case NameSeparator:
    case ValueSeparator:
    case EndArray:
    case EndObject:
        eatSpace();
    case Quote:
        break;
    default:
        token = 0;
        break;
    }
    return token;
}

/*
    JSON-text = object / array
*/
QJsonDocument Parser::parse(QJsonParseError *error)
{
#ifdef PARSER_DEBUG
    indent = 0;
    qDebug() << ">>>>> parser begin";
#endif
    // allocate some space
    dataLength = qMax(end - json, (ptrdiff_t) 256);
    data = (char *)malloc(dataLength);

    // fill in Header data
    QJsonPrivate::Header *h = (QJsonPrivate::Header *)data;
    h->tag = QJsonDocument::BinaryFormatTag;
    h->version = 1u;

    current = sizeof(QJsonPrivate::Header);

    eatBOM();
    char token = nextToken();

    DEBUG << hex << (uint)token;
    if (token == BeginArray) {
        if (!parseArray())
            goto error;
    } else if (token == BeginObject) {
        if (!parseObject())
            goto error;
    } else {
        lastError = QJsonParseError::IllegalValue;
        goto error;
    }

    END;
    {
        if (error) {
            error->offset = 0;
            error->error = QJsonParseError::NoError;
        }
        QJsonPrivate::Data *d = new QJsonPrivate::Data(data, current);
        return QJsonDocument(d);
    }

error:
#ifdef PARSER_DEBUG
    qDebug() << ">>>>> parser error";
#endif
    if (error) {
        error->offset = json - head;
        error->error  = lastError;
    }
    free(data);
    return QJsonDocument();
}


void Parser::ParsedObject::insert(uint offset) {
    const QJsonPrivate::Entry *newEntry = reinterpret_cast<const QJsonPrivate::Entry *>(parser->data + objectPosition + offset);
    int min = 0;
    int n = offsets.size();
    while (n > 0) {
        int half = n >> 1;
        int middle = min + half;
        if (*entryAt(middle) >= *newEntry) {
            n = half;
        } else {
            min = middle + 1;
            n -= half + 1;
        }
    }
    if (min < offsets.size() && *entryAt(min) == *newEntry) {
        offsets[min] = offset;
    } else {
        offsets.insert(min, offset);
    }
}

/*
    object = begin-object [ member *( value-separator member ) ]
    end-object
*/

bool Parser::parseObject()
{
    if (++nestingLevel > nestingLimit) {
        lastError = QJsonParseError::DeepNesting;
        return false;
    }

    int objectOffset = reserveSpace(sizeof(QJsonPrivate::Object));
    BEGIN << "parseObject pos=" << objectOffset << current << json;

    ParsedObject parsedObject(this, objectOffset);

    char token = nextToken();
    while (token == Quote) {
        int off = current - objectOffset;
        if (!parseMember(objectOffset))
            return false;
        parsedObject.insert(off);
        token = nextToken();
        if (token != ValueSeparator)
            break;
        token = nextToken();
        if (token == EndObject) {
            lastError = QJsonParseError::MissingObject;
            return false;
        }
    }

    DEBUG << "end token=" << token;
    if (token != EndObject) {
        lastError = QJsonParseError::UnterminatedObject;
        return false;
    }

    DEBUG << "numEntries" << parsedObject.offsets.size();
    int table = objectOffset;
    // finalize the object
    if (parsedObject.offsets.size()) {
        int tableSize = parsedObject.offsets.size()*sizeof(uint);
        table = reserveSpace(tableSize);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        memcpy(data + table, parsedObject.offsets.constData(), tableSize);
#else
        offset *o = (offset *)(data + table);
        for (int i = 0; i < parsedObject.offsets.size(); ++i)
            o[i] = parsedObject.offsets[i];

#endif
    }

    QJsonPrivate::Object *o = (QJsonPrivate::Object *)(data + objectOffset);
    o->tableOffset = table - objectOffset;
    o->size = current - objectOffset;
    o->is_object = true;
    o->length = parsedObject.offsets.size();

    DEBUG << "current=" << current;
    END;

    --nestingLevel;
    return true;
}

/*
    member = string name-separator value
*/
bool Parser::parseMember(int baseOffset)
{
    int entryOffset = reserveSpace(sizeof(QJsonPrivate::Entry));
    BEGIN << "parseMember pos=" << entryOffset;

    bool latin1;
    if (!parseString(&latin1))
        return false;
    char token = nextToken();
    if (token != NameSeparator) {
        lastError = QJsonParseError::MissingNameSeparator;
        return false;
    }
    QJsonPrivate::Value val;
    if (!parseValue(&val, baseOffset))
        return false;

    // finalize the entry
    QJsonPrivate::Entry *e = (QJsonPrivate::Entry *)(data + entryOffset);
    e->value = val;
    e->value.latinKey = latin1;

    END;
    return true;
}

/*
    array = begin-array [ value *( value-separator value ) ] end-array
*/
bool Parser::parseArray()
{
    BEGIN << "parseArray";

    if (++nestingLevel > nestingLimit) {
        lastError = QJsonParseError::DeepNesting;
        return false;
    }

    int arrayOffset = reserveSpace(sizeof(QJsonPrivate::Array));

    QVarLengthArray<QJsonPrivate::Value, 64> values;

    if (!eatSpace()) {
        lastError = QJsonParseError::UnterminatedArray;
        return false;
    }
    if (*json == EndArray) {
        nextToken();
    } else {
        while (1) {
            QJsonPrivate::Value val;
            if (!parseValue(&val, arrayOffset))
                return false;
            values.append(val);
            char token = nextToken();
            if (token == EndArray)
                break;
            else if (token != ValueSeparator) {
                if (!eatSpace())
                    lastError = QJsonParseError::UnterminatedArray;
                else
                    lastError = QJsonParseError::MissingValueSeparator;
                return false;
            }
        }
    }

    DEBUG << "size =" << values.size();
    int table = arrayOffset;
    // finalize the object
    if (values.size()) {
        int tableSize = values.size()*sizeof(QJsonPrivate::Value);
        table = reserveSpace(tableSize);
        memcpy(data + table, values.constData(), tableSize);
    }

    QJsonPrivate::Array *a = (QJsonPrivate::Array *)(data + arrayOffset);
    a->tableOffset = table - arrayOffset;
    a->size = current - arrayOffset;
    a->is_object = false;
    a->length = values.size();

    DEBUG << "current=" << current;
    END;

    --nestingLevel;
    return true;
}

/*
value = false / null / true / object / array / number / string

*/

bool Parser::parseValue(QJsonPrivate::Value *val, int baseOffset)
{
    BEGIN << "parse Value" << json;
    val->_dummy = 0;

    switch (*json++) {
    case 'n':
        if (end - json < 4) {
            lastError = QJsonParseError::IllegalValue;
            return false;
        }
        if (*json++ == 'u' &&
            *json++ == 'l' &&
            *json++ == 'l') {
            val->type = QJsonValue::Null;
            DEBUG << "value: null";
            END;
            return true;
        }
        lastError = QJsonParseError::IllegalValue;
        return false;
    case 't':
        if (end - json < 4) {
            lastError = QJsonParseError::IllegalValue;
            return false;
        }
        if (*json++ == 'r' &&
            *json++ == 'u' &&
            *json++ == 'e') {
            val->type = QJsonValue::Bool;
            val->value = true;
            DEBUG << "value: true";
            END;
            return true;
        }
        lastError = QJsonParseError::IllegalValue;
        return false;
    case 'f':
        if (end - json < 5) {
            lastError = QJsonParseError::IllegalValue;
            return false;
        }
        if (*json++ == 'a' &&
            *json++ == 'l' &&
            *json++ == 's' &&
            *json++ == 'e') {
            val->type = QJsonValue::Bool;
            val->value = false;
            DEBUG << "value: false";
            END;
            return true;
        }
        lastError = QJsonParseError::IllegalValue;
        return false;
    case Quote: {
        val->type = QJsonValue::String;
        if (current - baseOffset >= Value::MaxSize) {
            lastError = QJsonParseError::DocumentTooLarge;
            return false;
        }
        val->value = current - baseOffset;
        bool latin1;
        if (!parseString(&latin1))
            return false;
        val->latinOrIntValue = latin1;
        DEBUG << "value: string";
        END;
        return true;
    }
    case BeginArray:
        val->type = QJsonValue::Array;
        if (current - baseOffset >= Value::MaxSize) {
            lastError = QJsonParseError::DocumentTooLarge;
            return false;
        }
        val->value = current - baseOffset;
        if (!parseArray())
            return false;
        DEBUG << "value: array";
        END;
        return true;
    case BeginObject:
        val->type = QJsonValue::Object;
        if (current - baseOffset >= Value::MaxSize) {
            lastError = QJsonParseError::DocumentTooLarge;
            return false;
        }
        val->value = current - baseOffset;
        if (!parseObject())
            return false;
        DEBUG << "value: object";
        END;
        return true;
    case EndArray:
        lastError = QJsonParseError::MissingObject;
        return false;
    default:
        --json;
        if (!parseNumber(val, baseOffset))
            return false;
        DEBUG << "value: number";
        END;
    }

    return true;
}





/*
        number = [ minus ] int [ frac ] [ exp ]
        decimal-point = %x2E       ; .
        digit1-9 = %x31-39         ; 1-9
        e = %x65 / %x45            ; e E
        exp = e [ minus / plus ] 1*DIGIT
        frac = decimal-point 1*DIGIT
        int = zero / ( digit1-9 *DIGIT )
        minus = %x2D               ; -
        plus = %x2B                ; +
        zero = %x30                ; 0

*/

bool Parser::parseNumber(QJsonPrivate::Value *val, int baseOffset)
{
    BEGIN << "parseNumber" << json;
    val->type = QJsonValue::Double;

    const char *start = json;
    bool isInt = true;

    // minus
    if (json < end && *json == '-')
        ++json;

    // int = zero / ( digit1-9 *DIGIT )
    if (json < end && *json == '0') {
        ++json;
    } else {
        while (json < end && *json >= '0' && *json <= '9')
            ++json;
    }

    // frac = decimal-point 1*DIGIT
    if (json < end && *json == '.') {
        isInt = false;
        ++json;
        while (json < end && *json >= '0' && *json <= '9')
            ++json;
    }

    // exp = e [ minus / plus ] 1*DIGIT
    if (json < end && (*json == 'e' || *json == 'E')) {
        isInt = false;
        ++json;
        if (json < end && (*json == '-' || *json == '+'))
            ++json;
        while (json < end && *json >= '0' && *json <= '9')
            ++json;
    }

    if (json >= end) {
        lastError = QJsonParseError::TerminationByNumber;
        return false;
    }

    QByteArray number(start, json - start);
    DEBUG << "numberstring" << number;

    if (isInt) {
        bool ok;
        int n = number.toInt(&ok);
        if (ok && n < (1<<25) && n > -(1<<25)) {
            val->int_value = n;
            val->latinOrIntValue = true;
            END;
            return true;
        }
    }

    bool ok;
    union {
        quint64 ui;
        double d;
    };
    d = number.toDouble(&ok);

    if (!ok) {
        lastError = QJsonParseError::IllegalNumber;
        return false;
    }

    int pos = reserveSpace(sizeof(double));
    *(quint64 *)(data + pos) = qToLittleEndian(ui);
    if (current - baseOffset >= Value::MaxSize) {
        lastError = QJsonParseError::DocumentTooLarge;
        return false;
    }
    val->value = pos - baseOffset;
    val->latinOrIntValue = false;

    END;
    return true;
}

/*

        string = quotation-mark *char quotation-mark

        char = unescaped /
               escape (
                   %x22 /          ; "    quotation mark  U+0022
                   %x5C /          ; \    reverse solidus U+005C
                   %x2F /          ; /    solidus         U+002F
                   %x62 /          ; b    backspace       U+0008
                   %x66 /          ; f    form feed       U+000C
                   %x6E /          ; n    line feed       U+000A
                   %x72 /          ; r    carriage return U+000D
                   %x74 /          ; t    tab             U+0009
                   %x75 4HEXDIG )  ; uXXXX                U+XXXX

        escape = %x5C              ; \

        quotation-mark = %x22      ; "

        unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
 */
static inline bool addHexDigit(char digit, uint *result)
{
    *result <<= 4;
    if (digit >= '0' && digit <= '9')
        *result |= (digit - '0');
    else if (digit >= 'a' && digit <= 'f')
        *result |= (digit - 'a') + 10;
    else if (digit >= 'A' && digit <= 'F')
        *result |= (digit - 'A') + 10;
    else
        return false;
    return true;
}

static inline bool scanEscapeSequence(const char *&json, const char *end, uint *ch)
{
    ++json;
    if (json >= end)
        return false;

    DEBUG << "scan escape" << (char)*json;
    uint escaped = *json++;
    switch (escaped) {
    case '"':
        *ch = '"'; break;
    case '\\':
        *ch = '\\'; break;
    case '/':
        *ch = '/'; break;
    case 'b':
        *ch = 0x8; break;
    case 'f':
        *ch = 0xc; break;
    case 'n':
        *ch = 0xa; break;
    case 'r':
        *ch = 0xd; break;
    case 't':
        *ch = 0x9; break;
    case 'u': {
        *ch = 0;
        if (json > end - 4)
            return false;
        for (int i = 0; i < 4; ++i) {
            if (!addHexDigit(*json, ch))
                return false;
            ++json;
        }
        return true;
    }
    default:
        // this is not as strict as one could be, but allows for more Json files
        // to be parsed correctly.
        *ch = escaped;
        return true;
    }
    return true;
}

static inline bool scanUtf8Char(const char *&json, const char *end, uint *result)
{
    const uchar *&src = reinterpret_cast<const uchar *&>(json);
    const uchar *uend = reinterpret_cast<const uchar *>(end);
    uchar b = *src++;
    int res = QUtf8Functions::fromUtf8<QUtf8BaseTraits>(b, result, src, uend);
    if (res < 0) {
        // decoding error, backtrack the character we read above
        --json;
        return false;
    }

    return true;
}

bool Parser::parseString(bool *latin1)
{
    *latin1 = true;

    const char *start = json;
    int outStart = current;

    // try to write out a latin1 string

    int stringPos = reserveSpace(2);
    BEGIN << "parse string stringPos=" << stringPos << json;
    while (json < end) {
        uint ch = 0;
        if (*json == '"')
            break;
        else if (*json == '\\') {
            if (!scanEscapeSequence(json, end, &ch)) {
                lastError = QJsonParseError::IllegalEscapeSequence;
                return false;
            }
        } else {
            if (!scanUtf8Char(json, end, &ch)) {
                lastError = QJsonParseError::IllegalUTF8String;
                return false;
            }
        }
        // bail out if the string is not pure latin1 or too long to hold as a latin1string (which has only 16 bit for the length)
        if (ch > 0xff || json - start >= 0x8000) {
            *latin1 = false;
            break;
        }
        int pos = reserveSpace(1);
        DEBUG << "  " << ch << (char)ch;
        data[pos] = (uchar)ch;
    }
    ++json;
    DEBUG << "end of string";
    if (json >= end) {
        lastError = QJsonParseError::UnterminatedString;
        return false;
    }

    // no unicode string, we are done
    if (*latin1) {
        // write string length
        *(QJsonPrivate::qle_ushort *)(data + stringPos) = ushort(current - outStart - sizeof(ushort));
        int pos = reserveSpace((4 - current) & 3);
        while (pos & 3)
            data[pos++] = 0;
        END;
        return true;
    }

    *latin1 = false;
    DEBUG << "not latin";

    json = start;
    current = outStart + sizeof(int);

    while (json < end) {
        uint ch = 0;
        if (*json == '"')
            break;
        else if (*json == '\\') {
            if (!scanEscapeSequence(json, end, &ch)) {
                lastError = QJsonParseError::IllegalEscapeSequence;
                return false;
            }
        } else {
            if (!scanUtf8Char(json, end, &ch)) {
                lastError = QJsonParseError::IllegalUTF8String;
                return false;
            }
        }
        if (QChar::requiresSurrogates(ch)) {
            int pos = reserveSpace(4);
            *(QJsonPrivate::qle_ushort *)(data + pos) = QChar::highSurrogate(ch);
            *(QJsonPrivate::qle_ushort *)(data + pos + 2) = QChar::lowSurrogate(ch);
        } else {
            int pos = reserveSpace(2);
            *(QJsonPrivate::qle_ushort *)(data + pos) = (ushort)ch;
        }
    }
    ++json;

    if (json >= end) {
        lastError = QJsonParseError::UnterminatedString;
        return false;
    }

    // write string length
    *(QJsonPrivate::qle_int *)(data + stringPos) = (current - outStart - sizeof(int))/2;
    int pos = reserveSpace((4 - current) & 3);
    while (pos & 3)
        data[pos++] = 0;
    END;
    return true;
}

QT_END_NAMESPACE
