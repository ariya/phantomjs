/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

/*!
    \class QUrl

    \brief The QUrl class provides a convenient interface for working
    with URLs.

    \reentrant
    \ingroup io
    \ingroup network
    \ingroup shared


    It can parse and construct URLs in both encoded and unencoded
    form. QUrl also has support for internationalized domain names
    (IDNs).

    The most common way to use QUrl is to initialize it via the
    constructor by passing a QString. Otherwise, setUrl() and
    setEncodedUrl() can also be used.

    URLs can be represented in two forms: encoded or unencoded. The
    unencoded representation is suitable for showing to users, but
    the encoded representation is typically what you would send to
    a web server. For example, the unencoded URL
    "http://b\uuml\c{}hler.example.com/List of applicants.xml" would be sent to the server as
    "http://xn--bhler-kva.example.com/List%20of%20applicants.xml",
    and this can be verified by calling the toEncoded() function.

    A URL can also be constructed piece by piece by calling
    setScheme(), setUserName(), setPassword(), setHost(), setPort(),
    setPath(), setEncodedQuery() and setFragment(). Some convenience
    functions are also available: setAuthority() sets the user name,
    password, host and port. setUserInfo() sets the user name and
    password at once.

    Call isValid() to check if the URL is valid. This can be done at
    any point during the constructing of a URL.

    Constructing a query is particularly convenient through the use
    of setQueryItems(), addQueryItem() and removeQueryItem(). Use
    setQueryDelimiters() to customize the delimiters used for
    generating the query string.

    For the convenience of generating encoded URL strings or query
    strings, there are two static functions called
    fromPercentEncoding() and toPercentEncoding() which deal with
    percent encoding and decoding of QStrings.

    Calling isRelative() will tell whether or not the URL is
    relative. A relative URL can be resolved by passing it as argument
    to resolved(), which returns an absolute URL. isParentOf() is used
    for determining whether one URL is a parent of another.

    fromLocalFile() constructs a QUrl by parsing a local
    file path. toLocalFile() converts a URL to a local file path.

    The human readable representation of the URL is fetched with
    toString(). This representation is appropriate for displaying a
    URL to a user in unencoded form. The encoded form however, as
    returned by toEncoded(), is for internal use, passing to web
    servers, mail clients and so on.

    QUrl conforms to the URI specification from
    \l{RFC 3986} (Uniform Resource Identifier: Generic Syntax), and includes
    scheme extensions from \l{RFC 1738} (Uniform Resource Locators). Case
    folding rules in QUrl conform to \l{RFC 3491} (Nameprep: A Stringprep
    Profile for Internationalized Domain Names (IDN)).

    \section2 Character Conversions

    Follow these rules to avoid erroneous character conversion when
    dealing with URLs and strings:

    \list
    \o When creating an QString to contain a URL from a QByteArray or a
       char*, always use QString::fromUtf8().
    \o Favor the use of QUrl::fromEncoded() and QUrl::toEncoded() instead of
       QUrl(string) and QUrl::toString() when converting a QUrl to or from
       a string.
    \endlist

    \sa QUrlInfo
*/

/*!
    \enum QUrl::ParsingMode

    The parsing mode controls the way QUrl parses strings.

    \value TolerantMode QUrl will try to correct some common errors in URLs.
                        This mode is useful when processing URLs entered by
                        users.

    \value StrictMode Only valid URLs are accepted. This mode is useful for
                      general URL validation.

    In TolerantMode, the parser corrects the following invalid input:

    \list

    \o Spaces and "%20": If an encoded URL contains a space, this will be
    replaced with "%20". If a decoded URL contains "%20", this will be
    replaced with a single space before the URL is parsed.

    \o Single "%" characters: Any occurrences of a percent character "%" not
    followed by exactly two hexadecimal characters (e.g., "13% coverage.html")
    will be replaced by "%25".

    \o Reserved and unreserved characters: An encoded URL should only
    contain a few characters as literals; all other characters should
    be percent-encoded. In TolerantMode, these characters will be
    automatically percent-encoded where they are not allowed:
            space / double-quote / "<" / ">" / "[" / "\" /
            "]" / "^" / "`" / "{" / "|" / "}"

    \endlist
*/

/*!
    \enum QUrl::FormattingOption

    The formatting options define how the URL is formatted when written out
    as text.

    \value None The format of the URL is unchanged.
    \value RemoveScheme  The scheme is removed from the URL.
    \value RemovePassword  Any password in the URL is removed.
    \value RemoveUserInfo  Any user information in the URL is removed.
    \value RemovePort      Any specified port is removed from the URL.
    \value RemoveAuthority
    \value RemovePath   The URL's path is removed, leaving only the scheme,
                        host address, and port (if present).
    \value RemoveQuery  The query part of the URL (following a '?' character)
                        is removed.
    \value RemoveFragment
    \value StripTrailingSlash  The trailing slash is removed if one is present.

    Note that the case folding rules in \l{RFC 3491}{Nameprep}, which QUrl
    conforms to, require host names to always be converted to lower case,
    regardless of the Qt::FormattingOptions used.
*/

/*!
 \fn uint qHash(const QUrl &url)
 \since 4.7
 \relates QUrl

 Computes a hash key from the normalized version of \a url.
 */
#include "qplatformdefs.h"
#include "qurl.h"
#include "qatomic.h"
#include "qbytearray.h"
#include "qdir.h"
#include "qfile.h"
#include "qlist.h"
#ifndef QT_NO_REGEXP
#include "qregexp.h"
#endif
#include "qstring.h"
#include "qstringlist.h"
#include "qstack.h"
#include "qvarlengtharray.h"
#include "qdebug.h"
#if defined QT3_SUPPORT
#include "qfileinfo.h"
#endif
#ifndef QT_BOOTSTRAPPED
#include "qtldurl_p.h"
#endif
#if defined(Q_OS_WINCE_WM)
#pragma optimize("g", off)
#endif
#include "qmutex.h"
#include "private/qorderedmutexlocker_p.h"

QT_BEGIN_NAMESPACE

extern void q_normalizePercentEncoding(QByteArray *ba, const char *exclude);
extern void q_toPercentEncoding(QByteArray *ba, const char *exclude, const char *include = 0);
extern void q_fromPercentEncoding(QByteArray *ba);

static QByteArray toPercentEncodingHelper(const QString &s, const char *exclude, const char *include = 0)
{
    if (s.isNull())
        return QByteArray();    // null
    QByteArray ba = s.toUtf8();
    q_toPercentEncoding(&ba, exclude, include);
    return ba;
}

static QString fromPercentEncodingHelper(const QByteArray &ba)
{
    if (ba.isNull())
        return QString();       // null
    QByteArray copy = ba;
    q_fromPercentEncoding(&copy);
    return QString::fromUtf8(copy.constData(), copy.length());
}

static QString fromPercentEncodingMutable(QByteArray *ba)
{
    if (ba->isNull())
        return QString();       // null
    q_fromPercentEncoding(ba);
    return QString::fromUtf8(ba->constData(), ba->length());
}

// ### Qt 5: Consider accepting empty strings as valid. See task 144227.

//#define QURL_DEBUG

// implemented in qvsnprintf.cpp
Q_CORE_EXPORT int qsnprintf(char *str, size_t n, const char *fmt, ...);

// needed by the punycode encoder/decoder
#define Q_MAXINT ((uint)((uint)(-1)>>1))
static const uint base = 36;
static const uint tmin = 1;
static const uint tmax = 26;
static const uint skew = 38;
static const uint damp = 700;
static const uint initial_bias = 72;
static const uint initial_n = 128;

#define QURL_SETFLAG(a, b) { (a) |= (b); }
#define QURL_UNSETFLAG(a, b) { (a) &= ~(b); }
#define QURL_HASFLAG(a, b) (((a) & (b)) == (b))

struct QUrlErrorInfo {
    inline QUrlErrorInfo() : _source(0), _message(0), _expected(0), _found(0)
    { }

    const char *_source;
    const char *_message;
    char _expected;
    char _found;

    inline void setParams(const char *source, const char *message, char expected, char found)
    {
        _source = source;
        _message = message;
        _expected = expected;
        _found = found;
    }
};

struct QUrlParseData
{
    const char *scheme;
    int schemeLength;

    const char *userInfo;
    int userInfoDelimIndex;
    int userInfoLength;

    const char *host;
    int hostLength;
    int port;

    const char *path;
    int pathLength;
    const char *query;
    int queryLength;
    const char *fragment;
    int fragmentLength;
};


class QUrlPrivate
{
public:
    QUrlPrivate();
    QUrlPrivate(const QUrlPrivate &other);

    bool setUrl(const QString &url);

    QString canonicalHost() const;
    void ensureEncodedParts() const;
    QString authority(QUrl::FormattingOptions options = QUrl::None) const;
    void setAuthority(const QString &auth);
    void setUserInfo(const QString &userInfo);
    QString userInfo(QUrl::FormattingOptions options = QUrl::None) const;
    void setEncodedAuthority(const QByteArray &authority);
    void setEncodedUserInfo(const QUrlParseData *parseData);
    void setEncodedUrl(const QByteArray &encodedUrl, QUrl::ParsingMode parsingMode);
    QString fragmentImpl() const;

    QByteArray mergePaths(const QByteArray &relativePath) const;

    void queryItem(int pos, int *value, int *end);

    enum ParseOptions {
        ParseAndSet,
        ParseOnly
    };

    void validate() const;
    void parse(ParseOptions parseOptions = ParseAndSet) const;
    void clear();

    QByteArray toEncoded(QUrl::FormattingOptions options = QUrl::None) const;

    QAtomicInt ref;

    QString scheme;
    QString userName;
    QString password;
    QString host;
    QString path;
    QByteArray query;
    QString fragment;

    QByteArray encodedOriginal;
    QByteArray encodedUserName;
    QByteArray encodedPassword;
    QByteArray encodedPath;
    QByteArray encodedFragment;

    int port;
    QUrl::ParsingMode parsingMode;

    bool hasQuery;
    bool hasFragment;
    bool isValid;
    bool isHostValid;

    char valueDelimiter;
    char pairDelimiter;

    enum State {
        Parsed = 0x1,
        Validated = 0x2,
        Normalized = 0x4,
        HostCanonicalized = 0x8
    };
    int stateFlags;

    // This mutex protects stateFlags, errorInfo, host,
    // hasQuery, hasFragment, isValid, isHostValid,
    // encodedNormalized and all other encoded* variables above.
    // This also protects all other variables when set by parse,
    // but not when set by setters.
    mutable QMutex mutex;

    mutable QByteArray encodedNormalized;
    const QByteArray & normalized() const;

    mutable QUrlErrorInfo errorInfo;
    QString createErrorString();
};


static bool QT_FASTCALL _HEXDIG(const char **ptr)
{
    char ch = **ptr;
    if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) {
        ++(*ptr);
        return true;
    }

    return false;
}

// pct-encoded = "%" HEXDIG HEXDIG
static bool QT_FASTCALL _pctEncoded(const char **ptr)
{
    const char *ptrBackup = *ptr;

    if (**ptr != '%')
        return false;
    ++(*ptr);

    if (!_HEXDIG(ptr)) {
        *ptr = ptrBackup;
        return false;
    }
    if (!_HEXDIG(ptr)) {
        *ptr = ptrBackup;
        return false;
    }

    return true;
}

#if 0
// gen-delims  = ":" / "/" / "?" / "#" / "[" / "]" / "@"
static bool QT_FASTCALL _genDelims(const char **ptr, char *c)
{
    char ch = **ptr;
    switch (ch) {
    case ':': case '/': case '?': case '#':
    case '[': case ']': case '@':
        *c = ch;
        ++(*ptr);
        return true;
    default:
        return false;
    }
}
#endif

// sub-delims  = "!" / "$" / "&" / "'" / "(" / ")"
//             / "*" / "+" / "," / ";" / "="
static bool QT_FASTCALL _subDelims(const char **ptr)
{
    char ch = **ptr;
    switch (ch) {
    case '!': case '$': case '&': case '\'':
    case '(': case ')': case '*': case '+':
    case ',': case ';': case '=':
        ++(*ptr);
        return true;
    default:
        return false;
    }
}

// unreserved  = ALPHA / DIGIT / "-" / "." / "_" / "~"
static bool QT_FASTCALL _unreserved(const char **ptr)
{
    char ch = **ptr;
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
        || (ch >= '0' && ch <= '9')
        || ch == '-' || ch == '.' || ch == '_' || ch == '~') {
        ++(*ptr);
        return true;
    }
    return false;
}

// scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
static bool QT_FASTCALL _scheme(const char **ptr, QUrlParseData *parseData)
{
    bool first = true;
    bool isSchemeValid = true;

    parseData->scheme = *ptr;
    for (;;) {
        char ch = **ptr;
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
            ;
        } else if ((ch >= '0' && ch <= '9') || ch == '+' || ch == '-' || ch == '.') {
            if (first)
                isSchemeValid = false;
        } else {
            break;
        }

        ++(*ptr);
        first = false;
    }

    if (**ptr != ':') {
        isSchemeValid = true;
        *ptr = parseData->scheme;
    } else {
        parseData->schemeLength = *ptr - parseData->scheme;
        ++(*ptr); // skip ':'
    }

    return isSchemeValid;
}

// IPvFuture  = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
static bool QT_FASTCALL _IPvFuture(const char **ptr)
{
    if (**ptr != 'v')
        return false;

    const char *ptrBackup = *ptr;
    ++(*ptr);

    if (!_HEXDIG(ptr)) {
        *ptr = ptrBackup;
        return false;
    }

    while (_HEXDIG(ptr))
        ;

    if (**ptr != '.') {
        *ptr = ptrBackup;
        return false;
    }
    ++(*ptr);

    if (!_unreserved(ptr) && !_subDelims(ptr) && *((*ptr)++) != ':') {
        *ptr = ptrBackup;
        return false;
    }


    while (_unreserved(ptr) || _subDelims(ptr) || *((*ptr)++) == ':')
        ;

    return true;
}

// h16         = 1*4HEXDIG
//             ; 16 bits of address represented in hexadecimal
static bool QT_FASTCALL _h16(const char **ptr)
{
    int i = 0;
    for (; i < 4; ++i) {
        if (!_HEXDIG(ptr))
            break;
    }
    return (i != 0);
}

// dec-octet   = DIGIT                 ; 0-9
//             / %x31-39 DIGIT         ; 10-99
//             / "1" 2DIGIT            ; 100-199
//             / "2" %x30-34 DIGIT     ; 200-249
//             / "25" %x30-35          ; 250-255
static bool QT_FASTCALL _decOctet(const char **ptr)
{
    const char *ptrBackup = *ptr;
    char c1 = **ptr;

    if (c1 < '0' || c1 > '9')
        return false;

    ++(*ptr);

    if (c1 == '0')
        return true;

    char c2 = **ptr;

    if (c2 < '0' || c2 > '9')
        return true;

    ++(*ptr);

    char c3 = **ptr;
    if (c3 < '0' || c3 > '9')
        return true;

    // If there is a three digit number larger than 255, reject the
    // whole token.
    if (c1 >= '2' && c2 >= '5' && c3 > '5') {
        *ptr = ptrBackup;
        return false;
    }

    ++(*ptr);

    return true;
}

// IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
static bool QT_FASTCALL _IPv4Address(const char **ptr)
{
    const char *ptrBackup = *ptr;

    if (!_decOctet(ptr)) {
        *ptr = ptrBackup;
        return false;
    }

    for (int i = 0; i < 3; ++i) {
        char ch = *((*ptr)++);
        if (ch != '.') {
            *ptr = ptrBackup;
            return false;
        }

        if (!_decOctet(ptr)) {
            *ptr = ptrBackup;
            return false;
        }
    }

    return true;
}

// ls32        = ( h16 ":" h16 ) / IPv4address
//             ; least-significant 32 bits of address
static bool QT_FASTCALL _ls32(const char **ptr)
{
    const char *ptrBackup = *ptr;
    if (_h16(ptr) && *((*ptr)++) == ':' && _h16(ptr))
        return true;

    *ptr = ptrBackup;
    return _IPv4Address(ptr);
}

// IPv6address =                            6( h16 ":" ) ls32 // case 1
//             /                       "::" 5( h16 ":" ) ls32 // case 2
//             / [               h16 ] "::" 4( h16 ":" ) ls32 // case 3
//             / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32 // case 4
//             / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32 // case 5
//             / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32 // case 6
//             / [ *4( h16 ":" ) h16 ] "::"              ls32 // case 7
//             / [ *5( h16 ":" ) h16 ] "::"              h16  // case 8
//             / [ *6( h16 ":" ) h16 ] "::"                   // case 9
static bool QT_FASTCALL _IPv6Address(const char **ptr)
{
    const char *ptrBackup = *ptr;

    // count of (h16 ":") to the left of and including ::
    int leftHexColons = 0;
    // count of (h16 ":") to the right of ::
    int rightHexColons = 0;

    // first count the number of (h16 ":") on the left of ::
    while (_h16(ptr)) {

        // an h16 not followed by a colon is considered an
        // error.
        if (**ptr != ':') {
            *ptr = ptrBackup;
            return false;
        }
        ++(*ptr);
        ++leftHexColons;

        // check for case 1, the only time when there can be no ::
        if (leftHexColons == 6 && _ls32(ptr)) {
            return true;
        }
    }

    // check for case 2 where the address starts with a :
    if (leftHexColons == 0 && *((*ptr)++) != ':') {
        *ptr = ptrBackup;
        return false;
    }

    // check for the second colon in ::
    if (*((*ptr)++) != ':') {
        *ptr = ptrBackup;
        return false;
    }

    int canBeCase = -1;
    bool ls32WasRead = false;

    const char *tmpBackup = *ptr;

    // count the number of (h16 ":") on the right of ::
    for (;;) {
        tmpBackup = *ptr;
        if (!_h16(ptr)) {
            if (!_ls32(ptr)) {
                if (rightHexColons != 0) {
                    *ptr = ptrBackup;
                    return false;
                }

                // the address ended with :: (case 9)
                // only valid if 1 <= leftHexColons <= 7
                canBeCase = 9;
            } else {
                ls32WasRead = true;
            }
            break;
        }
        ++rightHexColons;
        if (**ptr != ':') {
            // no colon could mean that what was read as an h16
            // was in fact the first part of an ls32. we backtrack
            // and retry.
            const char *pb = *ptr;
            *ptr = tmpBackup;
            if (_ls32(ptr)) {
                ls32WasRead = true;
                --rightHexColons;
            } else {
                *ptr = pb;
                // address ends with only 1 h16 after :: (case 8)
                if (rightHexColons == 1)
                    canBeCase = 8;
            }
            break;
        }
        ++(*ptr);
    }

    // determine which case it is based on the number of rightHexColons
    if (canBeCase == -1) {

        // check if a ls32 was read. If it wasn't and rightHexColons >= 2 then the
        // last 2 HexColons are in fact a ls32
        if (!ls32WasRead && rightHexColons >= 2)
            rightHexColons -= 2;

        canBeCase = 7 - rightHexColons;
    }

    // based on the case we need to check that the number of leftHexColons is valid
    if (leftHexColons > (canBeCase - 2)) {
        *ptr = ptrBackup;
        return false;
    }

    return true;
}

// IP-literal = "[" ( IPv6address / IPvFuture  ) "]"
static bool QT_FASTCALL _IPLiteral(const char **ptr)
{
    const char *ptrBackup = *ptr;
    if (**ptr != '[')
        return false;
    ++(*ptr);

    if (!_IPv6Address(ptr) && !_IPvFuture(ptr)) {
        *ptr = ptrBackup;
        return false;
    }

    if (**ptr != ']') {
        *ptr = ptrBackup;
        return false;
    }
    ++(*ptr);

    return true;
}

// reg-name    = *( unreserved / pct-encoded / sub-delims )
static void QT_FASTCALL _regName(const char **ptr)
{
    for (;;) {
        if (!_unreserved(ptr) && !_subDelims(ptr)) {
            if (!_pctEncoded(ptr))
                break;
        }
    }
}

// host        = IP-literal / IPv4address / reg-name
static void QT_FASTCALL _host(const char **ptr, QUrlParseData *parseData)
{
    parseData->host = *ptr;
    if (!_IPLiteral(ptr)) {
        if (_IPv4Address(ptr)) {
            char ch = **ptr;
            if (ch && ch != ':' && ch != '/') {
                // reset
                *ptr = parseData->host;
                _regName(ptr);
            }
        } else {
            _regName(ptr);
        }
    }
    parseData->hostLength = *ptr - parseData->host;
}

// userinfo    = *( unreserved / pct-encoded / sub-delims / ":" )
static void QT_FASTCALL _userInfo(const char **ptr, QUrlParseData *parseData)
{
    parseData->userInfo = *ptr;
    for (;;) {
        if (_unreserved(ptr) || _subDelims(ptr)) {
            ;
        } else {
            if (_pctEncoded(ptr)) {
                ;
            } else if (**ptr == ':') {
                parseData->userInfoDelimIndex = *ptr - parseData->userInfo;
                ++(*ptr);
            } else {
                break;
            }
        }
    }
    if (**ptr != '@') {
        *ptr = parseData->userInfo;
        parseData->userInfoDelimIndex = -1;
        return;
    }
    parseData->userInfoLength = *ptr - parseData->userInfo;
    ++(*ptr);
}

// port        = *DIGIT
static void QT_FASTCALL _port(const char **ptr, int *port)
{
    bool first = true;

    for (;;) {
        const char *ptrBackup = *ptr;
        char ch = *((*ptr)++);
        if (ch < '0' || ch > '9') {
            *ptr = ptrBackup;
            break;
        }

        if (first) {
            first = false;
            *port = 0;
        }

        *port *= 10;
        *port += ch - '0';
    }
}

// authority   = [ userinfo "@" ] host [ ":" port ]
static void QT_FASTCALL _authority(const char **ptr, QUrlParseData *parseData)
{
    _userInfo(ptr, parseData);
    _host(ptr, parseData);

    if (**ptr != ':')
        return;

    ++(*ptr);
    _port(ptr, &parseData->port);
}

// pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
static bool QT_FASTCALL _pchar(const char **ptr)
{
    char c = *(*ptr);

    switch (c) {
    case '!': case '$': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case ';': case '=': case ':': case '@':
    case '-': case '.': case '_': case '~':
        ++(*ptr);
        return true;
    default:
        break;
    };

    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
        ++(*ptr);
        return true;
    }

    if (_pctEncoded(ptr))
        return true;

    return false;
}

// segment       = *pchar
static bool QT_FASTCALL _segmentNZ(const char **ptr)
{
    if (!_pchar(ptr))
        return false;

    while(_pchar(ptr))
        ;

    return true;
}

// path-abempty  = *( "/" segment )
static void QT_FASTCALL _pathAbEmpty(const char **ptr)
{
    for (;;) {
        if (**ptr != '/')
            break;
        ++(*ptr);

        while (_pchar(ptr))
            ;
    }
}

// path-abs      = "/" [ segment-nz *( "/" segment ) ]
static bool QT_FASTCALL _pathAbs(const char **ptr)
{
    // **ptr == '/' already checked in caller
    ++(*ptr);

    // we might be able to unnest this to gain some performance.
    if (!_segmentNZ(ptr))
        return true;

    _pathAbEmpty(ptr);

    return true;
}

// path-rootless = segment-nz *( "/" segment )
static bool QT_FASTCALL _pathRootless(const char **ptr)
{
    // we might be able to unnest this to gain some performance.
    if (!_segmentNZ(ptr))
        return false;

    _pathAbEmpty(ptr);

    return true;
}


// hier-part   = "//" authority path-abempty
//             / path-abs
//             / path-rootless
//             / path-empty
static void QT_FASTCALL _hierPart(const char **ptr, QUrlParseData *parseData)
{
    const char *ptrBackup = *ptr;
    const char *pathStart = 0;
    if (*((*ptr)++) == '/' && *((*ptr)++) == '/') {
        _authority(ptr, parseData);
        pathStart = *ptr;
        _pathAbEmpty(ptr);
    } else {
        *ptr = ptrBackup;
        pathStart = *ptr;
        if (**ptr == '/')
            _pathAbs(ptr);
        else
            _pathRootless(ptr);
    }
    parseData->path = pathStart;
    parseData->pathLength = *ptr - pathStart;
}

// query       = *( pchar / "/" / "?" )
static void QT_FASTCALL _query(const char **ptr, QUrlParseData *parseData)
{
    parseData->query = *ptr;
    for (;;) {
        if (_pchar(ptr)) {
            ;
        } else if (**ptr == '/' || **ptr == '?') {
            ++(*ptr);
        } else {
            break;
        }
    }
    parseData->queryLength = *ptr - parseData->query;
}

// fragment    = *( pchar / "/" / "?" )
static void QT_FASTCALL _fragment(const char **ptr, QUrlParseData *parseData)
{
    parseData->fragment = *ptr;
    for (;;) {
        if (_pchar(ptr)) {
            ;
        } else if (**ptr == '/' || **ptr == '?' || **ptr == '#') {
            ++(*ptr);
        } else {
            break;
        }
    }
    parseData->fragmentLength = *ptr - parseData->fragment;
}

struct NameprepCaseFoldingEntry {
    uint uc;
    ushort mapping[4];
};

inline bool operator<(uint one, const NameprepCaseFoldingEntry &other)
{ return one < other.uc; }

inline bool operator<(const NameprepCaseFoldingEntry &one, uint other)
{ return one.uc < other; }

static const NameprepCaseFoldingEntry NameprepCaseFolding[] = {
/*	{ 0x0041, { 0x0061, 0x0000, 0x0000, 0x0000 } },
	{ 0x0042, { 0x0062, 0x0000, 0x0000, 0x0000 } },
	{ 0x0043, { 0x0063, 0x0000, 0x0000, 0x0000 } },
	{ 0x0044, { 0x0064, 0x0000, 0x0000, 0x0000 } },
	{ 0x0045, { 0x0065, 0x0000, 0x0000, 0x0000 } },
	{ 0x0046, { 0x0066, 0x0000, 0x0000, 0x0000 } },
	{ 0x0047, { 0x0067, 0x0000, 0x0000, 0x0000 } },
	{ 0x0048, { 0x0068, 0x0000, 0x0000, 0x0000 } },
	{ 0x0049, { 0x0069, 0x0000, 0x0000, 0x0000 } },
	{ 0x004A, { 0x006A, 0x0000, 0x0000, 0x0000 } },
	{ 0x004B, { 0x006B, 0x0000, 0x0000, 0x0000 } },
	{ 0x004C, { 0x006C, 0x0000, 0x0000, 0x0000 } },
	{ 0x004D, { 0x006D, 0x0000, 0x0000, 0x0000 } },
	{ 0x004E, { 0x006E, 0x0000, 0x0000, 0x0000 } },
	{ 0x004F, { 0x006F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0050, { 0x0070, 0x0000, 0x0000, 0x0000 } },
	{ 0x0051, { 0x0071, 0x0000, 0x0000, 0x0000 } },
	{ 0x0052, { 0x0072, 0x0000, 0x0000, 0x0000 } },
	{ 0x0053, { 0x0073, 0x0000, 0x0000, 0x0000 } },
	{ 0x0054, { 0x0074, 0x0000, 0x0000, 0x0000 } },
	{ 0x0055, { 0x0075, 0x0000, 0x0000, 0x0000 } },
	{ 0x0056, { 0x0076, 0x0000, 0x0000, 0x0000 } },
	{ 0x0057, { 0x0077, 0x0000, 0x0000, 0x0000 } },
	{ 0x0058, { 0x0078, 0x0000, 0x0000, 0x0000 } },
	{ 0x0059, { 0x0079, 0x0000, 0x0000, 0x0000 } },
	{ 0x005A, { 0x007A, 0x0000, 0x0000, 0x0000 } },*/
	{ 0x00B5, { 0x03BC, 0x0000, 0x0000, 0x0000 } },
	{ 0x00C0, { 0x00E0, 0x0000, 0x0000, 0x0000 } },
	{ 0x00C1, { 0x00E1, 0x0000, 0x0000, 0x0000 } },
	{ 0x00C2, { 0x00E2, 0x0000, 0x0000, 0x0000 } },
	{ 0x00C3, { 0x00E3, 0x0000, 0x0000, 0x0000 } },
	{ 0x00C4, { 0x00E4, 0x0000, 0x0000, 0x0000 } },
	{ 0x00C5, { 0x00E5, 0x0000, 0x0000, 0x0000 } },
	{ 0x00C6, { 0x00E6, 0x0000, 0x0000, 0x0000 } },
	{ 0x00C7, { 0x00E7, 0x0000, 0x0000, 0x0000 } },
	{ 0x00C8, { 0x00E8, 0x0000, 0x0000, 0x0000 } },
	{ 0x00C9, { 0x00E9, 0x0000, 0x0000, 0x0000 } },
	{ 0x00CA, { 0x00EA, 0x0000, 0x0000, 0x0000 } },
	{ 0x00CB, { 0x00EB, 0x0000, 0x0000, 0x0000 } },
	{ 0x00CC, { 0x00EC, 0x0000, 0x0000, 0x0000 } },
	{ 0x00CD, { 0x00ED, 0x0000, 0x0000, 0x0000 } },
	{ 0x00CE, { 0x00EE, 0x0000, 0x0000, 0x0000 } },
	{ 0x00CF, { 0x00EF, 0x0000, 0x0000, 0x0000 } },
	{ 0x00D0, { 0x00F0, 0x0000, 0x0000, 0x0000 } },
	{ 0x00D1, { 0x00F1, 0x0000, 0x0000, 0x0000 } },
	{ 0x00D2, { 0x00F2, 0x0000, 0x0000, 0x0000 } },
	{ 0x00D3, { 0x00F3, 0x0000, 0x0000, 0x0000 } },
	{ 0x00D4, { 0x00F4, 0x0000, 0x0000, 0x0000 } },
	{ 0x00D5, { 0x00F5, 0x0000, 0x0000, 0x0000 } },
	{ 0x00D6, { 0x00F6, 0x0000, 0x0000, 0x0000 } },
	{ 0x00D8, { 0x00F8, 0x0000, 0x0000, 0x0000 } },
	{ 0x00D9, { 0x00F9, 0x0000, 0x0000, 0x0000 } },
	{ 0x00DA, { 0x00FA, 0x0000, 0x0000, 0x0000 } },
	{ 0x00DB, { 0x00FB, 0x0000, 0x0000, 0x0000 } },
	{ 0x00DC, { 0x00FC, 0x0000, 0x0000, 0x0000 } },
	{ 0x00DD, { 0x00FD, 0x0000, 0x0000, 0x0000 } },
	{ 0x00DE, { 0x00FE, 0x0000, 0x0000, 0x0000 } },
	{ 0x00DF, { 0x0073, 0x0073, 0x0000, 0x0000 } },
	{ 0x0100, { 0x0101, 0x0000, 0x0000, 0x0000 } },
	{ 0x0102, { 0x0103, 0x0000, 0x0000, 0x0000 } },
	{ 0x0104, { 0x0105, 0x0000, 0x0000, 0x0000 } },
	{ 0x0106, { 0x0107, 0x0000, 0x0000, 0x0000 } },
	{ 0x0108, { 0x0109, 0x0000, 0x0000, 0x0000 } },
	{ 0x010A, { 0x010B, 0x0000, 0x0000, 0x0000 } },
	{ 0x010C, { 0x010D, 0x0000, 0x0000, 0x0000 } },
	{ 0x010E, { 0x010F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0110, { 0x0111, 0x0000, 0x0000, 0x0000 } },
	{ 0x0112, { 0x0113, 0x0000, 0x0000, 0x0000 } },
	{ 0x0114, { 0x0115, 0x0000, 0x0000, 0x0000 } },
	{ 0x0116, { 0x0117, 0x0000, 0x0000, 0x0000 } },
	{ 0x0118, { 0x0119, 0x0000, 0x0000, 0x0000 } },
	{ 0x011A, { 0x011B, 0x0000, 0x0000, 0x0000 } },
	{ 0x011C, { 0x011D, 0x0000, 0x0000, 0x0000 } },
	{ 0x011E, { 0x011F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0120, { 0x0121, 0x0000, 0x0000, 0x0000 } },
	{ 0x0122, { 0x0123, 0x0000, 0x0000, 0x0000 } },
	{ 0x0124, { 0x0125, 0x0000, 0x0000, 0x0000 } },
	{ 0x0126, { 0x0127, 0x0000, 0x0000, 0x0000 } },
	{ 0x0128, { 0x0129, 0x0000, 0x0000, 0x0000 } },
	{ 0x012A, { 0x012B, 0x0000, 0x0000, 0x0000 } },
	{ 0x012C, { 0x012D, 0x0000, 0x0000, 0x0000 } },
	{ 0x012E, { 0x012F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0130, { 0x0069, 0x0307, 0x0000, 0x0000 } },
	{ 0x0132, { 0x0133, 0x0000, 0x0000, 0x0000 } },
	{ 0x0134, { 0x0135, 0x0000, 0x0000, 0x0000 } },
	{ 0x0136, { 0x0137, 0x0000, 0x0000, 0x0000 } },
	{ 0x0139, { 0x013A, 0x0000, 0x0000, 0x0000 } },
	{ 0x013B, { 0x013C, 0x0000, 0x0000, 0x0000 } },
	{ 0x013D, { 0x013E, 0x0000, 0x0000, 0x0000 } },
	{ 0x013F, { 0x0140, 0x0000, 0x0000, 0x0000 } },
	{ 0x0141, { 0x0142, 0x0000, 0x0000, 0x0000 } },
	{ 0x0143, { 0x0144, 0x0000, 0x0000, 0x0000 } },
	{ 0x0145, { 0x0146, 0x0000, 0x0000, 0x0000 } },
	{ 0x0147, { 0x0148, 0x0000, 0x0000, 0x0000 } },
	{ 0x0149, { 0x02BC, 0x006E, 0x0000, 0x0000 } },
	{ 0x014A, { 0x014B, 0x0000, 0x0000, 0x0000 } },
	{ 0x014C, { 0x014D, 0x0000, 0x0000, 0x0000 } },
	{ 0x014E, { 0x014F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0150, { 0x0151, 0x0000, 0x0000, 0x0000 } },
	{ 0x0152, { 0x0153, 0x0000, 0x0000, 0x0000 } },
	{ 0x0154, { 0x0155, 0x0000, 0x0000, 0x0000 } },
	{ 0x0156, { 0x0157, 0x0000, 0x0000, 0x0000 } },
	{ 0x0158, { 0x0159, 0x0000, 0x0000, 0x0000 } },
	{ 0x015A, { 0x015B, 0x0000, 0x0000, 0x0000 } },
	{ 0x015C, { 0x015D, 0x0000, 0x0000, 0x0000 } },
	{ 0x015E, { 0x015F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0160, { 0x0161, 0x0000, 0x0000, 0x0000 } },
	{ 0x0162, { 0x0163, 0x0000, 0x0000, 0x0000 } },
	{ 0x0164, { 0x0165, 0x0000, 0x0000, 0x0000 } },
	{ 0x0166, { 0x0167, 0x0000, 0x0000, 0x0000 } },
	{ 0x0168, { 0x0169, 0x0000, 0x0000, 0x0000 } },
	{ 0x016A, { 0x016B, 0x0000, 0x0000, 0x0000 } },
	{ 0x016C, { 0x016D, 0x0000, 0x0000, 0x0000 } },
	{ 0x016E, { 0x016F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0170, { 0x0171, 0x0000, 0x0000, 0x0000 } },
	{ 0x0172, { 0x0173, 0x0000, 0x0000, 0x0000 } },
	{ 0x0174, { 0x0175, 0x0000, 0x0000, 0x0000 } },
	{ 0x0176, { 0x0177, 0x0000, 0x0000, 0x0000 } },
	{ 0x0178, { 0x00FF, 0x0000, 0x0000, 0x0000 } },
	{ 0x0179, { 0x017A, 0x0000, 0x0000, 0x0000 } },
	{ 0x017B, { 0x017C, 0x0000, 0x0000, 0x0000 } },
	{ 0x017D, { 0x017E, 0x0000, 0x0000, 0x0000 } },
	{ 0x017F, { 0x0073, 0x0000, 0x0000, 0x0000 } },
	{ 0x0181, { 0x0253, 0x0000, 0x0000, 0x0000 } },
	{ 0x0182, { 0x0183, 0x0000, 0x0000, 0x0000 } },
	{ 0x0184, { 0x0185, 0x0000, 0x0000, 0x0000 } },
	{ 0x0186, { 0x0254, 0x0000, 0x0000, 0x0000 } },
	{ 0x0187, { 0x0188, 0x0000, 0x0000, 0x0000 } },
	{ 0x0189, { 0x0256, 0x0000, 0x0000, 0x0000 } },
	{ 0x018A, { 0x0257, 0x0000, 0x0000, 0x0000 } },
	{ 0x018B, { 0x018C, 0x0000, 0x0000, 0x0000 } },
	{ 0x018E, { 0x01DD, 0x0000, 0x0000, 0x0000 } },
	{ 0x018F, { 0x0259, 0x0000, 0x0000, 0x0000 } },
	{ 0x0190, { 0x025B, 0x0000, 0x0000, 0x0000 } },
	{ 0x0191, { 0x0192, 0x0000, 0x0000, 0x0000 } },
	{ 0x0193, { 0x0260, 0x0000, 0x0000, 0x0000 } },
	{ 0x0194, { 0x0263, 0x0000, 0x0000, 0x0000 } },
	{ 0x0196, { 0x0269, 0x0000, 0x0000, 0x0000 } },
	{ 0x0197, { 0x0268, 0x0000, 0x0000, 0x0000 } },
	{ 0x0198, { 0x0199, 0x0000, 0x0000, 0x0000 } },
	{ 0x019C, { 0x026F, 0x0000, 0x0000, 0x0000 } },
	{ 0x019D, { 0x0272, 0x0000, 0x0000, 0x0000 } },
	{ 0x019F, { 0x0275, 0x0000, 0x0000, 0x0000 } },
	{ 0x01A0, { 0x01A1, 0x0000, 0x0000, 0x0000 } },
	{ 0x01A2, { 0x01A3, 0x0000, 0x0000, 0x0000 } },
	{ 0x01A4, { 0x01A5, 0x0000, 0x0000, 0x0000 } },
	{ 0x01A6, { 0x0280, 0x0000, 0x0000, 0x0000 } },
	{ 0x01A7, { 0x01A8, 0x0000, 0x0000, 0x0000 } },
	{ 0x01A9, { 0x0283, 0x0000, 0x0000, 0x0000 } },
	{ 0x01AC, { 0x01AD, 0x0000, 0x0000, 0x0000 } },
	{ 0x01AE, { 0x0288, 0x0000, 0x0000, 0x0000 } },
	{ 0x01AF, { 0x01B0, 0x0000, 0x0000, 0x0000 } },
	{ 0x01B1, { 0x028A, 0x0000, 0x0000, 0x0000 } },
	{ 0x01B2, { 0x028B, 0x0000, 0x0000, 0x0000 } },
	{ 0x01B3, { 0x01B4, 0x0000, 0x0000, 0x0000 } },
	{ 0x01B5, { 0x01B6, 0x0000, 0x0000, 0x0000 } },
	{ 0x01B7, { 0x0292, 0x0000, 0x0000, 0x0000 } },
	{ 0x01B8, { 0x01B9, 0x0000, 0x0000, 0x0000 } },
	{ 0x01BC, { 0x01BD, 0x0000, 0x0000, 0x0000 } },
	{ 0x01C4, { 0x01C6, 0x0000, 0x0000, 0x0000 } },
	{ 0x01C5, { 0x01C6, 0x0000, 0x0000, 0x0000 } },
	{ 0x01C7, { 0x01C9, 0x0000, 0x0000, 0x0000 } },
	{ 0x01C8, { 0x01C9, 0x0000, 0x0000, 0x0000 } },
	{ 0x01CA, { 0x01CC, 0x0000, 0x0000, 0x0000 } },
	{ 0x01CB, { 0x01CC, 0x0000, 0x0000, 0x0000 } },
	{ 0x01CD, { 0x01CE, 0x0000, 0x0000, 0x0000 } },
	{ 0x01CF, { 0x01D0, 0x0000, 0x0000, 0x0000 } },
	{ 0x01D1, { 0x01D2, 0x0000, 0x0000, 0x0000 } },
	{ 0x01D3, { 0x01D4, 0x0000, 0x0000, 0x0000 } },
	{ 0x01D5, { 0x01D6, 0x0000, 0x0000, 0x0000 } },
	{ 0x01D7, { 0x01D8, 0x0000, 0x0000, 0x0000 } },
	{ 0x01D9, { 0x01DA, 0x0000, 0x0000, 0x0000 } },
	{ 0x01DB, { 0x01DC, 0x0000, 0x0000, 0x0000 } },
	{ 0x01DE, { 0x01DF, 0x0000, 0x0000, 0x0000 } },
	{ 0x01E0, { 0x01E1, 0x0000, 0x0000, 0x0000 } },
	{ 0x01E2, { 0x01E3, 0x0000, 0x0000, 0x0000 } },
	{ 0x01E4, { 0x01E5, 0x0000, 0x0000, 0x0000 } },
	{ 0x01E6, { 0x01E7, 0x0000, 0x0000, 0x0000 } },
	{ 0x01E8, { 0x01E9, 0x0000, 0x0000, 0x0000 } },
	{ 0x01EA, { 0x01EB, 0x0000, 0x0000, 0x0000 } },
	{ 0x01EC, { 0x01ED, 0x0000, 0x0000, 0x0000 } },
	{ 0x01EE, { 0x01EF, 0x0000, 0x0000, 0x0000 } },
	{ 0x01F0, { 0x006A, 0x030C, 0x0000, 0x0000 } },
	{ 0x01F1, { 0x01F3, 0x0000, 0x0000, 0x0000 } },
	{ 0x01F2, { 0x01F3, 0x0000, 0x0000, 0x0000 } },
	{ 0x01F4, { 0x01F5, 0x0000, 0x0000, 0x0000 } },
	{ 0x01F6, { 0x0195, 0x0000, 0x0000, 0x0000 } },
	{ 0x01F7, { 0x01BF, 0x0000, 0x0000, 0x0000 } },
	{ 0x01F8, { 0x01F9, 0x0000, 0x0000, 0x0000 } },
	{ 0x01FA, { 0x01FB, 0x0000, 0x0000, 0x0000 } },
	{ 0x01FC, { 0x01FD, 0x0000, 0x0000, 0x0000 } },
	{ 0x01FE, { 0x01FF, 0x0000, 0x0000, 0x0000 } },
	{ 0x0200, { 0x0201, 0x0000, 0x0000, 0x0000 } },
	{ 0x0202, { 0x0203, 0x0000, 0x0000, 0x0000 } },
	{ 0x0204, { 0x0205, 0x0000, 0x0000, 0x0000 } },
	{ 0x0206, { 0x0207, 0x0000, 0x0000, 0x0000 } },
	{ 0x0208, { 0x0209, 0x0000, 0x0000, 0x0000 } },
	{ 0x020A, { 0x020B, 0x0000, 0x0000, 0x0000 } },
	{ 0x020C, { 0x020D, 0x0000, 0x0000, 0x0000 } },
	{ 0x020E, { 0x020F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0210, { 0x0211, 0x0000, 0x0000, 0x0000 } },
	{ 0x0212, { 0x0213, 0x0000, 0x0000, 0x0000 } },
	{ 0x0214, { 0x0215, 0x0000, 0x0000, 0x0000 } },
	{ 0x0216, { 0x0217, 0x0000, 0x0000, 0x0000 } },
	{ 0x0218, { 0x0219, 0x0000, 0x0000, 0x0000 } },
	{ 0x021A, { 0x021B, 0x0000, 0x0000, 0x0000 } },
	{ 0x021C, { 0x021D, 0x0000, 0x0000, 0x0000 } },
	{ 0x021E, { 0x021F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0220, { 0x019E, 0x0000, 0x0000, 0x0000 } },
	{ 0x0222, { 0x0223, 0x0000, 0x0000, 0x0000 } },
	{ 0x0224, { 0x0225, 0x0000, 0x0000, 0x0000 } },
	{ 0x0226, { 0x0227, 0x0000, 0x0000, 0x0000 } },
	{ 0x0228, { 0x0229, 0x0000, 0x0000, 0x0000 } },
	{ 0x022A, { 0x022B, 0x0000, 0x0000, 0x0000 } },
	{ 0x022C, { 0x022D, 0x0000, 0x0000, 0x0000 } },
	{ 0x022E, { 0x022F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0230, { 0x0231, 0x0000, 0x0000, 0x0000 } },
	{ 0x0232, { 0x0233, 0x0000, 0x0000, 0x0000 } },
	{ 0x0345, { 0x03B9, 0x0000, 0x0000, 0x0000 } },
	{ 0x037A, { 0x0020, 0x03B9, 0x0000, 0x0000 } },
	{ 0x0386, { 0x03AC, 0x0000, 0x0000, 0x0000 } },
	{ 0x0388, { 0x03AD, 0x0000, 0x0000, 0x0000 } },
	{ 0x0389, { 0x03AE, 0x0000, 0x0000, 0x0000 } },
	{ 0x038A, { 0x03AF, 0x0000, 0x0000, 0x0000 } },
	{ 0x038C, { 0x03CC, 0x0000, 0x0000, 0x0000 } },
	{ 0x038E, { 0x03CD, 0x0000, 0x0000, 0x0000 } },
	{ 0x038F, { 0x03CE, 0x0000, 0x0000, 0x0000 } },
	{ 0x0390, { 0x03B9, 0x0308, 0x0301, 0x0000 } },
	{ 0x0391, { 0x03B1, 0x0000, 0x0000, 0x0000 } },
	{ 0x0392, { 0x03B2, 0x0000, 0x0000, 0x0000 } },
	{ 0x0393, { 0x03B3, 0x0000, 0x0000, 0x0000 } },
	{ 0x0394, { 0x03B4, 0x0000, 0x0000, 0x0000 } },
	{ 0x0395, { 0x03B5, 0x0000, 0x0000, 0x0000 } },
	{ 0x0396, { 0x03B6, 0x0000, 0x0000, 0x0000 } },
	{ 0x0397, { 0x03B7, 0x0000, 0x0000, 0x0000 } },
	{ 0x0398, { 0x03B8, 0x0000, 0x0000, 0x0000 } },
	{ 0x0399, { 0x03B9, 0x0000, 0x0000, 0x0000 } },
	{ 0x039A, { 0x03BA, 0x0000, 0x0000, 0x0000 } },
	{ 0x039B, { 0x03BB, 0x0000, 0x0000, 0x0000 } },
	{ 0x039C, { 0x03BC, 0x0000, 0x0000, 0x0000 } },
	{ 0x039D, { 0x03BD, 0x0000, 0x0000, 0x0000 } },
	{ 0x039E, { 0x03BE, 0x0000, 0x0000, 0x0000 } },
	{ 0x039F, { 0x03BF, 0x0000, 0x0000, 0x0000 } },
	{ 0x03A0, { 0x03C0, 0x0000, 0x0000, 0x0000 } },
	{ 0x03A1, { 0x03C1, 0x0000, 0x0000, 0x0000 } },
	{ 0x03A3, { 0x03C3, 0x0000, 0x0000, 0x0000 } },
	{ 0x03A4, { 0x03C4, 0x0000, 0x0000, 0x0000 } },
	{ 0x03A5, { 0x03C5, 0x0000, 0x0000, 0x0000 } },
	{ 0x03A6, { 0x03C6, 0x0000, 0x0000, 0x0000 } },
	{ 0x03A7, { 0x03C7, 0x0000, 0x0000, 0x0000 } },
	{ 0x03A8, { 0x03C8, 0x0000, 0x0000, 0x0000 } },
	{ 0x03A9, { 0x03C9, 0x0000, 0x0000, 0x0000 } },
	{ 0x03AA, { 0x03CA, 0x0000, 0x0000, 0x0000 } },
	{ 0x03AB, { 0x03CB, 0x0000, 0x0000, 0x0000 } },
	{ 0x03B0, { 0x03C5, 0x0308, 0x0301, 0x0000 } },
	{ 0x03C2, { 0x03C3, 0x0000, 0x0000, 0x0000 } },
	{ 0x03D0, { 0x03B2, 0x0000, 0x0000, 0x0000 } },
	{ 0x03D1, { 0x03B8, 0x0000, 0x0000, 0x0000 } },
	{ 0x03D2, { 0x03C5, 0x0000, 0x0000, 0x0000 } },
	{ 0x03D3, { 0x03CD, 0x0000, 0x0000, 0x0000 } },
	{ 0x03D4, { 0x03CB, 0x0000, 0x0000, 0x0000 } },
	{ 0x03D5, { 0x03C6, 0x0000, 0x0000, 0x0000 } },
	{ 0x03D6, { 0x03C0, 0x0000, 0x0000, 0x0000 } },
	{ 0x03D8, { 0x03D9, 0x0000, 0x0000, 0x0000 } },
	{ 0x03DA, { 0x03DB, 0x0000, 0x0000, 0x0000 } },
	{ 0x03DC, { 0x03DD, 0x0000, 0x0000, 0x0000 } },
	{ 0x03DE, { 0x03DF, 0x0000, 0x0000, 0x0000 } },
	{ 0x03E0, { 0x03E1, 0x0000, 0x0000, 0x0000 } },
	{ 0x03E2, { 0x03E3, 0x0000, 0x0000, 0x0000 } },
	{ 0x03E4, { 0x03E5, 0x0000, 0x0000, 0x0000 } },
	{ 0x03E6, { 0x03E7, 0x0000, 0x0000, 0x0000 } },
	{ 0x03E8, { 0x03E9, 0x0000, 0x0000, 0x0000 } },
	{ 0x03EA, { 0x03EB, 0x0000, 0x0000, 0x0000 } },
	{ 0x03EC, { 0x03ED, 0x0000, 0x0000, 0x0000 } },
	{ 0x03EE, { 0x03EF, 0x0000, 0x0000, 0x0000 } },
	{ 0x03F0, { 0x03BA, 0x0000, 0x0000, 0x0000 } },
	{ 0x03F1, { 0x03C1, 0x0000, 0x0000, 0x0000 } },
	{ 0x03F2, { 0x03C3, 0x0000, 0x0000, 0x0000 } },
	{ 0x03F4, { 0x03B8, 0x0000, 0x0000, 0x0000 } },
	{ 0x03F5, { 0x03B5, 0x0000, 0x0000, 0x0000 } },
	{ 0x0400, { 0x0450, 0x0000, 0x0000, 0x0000 } },
	{ 0x0401, { 0x0451, 0x0000, 0x0000, 0x0000 } },
	{ 0x0402, { 0x0452, 0x0000, 0x0000, 0x0000 } },
	{ 0x0403, { 0x0453, 0x0000, 0x0000, 0x0000 } },
	{ 0x0404, { 0x0454, 0x0000, 0x0000, 0x0000 } },
	{ 0x0405, { 0x0455, 0x0000, 0x0000, 0x0000 } },
	{ 0x0406, { 0x0456, 0x0000, 0x0000, 0x0000 } },
	{ 0x0407, { 0x0457, 0x0000, 0x0000, 0x0000 } },
	{ 0x0408, { 0x0458, 0x0000, 0x0000, 0x0000 } },
	{ 0x0409, { 0x0459, 0x0000, 0x0000, 0x0000 } },
	{ 0x040A, { 0x045A, 0x0000, 0x0000, 0x0000 } },
	{ 0x040B, { 0x045B, 0x0000, 0x0000, 0x0000 } },
	{ 0x040C, { 0x045C, 0x0000, 0x0000, 0x0000 } },
	{ 0x040D, { 0x045D, 0x0000, 0x0000, 0x0000 } },
	{ 0x040E, { 0x045E, 0x0000, 0x0000, 0x0000 } },
	{ 0x040F, { 0x045F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0410, { 0x0430, 0x0000, 0x0000, 0x0000 } },
	{ 0x0411, { 0x0431, 0x0000, 0x0000, 0x0000 } },
	{ 0x0412, { 0x0432, 0x0000, 0x0000, 0x0000 } },
	{ 0x0413, { 0x0433, 0x0000, 0x0000, 0x0000 } },
	{ 0x0414, { 0x0434, 0x0000, 0x0000, 0x0000 } },
	{ 0x0415, { 0x0435, 0x0000, 0x0000, 0x0000 } },
	{ 0x0416, { 0x0436, 0x0000, 0x0000, 0x0000 } },
	{ 0x0417, { 0x0437, 0x0000, 0x0000, 0x0000 } },
	{ 0x0418, { 0x0438, 0x0000, 0x0000, 0x0000 } },
	{ 0x0419, { 0x0439, 0x0000, 0x0000, 0x0000 } },
	{ 0x041A, { 0x043A, 0x0000, 0x0000, 0x0000 } },
	{ 0x041B, { 0x043B, 0x0000, 0x0000, 0x0000 } },
	{ 0x041C, { 0x043C, 0x0000, 0x0000, 0x0000 } },
	{ 0x041D, { 0x043D, 0x0000, 0x0000, 0x0000 } },
	{ 0x041E, { 0x043E, 0x0000, 0x0000, 0x0000 } },
	{ 0x041F, { 0x043F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0420, { 0x0440, 0x0000, 0x0000, 0x0000 } },
	{ 0x0421, { 0x0441, 0x0000, 0x0000, 0x0000 } },
	{ 0x0422, { 0x0442, 0x0000, 0x0000, 0x0000 } },
	{ 0x0423, { 0x0443, 0x0000, 0x0000, 0x0000 } },
	{ 0x0424, { 0x0444, 0x0000, 0x0000, 0x0000 } },
	{ 0x0425, { 0x0445, 0x0000, 0x0000, 0x0000 } },
	{ 0x0426, { 0x0446, 0x0000, 0x0000, 0x0000 } },
	{ 0x0427, { 0x0447, 0x0000, 0x0000, 0x0000 } },
	{ 0x0428, { 0x0448, 0x0000, 0x0000, 0x0000 } },
	{ 0x0429, { 0x0449, 0x0000, 0x0000, 0x0000 } },
	{ 0x042A, { 0x044A, 0x0000, 0x0000, 0x0000 } },
	{ 0x042B, { 0x044B, 0x0000, 0x0000, 0x0000 } },
	{ 0x042C, { 0x044C, 0x0000, 0x0000, 0x0000 } },
	{ 0x042D, { 0x044D, 0x0000, 0x0000, 0x0000 } },
	{ 0x042E, { 0x044E, 0x0000, 0x0000, 0x0000 } },
	{ 0x042F, { 0x044F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0460, { 0x0461, 0x0000, 0x0000, 0x0000 } },
	{ 0x0462, { 0x0463, 0x0000, 0x0000, 0x0000 } },
	{ 0x0464, { 0x0465, 0x0000, 0x0000, 0x0000 } },
	{ 0x0466, { 0x0467, 0x0000, 0x0000, 0x0000 } },
	{ 0x0468, { 0x0469, 0x0000, 0x0000, 0x0000 } },
	{ 0x046A, { 0x046B, 0x0000, 0x0000, 0x0000 } },
	{ 0x046C, { 0x046D, 0x0000, 0x0000, 0x0000 } },
	{ 0x046E, { 0x046F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0470, { 0x0471, 0x0000, 0x0000, 0x0000 } },
	{ 0x0472, { 0x0473, 0x0000, 0x0000, 0x0000 } },
	{ 0x0474, { 0x0475, 0x0000, 0x0000, 0x0000 } },
	{ 0x0476, { 0x0477, 0x0000, 0x0000, 0x0000 } },
	{ 0x0478, { 0x0479, 0x0000, 0x0000, 0x0000 } },
	{ 0x047A, { 0x047B, 0x0000, 0x0000, 0x0000 } },
	{ 0x047C, { 0x047D, 0x0000, 0x0000, 0x0000 } },
	{ 0x047E, { 0x047F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0480, { 0x0481, 0x0000, 0x0000, 0x0000 } },
	{ 0x048A, { 0x048B, 0x0000, 0x0000, 0x0000 } },
	{ 0x048C, { 0x048D, 0x0000, 0x0000, 0x0000 } },
	{ 0x048E, { 0x048F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0490, { 0x0491, 0x0000, 0x0000, 0x0000 } },
	{ 0x0492, { 0x0493, 0x0000, 0x0000, 0x0000 } },
	{ 0x0494, { 0x0495, 0x0000, 0x0000, 0x0000 } },
	{ 0x0496, { 0x0497, 0x0000, 0x0000, 0x0000 } },
	{ 0x0498, { 0x0499, 0x0000, 0x0000, 0x0000 } },
	{ 0x049A, { 0x049B, 0x0000, 0x0000, 0x0000 } },
	{ 0x049C, { 0x049D, 0x0000, 0x0000, 0x0000 } },
	{ 0x049E, { 0x049F, 0x0000, 0x0000, 0x0000 } },
	{ 0x04A0, { 0x04A1, 0x0000, 0x0000, 0x0000 } },
	{ 0x04A2, { 0x04A3, 0x0000, 0x0000, 0x0000 } },
	{ 0x04A4, { 0x04A5, 0x0000, 0x0000, 0x0000 } },
	{ 0x04A6, { 0x04A7, 0x0000, 0x0000, 0x0000 } },
	{ 0x04A8, { 0x04A9, 0x0000, 0x0000, 0x0000 } },
	{ 0x04AA, { 0x04AB, 0x0000, 0x0000, 0x0000 } },
	{ 0x04AC, { 0x04AD, 0x0000, 0x0000, 0x0000 } },
	{ 0x04AE, { 0x04AF, 0x0000, 0x0000, 0x0000 } },
	{ 0x04B0, { 0x04B1, 0x0000, 0x0000, 0x0000 } },
	{ 0x04B2, { 0x04B3, 0x0000, 0x0000, 0x0000 } },
	{ 0x04B4, { 0x04B5, 0x0000, 0x0000, 0x0000 } },
	{ 0x04B6, { 0x04B7, 0x0000, 0x0000, 0x0000 } },
	{ 0x04B8, { 0x04B9, 0x0000, 0x0000, 0x0000 } },
	{ 0x04BA, { 0x04BB, 0x0000, 0x0000, 0x0000 } },
	{ 0x04BC, { 0x04BD, 0x0000, 0x0000, 0x0000 } },
	{ 0x04BE, { 0x04BF, 0x0000, 0x0000, 0x0000 } },
	{ 0x04C1, { 0x04C2, 0x0000, 0x0000, 0x0000 } },
	{ 0x04C3, { 0x04C4, 0x0000, 0x0000, 0x0000 } },
	{ 0x04C5, { 0x04C6, 0x0000, 0x0000, 0x0000 } },
	{ 0x04C7, { 0x04C8, 0x0000, 0x0000, 0x0000 } },
	{ 0x04C9, { 0x04CA, 0x0000, 0x0000, 0x0000 } },
	{ 0x04CB, { 0x04CC, 0x0000, 0x0000, 0x0000 } },
	{ 0x04CD, { 0x04CE, 0x0000, 0x0000, 0x0000 } },
	{ 0x04D0, { 0x04D1, 0x0000, 0x0000, 0x0000 } },
	{ 0x04D2, { 0x04D3, 0x0000, 0x0000, 0x0000 } },
	{ 0x04D4, { 0x04D5, 0x0000, 0x0000, 0x0000 } },
	{ 0x04D6, { 0x04D7, 0x0000, 0x0000, 0x0000 } },
	{ 0x04D8, { 0x04D9, 0x0000, 0x0000, 0x0000 } },
	{ 0x04DA, { 0x04DB, 0x0000, 0x0000, 0x0000 } },
	{ 0x04DC, { 0x04DD, 0x0000, 0x0000, 0x0000 } },
	{ 0x04DE, { 0x04DF, 0x0000, 0x0000, 0x0000 } },
	{ 0x04E0, { 0x04E1, 0x0000, 0x0000, 0x0000 } },
	{ 0x04E2, { 0x04E3, 0x0000, 0x0000, 0x0000 } },
	{ 0x04E4, { 0x04E5, 0x0000, 0x0000, 0x0000 } },
	{ 0x04E6, { 0x04E7, 0x0000, 0x0000, 0x0000 } },
	{ 0x04E8, { 0x04E9, 0x0000, 0x0000, 0x0000 } },
	{ 0x04EA, { 0x04EB, 0x0000, 0x0000, 0x0000 } },
	{ 0x04EC, { 0x04ED, 0x0000, 0x0000, 0x0000 } },
	{ 0x04EE, { 0x04EF, 0x0000, 0x0000, 0x0000 } },
	{ 0x04F0, { 0x04F1, 0x0000, 0x0000, 0x0000 } },
	{ 0x04F2, { 0x04F3, 0x0000, 0x0000, 0x0000 } },
	{ 0x04F4, { 0x04F5, 0x0000, 0x0000, 0x0000 } },
	{ 0x04F8, { 0x04F9, 0x0000, 0x0000, 0x0000 } },
	{ 0x0500, { 0x0501, 0x0000, 0x0000, 0x0000 } },
	{ 0x0502, { 0x0503, 0x0000, 0x0000, 0x0000 } },
	{ 0x0504, { 0x0505, 0x0000, 0x0000, 0x0000 } },
	{ 0x0506, { 0x0507, 0x0000, 0x0000, 0x0000 } },
	{ 0x0508, { 0x0509, 0x0000, 0x0000, 0x0000 } },
	{ 0x050A, { 0x050B, 0x0000, 0x0000, 0x0000 } },
	{ 0x050C, { 0x050D, 0x0000, 0x0000, 0x0000 } },
	{ 0x050E, { 0x050F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0531, { 0x0561, 0x0000, 0x0000, 0x0000 } },
	{ 0x0532, { 0x0562, 0x0000, 0x0000, 0x0000 } },
	{ 0x0533, { 0x0563, 0x0000, 0x0000, 0x0000 } },
	{ 0x0534, { 0x0564, 0x0000, 0x0000, 0x0000 } },
	{ 0x0535, { 0x0565, 0x0000, 0x0000, 0x0000 } },
	{ 0x0536, { 0x0566, 0x0000, 0x0000, 0x0000 } },
	{ 0x0537, { 0x0567, 0x0000, 0x0000, 0x0000 } },
	{ 0x0538, { 0x0568, 0x0000, 0x0000, 0x0000 } },
	{ 0x0539, { 0x0569, 0x0000, 0x0000, 0x0000 } },
	{ 0x053A, { 0x056A, 0x0000, 0x0000, 0x0000 } },
	{ 0x053B, { 0x056B, 0x0000, 0x0000, 0x0000 } },
	{ 0x053C, { 0x056C, 0x0000, 0x0000, 0x0000 } },
	{ 0x053D, { 0x056D, 0x0000, 0x0000, 0x0000 } },
	{ 0x053E, { 0x056E, 0x0000, 0x0000, 0x0000 } },
	{ 0x053F, { 0x056F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0540, { 0x0570, 0x0000, 0x0000, 0x0000 } },
	{ 0x0541, { 0x0571, 0x0000, 0x0000, 0x0000 } },
	{ 0x0542, { 0x0572, 0x0000, 0x0000, 0x0000 } },
	{ 0x0543, { 0x0573, 0x0000, 0x0000, 0x0000 } },
	{ 0x0544, { 0x0574, 0x0000, 0x0000, 0x0000 } },
	{ 0x0545, { 0x0575, 0x0000, 0x0000, 0x0000 } },
	{ 0x0546, { 0x0576, 0x0000, 0x0000, 0x0000 } },
	{ 0x0547, { 0x0577, 0x0000, 0x0000, 0x0000 } },
	{ 0x0548, { 0x0578, 0x0000, 0x0000, 0x0000 } },
	{ 0x0549, { 0x0579, 0x0000, 0x0000, 0x0000 } },
	{ 0x054A, { 0x057A, 0x0000, 0x0000, 0x0000 } },
	{ 0x054B, { 0x057B, 0x0000, 0x0000, 0x0000 } },
	{ 0x054C, { 0x057C, 0x0000, 0x0000, 0x0000 } },
	{ 0x054D, { 0x057D, 0x0000, 0x0000, 0x0000 } },
	{ 0x054E, { 0x057E, 0x0000, 0x0000, 0x0000 } },
	{ 0x054F, { 0x057F, 0x0000, 0x0000, 0x0000 } },
	{ 0x0550, { 0x0580, 0x0000, 0x0000, 0x0000 } },
	{ 0x0551, { 0x0581, 0x0000, 0x0000, 0x0000 } },
	{ 0x0552, { 0x0582, 0x0000, 0x0000, 0x0000 } },
	{ 0x0553, { 0x0583, 0x0000, 0x0000, 0x0000 } },
	{ 0x0554, { 0x0584, 0x0000, 0x0000, 0x0000 } },
	{ 0x0555, { 0x0585, 0x0000, 0x0000, 0x0000 } },
	{ 0x0556, { 0x0586, 0x0000, 0x0000, 0x0000 } },
	{ 0x0587, { 0x0565, 0x0582, 0x0000, 0x0000 } },
	{ 0x1E00, { 0x1E01, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E02, { 0x1E03, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E04, { 0x1E05, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E06, { 0x1E07, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E08, { 0x1E09, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E0A, { 0x1E0B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E0C, { 0x1E0D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E0E, { 0x1E0F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E10, { 0x1E11, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E12, { 0x1E13, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E14, { 0x1E15, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E16, { 0x1E17, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E18, { 0x1E19, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E1A, { 0x1E1B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E1C, { 0x1E1D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E1E, { 0x1E1F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E20, { 0x1E21, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E22, { 0x1E23, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E24, { 0x1E25, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E26, { 0x1E27, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E28, { 0x1E29, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E2A, { 0x1E2B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E2C, { 0x1E2D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E2E, { 0x1E2F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E30, { 0x1E31, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E32, { 0x1E33, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E34, { 0x1E35, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E36, { 0x1E37, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E38, { 0x1E39, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E3A, { 0x1E3B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E3C, { 0x1E3D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E3E, { 0x1E3F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E40, { 0x1E41, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E42, { 0x1E43, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E44, { 0x1E45, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E46, { 0x1E47, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E48, { 0x1E49, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E4A, { 0x1E4B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E4C, { 0x1E4D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E4E, { 0x1E4F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E50, { 0x1E51, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E52, { 0x1E53, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E54, { 0x1E55, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E56, { 0x1E57, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E58, { 0x1E59, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E5A, { 0x1E5B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E5C, { 0x1E5D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E5E, { 0x1E5F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E60, { 0x1E61, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E62, { 0x1E63, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E64, { 0x1E65, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E66, { 0x1E67, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E68, { 0x1E69, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E6A, { 0x1E6B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E6C, { 0x1E6D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E6E, { 0x1E6F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E70, { 0x1E71, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E72, { 0x1E73, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E74, { 0x1E75, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E76, { 0x1E77, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E78, { 0x1E79, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E7A, { 0x1E7B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E7C, { 0x1E7D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E7E, { 0x1E7F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E80, { 0x1E81, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E82, { 0x1E83, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E84, { 0x1E85, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E86, { 0x1E87, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E88, { 0x1E89, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E8A, { 0x1E8B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E8C, { 0x1E8D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E8E, { 0x1E8F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E90, { 0x1E91, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E92, { 0x1E93, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E94, { 0x1E95, 0x0000, 0x0000, 0x0000 } },
	{ 0x1E96, { 0x0068, 0x0331, 0x0000, 0x0000 } },
	{ 0x1E97, { 0x0074, 0x0308, 0x0000, 0x0000 } },
	{ 0x1E98, { 0x0077, 0x030A, 0x0000, 0x0000 } },
	{ 0x1E99, { 0x0079, 0x030A, 0x0000, 0x0000 } },
	{ 0x1E9A, { 0x0061, 0x02BE, 0x0000, 0x0000 } },
	{ 0x1E9B, { 0x1E61, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EA0, { 0x1EA1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EA2, { 0x1EA3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EA4, { 0x1EA5, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EA6, { 0x1EA7, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EA8, { 0x1EA9, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EAA, { 0x1EAB, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EAC, { 0x1EAD, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EAE, { 0x1EAF, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EB0, { 0x1EB1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EB2, { 0x1EB3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EB4, { 0x1EB5, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EB6, { 0x1EB7, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EB8, { 0x1EB9, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EBA, { 0x1EBB, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EBC, { 0x1EBD, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EBE, { 0x1EBF, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EC0, { 0x1EC1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EC2, { 0x1EC3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EC4, { 0x1EC5, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EC6, { 0x1EC7, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EC8, { 0x1EC9, 0x0000, 0x0000, 0x0000 } },
	{ 0x1ECA, { 0x1ECB, 0x0000, 0x0000, 0x0000 } },
	{ 0x1ECC, { 0x1ECD, 0x0000, 0x0000, 0x0000 } },
	{ 0x1ECE, { 0x1ECF, 0x0000, 0x0000, 0x0000 } },
	{ 0x1ED0, { 0x1ED1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1ED2, { 0x1ED3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1ED4, { 0x1ED5, 0x0000, 0x0000, 0x0000 } },
	{ 0x1ED6, { 0x1ED7, 0x0000, 0x0000, 0x0000 } },
	{ 0x1ED8, { 0x1ED9, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EDA, { 0x1EDB, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EDC, { 0x1EDD, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EDE, { 0x1EDF, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EE0, { 0x1EE1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EE2, { 0x1EE3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EE4, { 0x1EE5, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EE6, { 0x1EE7, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EE8, { 0x1EE9, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EEA, { 0x1EEB, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EEC, { 0x1EED, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EEE, { 0x1EEF, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EF0, { 0x1EF1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EF2, { 0x1EF3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EF4, { 0x1EF5, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EF6, { 0x1EF7, 0x0000, 0x0000, 0x0000 } },
	{ 0x1EF8, { 0x1EF9, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F08, { 0x1F00, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F09, { 0x1F01, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F0A, { 0x1F02, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F0B, { 0x1F03, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F0C, { 0x1F04, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F0D, { 0x1F05, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F0E, { 0x1F06, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F0F, { 0x1F07, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F18, { 0x1F10, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F19, { 0x1F11, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F1A, { 0x1F12, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F1B, { 0x1F13, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F1C, { 0x1F14, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F1D, { 0x1F15, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F28, { 0x1F20, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F29, { 0x1F21, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F2A, { 0x1F22, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F2B, { 0x1F23, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F2C, { 0x1F24, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F2D, { 0x1F25, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F2E, { 0x1F26, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F2F, { 0x1F27, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F38, { 0x1F30, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F39, { 0x1F31, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F3A, { 0x1F32, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F3B, { 0x1F33, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F3C, { 0x1F34, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F3D, { 0x1F35, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F3E, { 0x1F36, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F3F, { 0x1F37, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F48, { 0x1F40, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F49, { 0x1F41, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F4A, { 0x1F42, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F4B, { 0x1F43, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F4C, { 0x1F44, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F4D, { 0x1F45, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F50, { 0x03C5, 0x0313, 0x0000, 0x0000 } },
	{ 0x1F52, { 0x03C5, 0x0313, 0x0300, 0x0000 } },
	{ 0x1F54, { 0x03C5, 0x0313, 0x0301, 0x0000 } },
	{ 0x1F56, { 0x03C5, 0x0313, 0x0342, 0x0000 } },
	{ 0x1F59, { 0x1F51, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F5B, { 0x1F53, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F5D, { 0x1F55, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F5F, { 0x1F57, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F68, { 0x1F60, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F69, { 0x1F61, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F6A, { 0x1F62, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F6B, { 0x1F63, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F6C, { 0x1F64, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F6D, { 0x1F65, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F6E, { 0x1F66, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F6F, { 0x1F67, 0x0000, 0x0000, 0x0000 } },
	{ 0x1F80, { 0x1F00, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F81, { 0x1F01, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F82, { 0x1F02, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F83, { 0x1F03, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F84, { 0x1F04, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F85, { 0x1F05, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F86, { 0x1F06, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F87, { 0x1F07, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F88, { 0x1F00, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F89, { 0x1F01, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F8A, { 0x1F02, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F8B, { 0x1F03, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F8C, { 0x1F04, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F8D, { 0x1F05, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F8E, { 0x1F06, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F8F, { 0x1F07, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F90, { 0x1F20, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F91, { 0x1F21, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F92, { 0x1F22, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F93, { 0x1F23, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F94, { 0x1F24, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F95, { 0x1F25, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F96, { 0x1F26, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F97, { 0x1F27, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F98, { 0x1F20, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F99, { 0x1F21, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F9A, { 0x1F22, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F9B, { 0x1F23, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F9C, { 0x1F24, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F9D, { 0x1F25, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F9E, { 0x1F26, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1F9F, { 0x1F27, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FA0, { 0x1F60, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FA1, { 0x1F61, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FA2, { 0x1F62, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FA3, { 0x1F63, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FA4, { 0x1F64, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FA5, { 0x1F65, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FA6, { 0x1F66, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FA7, { 0x1F67, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FA8, { 0x1F60, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FA9, { 0x1F61, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FAA, { 0x1F62, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FAB, { 0x1F63, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FAC, { 0x1F64, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FAD, { 0x1F65, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FAE, { 0x1F66, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FAF, { 0x1F67, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FB2, { 0x1F70, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FB3, { 0x03B1, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FB4, { 0x03AC, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FB6, { 0x03B1, 0x0342, 0x0000, 0x0000 } },
	{ 0x1FB7, { 0x03B1, 0x0342, 0x03B9, 0x0000 } },
	{ 0x1FB8, { 0x1FB0, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FB9, { 0x1FB1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FBA, { 0x1F70, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FBB, { 0x1F71, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FBC, { 0x03B1, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FBE, { 0x03B9, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FC2, { 0x1F74, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FC3, { 0x03B7, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FC4, { 0x03AE, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FC6, { 0x03B7, 0x0342, 0x0000, 0x0000 } },
	{ 0x1FC7, { 0x03B7, 0x0342, 0x03B9, 0x0000 } },
	{ 0x1FC8, { 0x1F72, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FC9, { 0x1F73, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FCA, { 0x1F74, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FCB, { 0x1F75, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FCC, { 0x03B7, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FD2, { 0x03B9, 0x0308, 0x0300, 0x0000 } },
	{ 0x1FD3, { 0x03B9, 0x0308, 0x0301, 0x0000 } },
	{ 0x1FD6, { 0x03B9, 0x0342, 0x0000, 0x0000 } },
	{ 0x1FD7, { 0x03B9, 0x0308, 0x0342, 0x0000 } },
	{ 0x1FD8, { 0x1FD0, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FD9, { 0x1FD1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FDA, { 0x1F76, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FDB, { 0x1F77, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FE2, { 0x03C5, 0x0308, 0x0300, 0x0000 } },
	{ 0x1FE3, { 0x03C5, 0x0308, 0x0301, 0x0000 } },
	{ 0x1FE4, { 0x03C1, 0x0313, 0x0000, 0x0000 } },
	{ 0x1FE6, { 0x03C5, 0x0342, 0x0000, 0x0000 } },
	{ 0x1FE7, { 0x03C5, 0x0308, 0x0342, 0x0000 } },
	{ 0x1FE8, { 0x1FE0, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FE9, { 0x1FE1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FEA, { 0x1F7A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FEB, { 0x1F7B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FEC, { 0x1FE5, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FF2, { 0x1F7C, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FF3, { 0x03C9, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FF4, { 0x03CE, 0x03B9, 0x0000, 0x0000 } },
	{ 0x1FF6, { 0x03C9, 0x0342, 0x0000, 0x0000 } },
	{ 0x1FF7, { 0x03C9, 0x0342, 0x03B9, 0x0000 } },
	{ 0x1FF8, { 0x1F78, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FF9, { 0x1F79, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FFA, { 0x1F7C, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FFB, { 0x1F7D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1FFC, { 0x03C9, 0x03B9, 0x0000, 0x0000 } },
	{ 0x20A8, { 0x0072, 0x0073, 0x0000, 0x0000 } },
	{ 0x2102, { 0x0063, 0x0000, 0x0000, 0x0000 } },
	{ 0x2103, { 0x00B0, 0x0063, 0x0000, 0x0000 } },
	{ 0x2107, { 0x025B, 0x0000, 0x0000, 0x0000 } },
	{ 0x2109, { 0x00B0, 0x0066, 0x0000, 0x0000 } },
	{ 0x210B, { 0x0068, 0x0000, 0x0000, 0x0000 } },
	{ 0x210C, { 0x0068, 0x0000, 0x0000, 0x0000 } },
	{ 0x210D, { 0x0068, 0x0000, 0x0000, 0x0000 } },
	{ 0x2110, { 0x0069, 0x0000, 0x0000, 0x0000 } },
	{ 0x2111, { 0x0069, 0x0000, 0x0000, 0x0000 } },
	{ 0x2112, { 0x006C, 0x0000, 0x0000, 0x0000 } },
	{ 0x2115, { 0x006E, 0x0000, 0x0000, 0x0000 } },
	{ 0x2116, { 0x006E, 0x006F, 0x0000, 0x0000 } },
	{ 0x2119, { 0x0070, 0x0000, 0x0000, 0x0000 } },
	{ 0x211A, { 0x0071, 0x0000, 0x0000, 0x0000 } },
	{ 0x211B, { 0x0072, 0x0000, 0x0000, 0x0000 } },
	{ 0x211C, { 0x0072, 0x0000, 0x0000, 0x0000 } },
	{ 0x211D, { 0x0072, 0x0000, 0x0000, 0x0000 } },
	{ 0x2120, { 0x0073, 0x006D, 0x0000, 0x0000 } },
	{ 0x2121, { 0x0074, 0x0065, 0x006C, 0x0000 } },
	{ 0x2122, { 0x0074, 0x006D, 0x0000, 0x0000 } },
	{ 0x2124, { 0x007A, 0x0000, 0x0000, 0x0000 } },
	{ 0x2126, { 0x03C9, 0x0000, 0x0000, 0x0000 } },
	{ 0x2128, { 0x007A, 0x0000, 0x0000, 0x0000 } },
	{ 0x212A, { 0x006B, 0x0000, 0x0000, 0x0000 } },
	{ 0x212B, { 0x00E5, 0x0000, 0x0000, 0x0000 } },
	{ 0x212C, { 0x0062, 0x0000, 0x0000, 0x0000 } },
	{ 0x212D, { 0x0063, 0x0000, 0x0000, 0x0000 } },
	{ 0x2130, { 0x0065, 0x0000, 0x0000, 0x0000 } },
	{ 0x2131, { 0x0066, 0x0000, 0x0000, 0x0000 } },
	{ 0x2133, { 0x006D, 0x0000, 0x0000, 0x0000 } },
	{ 0x213E, { 0x03B3, 0x0000, 0x0000, 0x0000 } },
	{ 0x213F, { 0x03C0, 0x0000, 0x0000, 0x0000 } },
	{ 0x2145, { 0x0064, 0x0000, 0x0000, 0x0000 } },
	{ 0x2160, { 0x2170, 0x0000, 0x0000, 0x0000 } },
	{ 0x2161, { 0x2171, 0x0000, 0x0000, 0x0000 } },
	{ 0x2162, { 0x2172, 0x0000, 0x0000, 0x0000 } },
	{ 0x2163, { 0x2173, 0x0000, 0x0000, 0x0000 } },
	{ 0x2164, { 0x2174, 0x0000, 0x0000, 0x0000 } },
	{ 0x2165, { 0x2175, 0x0000, 0x0000, 0x0000 } },
	{ 0x2166, { 0x2176, 0x0000, 0x0000, 0x0000 } },
	{ 0x2167, { 0x2177, 0x0000, 0x0000, 0x0000 } },
	{ 0x2168, { 0x2178, 0x0000, 0x0000, 0x0000 } },
	{ 0x2169, { 0x2179, 0x0000, 0x0000, 0x0000 } },
	{ 0x216A, { 0x217A, 0x0000, 0x0000, 0x0000 } },
	{ 0x216B, { 0x217B, 0x0000, 0x0000, 0x0000 } },
	{ 0x216C, { 0x217C, 0x0000, 0x0000, 0x0000 } },
	{ 0x216D, { 0x217D, 0x0000, 0x0000, 0x0000 } },
	{ 0x216E, { 0x217E, 0x0000, 0x0000, 0x0000 } },
	{ 0x216F, { 0x217F, 0x0000, 0x0000, 0x0000 } },
	{ 0x24B6, { 0x24D0, 0x0000, 0x0000, 0x0000 } },
	{ 0x24B7, { 0x24D1, 0x0000, 0x0000, 0x0000 } },
	{ 0x24B8, { 0x24D2, 0x0000, 0x0000, 0x0000 } },
	{ 0x24B9, { 0x24D3, 0x0000, 0x0000, 0x0000 } },
	{ 0x24BA, { 0x24D4, 0x0000, 0x0000, 0x0000 } },
	{ 0x24BB, { 0x24D5, 0x0000, 0x0000, 0x0000 } },
	{ 0x24BC, { 0x24D6, 0x0000, 0x0000, 0x0000 } },
	{ 0x24BD, { 0x24D7, 0x0000, 0x0000, 0x0000 } },
	{ 0x24BE, { 0x24D8, 0x0000, 0x0000, 0x0000 } },
	{ 0x24BF, { 0x24D9, 0x0000, 0x0000, 0x0000 } },
	{ 0x24C0, { 0x24DA, 0x0000, 0x0000, 0x0000 } },
	{ 0x24C1, { 0x24DB, 0x0000, 0x0000, 0x0000 } },
	{ 0x24C2, { 0x24DC, 0x0000, 0x0000, 0x0000 } },
	{ 0x24C3, { 0x24DD, 0x0000, 0x0000, 0x0000 } },
	{ 0x24C4, { 0x24DE, 0x0000, 0x0000, 0x0000 } },
	{ 0x24C5, { 0x24DF, 0x0000, 0x0000, 0x0000 } },
	{ 0x24C6, { 0x24E0, 0x0000, 0x0000, 0x0000 } },
	{ 0x24C7, { 0x24E1, 0x0000, 0x0000, 0x0000 } },
	{ 0x24C8, { 0x24E2, 0x0000, 0x0000, 0x0000 } },
	{ 0x24C9, { 0x24E3, 0x0000, 0x0000, 0x0000 } },
	{ 0x24CA, { 0x24E4, 0x0000, 0x0000, 0x0000 } },
	{ 0x24CB, { 0x24E5, 0x0000, 0x0000, 0x0000 } },
	{ 0x24CC, { 0x24E6, 0x0000, 0x0000, 0x0000 } },
	{ 0x24CD, { 0x24E7, 0x0000, 0x0000, 0x0000 } },
	{ 0x24CE, { 0x24E8, 0x0000, 0x0000, 0x0000 } },
	{ 0x24CF, { 0x24E9, 0x0000, 0x0000, 0x0000 } },
	{ 0x3371, { 0x0068, 0x0070, 0x0061, 0x0000 } },
	{ 0x3373, { 0x0061, 0x0075, 0x0000, 0x0000 } },
	{ 0x3375, { 0x006F, 0x0076, 0x0000, 0x0000 } },
	{ 0x3380, { 0x0070, 0x0061, 0x0000, 0x0000 } },
	{ 0x3381, { 0x006E, 0x0061, 0x0000, 0x0000 } },
	{ 0x3382, { 0x03BC, 0x0061, 0x0000, 0x0000 } },
	{ 0x3383, { 0x006D, 0x0061, 0x0000, 0x0000 } },
	{ 0x3384, { 0x006B, 0x0061, 0x0000, 0x0000 } },
	{ 0x3385, { 0x006B, 0x0062, 0x0000, 0x0000 } },
	{ 0x3386, { 0x006D, 0x0062, 0x0000, 0x0000 } },
	{ 0x3387, { 0x0067, 0x0062, 0x0000, 0x0000 } },
	{ 0x338A, { 0x0070, 0x0066, 0x0000, 0x0000 } },
	{ 0x338B, { 0x006E, 0x0066, 0x0000, 0x0000 } },
	{ 0x338C, { 0x03BC, 0x0066, 0x0000, 0x0000 } },
	{ 0x3390, { 0x0068, 0x007A, 0x0000, 0x0000 } },
	{ 0x3391, { 0x006B, 0x0068, 0x007A, 0x0000 } },
	{ 0x3392, { 0x006D, 0x0068, 0x007A, 0x0000 } },
	{ 0x3393, { 0x0067, 0x0068, 0x007A, 0x0000 } },
	{ 0x3394, { 0x0074, 0x0068, 0x007A, 0x0000 } },
	{ 0x33A9, { 0x0070, 0x0061, 0x0000, 0x0000 } },
	{ 0x33AA, { 0x006B, 0x0070, 0x0061, 0x0000 } },
	{ 0x33AB, { 0x006D, 0x0070, 0x0061, 0x0000 } },
	{ 0x33AC, { 0x0067, 0x0070, 0x0061, 0x0000 } },
	{ 0x33B4, { 0x0070, 0x0076, 0x0000, 0x0000 } },
	{ 0x33B5, { 0x006E, 0x0076, 0x0000, 0x0000 } },
	{ 0x33B6, { 0x03BC, 0x0076, 0x0000, 0x0000 } },
	{ 0x33B7, { 0x006D, 0x0076, 0x0000, 0x0000 } },
	{ 0x33B8, { 0x006B, 0x0076, 0x0000, 0x0000 } },
	{ 0x33B9, { 0x006D, 0x0076, 0x0000, 0x0000 } },
	{ 0x33BA, { 0x0070, 0x0077, 0x0000, 0x0000 } },
	{ 0x33BB, { 0x006E, 0x0077, 0x0000, 0x0000 } },
	{ 0x33BC, { 0x03BC, 0x0077, 0x0000, 0x0000 } },
	{ 0x33BD, { 0x006D, 0x0077, 0x0000, 0x0000 } },
	{ 0x33BE, { 0x006B, 0x0077, 0x0000, 0x0000 } },
	{ 0x33BF, { 0x006D, 0x0077, 0x0000, 0x0000 } },
	{ 0x33C0, { 0x006B, 0x03C9, 0x0000, 0x0000 } },
	{ 0x33C1, { 0x006D, 0x03C9, 0x0000, 0x0000 } },
	{ 0x33C3, { 0x0062, 0x0071, 0x0000, 0x0000 } },
	{ 0x33C6, { 0x0063, 0x2215, 0x006B, 0x0067 } },
	{ 0x33C7, { 0x0063, 0x006F, 0x002E, 0x0000 } },
	{ 0x33C8, { 0x0064, 0x0062, 0x0000, 0x0000 } },
	{ 0x33C9, { 0x0067, 0x0079, 0x0000, 0x0000 } },
	{ 0x33CB, { 0x0068, 0x0070, 0x0000, 0x0000 } },
	{ 0x33CD, { 0x006B, 0x006B, 0x0000, 0x0000 } },
	{ 0x33CE, { 0x006B, 0x006D, 0x0000, 0x0000 } },
	{ 0x33D7, { 0x0070, 0x0068, 0x0000, 0x0000 } },
	{ 0x33D9, { 0x0070, 0x0070, 0x006D, 0x0000 } },
	{ 0x33DA, { 0x0070, 0x0072, 0x0000, 0x0000 } },
	{ 0x33DC, { 0x0073, 0x0076, 0x0000, 0x0000 } },
	{ 0x33DD, { 0x0077, 0x0062, 0x0000, 0x0000 } },
	{ 0xFB00, { 0x0066, 0x0066, 0x0000, 0x0000 } },
	{ 0xFB01, { 0x0066, 0x0069, 0x0000, 0x0000 } },
	{ 0xFB02, { 0x0066, 0x006C, 0x0000, 0x0000 } },
	{ 0xFB03, { 0x0066, 0x0066, 0x0069, 0x0000 } },
	{ 0xFB04, { 0x0066, 0x0066, 0x006C, 0x0000 } },
	{ 0xFB05, { 0x0073, 0x0074, 0x0000, 0x0000 } },
	{ 0xFB06, { 0x0073, 0x0074, 0x0000, 0x0000 } },
	{ 0xFB13, { 0x0574, 0x0576, 0x0000, 0x0000 } },
	{ 0xFB14, { 0x0574, 0x0565, 0x0000, 0x0000 } },
	{ 0xFB15, { 0x0574, 0x056B, 0x0000, 0x0000 } },
	{ 0xFB16, { 0x057E, 0x0576, 0x0000, 0x0000 } },
	{ 0xFB17, { 0x0574, 0x056D, 0x0000, 0x0000 } },
	{ 0xFF21, { 0xFF41, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF22, { 0xFF42, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF23, { 0xFF43, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF24, { 0xFF44, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF25, { 0xFF45, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF26, { 0xFF46, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF27, { 0xFF47, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF28, { 0xFF48, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF29, { 0xFF49, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF2A, { 0xFF4A, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF2B, { 0xFF4B, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF2C, { 0xFF4C, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF2D, { 0xFF4D, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF2E, { 0xFF4E, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF2F, { 0xFF4F, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF30, { 0xFF50, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF31, { 0xFF51, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF32, { 0xFF52, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF33, { 0xFF53, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF34, { 0xFF54, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF35, { 0xFF55, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF36, { 0xFF56, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF37, { 0xFF57, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF38, { 0xFF58, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF39, { 0xFF59, 0x0000, 0x0000, 0x0000 } },
	{ 0xFF3A, { 0xFF5A, 0x0000, 0x0000, 0x0000 } },
	{ 0x10400, { 0xd801, 0xdc28, 0x0000, 0x0000 } },
	{ 0x10401, { 0xd801, 0xdc29, 0x0000, 0x0000 } },
	{ 0x10402, { 0xd801, 0xdc2A, 0x0000, 0x0000 } },
	{ 0x10403, { 0xd801, 0xdc2B, 0x0000, 0x0000 } },
	{ 0x10404, { 0xd801, 0xdc2C, 0x0000, 0x0000 } },
	{ 0x10405, { 0xd801, 0xdc2D, 0x0000, 0x0000 } },
	{ 0x10406, { 0xd801, 0xdc2E, 0x0000, 0x0000 } },
	{ 0x10407, { 0xd801, 0xdc2F, 0x0000, 0x0000 } },
	{ 0x10408, { 0xd801, 0xdc30, 0x0000, 0x0000 } },
	{ 0x10409, { 0xd801, 0xdc31, 0x0000, 0x0000 } },
	{ 0x1040A, { 0xd801, 0xdc32, 0x0000, 0x0000 } },
	{ 0x1040B, { 0xd801, 0xdc33, 0x0000, 0x0000 } },
	{ 0x1040C, { 0xd801, 0xdc34, 0x0000, 0x0000 } },
	{ 0x1040D, { 0xd801, 0xdc35, 0x0000, 0x0000 } },
	{ 0x1040E, { 0xd801, 0xdc36, 0x0000, 0x0000 } },
	{ 0x1040F, { 0xd801, 0xdc37, 0x0000, 0x0000 } },
	{ 0x10410, { 0xd801, 0xdc38, 0x0000, 0x0000 } },
	{ 0x10411, { 0xd801, 0xdc39, 0x0000, 0x0000 } },
	{ 0x10412, { 0xd801, 0xdc3A, 0x0000, 0x0000 } },
	{ 0x10413, { 0xd801, 0xdc3B, 0x0000, 0x0000 } },
	{ 0x10414, { 0xd801, 0xdc3C, 0x0000, 0x0000 } },
	{ 0x10415, { 0xd801, 0xdc3D, 0x0000, 0x0000 } },
	{ 0x10416, { 0xd801, 0xdc3E, 0x0000, 0x0000 } },
	{ 0x10417, { 0xd801, 0xdc3F, 0x0000, 0x0000 } },
	{ 0x10418, { 0xd801, 0xdc40, 0x0000, 0x0000 } },
	{ 0x10419, { 0xd801, 0xdc41, 0x0000, 0x0000 } },
	{ 0x1041A, { 0xd801, 0xdc42, 0x0000, 0x0000 } },
	{ 0x1041B, { 0xd801, 0xdc43, 0x0000, 0x0000 } },
	{ 0x1041C, { 0xd801, 0xdc44, 0x0000, 0x0000 } },
	{ 0x1041D, { 0xd801, 0xdc45, 0x0000, 0x0000 } },
	{ 0x1041E, { 0xd801, 0xdc46, 0x0000, 0x0000 } },
	{ 0x1041F, { 0xd801, 0xdc47, 0x0000, 0x0000 } },
	{ 0x10420, { 0xd801, 0xdc48, 0x0000, 0x0000 } },
	{ 0x10421, { 0xd801, 0xdc49, 0x0000, 0x0000 } },
	{ 0x10422, { 0xd801, 0xdc4A, 0x0000, 0x0000 } },
	{ 0x10423, { 0xd801, 0xdc4B, 0x0000, 0x0000 } },
	{ 0x10424, { 0xd801, 0xdc4C, 0x0000, 0x0000 } },
	{ 0x10425, { 0xd801, 0xdc4D, 0x0000, 0x0000 } },
	{ 0x1D400, { 0x0061, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D401, { 0x0062, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D402, { 0x0063, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D403, { 0x0064, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D404, { 0x0065, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D405, { 0x0066, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D406, { 0x0067, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D407, { 0x0068, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D408, { 0x0069, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D409, { 0x006A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D40A, { 0x006B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D40B, { 0x006C, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D40C, { 0x006D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D40D, { 0x006E, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D40E, { 0x006F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D40F, { 0x0070, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D410, { 0x0071, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D411, { 0x0072, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D412, { 0x0073, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D413, { 0x0074, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D414, { 0x0075, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D415, { 0x0076, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D416, { 0x0077, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D417, { 0x0078, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D418, { 0x0079, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D419, { 0x007A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D434, { 0x0061, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D435, { 0x0062, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D436, { 0x0063, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D437, { 0x0064, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D438, { 0x0065, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D439, { 0x0066, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D43A, { 0x0067, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D43B, { 0x0068, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D43C, { 0x0069, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D43D, { 0x006A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D43E, { 0x006B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D43F, { 0x006C, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D440, { 0x006D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D441, { 0x006E, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D442, { 0x006F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D443, { 0x0070, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D444, { 0x0071, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D445, { 0x0072, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D446, { 0x0073, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D447, { 0x0074, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D448, { 0x0075, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D449, { 0x0076, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D44A, { 0x0077, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D44B, { 0x0078, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D44C, { 0x0079, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D44D, { 0x007A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D468, { 0x0061, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D469, { 0x0062, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D46A, { 0x0063, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D46B, { 0x0064, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D46C, { 0x0065, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D46D, { 0x0066, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D46E, { 0x0067, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D46F, { 0x0068, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D470, { 0x0069, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D471, { 0x006A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D472, { 0x006B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D473, { 0x006C, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D474, { 0x006D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D475, { 0x006E, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D476, { 0x006F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D477, { 0x0070, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D478, { 0x0071, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D479, { 0x0072, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D47A, { 0x0073, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D47B, { 0x0074, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D47C, { 0x0075, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D47D, { 0x0076, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D47E, { 0x0077, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D47F, { 0x0078, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D480, { 0x0079, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D481, { 0x007A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D49C, { 0x0061, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D49E, { 0x0063, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D49F, { 0x0064, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4A2, { 0x0067, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4A5, { 0x006A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4A6, { 0x006B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4A9, { 0x006E, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4AA, { 0x006F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4AB, { 0x0070, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4AC, { 0x0071, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4AE, { 0x0073, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4AF, { 0x0074, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4B0, { 0x0075, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4B1, { 0x0076, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4B2, { 0x0077, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4B3, { 0x0078, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4B4, { 0x0079, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4B5, { 0x007A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4D0, { 0x0061, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4D1, { 0x0062, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4D2, { 0x0063, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4D3, { 0x0064, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4D4, { 0x0065, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4D5, { 0x0066, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4D6, { 0x0067, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4D7, { 0x0068, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4D8, { 0x0069, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4D9, { 0x006A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4DA, { 0x006B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4DB, { 0x006C, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4DC, { 0x006D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4DD, { 0x006E, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4DE, { 0x006F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4DF, { 0x0070, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4E0, { 0x0071, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4E1, { 0x0072, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4E2, { 0x0073, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4E3, { 0x0074, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4E4, { 0x0075, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4E5, { 0x0076, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4E6, { 0x0077, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4E7, { 0x0078, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4E8, { 0x0079, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D4E9, { 0x007A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D504, { 0x0061, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D505, { 0x0062, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D507, { 0x0064, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D508, { 0x0065, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D509, { 0x0066, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D50A, { 0x0067, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D50D, { 0x006A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D50E, { 0x006B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D50F, { 0x006C, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D510, { 0x006D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D511, { 0x006E, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D512, { 0x006F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D513, { 0x0070, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D514, { 0x0071, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D516, { 0x0073, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D517, { 0x0074, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D518, { 0x0075, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D519, { 0x0076, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D51A, { 0x0077, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D51B, { 0x0078, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D51C, { 0x0079, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D538, { 0x0061, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D539, { 0x0062, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D53B, { 0x0064, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D53C, { 0x0065, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D53D, { 0x0066, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D53E, { 0x0067, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D540, { 0x0069, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D541, { 0x006A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D542, { 0x006B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D543, { 0x006C, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D544, { 0x006D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D546, { 0x006F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D54A, { 0x0073, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D54B, { 0x0074, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D54C, { 0x0075, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D54D, { 0x0076, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D54E, { 0x0077, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D54F, { 0x0078, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D550, { 0x0079, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D56C, { 0x0061, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D56D, { 0x0062, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D56E, { 0x0063, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D56F, { 0x0064, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D570, { 0x0065, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D571, { 0x0066, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D572, { 0x0067, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D573, { 0x0068, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D574, { 0x0069, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D575, { 0x006A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D576, { 0x006B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D577, { 0x006C, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D578, { 0x006D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D579, { 0x006E, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D57A, { 0x006F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D57B, { 0x0070, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D57C, { 0x0071, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D57D, { 0x0072, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D57E, { 0x0073, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D57F, { 0x0074, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D580, { 0x0075, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D581, { 0x0076, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D582, { 0x0077, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D583, { 0x0078, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D584, { 0x0079, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D585, { 0x007A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5A0, { 0x0061, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5A1, { 0x0062, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5A2, { 0x0063, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5A3, { 0x0064, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5A4, { 0x0065, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5A5, { 0x0066, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5A6, { 0x0067, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5A7, { 0x0068, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5A8, { 0x0069, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5A9, { 0x006A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5AA, { 0x006B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5AB, { 0x006C, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5AC, { 0x006D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5AD, { 0x006E, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5AE, { 0x006F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5AF, { 0x0070, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5B0, { 0x0071, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5B1, { 0x0072, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5B2, { 0x0073, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5B3, { 0x0074, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5B4, { 0x0075, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5B5, { 0x0076, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5B6, { 0x0077, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5B7, { 0x0078, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5B8, { 0x0079, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5B9, { 0x007A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5D4, { 0x0061, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5D5, { 0x0062, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5D6, { 0x0063, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5D7, { 0x0064, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5D8, { 0x0065, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5D9, { 0x0066, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5DA, { 0x0067, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5DB, { 0x0068, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5DC, { 0x0069, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5DD, { 0x006A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5DE, { 0x006B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5DF, { 0x006C, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5E0, { 0x006D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5E1, { 0x006E, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5E2, { 0x006F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5E3, { 0x0070, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5E4, { 0x0071, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5E5, { 0x0072, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5E6, { 0x0073, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5E7, { 0x0074, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5E8, { 0x0075, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5E9, { 0x0076, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5EA, { 0x0077, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5EB, { 0x0078, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5EC, { 0x0079, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D5ED, { 0x007A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D608, { 0x0061, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D609, { 0x0062, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D60A, { 0x0063, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D60B, { 0x0064, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D60C, { 0x0065, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D60D, { 0x0066, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D60E, { 0x0067, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D60F, { 0x0068, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D610, { 0x0069, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D611, { 0x006A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D612, { 0x006B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D613, { 0x006C, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D614, { 0x006D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D615, { 0x006E, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D616, { 0x006F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D617, { 0x0070, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D618, { 0x0071, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D619, { 0x0072, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D61A, { 0x0073, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D61B, { 0x0074, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D61C, { 0x0075, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D61D, { 0x0076, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D61E, { 0x0077, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D61F, { 0x0078, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D620, { 0x0079, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D621, { 0x007A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D63C, { 0x0061, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D63D, { 0x0062, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D63E, { 0x0063, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D63F, { 0x0064, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D640, { 0x0065, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D641, { 0x0066, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D642, { 0x0067, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D643, { 0x0068, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D644, { 0x0069, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D645, { 0x006A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D646, { 0x006B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D647, { 0x006C, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D648, { 0x006D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D649, { 0x006E, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D64A, { 0x006F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D64B, { 0x0070, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D64C, { 0x0071, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D64D, { 0x0072, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D64E, { 0x0073, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D64F, { 0x0074, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D650, { 0x0075, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D651, { 0x0076, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D652, { 0x0077, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D653, { 0x0078, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D654, { 0x0079, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D655, { 0x007A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D670, { 0x0061, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D671, { 0x0062, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D672, { 0x0063, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D673, { 0x0064, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D674, { 0x0065, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D675, { 0x0066, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D676, { 0x0067, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D677, { 0x0068, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D678, { 0x0069, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D679, { 0x006A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D67A, { 0x006B, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D67B, { 0x006C, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D67C, { 0x006D, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D67D, { 0x006E, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D67E, { 0x006F, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D67F, { 0x0070, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D680, { 0x0071, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D681, { 0x0072, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D682, { 0x0073, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D683, { 0x0074, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D684, { 0x0075, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D685, { 0x0076, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D686, { 0x0077, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D687, { 0x0078, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D688, { 0x0079, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D689, { 0x007A, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6A8, { 0x03B1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6A9, { 0x03B2, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6AA, { 0x03B3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6AB, { 0x03B4, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6AC, { 0x03B5, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6AD, { 0x03B6, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6AE, { 0x03B7, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6AF, { 0x03B8, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6B0, { 0x03B9, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6B1, { 0x03BA, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6B2, { 0x03BB, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6B3, { 0x03BC, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6B4, { 0x03BD, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6B5, { 0x03BE, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6B6, { 0x03BF, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6B7, { 0x03C0, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6B8, { 0x03C1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6B9, { 0x03B8, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6BA, { 0x03C3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6BB, { 0x03C4, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6BC, { 0x03C5, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6BD, { 0x03C6, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6BE, { 0x03C7, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6BF, { 0x03C8, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6C0, { 0x03C9, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6D3, { 0x03C3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6E2, { 0x03B1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6E3, { 0x03B2, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6E4, { 0x03B3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6E5, { 0x03B4, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6E6, { 0x03B5, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6E7, { 0x03B6, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6E8, { 0x03B7, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6E9, { 0x03B8, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6EA, { 0x03B9, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6EB, { 0x03BA, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6EC, { 0x03BB, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6ED, { 0x03BC, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6EE, { 0x03BD, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6EF, { 0x03BE, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6F0, { 0x03BF, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6F1, { 0x03C0, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6F2, { 0x03C1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6F3, { 0x03B8, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6F4, { 0x03C3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6F5, { 0x03C4, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6F6, { 0x03C5, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6F7, { 0x03C6, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6F8, { 0x03C7, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6F9, { 0x03C8, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D6FA, { 0x03C9, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D70D, { 0x03C3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D71C, { 0x03B1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D71D, { 0x03B2, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D71E, { 0x03B3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D71F, { 0x03B4, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D720, { 0x03B5, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D721, { 0x03B6, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D722, { 0x03B7, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D723, { 0x03B8, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D724, { 0x03B9, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D725, { 0x03BA, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D726, { 0x03BB, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D727, { 0x03BC, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D728, { 0x03BD, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D729, { 0x03BE, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D72A, { 0x03BF, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D72B, { 0x03C0, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D72C, { 0x03C1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D72D, { 0x03B8, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D72E, { 0x03C3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D72F, { 0x03C4, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D730, { 0x03C5, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D731, { 0x03C6, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D732, { 0x03C7, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D733, { 0x03C8, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D734, { 0x03C9, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D747, { 0x03C3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D756, { 0x03B1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D757, { 0x03B2, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D758, { 0x03B3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D759, { 0x03B4, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D75A, { 0x03B5, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D75B, { 0x03B6, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D75C, { 0x03B7, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D75D, { 0x03B8, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D75E, { 0x03B9, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D75F, { 0x03BA, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D760, { 0x03BB, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D761, { 0x03BC, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D762, { 0x03BD, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D763, { 0x03BE, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D764, { 0x03BF, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D765, { 0x03C0, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D766, { 0x03C1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D767, { 0x03B8, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D768, { 0x03C3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D769, { 0x03C4, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D76A, { 0x03C5, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D76B, { 0x03C6, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D76C, { 0x03C7, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D76D, { 0x03C8, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D76E, { 0x03C9, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D781, { 0x03C3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D790, { 0x03B1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D791, { 0x03B2, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D792, { 0x03B3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D793, { 0x03B4, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D794, { 0x03B5, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D795, { 0x03B6, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D796, { 0x03B7, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D797, { 0x03B8, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D798, { 0x03B9, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D799, { 0x03BA, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D79A, { 0x03BB, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D79B, { 0x03BC, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D79C, { 0x03BD, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D79D, { 0x03BE, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D79E, { 0x03BF, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D79F, { 0x03C0, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D7A0, { 0x03C1, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D7A1, { 0x03B8, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D7A2, { 0x03C3, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D7A3, { 0x03C4, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D7A4, { 0x03C5, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D7A5, { 0x03C6, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D7A6, { 0x03C7, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D7A7, { 0x03C8, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D7A8, { 0x03C9, 0x0000, 0x0000, 0x0000 } },
	{ 0x1D7BB, { 0x03C3, 0x0000, 0x0000, 0x0000 } }
};

static void mapToLowerCase(QString *str, int from)
{
    int N = sizeof(NameprepCaseFolding) / sizeof(NameprepCaseFolding[0]);

    ushort *d = 0;
    for (int i = from; i < str->size(); ++i) {
        uint uc = str->at(i).unicode();
        if (uc < 0x80) {
            if (uc <= 'Z' && uc >= 'A') {
                if (!d)
                    d = reinterpret_cast<ushort *>(str->data());
                d[i] = (uc | 0x20);
            }
        } else {
            if (QChar(uc).isHighSurrogate() && i < str->size() - 1) {
                ushort low = str->at(i + 1).unicode();
                if (QChar(low).isLowSurrogate()) {
                    uc = QChar::surrogateToUcs4(uc, low);
                    ++i;
                }
            }
            const NameprepCaseFoldingEntry *entry = qBinaryFind(NameprepCaseFolding,
                                                                NameprepCaseFolding + N,
                                                                uc);
            if ((entry - NameprepCaseFolding) != N) {
                int l = 1;
                while (l < 4 && entry->mapping[l])
                    ++l;
                if (l > 1) {
                    if (uc <= 0xffff)
                        str->replace(i, 1, reinterpret_cast<const QChar *>(&entry->mapping[0]), l);
                    else
                        str->replace(i-1, 2, reinterpret_cast<const QChar *>(&entry->mapping[0]), l);
                    d = 0;
                } else {
                    if (!d)
                        d = reinterpret_cast<ushort *>(str->data());
                    d[i] = entry->mapping[0];
                }
            }
        }
    }
}

static bool isMappedToNothing(uint uc)
{
    if (uc < 0xad)
        return false;
    switch (uc) {
    case 0x00AD: case 0x034F: case 0x1806: case 0x180B: case 0x180C: case 0x180D:
    case 0x200B: case 0x200C: case 0x200D: case 0x2060: case 0xFE00: case 0xFE01:
    case 0xFE02: case 0xFE03: case 0xFE04: case 0xFE05: case 0xFE06: case 0xFE07:
    case 0xFE08: case 0xFE09: case 0xFE0A: case 0xFE0B: case 0xFE0C: case 0xFE0D:
    case 0xFE0E: case 0xFE0F: case 0xFEFF:
        return true;
    default:
        return false;
    }
}


static void stripProhibitedOutput(QString *str, int from)
{
    ushort *out = (ushort *)str->data() + from;
    const ushort *in = out;
    const ushort *end = (ushort *)str->data() + str->size();
    while (in < end) {
        uint uc = *in;
        if (QChar(uc).isHighSurrogate() && in < end - 1) {
            ushort low = *(in + 1);
            if (QChar(low).isLowSurrogate()) {
                ++in;
                uc = QChar::surrogateToUcs4(uc, low);
            }
        }
        if (uc <= 0xFFFF) {
            if (uc < 0x80 ||
                !(uc <= 0x009F
                || uc == 0x00A0
                || uc == 0x0340
                || uc == 0x0341
                || uc == 0x06DD
                || uc == 0x070F
                || uc == 0x1680
                || uc == 0x180E
                || (uc >= 0x2000 && uc <= 0x200F)
                || (uc >= 0x2028 && uc <= 0x202F)
                || uc == 0x205F
                || (uc >= 0x2060 && uc <= 0x2063)
                || (uc >= 0x206A && uc <= 0x206F)
                || (uc >= 0x2FF0 && uc <= 0x2FFB)
                || uc == 0x3000
                || (uc >= 0xD800 && uc <= 0xDFFF)
                || (uc >= 0xE000 && uc <= 0xF8FF)
                || (uc >= 0xFDD0 && uc <= 0xFDEF)
                || uc == 0xFEFF
                || (uc >= 0xFFF9 && uc <= 0xFFFF))) {
                *out++ = *in;
            }
        } else {
            if (!((uc >= 0x1D173 && uc <= 0x1D17A)
                || (uc >= 0x1FFFE && uc <= 0x1FFFF)
                || (uc >= 0x2FFFE && uc <= 0x2FFFF)
                || (uc >= 0x3FFFE && uc <= 0x3FFFF)
                || (uc >= 0x4FFFE && uc <= 0x4FFFF)
                || (uc >= 0x5FFFE && uc <= 0x5FFFF)
                || (uc >= 0x6FFFE && uc <= 0x6FFFF)
                || (uc >= 0x7FFFE && uc <= 0x7FFFF)
                || (uc >= 0x8FFFE && uc <= 0x8FFFF)
                || (uc >= 0x9FFFE && uc <= 0x9FFFF)
                || (uc >= 0xAFFFE && uc <= 0xAFFFF)
                || (uc >= 0xBFFFE && uc <= 0xBFFFF)
                || (uc >= 0xCFFFE && uc <= 0xCFFFF)
                || (uc >= 0xDFFFE && uc <= 0xDFFFF)
                || uc == 0xE0001
                || (uc >= 0xE0020 && uc <= 0xE007F)
                || (uc >= 0xEFFFE && uc <= 0xEFFFF)
                || (uc >= 0xF0000 && uc <= 0xFFFFD)
                || (uc >= 0xFFFFE && uc <= 0xFFFFF)
                || (uc >= 0x100000 && uc <= 0x10FFFD)
                || (uc >= 0x10FFFE && uc <= 0x10FFFF))) {
                *out++ = QChar::highSurrogate(uc);
                *out++ = QChar::lowSurrogate(uc);
            }
        }
        ++in;
    }
    if (in != out)
        str->truncate(out - str->utf16());
}

static bool isBidirectionalRorAL(uint uc)
{
    if (uc < 0x5b0)
        return false;
    return uc == 0x05BE
        || uc == 0x05C0
        || uc == 0x05C3
        || (uc >= 0x05D0 && uc <= 0x05EA)
        || (uc >= 0x05F0 && uc <= 0x05F4)
        || uc == 0x061B
        || uc == 0x061F
        || (uc >= 0x0621 && uc <= 0x063A)
        || (uc >= 0x0640 && uc <= 0x064A)
        || (uc >= 0x066D && uc <= 0x066F)
        || (uc >= 0x0671 && uc <= 0x06D5)
        || uc == 0x06DD
        || (uc >= 0x06E5 && uc <= 0x06E6)
        || (uc >= 0x06FA && uc <= 0x06FE)
        || (uc >= 0x0700 && uc <= 0x070D)
        || uc == 0x0710
        || (uc >= 0x0712 && uc <= 0x072C)
        || (uc >= 0x0780 && uc <= 0x07A5)
        || uc == 0x07B1
        || uc == 0x200F
        || uc == 0xFB1D
        || (uc >= 0xFB1F && uc <= 0xFB28)
        || (uc >= 0xFB2A && uc <= 0xFB36)
        || (uc >= 0xFB38 && uc <= 0xFB3C)
        || uc == 0xFB3E
        || (uc >= 0xFB40 && uc <= 0xFB41)
        || (uc >= 0xFB43 && uc <= 0xFB44)
        || (uc >= 0xFB46 && uc <= 0xFBB1)
        || (uc >= 0xFBD3 && uc <= 0xFD3D)
        || (uc >= 0xFD50 && uc <= 0xFD8F)
        || (uc >= 0xFD92 && uc <= 0xFDC7)
        || (uc >= 0xFDF0 && uc <= 0xFDFC)
        || (uc >= 0xFE70 && uc <= 0xFE74)
        || (uc >= 0xFE76 && uc <= 0xFEFC);
}

static bool isBidirectionalL(uint uc)
{
    if (uc < 0xaa)
        return (uc >= 0x0041 && uc <= 0x005A)
            || (uc >= 0x0061 && uc <= 0x007A);

    if (uc == 0x00AA
        || uc == 0x00B5
        || uc == 0x00BA
        || (uc >= 0x00C0 && uc <= 0x00D6)
        || (uc >= 0x00D8 && uc <= 0x00F6)
        || (uc >= 0x00F8 && uc <= 0x0220)
        || (uc >= 0x0222 && uc <= 0x0233)
        || (uc >= 0x0250 && uc <= 0x02AD)
        || (uc >= 0x02B0 && uc <= 0x02B8)
        || (uc >= 0x02BB && uc <= 0x02C1)
        || (uc >= 0x02D0 && uc <= 0x02D1)
        || (uc >= 0x02E0 && uc <= 0x02E4)
        || uc == 0x02EE
        || uc == 0x037A
        || uc == 0x0386
        || (uc >= 0x0388 && uc <= 0x038A)) {
        return true;
    }

    if (uc == 0x038C
        || (uc >= 0x038E && uc <= 0x03A1)
        || (uc >= 0x03A3 && uc <= 0x03CE)
        || (uc >= 0x03D0 && uc <= 0x03F5)
        || (uc >= 0x0400 && uc <= 0x0482)
        || (uc >= 0x048A && uc <= 0x04CE)
        || (uc >= 0x04D0 && uc <= 0x04F5)
        || (uc >= 0x04F8 && uc <= 0x04F9)
        || (uc >= 0x0500 && uc <= 0x050F)
        || (uc >= 0x0531 && uc <= 0x0556)
        || (uc >= 0x0559 && uc <= 0x055F)
        || (uc >= 0x0561 && uc <= 0x0587)
        || uc == 0x0589
        || uc == 0x0903
        || (uc >= 0x0905 && uc <= 0x0939)
        || (uc >= 0x093D && uc <= 0x0940)
        || (uc >= 0x0949 && uc <= 0x094C)
        || uc == 0x0950) {
        return true;
    }

    if ((uc >= 0x0958 && uc <= 0x0961)
        || (uc >= 0x0964 && uc <= 0x0970)
        || (uc >= 0x0982 && uc <= 0x0983)
        || (uc >= 0x0985 && uc <= 0x098C)
        || (uc >= 0x098F && uc <= 0x0990)
        || (uc >= 0x0993 && uc <= 0x09A8)
        || (uc >= 0x09AA && uc <= 0x09B0)
        || uc == 0x09B2
        || (uc >= 0x09B6 && uc <= 0x09B9)
        || (uc >= 0x09BE && uc <= 0x09C0)
        || (uc >= 0x09C7 && uc <= 0x09C8)
        || (uc >= 0x09CB && uc <= 0x09CC)
        || uc == 0x09D7
        || (uc >= 0x09DC && uc <= 0x09DD)
        || (uc >= 0x09DF && uc <= 0x09E1)
        || (uc >= 0x09E6 && uc <= 0x09F1)
        || (uc >= 0x09F4 && uc <= 0x09FA)
        || (uc >= 0x0A05 && uc <= 0x0A0A)
        || (uc >= 0x0A0F && uc <= 0x0A10)
        || (uc >= 0x0A13 && uc <= 0x0A28)
        || (uc >= 0x0A2A && uc <= 0x0A30)
        || (uc >= 0x0A32 && uc <= 0x0A33)) {
        return true;
    }

    if ((uc >= 0x0A35 && uc <= 0x0A36)
        || (uc >= 0x0A38 && uc <= 0x0A39)
        || (uc >= 0x0A3E && uc <= 0x0A40)
        || (uc >= 0x0A59 && uc <= 0x0A5C)
        || uc == 0x0A5E
        || (uc >= 0x0A66 && uc <= 0x0A6F)
        || (uc >= 0x0A72 && uc <= 0x0A74)
        || uc == 0x0A83
        || (uc >= 0x0A85 && uc <= 0x0A8B)
        || uc == 0x0A8D
        || (uc >= 0x0A8F && uc <= 0x0A91)
        || (uc >= 0x0A93 && uc <= 0x0AA8)
        || (uc >= 0x0AAA && uc <= 0x0AB0)
        || (uc >= 0x0AB2 && uc <= 0x0AB3)
        || (uc >= 0x0AB5 && uc <= 0x0AB9)
        || (uc >= 0x0ABD && uc <= 0x0AC0)
        || uc == 0x0AC9
        || (uc >= 0x0ACB && uc <= 0x0ACC)
        || uc == 0x0AD0
        || uc == 0x0AE0
        || (uc >= 0x0AE6 && uc <= 0x0AEF)
        || (uc >= 0x0B02 && uc <= 0x0B03)
        || (uc >= 0x0B05 && uc <= 0x0B0C)
        || (uc >= 0x0B0F && uc <= 0x0B10)
        || (uc >= 0x0B13 && uc <= 0x0B28)
        || (uc >= 0x0B2A && uc <= 0x0B30)) {
        return true;
    }

    if ((uc >= 0x0B32 && uc <= 0x0B33)
        || (uc >= 0x0B36 && uc <= 0x0B39)
        || (uc >= 0x0B3D && uc <= 0x0B3E)
        || uc == 0x0B40
        || (uc >= 0x0B47 && uc <= 0x0B48)
        || (uc >= 0x0B4B && uc <= 0x0B4C)
        || uc == 0x0B57
        || (uc >= 0x0B5C && uc <= 0x0B5D)
        || (uc >= 0x0B5F && uc <= 0x0B61)
        || (uc >= 0x0B66 && uc <= 0x0B70)
        || uc == 0x0B83
        || (uc >= 0x0B85 && uc <= 0x0B8A)
        || (uc >= 0x0B8E && uc <= 0x0B90)
        || (uc >= 0x0B92 && uc <= 0x0B95)
        || (uc >= 0x0B99 && uc <= 0x0B9A)
        || uc == 0x0B9C
        || (uc >= 0x0B9E && uc <= 0x0B9F)
        || (uc >= 0x0BA3 && uc <= 0x0BA4)
        || (uc >= 0x0BA8 && uc <= 0x0BAA)
        || (uc >= 0x0BAE && uc <= 0x0BB5)
        || (uc >= 0x0BB7 && uc <= 0x0BB9)
        || (uc >= 0x0BBE && uc <= 0x0BBF)
        || (uc >= 0x0BC1 && uc <= 0x0BC2)
        || (uc >= 0x0BC6 && uc <= 0x0BC8)
        || (uc >= 0x0BCA && uc <= 0x0BCC)
        || uc == 0x0BD7
        || (uc >= 0x0BE7 && uc <= 0x0BF2)
        || (uc >= 0x0C01 && uc <= 0x0C03)
        || (uc >= 0x0C05 && uc <= 0x0C0C)
        || (uc >= 0x0C0E && uc <= 0x0C10)
        || (uc >= 0x0C12 && uc <= 0x0C28)
        || (uc >= 0x0C2A && uc <= 0x0C33)
        || (uc >= 0x0C35 && uc <= 0x0C39)) {
        return true;
    }
    if ((uc >= 0x0C41 && uc <= 0x0C44)
        || (uc >= 0x0C60 && uc <= 0x0C61)
        || (uc >= 0x0C66 && uc <= 0x0C6F)
        || (uc >= 0x0C82 && uc <= 0x0C83)
        || (uc >= 0x0C85 && uc <= 0x0C8C)
        || (uc >= 0x0C8E && uc <= 0x0C90)
        || (uc >= 0x0C92 && uc <= 0x0CA8)
        || (uc >= 0x0CAA && uc <= 0x0CB3)
        || (uc >= 0x0CB5 && uc <= 0x0CB9)
        || uc == 0x0CBE
        || (uc >= 0x0CC0 && uc <= 0x0CC4)
        || (uc >= 0x0CC7 && uc <= 0x0CC8)
        || (uc >= 0x0CCA && uc <= 0x0CCB)
        || (uc >= 0x0CD5 && uc <= 0x0CD6)
        || uc == 0x0CDE
        || (uc >= 0x0CE0 && uc <= 0x0CE1)
        || (uc >= 0x0CE6 && uc <= 0x0CEF)
        || (uc >= 0x0D02 && uc <= 0x0D03)
        || (uc >= 0x0D05 && uc <= 0x0D0C)
        || (uc >= 0x0D0E && uc <= 0x0D10)
        || (uc >= 0x0D12 && uc <= 0x0D28)
        || (uc >= 0x0D2A && uc <= 0x0D39)
        || (uc >= 0x0D3E && uc <= 0x0D40)
        || (uc >= 0x0D46 && uc <= 0x0D48)
        || (uc >= 0x0D4A && uc <= 0x0D4C)
        || uc == 0x0D57
        || (uc >= 0x0D60 && uc <= 0x0D61)
        || (uc >= 0x0D66 && uc <= 0x0D6F)
        || (uc >= 0x0D82 && uc <= 0x0D83)
        || (uc >= 0x0D85 && uc <= 0x0D96)
        || (uc >= 0x0D9A && uc <= 0x0DB1)
        || (uc >= 0x0DB3 && uc <= 0x0DBB)
        || uc == 0x0DBD) {
        return true;
    }
    if ((uc >= 0x0DC0 && uc <= 0x0DC6)
        || (uc >= 0x0DCF && uc <= 0x0DD1)
        || (uc >= 0x0DD8 && uc <= 0x0DDF)
        || (uc >= 0x0DF2 && uc <= 0x0DF4)
        || (uc >= 0x0E01 && uc <= 0x0E30)
        || (uc >= 0x0E32 && uc <= 0x0E33)
        || (uc >= 0x0E40 && uc <= 0x0E46)
        || (uc >= 0x0E4F && uc <= 0x0E5B)
        || (uc >= 0x0E81 && uc <= 0x0E82)
        || uc == 0x0E84
        || (uc >= 0x0E87 && uc <= 0x0E88)
        || uc == 0x0E8A
        || uc == 0x0E8D
        || (uc >= 0x0E94 && uc <= 0x0E97)
        || (uc >= 0x0E99 && uc <= 0x0E9F)
        || (uc >= 0x0EA1 && uc <= 0x0EA3)
        || uc == 0x0EA5
        || uc == 0x0EA7
        || (uc >= 0x0EAA && uc <= 0x0EAB)
        || (uc >= 0x0EAD && uc <= 0x0EB0)
        || (uc >= 0x0EB2 && uc <= 0x0EB3)
        || uc == 0x0EBD
        || (uc >= 0x0EC0 && uc <= 0x0EC4)
        || uc == 0x0EC6
        || (uc >= 0x0ED0 && uc <= 0x0ED9)
        || (uc >= 0x0EDC && uc <= 0x0EDD)
        || (uc >= 0x0F00 && uc <= 0x0F17)
        || (uc >= 0x0F1A && uc <= 0x0F34)
        || uc == 0x0F36
        || uc == 0x0F38
        || (uc >= 0x0F3E && uc <= 0x0F47)
        || (uc >= 0x0F49 && uc <= 0x0F6A)
        || uc == 0x0F7F
        || uc == 0x0F85
        || (uc >= 0x0F88 && uc <= 0x0F8B)
        || (uc >= 0x0FBE && uc <= 0x0FC5)
        || (uc >= 0x0FC7 && uc <= 0x0FCC)
        || uc == 0x0FCF) {
        return true;
    }

    if ((uc >= 0x1000 && uc <= 0x1021)
        || (uc >= 0x1023 && uc <= 0x1027)
        || (uc >= 0x1029 && uc <= 0x102A)
        || uc == 0x102C
        || uc == 0x1031
        || uc == 0x1038
        || (uc >= 0x1040 && uc <= 0x1057)
        || (uc >= 0x10A0 && uc <= 0x10C5)
        || (uc >= 0x10D0 && uc <= 0x10F8)
        || uc == 0x10FB
        || (uc >= 0x1100 && uc <= 0x1159)
        || (uc >= 0x115F && uc <= 0x11A2)
        || (uc >= 0x11A8 && uc <= 0x11F9)
        || (uc >= 0x1200 && uc <= 0x1206)
        || (uc >= 0x1208 && uc <= 0x1246)
        || uc == 0x1248
        || (uc >= 0x124A && uc <= 0x124D)
        || (uc >= 0x1250 && uc <= 0x1256)
        || uc == 0x1258
        || (uc >= 0x125A && uc <= 0x125D)
        || (uc >= 0x1260 && uc <= 0x1286)
        || uc == 0x1288
        || (uc >= 0x128A && uc <= 0x128D)
        || (uc >= 0x1290 && uc <= 0x12AE)
        || uc == 0x12B0
        || (uc >= 0x12B2 && uc <= 0x12B5)
        || (uc >= 0x12B8 && uc <= 0x12BE)
        || uc == 0x12C0
        || (uc >= 0x12C2 && uc <= 0x12C5)
        || (uc >= 0x12C8 && uc <= 0x12CE)
        || (uc >= 0x12D0 && uc <= 0x12D6)
        || (uc >= 0x12D8 && uc <= 0x12EE)
        || (uc >= 0x12F0 && uc <= 0x130E)
        || uc == 0x1310) {
        return true;
    }

    if ((uc >= 0x1312 && uc <= 0x1315)
        || (uc >= 0x1318 && uc <= 0x131E)
        || (uc >= 0x1320 && uc <= 0x1346)
        || (uc >= 0x1348 && uc <= 0x135A)
        || (uc >= 0x1361 && uc <= 0x137C)
        || (uc >= 0x13A0 && uc <= 0x13F4)
        || (uc >= 0x1401 && uc <= 0x1676)
        || (uc >= 0x1681 && uc <= 0x169A)
        || (uc >= 0x16A0 && uc <= 0x16F0)
        || (uc >= 0x1700 && uc <= 0x170C)
        || (uc >= 0x170E && uc <= 0x1711)
        || (uc >= 0x1720 && uc <= 0x1731)
        || (uc >= 0x1735 && uc <= 0x1736)
        || (uc >= 0x1740 && uc <= 0x1751)
        || (uc >= 0x1760 && uc <= 0x176C)
        || (uc >= 0x176E && uc <= 0x1770)
        || (uc >= 0x1780 && uc <= 0x17B6)
        || (uc >= 0x17BE && uc <= 0x17C5)
        || (uc >= 0x17C7 && uc <= 0x17C8)
        || (uc >= 0x17D4 && uc <= 0x17DA)
        || uc == 0x17DC
        || (uc >= 0x17E0 && uc <= 0x17E9)
        || (uc >= 0x1810 && uc <= 0x1819)
        || (uc >= 0x1820 && uc <= 0x1877)
        || (uc >= 0x1880 && uc <= 0x18A8)
        || (uc >= 0x1E00 && uc <= 0x1E9B)
        || (uc >= 0x1EA0 && uc <= 0x1EF9)
        || (uc >= 0x1F00 && uc <= 0x1F15)
        || (uc >= 0x1F18 && uc <= 0x1F1D)
        || (uc >= 0x1F20 && uc <= 0x1F45)
        || (uc >= 0x1F48 && uc <= 0x1F4D)
        || (uc >= 0x1F50 && uc <= 0x1F57)
        || uc == 0x1F59
        || uc == 0x1F5B
        || uc == 0x1F5D) {
        return true;
    }

    if ((uc >= 0x1F5F && uc <= 0x1F7D)
        || (uc >= 0x1F80 && uc <= 0x1FB4)
        || (uc >= 0x1FB6 && uc <= 0x1FBC)
        || uc == 0x1FBE
        || (uc >= 0x1FC2 && uc <= 0x1FC4)
        || (uc >= 0x1FC6 && uc <= 0x1FCC)
        || (uc >= 0x1FD0 && uc <= 0x1FD3)
        || (uc >= 0x1FD6 && uc <= 0x1FDB)
        || (uc >= 0x1FE0 && uc <= 0x1FEC)
        || (uc >= 0x1FF2 && uc <= 0x1FF4)
        || (uc >= 0x1FF6 && uc <= 0x1FFC)
        || uc == 0x200E
        || uc == 0x2071
        || uc == 0x207F
        || uc == 0x2102
        || uc == 0x2107
        || (uc >= 0x210A && uc <= 0x2113)
        || uc == 0x2115
        || (uc >= 0x2119 && uc <= 0x211D)) {
        return true;
    }

    if (uc == 0x2124
        || uc == 0x2126
        || uc == 0x2128
        || (uc >= 0x212A && uc <= 0x212D)
        || (uc >= 0x212F && uc <= 0x2131)
        || (uc >= 0x2133 && uc <= 0x2139)
        || (uc >= 0x213D && uc <= 0x213F)
        || (uc >= 0x2145 && uc <= 0x2149)
        || (uc >= 0x2160 && uc <= 0x2183)
        || (uc >= 0x2336 && uc <= 0x237A)
        || uc == 0x2395
        || (uc >= 0x249C && uc <= 0x24E9)
        || (uc >= 0x3005 && uc <= 0x3007)
        || (uc >= 0x3021 && uc <= 0x3029)
        || (uc >= 0x3031 && uc <= 0x3035)
        || (uc >= 0x3038 && uc <= 0x303C)
        || (uc >= 0x3041 && uc <= 0x3096)
        || (uc >= 0x309D && uc <= 0x309F)
        || (uc >= 0x30A1 && uc <= 0x30FA)) {
        return true;
    }

    if ((uc >= 0x30FC && uc <= 0x30FF)
        || (uc >= 0x3105 && uc <= 0x312C)
        || (uc >= 0x3131 && uc <= 0x318E)
        || (uc >= 0x3190 && uc <= 0x31B7)
        || (uc >= 0x31F0 && uc <= 0x321C)
        || (uc >= 0x3220 && uc <= 0x3243)) {
        return true;
    }

    if ((uc >= 0x3260 && uc <= 0x327B)
        || (uc >= 0x327F && uc <= 0x32B0)
        || (uc >= 0x32C0 && uc <= 0x32CB)
        || (uc >= 0x32D0 && uc <= 0x32FE)
        || (uc >= 0x3300 && uc <= 0x3376)
        || (uc >= 0x337B && uc <= 0x33DD)) {
        return true;
    }
    if  ((uc >= 0x33E0 && uc <= 0x33FE)
         || (uc >= 0x3400 && uc <= 0x4DB5)
         || (uc >= 0x4E00 && uc <= 0x9FA5)
         || (uc >= 0xA000 && uc <= 0xA48C)
         || (uc >= 0xAC00 && uc <= 0xD7A3)
         || (uc >= 0xD800 && uc <= 0xFA2D)
         || (uc >= 0xFA30 && uc <= 0xFA6A)
         || (uc >= 0xFB00 && uc <= 0xFB06)
         || (uc >= 0xFB13 && uc <= 0xFB17)
         || (uc >= 0xFF21 && uc <= 0xFF3A)
         || (uc >= 0xFF41 && uc <= 0xFF5A)
         || (uc >= 0xFF66 && uc <= 0xFFBE)
         || (uc >= 0xFFC2 && uc <= 0xFFC7)
         || (uc >= 0xFFCA && uc <= 0xFFCF)
         || (uc >= 0xFFD2 && uc <= 0xFFD7)
         || (uc >= 0xFFDA && uc <= 0xFFDC)) {
        return true;
    }

    if ((uc >= 0x10300 && uc <= 0x1031E)
       || (uc >= 0x10320 && uc <= 0x10323)
       || (uc >= 0x10330 && uc <= 0x1034A)
       || (uc >= 0x10400 && uc <= 0x10425)
       || (uc >= 0x10428 && uc <= 0x1044D)
       || (uc >= 0x1D000 && uc <= 0x1D0F5)
       || (uc >= 0x1D100 && uc <= 0x1D126)
       || (uc >= 0x1D12A && uc <= 0x1D166)
       || (uc >= 0x1D16A && uc <= 0x1D172)
       || (uc >= 0x1D183 && uc <= 0x1D184)
       || (uc >= 0x1D18C && uc <= 0x1D1A9)
       || (uc >= 0x1D1AE && uc <= 0x1D1DD)
       || (uc >= 0x1D400 && uc <= 0x1D454)
       || (uc >= 0x1D456 && uc <= 0x1D49C)
       || (uc >= 0x1D49E && uc <= 0x1D49F)
       || uc == 0x1D4A2
       || (uc >= 0x1D4A5 && uc <= 0x1D4A6)
       || (uc >= 0x1D4A9 && uc <= 0x1D4AC)
       || (uc >= 0x1D4AE && uc <= 0x1D4B9)
       || uc == 0x1D4BB
       || (uc >= 0x1D4BD && uc <= 0x1D4C0)
       || (uc >= 0x1D4C2 && uc <= 0x1D4C3)
       || (uc >= 0x1D4C5 && uc <= 0x1D505)
       || (uc >= 0x1D507 && uc <= 0x1D50A)
       || (uc >= 0x1D50D && uc <= 0x1D514)
       || (uc >= 0x1D516 && uc <= 0x1D51C)
       || (uc >= 0x1D51E && uc <= 0x1D539)
       || (uc >= 0x1D53B && uc <= 0x1D53E)
       || (uc >= 0x1D540 && uc <= 0x1D544)
       || uc == 0x1D546
       || (uc >= 0x1D54A && uc <= 0x1D550)
       || (uc >= 0x1D552 && uc <= 0x1D6A3)
       || (uc >= 0x1D6A8 && uc <= 0x1D7C9)
       || (uc >= 0x20000 && uc <= 0x2A6D6)
       || (uc >= 0x2F800 && uc <= 0x2FA1D)
       || (uc >= 0xF0000 && uc <= 0xFFFFD)
       || (uc >= 0x100000 && uc <= 0x10FFFD)) {
        return true;
    }

    return false;
}

#ifdef QT_BUILD_INTERNAL
// export for tst_qurl.cpp
Q_AUTOTEST_EXPORT void qt_nameprep(QString *source, int from);
Q_AUTOTEST_EXPORT bool qt_check_std3rules(const QChar *uc, int len);
#else
// non-test build, keep the symbols for ourselves
static void qt_nameprep(QString *source, int from);
static bool qt_check_std3rules(const QChar *uc, int len);
#endif

void qt_nameprep(QString *source, int from)
{
    QChar *src = source->data(); // causes a detach, so we're sure the only one using it
    QChar *out = src + from;
    const QChar *e = src + source->size();

    for ( ; out < e; ++out) {
        register ushort uc = out->unicode();
        if (uc > 0x80) {
            break;
        } else if (uc >= 'A' && uc <= 'Z') {
            *out = QChar(uc | 0x20);
        }
    }
    if (out == e)
        return; // everything was mapped easily (lowercased, actually)
    int firstNonAscii = out - src;

    // Characters unassigned in Unicode 3.2 are not allowed in "stored string" scheme
    // but allowed in "query" scheme
    // (Table A.1)
    const bool isUnassignedAllowed = false; // ###
    // Characters commonly mapped to nothing are simply removed
    // (Table B.1)
    const QChar *in = out;
    for ( ; in < e; ++in) {
        uint uc = in->unicode();
        if (QChar(uc).isHighSurrogate() && in < e - 1) {
            ushort low = in[1].unicode();
            if (QChar(low).isLowSurrogate()) {
                ++in;
                uc = QChar::surrogateToUcs4(uc, low);
            }
        }
        if (!isUnassignedAllowed) {
            QChar::UnicodeVersion version = QChar::unicodeVersion(uc);
            if (version == QChar::Unicode_Unassigned || version > QChar::Unicode_3_2) {
                source->resize(from); // not allowed, clear the label
                return;
            }
        }
        if (!isMappedToNothing(uc)) {
            if (uc <= 0xFFFF) {
                *out++ = *in;
            } else {
                *out++ = QChar::highSurrogate(uc);
                *out++ = QChar::lowSurrogate(uc);
            }
        }
    }
    if (out != in)
        source->truncate(out - src);

    // Map to lowercase (Table B.2)
    mapToLowerCase(source, firstNonAscii);

    // Normalize to Unicode 3.2 form KC
    extern void qt_string_normalize(QString *data, QString::NormalizationForm mode,
                                    QChar::UnicodeVersion version, int from);
    qt_string_normalize(source, QString::NormalizationForm_KC, QChar::Unicode_3_2,
                        firstNonAscii > from ? firstNonAscii - 1 : from);

    // Strip prohibited output
    stripProhibitedOutput(source, firstNonAscii);

    // Check for valid bidirectional characters
    bool containsLCat = false;
    bool containsRandALCat = false;
    src = source->data();
    e = src + source->size();
    for (in = src + from; in < e && (!containsLCat || !containsRandALCat); ++in) {
        uint uc = in->unicode();
        if (QChar(uc).isHighSurrogate() && in < e - 1) {
            ushort low = in[1].unicode();
            if (QChar(low).isLowSurrogate()) {
                ++in;
                uc = QChar::surrogateToUcs4(uc, low);
            }
        }
        if (isBidirectionalL(uc))
            containsLCat = true;
        else if (isBidirectionalRorAL(uc))
            containsRandALCat = true;
    }
    if (containsRandALCat) {
        if (containsLCat || (!isBidirectionalRorAL(src[from].unicode())
                             || !isBidirectionalRorAL(e[-1].unicode())))
            source->resize(from); // not allowed, clear the label
    }
}

bool qt_check_std3rules(const QChar *uc, int len)
{
    if (len > 63)
        return false;

    for (int i = 0; i < len; ++i) {
        register ushort c = uc[i].unicode();
        if (c == '-' && (i == 0 || i == len - 1))
            return false;

        // verifying the absence of LDH is the same as verifying that
        // only LDH is present
        if (c == '-' || (c >= '0' && c <= '9')
            || (c >= 'A' && c <= 'Z')
            || (c >= 'a' && c <= 'z')
            //underscore is not supposed to be allowed, but other browser accept it (QTBUG-7434)
            || c == '_')
            continue;

        return false;
    }

    return true;
}


static inline uint encodeDigit(uint digit)
{
  return digit + 22 + 75 * (digit < 26);
}

static inline uint adapt(uint delta, uint numpoints, bool firsttime)
{
    delta /= (firsttime ? damp : 2);
    delta += (delta / numpoints);

    uint k = 0;
    for (; delta > ((base - tmin) * tmax) / 2; k += base)
        delta /= (base - tmin);

    return k + (((base - tmin + 1) * delta) / (delta + skew));
}

static inline void appendEncode(QString* output, uint& delta, uint& bias, uint& b, uint& h)
{
    uint qq;
    uint k;
    uint t;

    // insert the variable length delta integer; fail on
    // overflow.
    for (qq = delta, k = base;; k += base) {
        // stop generating digits when the threshold is
        // detected.
        t = (k <= bias) ? tmin : (k >= bias + tmax) ? tmax : k - bias;
        if (qq < t) break;

        *output += QChar(encodeDigit(t + (qq - t) % (base - t)));
        qq = (qq - t) / (base - t);
    }

    *output += QChar(encodeDigit(qq));
    bias = adapt(delta, h + 1, h == b);
    delta = 0;
    ++h;
}

static void toPunycodeHelper(const QChar *s, int ucLength, QString *output)
{
    uint n = initial_n;
    uint delta = 0;
    uint bias = initial_bias;

    int outLen = output->length();
    output->resize(outLen + ucLength);

    QChar *d = output->data() + outLen;
    bool skipped = false;
    // copy all basic code points verbatim to output.
    for (uint j = 0; j < (uint) ucLength; ++j) {
        ushort js = s[j].unicode();
        if (js < 0x80)
            *d++ = js;
        else
            skipped = true;
    }

    // if there were only basic code points, just return them
    // directly; don't do any encoding.
    if (!skipped)
        return;

    output->truncate(d - output->constData());
    int copied = output->size() - outLen;

    // h and b now contain the number of basic code points in input.
    uint b = copied;
    uint h = copied;

    // if basic code points were copied, add the delimiter character.
    if (h > 0)
        *output += QChar(0x2d);

    // while there are still unprocessed non-basic code points left in
    // the input string...
    while (h < (uint) ucLength) {
        // find the character in the input string with the lowest
        // unicode value.
        uint m = Q_MAXINT;
        uint j;
        for (j = 0; j < (uint) ucLength; ++j) {
            if (s[j].unicode() >= n && s[j].unicode() < m)
                m = (uint) s[j].unicode();
        }

        // reject out-of-bounds unicode characters
        if (m - n > (Q_MAXINT - delta) / (h + 1)) {
            output->truncate(outLen);
            return; // punycode_overflow
        }

        delta += (m - n) * (h + 1);
        n = m;

        // for each code point in the input string
        for (j = 0; j < (uint) ucLength; ++j) {

            // increase delta until we reach the character with the
            // lowest unicode code. fail if delta overflows.
            if (s[j].unicode() < n) {
                ++delta;
                if (!delta) {
                    output->truncate(outLen);
                    return; // punycode_overflow
                }
            }

            // if j is the index of the character with the lowest
            // unicode code...
            if (s[j].unicode() == n) {
                appendEncode(output, delta, bias, b, h);
            }
        }

        ++delta;
        ++n;
    }

    // prepend ACE prefix
    output->insert(outLen, QLatin1String("xn--"));
    return;
}


static const char * const idn_whitelist[] = {
    "ac", "ar", "at",
    "biz", "br",
    "cat", "ch", "cl", "cn",
    "de", "dk",
    "es",
    "fi",
    "gr",
    "hu",
    "info", "io", "is",
    "jp",
    "kr",
    "li", "lt",
    "museum",
    "no",
    "org",
    "se", "sh",
    "th", "tm", "tw",
    "vn",
    "xn--mgbaam7a8h",           // UAE
    "xn--mgberp4a5d4ar",        // Saudi Arabia
    "xn--wgbh1c"                // Egypt
};

static QStringList *user_idn_whitelist = 0;

static bool lessThan(const QChar *a, int l, const char *c)
{
    const ushort *uc = (const ushort *)a;
    const ushort *e = uc + l;

    if (!c || *c == 0)
        return false;

    while (*c) {
        if (uc == e || *uc != *c)
            break;
        ++uc;
        ++c;
    }
    return (uc == e ? *c : *uc < *c);
}

static bool equal(const QChar *a, int l, const char *b)
{
    while (l && a->unicode() && *b) {
        if (*a != QLatin1Char(*b))
            return false;
        ++a;
        ++b;
        --l;
    }
    return l == 0;
}

static bool qt_is_idn_enabled(const QString &domain)
{
    int idx = domain.lastIndexOf(QLatin1Char('.'));
    if (idx == -1)
        return false;

    int len = domain.size() - idx - 1;
    QString tldString(domain.constData() + idx + 1, len);
    qt_nameprep(&tldString, 0);

    const QChar *tld = tldString.constData();

    if (user_idn_whitelist)
        return user_idn_whitelist->contains(tldString);

    int l = 0;
    int r = sizeof(idn_whitelist)/sizeof(const char *) - 1;
    int i = (l + r + 1) / 2;

    while (r != l) {
        if (lessThan(tld, len, idn_whitelist[i]))
            r = i - 1;
        else
            l = i;
        i = (l + r + 1) / 2;
    }
    return equal(tld, len, idn_whitelist[i]);
}

static inline bool isDotDelimiter(ushort uc)
{
    // IDNA / rfc3490 describes these four delimiters used for
    // separating labels in unicode international domain
    // names.
    return uc == 0x2e || uc == 0x3002 || uc == 0xff0e || uc == 0xff61;
}

static int nextDotDelimiter(const QString &domain, int from = 0)
{
    const QChar *b = domain.unicode();
    const QChar *ch = b + from;
    const QChar *e = b + domain.length();
    while (ch < e) {
        if (isDotDelimiter(ch->unicode()))
            break;
        else
            ++ch;
    }
    return ch - b;
}

enum AceOperation { ToAceOnly, NormalizeAce };
static QString qt_ACE_do(const QString &domain, AceOperation op)
{
    if (domain.isEmpty())
        return domain;

    QString result;
    result.reserve(domain.length());

    const bool isIdnEnabled = op == NormalizeAce ? qt_is_idn_enabled(domain) : false;
    int lastIdx = 0;
    QString aceForm; // this variable is here for caching

    while (1) {
        int idx = nextDotDelimiter(domain, lastIdx);
        int labelLength = idx - lastIdx;
        if (labelLength == 0) {
            if (idx == domain.length())
                break;
            return QString(); // two delimiters in a row -- empty label not allowed
        }

        // RFC 3490 says, about the ToASCII operation:
        //   3. If the UseSTD3ASCIIRules flag is set, then perform these checks:
        //
        //     (a) Verify the absence of non-LDH ASCII code points; that is, the
        //         absence of 0..2C, 2E..2F, 3A..40, 5B..60, and 7B..7F.
        //
        //     (b) Verify the absence of leading and trailing hyphen-minus; that
        //         is, the absence of U+002D at the beginning and end of the
        //         sequence.
        // and:
        //   8. Verify that the number of code points is in the range 1 to 63
        //      inclusive.

        // copy the label to the destination, which also serves as our scratch area, lowercasing it
        int prevLen = result.size();
        bool simple = true;
        result.resize(prevLen + labelLength);
        {
            QChar *out = result.data() + prevLen;
            const QChar *in = domain.constData() + lastIdx;
            const QChar *e = in + labelLength;
            for (; in < e; ++in, ++out) {
                register ushort uc = in->unicode();
                if (uc > 0x7f)
                    simple = false;
                if (uc >= 'A' && uc <= 'Z')
                    *out = QChar(uc | 0x20);
                else
                    *out = *in;
            }
        }

        if (simple && labelLength > 6) {
            // ACE form domains contain only ASCII characters, but we can't consider them simple
            // is this an ACE form?
            // the shortest valid ACE domain is 6 characters long (U+0080 would be 1, but it's not allowed)
            static const ushort acePrefixUtf16[] = { 'x', 'n', '-', '-' };
            if (memcmp(result.constData() + prevLen, acePrefixUtf16, sizeof acePrefixUtf16) == 0)
                simple = false;
        }

        if (simple) {
            // fastest case: this is the common case (non IDN-domains)
            // so we're done
            if (!qt_check_std3rules(result.constData() + prevLen, labelLength))
                return QString();
        } else {
            // Punycode encoding and decoding cannot be done in-place
            // That means we need one or two temporaries
            qt_nameprep(&result, prevLen);
            labelLength = result.length() - prevLen;
            register int toReserve = labelLength + 4 + 6; // "xn--" plus some extra bytes
            aceForm.resize(0);
            if (toReserve > aceForm.capacity())
                aceForm.reserve(toReserve);
            toPunycodeHelper(result.constData() + prevLen, result.size() - prevLen, &aceForm);

            // We use resize()+memcpy() here because we're overwriting the data we've copied
            if (isIdnEnabled) {
                QString tmp = QUrl::fromPunycode(aceForm.toLatin1());
                if (tmp.isEmpty())
                    return QString(); // shouldn't happen, since we've just punycode-encoded it
                result.resize(prevLen + tmp.size());
                memcpy(result.data() + prevLen, tmp.constData(), tmp.size() * sizeof(QChar));
            } else {
                result.resize(prevLen + aceForm.size());
                memcpy(result.data() + prevLen, aceForm.constData(), aceForm.size() * sizeof(QChar));
            }

            if (!qt_check_std3rules(aceForm.constData(), aceForm.size()))
                return QString();
        }


        lastIdx = idx + 1;
        if (lastIdx < domain.size() + 1)
            result += QLatin1Char('.');
        else
            break;
    }
    return result;
}


QUrlPrivate::QUrlPrivate()
{
    ref = 1;
    port = -1;
    isValid = false;
    isHostValid = true;
    parsingMode = QUrl::TolerantMode;
    valueDelimiter = '=';
    pairDelimiter = '&';
    stateFlags = 0;
    hasFragment = false;
    hasQuery = false;
}

// Called by normalized() and detach(). Must hold copy.mutex.
QUrlPrivate::QUrlPrivate(const QUrlPrivate &copy)
    : scheme(copy.scheme),
      userName(copy.userName),
      password(copy.password),
      host(copy.host),
      path(copy.path),
      query(copy.query),
      fragment(copy.fragment),
      encodedOriginal(copy.encodedOriginal),
      encodedUserName(copy.encodedUserName),
      encodedPassword(copy.encodedPassword),
      encodedPath(copy.encodedPath),
      encodedFragment(copy.encodedFragment),
      port(copy.port),
      parsingMode(copy.parsingMode),
      hasQuery(copy.hasQuery),
      hasFragment(copy.hasFragment),
      isValid(copy.isValid),
      isHostValid(copy.isHostValid),
      valueDelimiter(copy.valueDelimiter),
      pairDelimiter(copy.pairDelimiter),
      stateFlags(copy.stateFlags),
      encodedNormalized(copy.encodedNormalized)
{ ref = 1; }

QString QUrlPrivate::canonicalHost() const
{
    // Caller must lock mutex first
    if (QURL_HASFLAG(stateFlags, HostCanonicalized) || host.isEmpty())
        return host;

    QUrlPrivate *that = const_cast<QUrlPrivate *>(this);
    QURL_SETFLAG(that->stateFlags, HostCanonicalized);
    if (host.contains(QLatin1Char(':'))) {
        // This is an IP Literal, use _IPLiteral to validate
        QByteArray ba = host.toLatin1();
        bool needsBraces = false;
        if (!ba.startsWith('[')) {
            // surround the IP Literal with [ ] if it's not already done so
            ba.reserve(ba.length() + 2);
            ba.prepend('[');
            ba.append(']');
            needsBraces = true;
        }

        const char *ptr = ba.constData();
        if (!_IPLiteral(&ptr))
            that->host.clear();
        else if (needsBraces)
            that->host = QString::fromLatin1(ba.toLower());
        else
            that->host = host.toLower();
    } else {
        that->host = qt_ACE_do(host, NormalizeAce);
    }
    that->isHostValid = !that->host.isNull();
    return that->host;
}

// From RFC 3896, Appendix A Collected ABNF for URI
//    authority     = [ userinfo "@" ] host [ ":" port ]
//    userinfo      = *( unreserved / pct-encoded / sub-delims / ":" )
//    host          = IP-literal / IPv4address / reg-name
//    port          = *DIGIT
//[...]
//    pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
//
//    query         = *( pchar / "/" / "?" )
//
//    fragment      = *( pchar / "/" / "?" )
//
//    pct-encoded   = "%" HEXDIG HEXDIG
//
//    unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
//    reserved      = gen-delims / sub-delims
//    gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
//    sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
//                  / "*" / "+" / "," / ";" / "="

// use defines for concatenation:
#define ABNF_sub_delims         "!$&'()*+,;="
#define ABNF_gen_delims         ":/?#[]@"
#define ABNF_pchar              ABNF_sub_delims ":@"
#define ABNF_reserved           ABNF_sub_delims ABNF_gen_delims

// list the characters that don't have to be converted according to the list above.
// "unreserved" is already automatically not encoded, so we don't have to list it.
// the path component has a complex ABNF that basically boils down to
// slash-separated segments of "pchar"

static const char userNameExcludeChars[] = ABNF_sub_delims;
static const char passwordExcludeChars[] = ABNF_sub_delims ":";
static const char pathExcludeChars[]     = ABNF_pchar "/";
static const char queryExcludeChars[]    = ABNF_pchar "/?";
static const char fragmentExcludeChars[] = ABNF_pchar "/?";

void QUrlPrivate::ensureEncodedParts() const
{
    QUrlPrivate *that = const_cast<QUrlPrivate *>(this);

    if (encodedUserName.isNull())
        // userinfo = *( unreserved / pct-encoded / sub-delims / ":" )
        that->encodedUserName = toPercentEncodingHelper(userName, userNameExcludeChars);
    if (encodedPassword.isNull())
        // userinfo = *( unreserved / pct-encoded / sub-delims / ":" )
        that->encodedPassword = toPercentEncodingHelper(password, passwordExcludeChars);
    if (encodedPath.isNull())
        // pchar = unreserved / pct-encoded / sub-delims / ":" / "@" ... also "/"
        that->encodedPath = toPercentEncodingHelper(path, pathExcludeChars);
    if (encodedFragment.isNull())
        // fragment      = *( pchar / "/" / "?" )
        that->encodedFragment = toPercentEncodingHelper(fragment, fragmentExcludeChars);
}

QString QUrlPrivate::authority(QUrl::FormattingOptions options) const
{
    // Caller must lock mutex first

    if ((options & QUrl::RemoveAuthority) == QUrl::RemoveAuthority)
        return QString();

    QString tmp = userInfo(options);
    if (!tmp.isEmpty())
        tmp += QLatin1Char('@');
    tmp += canonicalHost();
    if (!(options & QUrl::RemovePort) && port != -1)
        tmp += QLatin1Char(':') + QString::number(port);

    return tmp;
}

void QUrlPrivate::setAuthority(const QString &auth)
{
    isHostValid = true;
    if (auth.isEmpty()) {
        setUserInfo(QString());
        host.clear();
        port = -1;
        return;
    }

    // find the port section of the authority by searching from the
    // end towards the beginning for numbers until a ':' is reached.
    int portIndex = auth.length() - 1;
    if (portIndex == 0) {
        portIndex = -1;
    } else {
        short c = auth.at(portIndex--).unicode();
        if (c < '0' || c > '9') {
            portIndex = -1;
        } else while (portIndex >= 0) {
            c = auth.at(portIndex).unicode();
            if (c == ':') {
                break;
            } else if (c == '.') {
                portIndex = -1;
                break;
            }
            --portIndex;
        }
    }

    if (portIndex != -1) {
        port = 0;
        for (int i = portIndex + 1; i < auth.length(); ++i)
            port = (port * 10) + (auth.at(i).unicode() - '0');
    } else {
        port = -1;
    }

    int userInfoIndex = auth.indexOf(QLatin1Char('@'));
    if (userInfoIndex != -1 && (portIndex == -1 || userInfoIndex < portIndex))
        setUserInfo(auth.left(userInfoIndex));

    int hostIndex = 0;
    if (userInfoIndex != -1)
        hostIndex = userInfoIndex + 1;
    int hostLength = auth.length() - hostIndex;
    if (portIndex != -1)
        hostLength -= (auth.length() - portIndex);

    host = auth.mid(hostIndex, hostLength).trimmed();
}

void QUrlPrivate::setUserInfo(const QString &userInfo)
{
    encodedUserName.clear();
    encodedPassword.clear();

    int delimIndex = userInfo.indexOf(QLatin1Char(':'));
    if (delimIndex == -1) {
        userName = userInfo;
        password.clear();
        return;
    }
    userName = userInfo.left(delimIndex);
    password = userInfo.right(userInfo.length() - delimIndex - 1);
}

void QUrlPrivate::setEncodedUserInfo(const QUrlParseData *parseData)
{
    userName.clear();
    password.clear();
    if (!parseData->userInfoLength) {
        encodedUserName.clear();
        encodedPassword.clear();
    } else if (parseData->userInfoDelimIndex == -1) {
        encodedUserName = QByteArray(parseData->userInfo, parseData->userInfoLength);
        encodedPassword.clear();
    } else {
        encodedUserName = QByteArray(parseData->userInfo, parseData->userInfoDelimIndex);
        encodedPassword = QByteArray(parseData->userInfo + parseData->userInfoDelimIndex + 1,
                                     parseData->userInfoLength - parseData->userInfoDelimIndex - 1);
    }
}

QString QUrlPrivate::userInfo(QUrl::FormattingOptions options) const
{
    if ((options & QUrl::RemoveUserInfo) == QUrl::RemoveUserInfo)
        return QString();

    QUrlPrivate *that = const_cast<QUrlPrivate *>(this);
    if (userName.isNull())
        that->userName = fromPercentEncodingHelper(encodedUserName);
    if (password.isNull())
        that->password = fromPercentEncodingHelper(encodedPassword);

    QString tmp = userName;

    if (!(options & QUrl::RemovePassword) && !password.isEmpty()) {
        tmp += QLatin1Char(':');
        tmp += password;
    }

    return tmp;
}

/*
    From http://www.ietf.org/rfc/rfc3986.txt, 5.2.3: Merge paths

    Returns a merge of the current path with the relative path passed
    as argument.
*/
QByteArray QUrlPrivate::mergePaths(const QByteArray &relativePath) const
{
    // Caller must lock mutex first

    if (encodedPath.isNull())
        ensureEncodedParts();

    // If the base URI has a defined authority component and an empty
    // path, then return a string consisting of "/" concatenated with
    // the reference's path; otherwise,
    if (!authority().isEmpty() && encodedPath.isEmpty())
        return '/' + relativePath;

    // Return a string consisting of the reference's path component
    // appended to all but the last segment of the base URI's path
    // (i.e., excluding any characters after the right-most "/" in the
    // base URI path, or excluding the entire base URI path if it does
    // not contain any "/" characters).
    QByteArray newPath;
    if (!encodedPath.contains('/'))
        newPath = relativePath;
    else
        newPath = encodedPath.left(encodedPath.lastIndexOf('/') + 1) + relativePath;

    return newPath;
}

void QUrlPrivate::queryItem(int pos, int *value, int *end)
{
    *end = query.indexOf(pairDelimiter, pos);
    if (*end == -1)
        *end = query.size();
    *value = pos;
    while (*value < *end) {
        if (query[*value] == valueDelimiter)
            break;
        ++*value;
    }
}

/*
    From http://www.ietf.org/rfc/rfc3986.txt, 5.2.4: Remove dot segments

    Removes unnecessary ../ and ./ from the path. Used for normalizing
    the URL.
*/
static void removeDotsFromPath(QByteArray *path)
{
    // The input buffer is initialized with the now-appended path
    // components and the output buffer is initialized to the empty
    // string.
    char *out = path->data();
    const char *in = out;
    const char *end = out + path->size();

    // If the input buffer consists only of
    // "." or "..", then remove that from the input
    // buffer;
    if (path->size() == 1 && in[0] == '.')
        ++in;
    else if (path->size() == 2 && in[0] == '.' && in[1] == '.')
        in += 2;
    // While the input buffer is not empty, loop:
    while (in < end) {

        // otherwise, if the input buffer begins with a prefix of "../" or "./",
        // then remove that prefix from the input buffer;
        if (path->size() >= 2 && in[0] == '.' && in[1] == '/')
            in += 2;
        else if (path->size() >= 3 && in[0] == '.' && in[1] == '.' && in[2] == '/')
            in += 3;

        // otherwise, if the input buffer begins with a prefix of
        // "/./" or "/.", where "." is a complete path segment,
        // then replace that prefix with "/" in the input buffer;
        if (in <= end - 3 && in[0] == '/' && in[1] == '.' && in[2] == '/') {
            in += 2;
            continue;
        } else if (in == end - 2 && in[0] == '/' && in[1] == '.') {
            *out++ = '/';
            in += 2;
            break;
        }

        // otherwise, if the input buffer begins with a prefix
        // of "/../" or "/..", where ".." is a complete path
        // segment, then replace that prefix with "/" in the
        // input buffer and remove the last //segment and its
        // preceding "/" (if any) from the output buffer;
        if (in <= end - 4 && in[0] == '/' && in[1] == '.' && in[2] == '.' && in[3] == '/') {
            while (out > path->constData() && *(--out) != '/')
                ;
            if (out == path->constData() && *out != '/')
                ++in;
            in += 3;
            continue;
        } else if (in == end - 3 && in[0] == '/' && in[1] == '.' && in[2] == '.') {
            while (out > path->constData() && *(--out) != '/')
                ;
            if (*out == '/')
                ++out;
            in += 3;
            break;
        }

        // otherwise move the first path segment in
        // the input buffer to the end of the output
        // buffer, including the initial "/" character
        // (if any) and any subsequent characters up
        // to, but not including, the next "/"
        // character or the end of the input buffer.
        *out++ = *in++;
        while (in < end && *in != '/')
            *out++ = *in++;
    }
    path->truncate(out - path->constData());
}

void QUrlPrivate::validate() const
{
    // Caller must lock mutex first
    QUrlPrivate *that = const_cast<QUrlPrivate *>(this);
    that->encodedOriginal = that->toEncoded(); // may detach
    parse(ParseOnly);

    QURL_SETFLAG(that->stateFlags, Validated);

    if (!isValid)
        return;

    QString auth = authority(); // causes the non-encoded forms to be valid

    // authority() calls canonicalHost() which sets this
    if (!isHostValid)
        return;

    if (scheme == QLatin1String("mailto")) {
        if (!host.isEmpty() || port != -1 || !userName.isEmpty() || !password.isEmpty()) {
            that->isValid = false;
            that->errorInfo.setParams(0, QT_TRANSLATE_NOOP(QUrl, "expected empty host, username,"
                                                           "port and password"),
                                      0, 0);
        }
    } else if (scheme == QLatin1String("ftp") || scheme == QLatin1String("http")) {
        if (host.isEmpty() && !(path.isEmpty() && encodedPath.isEmpty())) {
            that->isValid = false;
            that->errorInfo.setParams(0, QT_TRANSLATE_NOOP(QUrl, "the host is empty, but not the path"),
                                      0, 0);
        }
    }
}

void QUrlPrivate::parse(ParseOptions parseOptions) const
{
    // Caller must lock mutex first
    QUrlPrivate *that = const_cast<QUrlPrivate *>(this);
    that->errorInfo.setParams(0, 0, 0, 0);
    if (encodedOriginal.isEmpty()) {
        that->isValid = false;
        that->errorInfo.setParams(0, QT_TRANSLATE_NOOP(QUrl, "empty"),
                                  0, 0);
        QURL_SETFLAG(that->stateFlags, Validated | Parsed);
        return;
    }


    QUrlParseData parseData;
    memset(&parseData, 0, sizeof(parseData));
    parseData.userInfoDelimIndex = -1;
    parseData.port = -1;

    const char *pptr = (char *) encodedOriginal.constData();
    const char **ptr = &pptr;

#if defined (QURL_DEBUG)
    qDebug("QUrlPrivate::parse(), parsing \"%s\"", pptr);
#endif

    // optional scheme
    bool isSchemeValid = _scheme(ptr, &parseData);

    if (isSchemeValid == false) {
        that->isValid = false;
        char ch = *((*ptr)++);
        that->errorInfo.setParams(*ptr, QT_TRANSLATE_NOOP(QUrl, "unexpected URL scheme"),
                                  0, ch);
        QURL_SETFLAG(that->stateFlags, Validated | Parsed);
#if defined (QURL_DEBUG)
        qDebug("QUrlPrivate::parse(), unrecognized: %c%s", ch, *ptr);
#endif
        return;
    }

    // hierpart
    _hierPart(ptr, &parseData);

    // optional query
    char ch = *((*ptr)++);
    if (ch == '?') {
        that->hasQuery = true;
        _query(ptr, &parseData);
        ch = *((*ptr)++);
    }

    // optional fragment
    if (ch == '#') {
        that->hasFragment = true;
        _fragment(ptr, &parseData);
    } else if (ch != '\0') {
        that->isValid = false;
        that->errorInfo.setParams(*ptr, QT_TRANSLATE_NOOP(QUrl, "expected end of URL"),
                                  0, ch);
        QURL_SETFLAG(that->stateFlags, Validated | Parsed);
#if defined (QURL_DEBUG)
        qDebug("QUrlPrivate::parse(), unrecognized: %c%s", ch, *ptr);
#endif
        return;
    }

    // when doing lazy validation, this function is called after
    // encodedOriginal has been constructed from the individual parts,
    // only to see if the constructed URL can be parsed. in that case,
    // parse() is called in ParseOnly mode; we don't want to set all
    // the members over again.
    if (parseOptions == ParseAndSet) {
        QURL_UNSETFLAG(that->stateFlags, HostCanonicalized);

        if (parseData.scheme) {
            QByteArray s(parseData.scheme, parseData.schemeLength);
            that->scheme = fromPercentEncodingMutable(&s);
        }

        that->setEncodedUserInfo(&parseData);

        QByteArray h(parseData.host, parseData.hostLength);
        that->host = fromPercentEncodingMutable(&h);
        that->port = parseData.port;

        that->path.clear();
        that->encodedPath = QByteArray(parseData.path, parseData.pathLength);

        if (that->hasQuery)
            that->query = QByteArray(parseData.query, parseData.queryLength);
        else
            that->query.clear();

        that->fragment.clear();
        if (that->hasFragment) {
            that->encodedFragment = QByteArray(parseData.fragment, parseData.fragmentLength);
        } else {
            that->encodedFragment.clear();
        }
    }

    that->isValid = true;
    QURL_SETFLAG(that->stateFlags, Parsed);

#if defined (QURL_DEBUG)
    qDebug("QUrl::setUrl(), scheme = %s", that->scheme.toLatin1().constData());
    qDebug("QUrl::setUrl(), userInfo = %s", that->userInfo.toLatin1().constData());
    qDebug("QUrl::setUrl(), host = %s", that->host.toLatin1().constData());
    qDebug("QUrl::setUrl(), port = %i", that->port);
    qDebug("QUrl::setUrl(), path = %s", fromPercentEncodingHelper(__path).toLatin1().constData());
    qDebug("QUrl::setUrl(), query = %s", __query.constData());
    qDebug("QUrl::setUrl(), fragment = %s", fromPercentEncodingHelper(__fragment).toLatin1().constData());
#endif
}

void QUrlPrivate::clear()
{
    scheme.clear();
    userName.clear();
    password.clear();
    host.clear();
    port = -1;
    path.clear();
    query.clear();
    fragment.clear();

    encodedOriginal.clear();
    encodedUserName.clear();
    encodedPassword.clear();
    encodedPath.clear();
    encodedFragment.clear();
    encodedNormalized.clear();

    isValid = false;
    hasQuery = false;
    hasFragment = false;

    valueDelimiter = '=';
    pairDelimiter = '&';

    QURL_UNSETFLAG(stateFlags, Parsed | Validated | Normalized | HostCanonicalized);
}

QByteArray QUrlPrivate::toEncoded(QUrl::FormattingOptions options) const
{
    // Caller must lock mutex first
    if (!QURL_HASFLAG(stateFlags, Parsed)) parse();
    else ensureEncodedParts();

    if (options==0x100) // private - see qHash(QUrl)
        return normalized();

    QByteArray url;

    if (!(options & QUrl::RemoveScheme) && !scheme.isEmpty()) {
        url += scheme.toLatin1();
        url += ':';
    }
    QString savedHost = host;  // pre-validation, may be invalid!
    QString auth = authority();
    bool doFileScheme = scheme == QLatin1String("file") && encodedPath.startsWith('/');
    if ((options & QUrl::RemoveAuthority) != QUrl::RemoveAuthority && (!auth.isEmpty() || doFileScheme || !savedHost.isEmpty())) {
        if (doFileScheme && !encodedPath.startsWith('/'))
            url += '/';
        url += "//";

        if ((options & QUrl::RemoveUserInfo) != QUrl::RemoveUserInfo) {
            bool hasUserOrPass = false;
            if (!userName.isEmpty()) {
                url += encodedUserName;
                hasUserOrPass = true;
            }
            if (!(options & QUrl::RemovePassword) && !password.isEmpty()) {
                url += ':';
                url += encodedPassword;
                hasUserOrPass = true;
            }
            if (hasUserOrPass)
                url += '@';
        }

        if (host.startsWith(QLatin1Char('['))) {
            url += host.toLatin1();
        } else if (host.contains(QLatin1Char(':'))) {
            url += '[';
            url += host.toLatin1();
            url += ']';
        } else if (host.isEmpty() && !savedHost.isEmpty()) {
            // this case is only possible with an invalid URL
            // it's here only so that we can keep the original, invalid hostname
            // in encodedOriginal.
            // QUrl::isValid() will return false, so toEncoded() can be anything (it's not valid)
            url += savedHost.toUtf8();
        } else {
            url += QUrl::toAce(host);
        }
        if (!(options & QUrl::RemovePort) && port != -1) {
            url += ':';
            url += QString::number(port).toAscii();
        }
    }

    if (!(options & QUrl::RemovePath)) {
        // check if we need to insert a slash
        if (!encodedPath.isEmpty() && !auth.isEmpty()) {
            if (!encodedPath.startsWith('/'))
                url += '/';
        }
        url += encodedPath;

        // check if we need to remove trailing slashes
        while ((options & QUrl::StripTrailingSlash) && url.endsWith('/'))
            url.chop(1);
    }

    if (!(options & QUrl::RemoveQuery) && hasQuery) {
        url += '?';
        url += query;
    }
    if (!(options & QUrl::RemoveFragment) && hasFragment) {
        url += '#';
        url += encodedFragment;
    }

    return url;
}

#define qToLower(ch) (((ch|32) >= 'a' && (ch|32) <= 'z') ? (ch|32) : ch)

const QByteArray &QUrlPrivate::normalized() const
{
    // Caller must lock mutex first
    if (QURL_HASFLAG(stateFlags, QUrlPrivate::Normalized))
        return encodedNormalized;

    QUrlPrivate *that = const_cast<QUrlPrivate *>(this);
    QURL_SETFLAG(that->stateFlags, QUrlPrivate::Normalized);

    QUrlPrivate tmp = *this;
    tmp.scheme = tmp.scheme.toLower();
    tmp.host = tmp.canonicalHost();

    // ensure the encoded and normalized parts of the URL
    tmp.ensureEncodedParts();
    if (tmp.encodedUserName.contains('%'))
        q_normalizePercentEncoding(&tmp.encodedUserName, userNameExcludeChars);
    if (tmp.encodedPassword.contains('%'))
        q_normalizePercentEncoding(&tmp.encodedPassword, passwordExcludeChars);
    if (tmp.encodedFragment.contains('%'))
        q_normalizePercentEncoding(&tmp.encodedFragment, fragmentExcludeChars);

    if (tmp.encodedPath.contains('%')) {
        // the path is a bit special:
        // the slashes shouldn't be encoded or decoded.
        // They should remain exactly like they are right now
        //
        // treat the path as a slash-separated sequence of pchar
        QByteArray result;
        result.reserve(tmp.encodedPath.length());
        if (tmp.encodedPath.startsWith('/'))
            result.append('/');

        const char *data = tmp.encodedPath.constData();
        int lastSlash = 0;
        int nextSlash;
        do {
            ++lastSlash;
            nextSlash = tmp.encodedPath.indexOf('/', lastSlash);
            int len;
            if (nextSlash == -1)
                len = tmp.encodedPath.length() - lastSlash;
            else
                len = nextSlash - lastSlash;

            if (memchr(data + lastSlash, '%', len)) {
                // there's at least one percent before the next slash
                QByteArray block = QByteArray(data + lastSlash, len);
                q_normalizePercentEncoding(&block, pathExcludeChars);
                result.append(block);
            } else {
                // no percents in this path segment, append wholesale
                result.append(data + lastSlash, len);
            }

            // append the slash too, if it's there
            if (nextSlash != -1)
                result.append('/');

            lastSlash = nextSlash;
        } while (lastSlash != -1);

        tmp.encodedPath = result;
    }

    if (!tmp.scheme.isEmpty()) // relative test
        removeDotsFromPath(&tmp.encodedPath);

    int qLen = tmp.query.length();
    for (int i = 0; i < qLen; i++) {
        if (qLen - i > 2 && tmp.query.at(i) == '%') {
            ++i;
            tmp.query[i] = qToLower(tmp.query.at(i));
            ++i;
            tmp.query[i] = qToLower(tmp.query.at(i));
        }
    }
    encodedNormalized = tmp.toEncoded();

    return encodedNormalized;
}

QString QUrlPrivate::createErrorString()
{
    if (isValid && isHostValid)
        return QString();

    QString errorString(QLatin1String(QT_TRANSLATE_NOOP(QUrl, "Invalid URL \"")));
    errorString += QLatin1String(encodedOriginal.constData());
    errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, "\""));

    if (errorInfo._source) {
        int position = encodedOriginal.indexOf(errorInfo._source) - 1;
        if (position > 0) {
            errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, ": error at position "));
            errorString += QString::number(position);
        } else {
            errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, ": "));
            errorString += QLatin1String(errorInfo._source);
        }
    }

    if (errorInfo._expected) {
        errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, ": expected \'"));
        errorString += QLatin1Char(errorInfo._expected);
        errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, "\'"));
    } else {
        errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, ": "));
        if (isHostValid)
            errorString += QLatin1String(errorInfo._message);
        else
            errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, "invalid hostname"));
    }
    if (errorInfo._found) {
        errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, ", but found \'"));
        errorString += QLatin1Char(errorInfo._found);
        errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, "\'"));
    }
    return errorString;
}

/*!
    \macro QT_NO_URL_CAST_FROM_STRING
    \relates QUrl

    Disables automatic conversions from QString (or char *) to QUrl.

    Compiling your code with this define is useful when you have a lot of
    code that uses QString for file names and you wish to convert it to
    use QUrl for network transparency. In any code that uses QUrl, it can
    help avoid missing QUrl::resolved() calls, and other misuses of
    QString to QUrl conversions.

    \oldcode
        url = filename; // probably not what you want
    \newcode
        url = QUrl::fromLocalFile(filename);
        url = baseurl.resolved(QUrl(filename));
    \endcode

    \sa QT_NO_CAST_FROM_ASCII
*/


/*!
    Constructs a URL by parsing \a url. \a url is assumed to be in human
    readable representation, with no percent encoding. QUrl will automatically
    percent encode all characters that are not allowed in a URL.
    The default parsing mode is TolerantMode.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qurl.cpp 0

    To construct a URL from an encoded string, call fromEncoded():

    \snippet doc/src/snippets/code/src_corelib_io_qurl.cpp 1

    \sa setUrl(), setEncodedUrl(), fromEncoded(), TolerantMode
*/
QUrl::QUrl(const QString &url) : d(0)
{
    if (!url.isEmpty())
        setUrl(url);
}

/*!
    \overload

    Parses the \a url using the parser mode \a parsingMode.
    The default parsing mode is TolerantMode.

    \sa setUrl()
*/
QUrl::QUrl(const QString &url, ParsingMode parsingMode) : d(0)
{
    if (!url.isEmpty())
        setUrl(url, parsingMode);
    else {
        d = new QUrlPrivate;
        d->parsingMode = parsingMode;
    }
}

/*!
    Constructs an empty QUrl object.
*/
QUrl::QUrl() : d(0)
{
}

/*!
    Constructs a copy of \a other.
*/
QUrl::QUrl(const QUrl &other) : d(other.d)
{
    if (d)
        d->ref.ref();
}

/*!
    Destructor; called immediately before the object is deleted.
*/
QUrl::~QUrl()
{
    if (d && !d->ref.deref())
        delete d;
}

/*!
    Returns true if the URL is valid; otherwise returns false.

    The URL is run through a conformance test. Every part of the URL
    must conform to the standard encoding rules of the URI standard
    for the URL to be reported as valid.

    \snippet doc/src/snippets/code/src_corelib_io_qurl.cpp 2
*/
bool QUrl::isValid() const
{
    if (!d) return false;

    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Validated)) d->validate();

    return d->isValid && d->isHostValid;
}

/*!
    Returns true if the URL has no data; otherwise returns false.
*/
bool QUrl::isEmpty() const
{
    if (!d) return true;

    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed))
        return d->encodedOriginal.isEmpty();
    else
        return d->scheme.isEmpty()   // no encodedScheme
        && d->userName.isEmpty() && d->encodedUserName.isEmpty()
        && d->password.isEmpty() && d->encodedPassword.isEmpty()
        && d->host.isEmpty()   // no encodedHost
        && d->port == -1
        && d->path.isEmpty() && d->encodedPath.isEmpty()
        && d->query.isEmpty()
        && d->fragment.isEmpty() && d->encodedFragment.isEmpty();
}

/*!
    Resets the content of the QUrl. After calling this function, the
    QUrl is equal to one that has been constructed with the default
    empty constructor.
*/
void QUrl::clear()
{
    if (d && !d->ref.deref())
        delete d;
    d = 0;
}

/*!
    Constructs a URL by parsing the contents of \a url.

    \a url is assumed to be in unicode format, with no percent
    encoding.

    Calling isValid() will tell whether or not a valid URL was
    constructed.

    \sa setEncodedUrl()
*/
void QUrl::setUrl(const QString &url)
{
    setUrl(url, TolerantMode);
}

/*!
    \overload

    Parses \a url using the parsing mode \a parsingMode.

    \sa setEncodedUrl()
*/
void QUrl::setUrl(const QString &url, ParsingMode parsingMode)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    detach(lock);
    d->clear();

    // escape all reserved characters and delimiters
    // reserved      = gen-delims / sub-delims
    if (parsingMode != TolerantMode) {
        d->setEncodedUrl(toPercentEncodingHelper(url, ABNF_reserved), parsingMode);
        return;
    }

    // Tolerant preprocessing
    QString tmp = url;

    // Allow %20 in the QString variant
    tmp.replace(QLatin1String("%20"), QLatin1String(" "));

    // Percent-encode unsafe ASCII characters after host part
    int start = tmp.indexOf(QLatin1String("//"));
    if (start != -1) {
        // Has host part, find delimiter
        start += 2; // skip "//"
        const char delims[] = "/#?";
        const char *d = delims;
        int hostEnd = -1;
        while (*d && (hostEnd = tmp.indexOf(QLatin1Char(*d), start)) == -1)
            ++d;
        start = (hostEnd == -1) ? -1 : hostEnd + 1;
    } else {
        start = 0; // Has no host part
    }
    QByteArray encodedUrl;
    if (start != -1) {
        QString hostPart = tmp.left(start);
        QString otherPart = tmp.mid(start);
        encodedUrl = toPercentEncodingHelper(hostPart, ":/?#[]@!$&'()*+,;=")
                   + toPercentEncodingHelper(otherPart, ":/?#@!$&'()*+,;=");
    } else {
        encodedUrl = toPercentEncodingHelper(tmp, ABNF_reserved);
    }
    d->setEncodedUrl(encodedUrl, StrictMode);
}

/*!
    Constructs a URL by parsing the contents of \a encodedUrl.

    \a encodedUrl is assumed to be a URL string in percent encoded
    form, containing only ASCII characters.

    Use isValid() to determine if a valid URL was constructed.

    \sa setUrl()
*/
void QUrl::setEncodedUrl(const QByteArray &encodedUrl)
{
    setEncodedUrl(encodedUrl, TolerantMode);
}

inline static bool isHex(char c)
{
    c |= 0x20;
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

static inline char toHex(quint8 c)
{
    return c > 9 ? c - 10 + 'A' : c + '0';
}

/*!
    Constructs a URL by parsing the contents of \a encodedUrl using
    the given \a parsingMode.
*/
void QUrl::setEncodedUrl(const QByteArray &encodedUrl, ParsingMode parsingMode)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    detach(lock);
    d->clear();
    d->setEncodedUrl(encodedUrl, parsingMode);
}

void QUrlPrivate::setEncodedUrl(const QByteArray &encodedUrl, QUrl::ParsingMode parsingMode)
{
    // Caller must lock mutex first
    QByteArray tmp = encodedUrl;
    if ((this->parsingMode = parsingMode) == QUrl::TolerantMode) {
        // Replace stray % with %25
        QByteArray copy = tmp;
        for (int i = 0, j = 0; i < copy.size(); ++i, ++j) {
            if (copy.at(i) == '%') {
                if (i + 2 >= copy.size() || !isHex(copy.at(i + 1)) || !isHex(copy.at(i + 2))) {
                    tmp.replace(j, 1, "%25");
                    j += 2;
                }
            }
        }

        // Find the host part
        int hostStart = tmp.indexOf("//");
        int hostEnd = -1;
        if (hostStart != -1) {
            // Has host part, find delimiter
            hostStart += 2; // skip "//"
            hostEnd = tmp.indexOf('/', hostStart);
            if (hostEnd == -1)
                hostEnd = tmp.indexOf('#', hostStart);
            if (hostEnd == -1)
                hostEnd = tmp.indexOf('?');
            if (hostEnd == -1)
                hostEnd = tmp.length() - 1;
        }

        // Reserved and unreserved characters are fine
//         unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
//         reserved      = gen-delims / sub-delims
//         gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
//         sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
//                         / "*" / "+" / "," / ";" / "="
        // Replace everything else with percent encoding
        static const char doEncode[] = " \"<>[\\]^`{|}";
        static const char doEncodeHost[] = " \"<>\\^`{|}";
        for (int i = 0; i < tmp.size(); ++i) {
            quint8 c = quint8(tmp.at(i));
            if (c < 32 || c > 127 ||
                strchr(hostStart <= i && i <= hostEnd ? doEncodeHost : doEncode, c)) {
                char buf[4];
                buf[0] = '%';
                buf[1] = toHex(c >> 4);
                buf[2] = toHex(c & 0xf);
                buf[3] = '\0';
                tmp.replace(i, 1, buf);
                i += 2;
            }
        }
    }

    encodedOriginal = tmp;
}

/*!
    Sets the scheme of the URL to \a scheme. As a scheme can only
    contain ASCII characters, no conversion or encoding is done on the
    input.

    The scheme describes the type (or protocol) of the URL. It's
    represented by one or more ASCII characters at the start the URL,
    and is followed by a ':'. The following example shows a URL where
    the scheme is "ftp":

    \img qurl-authority2.png

    The scheme can also be empty, in which case the URL is interpreted
    as relative.

    \sa scheme(), isRelative()
*/
void QUrl::setScheme(const QString &scheme)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->scheme = scheme;
}

/*!
    Returns the scheme of the URL. If an empty string is returned,
    this means the scheme is undefined and the URL is then relative.

    \sa setScheme(), isRelative()
*/
QString QUrl::scheme() const
{
    if (!d) return QString();
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    return d->scheme;
}

/*!
    Sets the authority of the URL to \a authority.

    The authority of a URL is the combination of user info, a host
    name and a port. All of these elements are optional; an empty
    authority is therefore valid.

    The user info and host are separated by a '@', and the host and
    port are separated by a ':'. If the user info is empty, the '@'
    must be omitted; although a stray ':' is permitted if the port is
    empty.

    The following example shows a valid authority string:

    \img qurl-authority.png
*/
void QUrl::setAuthority(const QString &authority)
{
    if (!d) d = new QUrlPrivate;

    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized | QUrlPrivate::HostCanonicalized);
    d->setAuthority(authority);
}

/*!
    Returns the authority of the URL if it is defined; otherwise
    an empty string is returned.

    \sa setAuthority()
*/
QString QUrl::authority() const
{
    if (!d) return QString();

    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    return d->authority();
}

/*!
    Sets the user info of the URL to \a userInfo. The user info is an
    optional part of the authority of the URL, as described in
    setAuthority().

    The user info consists of a user name and optionally a password,
    separated by a ':'. If the password is empty, the colon must be
    omitted. The following example shows a valid user info string:

    \img qurl-authority3.png

    \sa userInfo(), setUserName(), setPassword(), setAuthority()
*/
void QUrl::setUserInfo(const QString &userInfo)
{
    if (!d) d = new QUrlPrivate;

    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->setUserInfo(userInfo.trimmed());
}

/*!
    Returns the user info of the URL, or an empty string if the user
    info is undefined.
*/
QString QUrl::userInfo() const
{
    if (!d) return QString();

    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    return d->userInfo();
}

/*!
    Sets the URL's user name to \a userName. The \a userName is part
    of the user info element in the authority of the URL, as described
    in setUserInfo().

    \sa setEncodedUserName(), userName(), setUserInfo()
*/
void QUrl::setUserName(const QString &userName)
{
    if (!d) d = new QUrlPrivate;

    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->userName = userName;
    d->encodedUserName.clear();
}

/*!
    Returns the user name of the URL if it is defined; otherwise
    an empty string is returned.

    \sa setUserName(), encodedUserName()
*/
QString QUrl::userName() const
{
    if (!d) return QString();

    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    d->userInfo();              // causes the unencoded form to be set
    return d->userName;
}

/*!
    \since 4.4

    Sets the URL's user name to the percent-encoded \a userName. The \a
    userName is part of the user info element in the authority of the
    URL, as described in setUserInfo().

    Note: this function does not verify that \a userName is properly
    encoded. It is the caller's responsibility to ensure that the any
    delimiters (such as colons or slashes) are properly encoded.

    \sa setUserName(), encodedUserName(), setUserInfo()
*/
void QUrl::setEncodedUserName(const QByteArray &userName)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->encodedUserName = userName;
    d->userName.clear();
}

/*!
    \since 4.4

    Returns the user name of the URL if it is defined; otherwise
    an empty string is returned. The returned value will have its
    non-ASCII and other control characters percent-encoded, as in
    toEncoded().

    \sa setEncodedUserName()
*/
QByteArray QUrl::encodedUserName() const
{
    if (!d) return QByteArray();
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    d->ensureEncodedParts();
    return d->encodedUserName;
}

/*!
    Sets the URL's password to \a password. The \a password is part of
    the user info element in the authority of the URL, as described in
    setUserInfo().

    \sa password(), setUserInfo()
*/
void QUrl::setPassword(const QString &password)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->password = password;
    d->encodedPassword.clear();
}

/*!
    Returns the password of the URL if it is defined; otherwise
    an empty string is returned.

    \sa setPassword()
*/
QString QUrl::password() const
{
    if (!d) return QString();
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    d->userInfo();              // causes the unencoded form to be set
    return d->password;
}

/*!
    \since 4.4

    Sets the URL's password to the percent-encoded \a password. The \a
    password is part of the user info element in the authority of the
    URL, as described in setUserInfo().

    Note: this function does not verify that \a password is properly
    encoded. It is the caller's responsibility to ensure that the any
    delimiters (such as colons or slashes) are properly encoded.

    \sa setPassword(), encodedPassword(), setUserInfo()
*/
void QUrl::setEncodedPassword(const QByteArray &password)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->encodedPassword = password;
    d->password.clear();
}

/*!
    \since 4.4

    Returns the password of the URL if it is defined; otherwise an
    empty string is returned. The returned value will have its
    non-ASCII and other control characters percent-encoded, as in
    toEncoded().

    \sa setEncodedPassword(), toEncoded()
*/
QByteArray QUrl::encodedPassword() const
{
    if (!d) return QByteArray();
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    d->ensureEncodedParts();
    return d->encodedPassword;
}

/*!
    Sets the host of the URL to \a host. The host is part of the
    authority.

    \sa host(), setAuthority()
*/
void QUrl::setHost(const QString &host)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);
    d->isHostValid = true;
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized | QUrlPrivate::HostCanonicalized);

    d->host = host;
}

/*!
    Returns the host of the URL if it is defined; otherwise
    an empty string is returned.
*/
QString QUrl::host() const
{
    if (!d) return QString();
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    QString result = d->canonicalHost();
    if (result.startsWith(QLatin1Char('[')))
        return result.mid(1, result.length() - 2);
    return result;
}

/*!
    \since 4.4

    Sets the URL's host to the ACE- or percent-encoded \a host. The \a
    host is part of the user info element in the authority of the
    URL, as described in setAuthority().

    \sa setHost(), encodedHost(), setAuthority(), fromAce()
*/
void QUrl::setEncodedHost(const QByteArray &host)
{
    setHost(fromPercentEncodingHelper(host));
}

/*!
    \since 4.4

    Returns the host part of the URL if it is defined; otherwise
    an empty string is returned.

    Note: encodedHost() does not return percent-encoded hostnames. Instead,
    the ACE-encoded (bare ASCII in Punycode encoding) form will be
    returned for any non-ASCII hostname.

    This function is equivalent to calling QUrl::toAce() on the return
    value of host().

    \sa setEncodedHost()
*/
QByteArray QUrl::encodedHost() const
{
    // should we cache this in d->encodedHost?
    return QUrl::toAce(host());
}

/*!
    Sets the port of the URL to \a port. The port is part of the
    authority of the URL, as described in setAuthority().

    \a port must be between 0 and 65535 inclusive. Setting the
    port to -1 indicates that the port is unspecified.
*/
void QUrl::setPort(int port)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    if (port < -1 || port > 65535) {
        qWarning("QUrl::setPort: Out of range");
        port = -1;
    }

    d->port = port;
}

/*!
    Returns the port of the URL, or -1 if the port is unspecified.
*/
int QUrl::port() const
{
    if (!d) return -1;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Validated)) d->validate();
    return d->port;
}

/*!
    \overload
    \since 4.1

    Returns the port of the URL, or \a defaultPort if the port is
    unspecified.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qurl.cpp 3
*/
int QUrl::port(int defaultPort) const
{
    if (!d) return defaultPort;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    return d->port == -1 ? defaultPort : d->port;
}

/*!
    Sets the path of the URL to \a path. The path is the part of the
    URL that comes after the authority but before the query string.

    \img qurl-ftppath.png

    For non-hierarchical schemes, the path will be everything
    following the scheme declaration, as in the following example:

    \img qurl-mailtopath.png

    \sa path()
*/
void QUrl::setPath(const QString &path)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->path = path;
    d->encodedPath.clear();
}

/*!
    Returns the path of the URL.

    \sa setPath()
*/
QString QUrl::path() const
{
    if (!d) return QString();
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    if (d->path.isNull()) {
        QUrlPrivate *that = const_cast<QUrlPrivate *>(d);
        that->path = fromPercentEncodingHelper(d->encodedPath);
    }
    return d->path;
}

/*!
    \since 4.4

    Sets the URL's path to the percent-encoded \a path.  The path is
    the part of the URL that comes after the authority but before the
    query string.

    \img qurl-ftppath.png

    For non-hierarchical schemes, the path will be everything
    following the scheme declaration, as in the following example:

    \img qurl-mailtopath.png

    Note: this function does not verify that \a path is properly
    encoded. It is the caller's responsibility to ensure that the any
    delimiters (such as '?' and '#') are properly encoded.

    \sa setPath(), encodedPath(), setUserInfo()
*/
void QUrl::setEncodedPath(const QByteArray &path)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->encodedPath = path;
    d->path.clear();
}

/*!
    \since 4.4

    Returns the path of the URL if it is defined; otherwise an
    empty string is returned. The returned value will have its
    non-ASCII and other control characters percent-encoded, as in
    toEncoded().

    \sa setEncodedPath(), toEncoded()
*/
QByteArray QUrl::encodedPath() const
{
    if (!d) return QByteArray();
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    d->ensureEncodedParts();
    return d->encodedPath;
}

/*!
    \since 4.2

    Returns true if this URL contains a Query (i.e., if ? was seen on it).

    \sa hasQueryItem(), encodedQuery()
*/
bool QUrl::hasQuery() const
{
    if (!d) return false;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    return d->hasQuery;
}

/*!
    Sets the characters used for delimiting between keys and values,
    and between key-value pairs in the URL's query string. The default
    value delimiter is '=' and the default pair delimiter is '&'.

    \img qurl-querystring.png

    \a valueDelimiter will be used for separating keys from values,
    and \a pairDelimiter will be used to separate key-value pairs.
    Any occurrences of these delimiting characters in the encoded
    representation of the keys and values of the query string are
    percent encoded.

    If \a valueDelimiter is set to '-' and \a pairDelimiter is '/',
    the above query string would instead be represented like this:

    \snippet doc/src/snippets/code/src_corelib_io_qurl.cpp 4

    Calling this function does not change the delimiters of the
    current query string. It only affects queryItems(),
    setQueryItems() and addQueryItems().
*/
void QUrl::setQueryDelimiters(char valueDelimiter, char pairDelimiter)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    detach(lock);

    d->valueDelimiter = valueDelimiter;
    d->pairDelimiter = pairDelimiter;
}

/*!
    Returns the character used to delimit between key-value pairs in
    the query string of the URL.
*/
char QUrl::queryPairDelimiter() const
{
    if (!d) return '&';
    return d->pairDelimiter;
}

/*!
    Returns the character used to delimit between keys and values in
    the query string of the URL.
*/
char QUrl::queryValueDelimiter() const
{
    if (!d) return '=';
    return d->valueDelimiter;
}

/*!
    Sets the query string of the URL to \a query. The string is
    inserted as-is, and no further encoding is performed when calling
    toEncoded().

    This function is useful if you need to pass a query string that
    does not fit into the key-value pattern, or that uses a different
    scheme for encoding special characters than what is suggested by
    QUrl.

    Passing a value of QByteArray() to \a query (a null QByteArray) unsets
    the query completely. However, passing a value of QByteArray("")
    will set the query to an empty value, as if the original URL
    had a lone "?".

    \sa encodedQuery(), hasQuery()
*/
void QUrl::setEncodedQuery(const QByteArray &query)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->query = query;
    d->hasQuery = !query.isNull();
}

/*!
    Sets the query string of the URL to an encoded version of \a
    query. The contents of \a query are converted to a string
    internally, each pair delimited by the character returned by
    pairDelimiter(), and the key and value are delimited by
    valueDelimiter().

    \note This method does not encode spaces (ASCII 0x20) as plus (+) signs,
    like HTML forms do. If you need that kind of encoding, you must encode
    the value yourself and use QUrl::setEncodedQueryItems.

    \sa setQueryDelimiters(), queryItems(), setEncodedQueryItems()
*/
void QUrl::setQueryItems(const QList<QPair<QString, QString> > &query)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);

    char alsoEncode[3];
    alsoEncode[0] = d->valueDelimiter;
    alsoEncode[1] = d->pairDelimiter;
    alsoEncode[2] = 0;

    QByteArray queryTmp;
    for (int i = 0; i < query.size(); i++) {
        if (i) queryTmp += d->pairDelimiter;
        // query = *( pchar / "/" / "?" )
        queryTmp += toPercentEncodingHelper(query.at(i).first, queryExcludeChars, alsoEncode);
        queryTmp += d->valueDelimiter;
        // query = *( pchar / "/" / "?" )
        queryTmp += toPercentEncodingHelper(query.at(i).second, queryExcludeChars, alsoEncode);
    }

    d->query = queryTmp;
    d->hasQuery = !query.isEmpty();
}

/*!
    \since 4.4

    Sets the query string of the URL to the encoded version of \a
    query. The contents of \a query are converted to a string
    internally, each pair delimited by the character returned by
    pairDelimiter(), and the key and value are delimited by
    valueDelimiter().

    Note: this function does not verify that the key-value pairs
    are properly encoded. It is the caller's responsibility to ensure
    that the query delimiters are properly encoded, if any.

    \sa setQueryDelimiters(), encodedQueryItems(), setQueryItems()
*/
void QUrl::setEncodedQueryItems(const QList<QPair<QByteArray, QByteArray> > &query)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);

    QByteArray queryTmp;
    for (int i = 0; i < query.size(); i++) {
        if (i) queryTmp += d->pairDelimiter;
        queryTmp += query.at(i).first;
        queryTmp += d->valueDelimiter;
        queryTmp += query.at(i).second;
    }

    d->query = queryTmp;
    d->hasQuery = !query.isEmpty();
}

/*!
    Inserts the pair \a key = \a value into the query string of the
    URL.

    The key/value pair is encoded before it is added to the query. The
    pair is converted into separate strings internally. The \a key and
    \a value is first encoded into UTF-8 and then delimited by the
    character returned by valueDelimiter(). Each key/value pair is
    delimited by the character returned by pairDelimiter().

    \note This method does not encode spaces (ASCII 0x20) as plus (+) signs,
    like HTML forms do. If you need that kind of encoding, you must encode
    the value yourself and use QUrl::addEncodedQueryItem.

    \sa addEncodedQueryItem()
*/
void QUrl::addQueryItem(const QString &key, const QString &value)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);

    char alsoEncode[3];
    alsoEncode[0] = d->valueDelimiter;
    alsoEncode[1] = d->pairDelimiter;
    alsoEncode[2] = 0;

    if (!d->query.isEmpty())
        d->query += d->pairDelimiter;

    // query = *( pchar / "/" / "?" )
    d->query += toPercentEncodingHelper(key, queryExcludeChars, alsoEncode);
    d->query += d->valueDelimiter;
    // query = *( pchar / "/" / "?" )
    d->query += toPercentEncodingHelper(value, queryExcludeChars, alsoEncode);

    d->hasQuery = !d->query.isEmpty();
}

/*!
    \since 4.4

    Inserts the pair \a key = \a value into the query string of the
    URL.

    Note: this function does not verify that either \a key or \a value
    are properly encoded. It is the caller's responsibility to ensure
    that the query delimiters are properly encoded, if any.

    \sa addQueryItem(), setQueryDelimiters()
*/
void QUrl::addEncodedQueryItem(const QByteArray &key, const QByteArray &value)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);

    if (!d->query.isEmpty())
        d->query += d->pairDelimiter;

    d->query += key;
    d->query += d->valueDelimiter;
    d->query += value;

    d->hasQuery = !d->query.isEmpty();
}

/*!
    Returns the query string of the URL, as a map of keys and values.

    \note This method does not decode spaces plus (+) signs as spaces (ASCII
    0x20), like HTML forms do. If you need that kind of decoding, you must
    use QUrl::encodedQueryItems and decode the data yourself.

    \sa setQueryItems(), setEncodedQuery()
*/
QList<QPair<QString, QString> > QUrl::queryItems() const
{
    if (!d) return QList<QPair<QString, QString> >();
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    QList<QPair<QString, QString> > itemMap;

    int pos = 0;
    const char *query = d->query.constData();
    while (pos < d->query.size()) {
        int valuedelim, end;
        d->queryItem(pos, &valuedelim, &end);
        QByteArray q(query + pos, valuedelim - pos);
        if (valuedelim < end) {
            QByteArray v(query + valuedelim + 1, end - valuedelim - 1);
            itemMap += qMakePair(fromPercentEncodingMutable(&q),
                                 fromPercentEncodingMutable(&v));
        } else {
            itemMap += qMakePair(fromPercentEncodingMutable(&q), QString());
        }
        pos = end + 1;
    }

    return itemMap;
}

/*!
    \since 4.4

    Returns the query string of the URL, as a map of encoded keys and values.

    \sa setEncodedQueryItems(), setQueryItems(), setEncodedQuery()
*/
QList<QPair<QByteArray, QByteArray> > QUrl::encodedQueryItems() const
{
    if (!d) return QList<QPair<QByteArray, QByteArray> >();
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    QList<QPair<QByteArray, QByteArray> > itemMap;

    int pos = 0;
    const char *query = d->query.constData();
    while (pos < d->query.size()) {
        int valuedelim, end;
        d->queryItem(pos, &valuedelim, &end);
        if (valuedelim < end)
            itemMap += qMakePair(QByteArray(query + pos, valuedelim - pos),
                                 QByteArray(query + valuedelim + 1, end - valuedelim - 1));
        else
            itemMap += qMakePair(QByteArray(query + pos, valuedelim - pos), QByteArray());
        pos = end + 1;
    }

    return itemMap;
}

/*!
    Returns true if there is a query string pair whose key is equal
    to \a key from the URL.

    \sa hasEncodedQueryItem()
*/
bool QUrl::hasQueryItem(const QString &key) const
{
    if (!d) return false;
    return hasEncodedQueryItem(toPercentEncoding(key, queryExcludeChars));
}

/*!
    \since 4.4

    Returns true if there is a query string pair whose key is equal
    to \a key from the URL.

    Note: if the encoded \a key does not match the encoded version of
    the query, this function will return false. That is, if the
    encoded query of this URL is "search=Qt%20Rules", calling this
    function with \a key = "%73earch" will return false.

    \sa hasQueryItem()
*/
bool QUrl::hasEncodedQueryItem(const QByteArray &key) const
{
    if (!d) return false;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    int pos = 0;
    const char *query = d->query.constData();
    while (pos < d->query.size()) {
        int valuedelim, end;
        d->queryItem(pos, &valuedelim, &end);
        if (key == QByteArray::fromRawData(query + pos, valuedelim - pos))
            return true;
        pos = end + 1;
    }
    return false;
}

/*!
    Returns the first query string value whose key is equal to \a key
    from the URL.

    \note This method does not decode spaces plus (+) signs as spaces (ASCII
    0x20), like HTML forms do. If you need that kind of decoding, you must
    use QUrl::encodedQueryItemValue and decode the data yourself.

    \sa allQueryItemValues()
*/
QString QUrl::queryItemValue(const QString &key) const
{
    if (!d) return QString();
    QByteArray tmp = encodedQueryItemValue(toPercentEncoding(key, queryExcludeChars));
    return fromPercentEncodingMutable(&tmp);
}

/*!
    \since 4.4

    Returns the first query string value whose key is equal to \a key
    from the URL.

    Note: if the encoded \a key does not match the encoded version of
    the query, this function will not work. That is, if the
    encoded query of this URL is "search=Qt%20Rules", calling this
    function with \a key = "%73earch" will return an empty string.

    \sa queryItemValue(), allQueryItemValues()
*/
QByteArray QUrl::encodedQueryItemValue(const QByteArray &key) const
{
    if (!d) return QByteArray();
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    int pos = 0;
    const char *query = d->query.constData();
    while (pos < d->query.size()) {
        int valuedelim, end;
        d->queryItem(pos, &valuedelim, &end);
        if (key == QByteArray::fromRawData(query + pos, valuedelim - pos))
            return valuedelim < end ?
                QByteArray(query + valuedelim + 1, end - valuedelim - 1) : QByteArray();
        pos = end + 1;
    }
    return QByteArray();
}

/*!
    Returns the a list of query string values whose key is equal to
    \a key from the URL.

    \note This method does not decode spaces plus (+) signs as spaces (ASCII
    0x20), like HTML forms do. If you need that kind of decoding, you must
    use QUrl::allEncodedQueryItemValues and decode the data yourself.

    \sa queryItemValue()
*/
QStringList QUrl::allQueryItemValues(const QString &key) const
{
    if (!d) return QStringList();
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    QByteArray encodedKey = toPercentEncoding(key, queryExcludeChars);
    QStringList values;

    int pos = 0;
    const char *query = d->query.constData();
    while (pos < d->query.size()) {
        int valuedelim, end;
        d->queryItem(pos, &valuedelim, &end);
        if (encodedKey == QByteArray::fromRawData(query + pos, valuedelim - pos)) {
            QByteArray v(query + valuedelim + 1, end - valuedelim - 1);
            values += valuedelim < end ?
                      fromPercentEncodingMutable(&v)
                      : QString();
        }
        pos = end + 1;
    }

    return values;
}

/*!
    \since 4.4

    Returns the a list of query string values whose key is equal to
    \a key from the URL.

    Note: if the encoded \a key does not match the encoded version of
    the query, this function will not work. That is, if the
    encoded query of this URL is "search=Qt%20Rules", calling this
    function with \a key = "%73earch" will return an empty list.

    \sa allQueryItemValues(), queryItemValue(), encodedQueryItemValue()
*/
QList<QByteArray> QUrl::allEncodedQueryItemValues(const QByteArray &key) const
{
    if (!d) return QList<QByteArray>();
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    QList<QByteArray> values;

    int pos = 0;
    const char *query = d->query.constData();
    while (pos < d->query.size()) {
        int valuedelim, end;
        d->queryItem(pos, &valuedelim, &end);
        if (key == QByteArray::fromRawData(query + pos, valuedelim - pos))
            values += valuedelim < end ?
                      QByteArray(query + valuedelim + 1, end - valuedelim - 1)
                      : QByteArray();
        pos = end + 1;
    }

    return values;
}

/*!
    Removes the first query string pair whose key is equal to \a key
    from the URL.

    \sa removeAllQueryItems()
*/
void QUrl::removeQueryItem(const QString &key)
{
    if (!d) return;
    removeEncodedQueryItem(toPercentEncoding(key, queryExcludeChars));
}

/*!
    \since 4.4

    Removes the first query string pair whose key is equal to \a key
    from the URL.

    Note: if the encoded \a key does not match the encoded version of
    the query, this function will not work. That is, if the
    encoded query of this URL is "search=Qt%20Rules", calling this
    function with \a key = "%73earch" will do nothing.

    \sa removeQueryItem(), removeAllQueryItems()
*/
void QUrl::removeEncodedQueryItem(const QByteArray &key)
{
    if (!d) return;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);

    int pos = 0;
    const char *query = d->query.constData();
    while (pos < d->query.size()) {
        int valuedelim, end;
        d->queryItem(pos, &valuedelim, &end);
        if (key == QByteArray::fromRawData(query + pos, valuedelim - pos)) {
            if (end < d->query.size())
                ++end; // remove additional '%'
            d->query.remove(pos, end - pos);
            return;
        }
        pos = end + 1;
    }
}

/*!
    Removes all the query string pairs whose key is equal to \a key
    from the URL.

   \sa removeQueryItem()
*/
void QUrl::removeAllQueryItems(const QString &key)
{
    if (!d) return;
    removeAllEncodedQueryItems(toPercentEncoding(key, queryExcludeChars));
}

/*!
    \since 4.4

    Removes all the query string pairs whose key is equal to \a key
    from the URL.

    Note: if the encoded \a key does not match the encoded version of
    the query, this function will not work. That is, if the
    encoded query of this URL is "search=Qt%20Rules", calling this
    function with \a key = "%73earch" will do nothing.

   \sa removeQueryItem()
*/
void QUrl::removeAllEncodedQueryItems(const QByteArray &key)
{
    if (!d) return;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);

    int pos = 0;
    const char *query = d->query.constData();
    while (pos < d->query.size()) {
        int valuedelim, end;
        d->queryItem(pos, &valuedelim, &end);
        if (key == QByteArray::fromRawData(query + pos, valuedelim - pos)) {
            if (end < d->query.size())
                ++end; // remove additional '%'
            d->query.remove(pos, end - pos);
            query = d->query.constData(); //required if remove detach;
        } else {
            pos = end + 1;
        }
    }
}

/*!
    Returns the query string of the URL in percent encoded form.
*/
QByteArray QUrl::encodedQuery() const
{
    if (!d) return QByteArray();
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    return d->query;
}

/*!
    Sets the fragment of the URL to \a fragment. The fragment is the
    last part of the URL, represented by a '#' followed by a string of
    characters. It is typically used in HTTP for referring to a
    certain link or point on a page:

    \img qurl-fragment.png

    The fragment is sometimes also referred to as the URL "reference".

    Passing an argument of QString() (a null QString) will unset the fragment.
    Passing an argument of QString("") (an empty but not null QString)
    will set the fragment to an empty string (as if the original URL
    had a lone "#").

    \sa fragment(), hasFragment()
*/
void QUrl::setFragment(const QString &fragment)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->fragment = fragment;
    d->hasFragment = !fragment.isNull();
    d->encodedFragment.clear();
}

/*!
    Returns the fragment of the URL.

    \sa setFragment()
*/
QString QUrl::fragment() const
{
    if (!d) return QString();
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    return d->fragmentImpl();
}

QString QUrlPrivate::fragmentImpl() const
{
    if (fragment.isNull() && !encodedFragment.isNull()) {
        QUrlPrivate *that = const_cast<QUrlPrivate *>(this);
        that->fragment = fromPercentEncodingHelper(encodedFragment);
    }
    return fragment;
}

/*!
    \since 4.4

    Sets the URL's fragment to the percent-encoded \a fragment. The fragment is the
    last part of the URL, represented by a '#' followed by a string of
    characters. It is typically used in HTTP for referring to a
    certain link or point on a page:

    \img qurl-fragment.png

    The fragment is sometimes also referred to as the URL "reference".

    Passing an argument of QByteArray() (a null QByteArray) will unset
    the fragment.  Passing an argument of QByteArray("") (an empty but
    not null QByteArray) will set the fragment to an empty string (as
    if the original URL had a lone "#").

    \sa setFragment(), encodedFragment()
*/
void QUrl::setEncodedFragment(const QByteArray &fragment)
{
    if (!d) d = new QUrlPrivate;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach(lock);
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->encodedFragment = fragment;
    d->hasFragment = !fragment.isNull();
    d->fragment.clear();
}

/*!
    \since 4.4

    Returns the fragment of the URL if it is defined; otherwise an
    empty string is returned. The returned value will have its
    non-ASCII and other control characters percent-encoded, as in
    toEncoded().

    \sa setEncodedFragment(), toEncoded()
*/
QByteArray QUrl::encodedFragment() const
{
    if (!d) return QByteArray();
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    d->ensureEncodedParts();
    return d->encodedFragment;
}

/*!
    \since 4.2

    Returns true if this URL contains a fragment (i.e., if # was seen on it).

    \sa fragment(), setFragment()
*/
bool QUrl::hasFragment() const
{
    if (!d) return false;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    return d->hasFragment;
}

/*!
    \since 4.8

    Returns the TLD (Top-Level Domain) of the URL, (e.g. .co.uk, .net).
    Note that the return value is prefixed with a '.' unless the
    URL does not contain a valid TLD, in which case the function returns
    an empty string.
*/
#ifndef QT_BOOTSTRAPPED
QString QUrl::topLevelDomain() const
{
    return qTopLevelDomain(host());
}
#endif

/*!
    Returns the result of the merge of this URL with \a relative. This
    URL is used as a base to convert \a relative to an absolute URL.

    If \a relative is not a relative URL, this function will return \a
    relative directly. Otherwise, the paths of the two URLs are
    merged, and the new URL returned has the scheme and authority of
    the base URL, but with the merged path, as in the following
    example:

    \snippet doc/src/snippets/code/src_corelib_io_qurl.cpp 5

    Calling resolved() with ".." returns a QUrl whose directory is
    one level higher than the original. Similarly, calling resolved()
    with "../.." removes two levels from the path. If \a relative is
    "/", the path becomes "/".

    \sa isRelative()
*/
QUrl QUrl::resolved(const QUrl &relative) const
{
    if (!d) return relative;
    if (!relative.d) return *this;

    QOrderedMutexLocker locker(&d->mutex, &relative.d->mutex);

    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    if (!QURL_HASFLAG(relative.d->stateFlags, QUrlPrivate::Parsed))
        relative.d->parse();

    d->ensureEncodedParts();
    relative.d->ensureEncodedParts();

    QUrl t;
    // be non strict and allow scheme in relative url
    if (!relative.d->scheme.isEmpty() && relative.d->scheme != d->scheme) {
        t = relative;
        // t.detach(locker) would unlock, so bypass it
        qAtomicDetach(t.d);
    } else {
        if (!relative.d->authority().isEmpty()) {
            t = relative;
            qAtomicDetach(t.d);
        } else {
            t.d = new QUrlPrivate;
            if (relative.d->encodedPath.isEmpty()) {
                t.d->encodedPath = d->encodedPath;
                t.d->query = relative.d->hasQuery ? relative.d->query : d->query;
            } else {
                t.d->encodedPath = relative.d->encodedPath.at(0) == '/'
                                       ? relative.d->encodedPath
                                       : d->mergePaths(relative.d->encodedPath);
                t.d->query = relative.d->query;
            }
            t.d->hasQuery = !t.d->query.isNull();
            t.d->encodedUserName = d->encodedUserName;
            t.d->encodedPassword = d->encodedPassword;
            t.d->host = d->host;
            t.d->port = d->port;
        }
        t.setScheme(d->scheme);
    }
    t.setFragment(relative.d->fragmentImpl());
    removeDotsFromPath(&t.d->encodedPath);
    t.d->path.clear();

#if defined(QURL_DEBUG)
    qDebug("QUrl(\"%s\").resolved(\"%s\") = \"%s\"",
           d->toEncoded().constData(),
           relative.d->toEncoded().constData(),
           t.toEncoded().constData());
#endif
    return t;
}

/*!
    Returns true if the URL is relative; otherwise returns false. A
    URL is relative if its scheme is undefined; this function is
    therefore equivalent to calling scheme().isEmpty().
*/
bool QUrl::isRelative() const
{
    if (!d) return true;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    return d->scheme.isEmpty();
}

/*!
    Returns the human-displayable string representation of the
    URL. The output can be customized by passing flags with \a
    options.

    \sa FormattingOptions, toEncoded()
*/
QString QUrl::toString(FormattingOptions options) const
{
    if (!d) return QString();
    QString ourPath = path();
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    QString url;

    if (!(options & QUrl::RemoveScheme) && !d->scheme.isEmpty())
        url += d->scheme + QLatin1Char(':');
    if ((options & QUrl::RemoveAuthority) != QUrl::RemoveAuthority) {
        bool doFileScheme = d->scheme == QLatin1String("file") && ourPath.startsWith(QLatin1Char('/'));
        QString tmp = d->authority(options);
        if (!tmp.isNull() || doFileScheme) {
            if (doFileScheme && !ourPath.startsWith(QLatin1Char('/')))
                url += QLatin1Char('/');
            url += QLatin1String("//");
            url += tmp;
        }
    }
    if (!(options & QUrl::RemovePath)) {
        // check if we need to insert a slash
        if ((options & QUrl::RemoveAuthority) != QUrl::RemoveAuthority
            && !d->authority(options).isEmpty() && !ourPath.isEmpty() && ourPath.at(0) != QLatin1Char('/'))
            url += QLatin1Char('/');
        url += ourPath;
        // check if we need to remove trailing slashes
        while ((options & StripTrailingSlash) && url.endsWith(QLatin1Char('/')))
            url.chop(1);
    }

    if (!(options & QUrl::RemoveQuery) && d->hasQuery) {
        url += QLatin1Char('?');
        url += fromPercentEncoding(d->query);
    }
    if (!(options & QUrl::RemoveFragment) && d->hasFragment) {
        url += QLatin1Char('#');
        url += d->fragmentImpl();
    }

    return url;
}

/*!
    Returns the encoded representation of the URL if it's valid;
    otherwise an empty QByteArray is returned. The output can be
    customized by passing flags with \a options.

    The user info, path and fragment are all converted to UTF-8, and
    all non-ASCII characters are then percent encoded. The host name
    is encoded using Punycode.
*/
QByteArray QUrl::toEncoded(FormattingOptions options) const
{
    if (!d) return QByteArray();
    QMutexLocker lock(&d->mutex);
    return d->toEncoded(options);
}

/*!
    Parses \a input and returns the corresponding QUrl. \a input is
    assumed to be in encoded form, containing only ASCII characters.

    The URL is parsed using TolerantMode.

    \sa toEncoded(), setUrl()
*/
QUrl QUrl::fromEncoded(const QByteArray &input)
{
    QUrl tmp;
    tmp.setEncodedUrl(input, TolerantMode);
    return tmp;
}

/*!
    \overload

    Parses the URL using \a parsingMode.

    \sa toEncoded(), setUrl()
*/
QUrl QUrl::fromEncoded(const QByteArray &input, ParsingMode parsingMode)
{
    QUrl tmp;
    tmp.setEncodedUrl(input, parsingMode);
    return tmp;
}

/*!
    Returns a decoded copy of \a input. \a input is first decoded from
    percent encoding, then converted from UTF-8 to unicode.
*/
QString QUrl::fromPercentEncoding(const QByteArray &input)
{
    return fromPercentEncodingHelper(input);
}

/*!
    Returns an encoded copy of \a input. \a input is first converted
    to UTF-8, and all ASCII-characters that are not in the unreserved group
    are percent encoded. To prevent characters from being percent encoded
    pass them to \a exclude. To force characters to be percent encoded pass
    them to \a include.

    Unreserved is defined as:
       ALPHA / DIGIT / "-" / "." / "_" / "~"

    \snippet doc/src/snippets/code/src_corelib_io_qurl.cpp 6
*/
QByteArray QUrl::toPercentEncoding(const QString &input, const QByteArray &exclude, const QByteArray &include)
{
    return toPercentEncodingHelper(input, exclude.constData(), include.constData());
}

/*!
    \obsolete
    Returns a \a uc in Punycode encoding.

    Punycode is a Unicode encoding used for internationalized domain
    names, as defined in RFC3492. If you want to convert a domain name from
    Unicode to its ASCII-compatible representation, use toAce().
*/
QByteArray QUrl::toPunycode(const QString &uc)
{
    QString output;
    toPunycodeHelper(uc.constData(), uc.size(), &output);
    return output.toLatin1();
}

/*!
    \obsolete
    Returns the Punycode decoded representation of \a pc.

    Punycode is a Unicode encoding used for internationalized domain
    names, as defined in RFC3492. If you want to convert a domain from
    its ASCII-compatible encoding to the Unicode representation, use
    fromAce().
*/
QString QUrl::fromPunycode(const QByteArray &pc)
{
    uint n = initial_n;
    uint i = 0;
    uint bias = initial_bias;

    // strip any ACE prefix
    int start = pc.startsWith("xn--") ? 4 : 0;
    if (!start)
        return QString::fromLatin1(pc);

    // find the last delimiter character '-' in the input array. copy
    // all data before this delimiter directly to the output array.
    int delimiterPos = pc.lastIndexOf(0x2d);
    QString output = delimiterPos < 4 ?
                     QString() : QString::fromLatin1(pc.constData() + start, delimiterPos - start);

    // if a delimiter was found, skip to the position after it;
    // otherwise start at the front of the input string. everything
    // before the delimiter is assumed to be basic code points.
    uint cnt = delimiterPos + 1;

    // loop through the rest of the input string, inserting non-basic
    // characters into output as we go.
    while (cnt < (uint) pc.size()) {
        uint oldi = i;
        uint w = 1;

        // find the next index for inserting a non-basic character.
        for (uint k = base; cnt < (uint) pc.size(); k += base) {
            // grab a character from the punycode input and find its
            // delta digit (each digit code is part of the
            // variable-length integer delta)
            uint digit = pc.at(cnt++);
            if (digit - 48 < 10) digit -= 22;
            else if (digit - 65 < 26) digit -= 65;
            else if (digit - 97 < 26) digit -= 97;
            else digit = base;

            // reject out of range digits
            if (digit >= base || digit > (Q_MAXINT - i) / w)
                return QLatin1String("");

            i += (digit * w);

            // detect threshold to stop reading delta digits
            uint t;
            if (k <= bias) t = tmin;
            else if (k >= bias + tmax) t = tmax;
            else t = k - bias;
            if (digit < t) break;

            w *= (base - t);
        }

        // find new bias and calculate the next non-basic code
        // character.
        bias = adapt(i - oldi, output.length() + 1, oldi == 0);
        n += i / (output.length() + 1);

        // allow the deltas to wrap around
        i %= (output.length() + 1);

        // insert the character n at position i
        output.insert((uint) i, QChar((ushort) n));
        ++i;
    }

    return output;
}

/*!
    \since 4.2

    Returns the Unicode form of the given domain name
    \a domain, which is encoded in the ASCII Compatible Encoding (ACE).
    The result of this function is considered equivalent to \a domain.

    If the value in \a domain cannot be encoded, it will be converted
    to QString and returned.

    The ASCII Compatible Encoding (ACE) is defined by RFC 3490, RFC 3491
    and RFC 3492. It is part of the Internationalizing Domain Names in
    Applications (IDNA) specification, which allows for domain names
    (like \c "example.com") to be written using international
    characters.
*/
QString QUrl::fromAce(const QByteArray &domain)
{
    return qt_ACE_do(QString::fromLatin1(domain), NormalizeAce);
}

/*!
    \since 4.2

    Returns the ASCII Compatible Encoding of the given domain name \a domain.
    The result of this function is considered equivalent to \a domain.

    The ASCII-Compatible Encoding (ACE) is defined by RFC 3490, RFC 3491
    and RFC 3492. It is part of the Internationalizing Domain Names in
    Applications (IDNA) specification, which allows for domain names
    (like \c "example.com") to be written using international
    characters.

    This function return an empty QByteArra if \a domain is not a valid
    hostname. Note, in particular, that IPv6 literals are not valid domain
    names.
*/
QByteArray QUrl::toAce(const QString &domain)
{
    QString result = qt_ACE_do(domain, ToAceOnly);
    return result.toLatin1();
}

/*!
    \since 4.2

    Returns the current whitelist of top-level domains that are allowed
    to have non-ASCII characters in their compositions.

    See setIdnWhitelist() for the rationale of this list.
*/
QStringList QUrl::idnWhitelist()
{
    if (user_idn_whitelist)
        return *user_idn_whitelist;
    QStringList list;
    unsigned int i = 0;
    while (i < sizeof(idn_whitelist)/sizeof(const char *)) {
        list << QLatin1String(idn_whitelist[i]);
        ++i;
    }
    return list;
}

/*!
    \since 4.2

    Sets the whitelist of Top-Level Domains (TLDs) that are allowed to have
    non-ASCII characters in domains to the value of \a list.

    Note that if you call this function, you need to do so \em before
    you start any threads that might access idnWhitelist().

    Qt has comes a default list that contains the Internet top-level domains
    that have published support for Internationalized Domain Names (IDNs)
    and rules to guarantee that no deception can happen between similarly-looking
    characters (such as the Latin lowercase letter \c 'a' and the Cyrillic
    equivalent, which in most fonts are visually identical).

    This list is periodically maintained, as registrars publish new rules.

    This function is provided for those who need to manipulate the list, in
    order to add or remove a TLD. It is not recommended to change its value
    for purposes other than testing, as it may expose users to security risks.
*/
void QUrl::setIdnWhitelist(const QStringList &list)
{
    if (!user_idn_whitelist)
        user_idn_whitelist = new QStringList;
    *user_idn_whitelist = list;
}

/*!
    \internal

    Returns true if this URL is "less than" the given \a url. This
    provides a means of ordering URLs.
*/
bool QUrl::operator <(const QUrl &url) const
{
    QOrderedMutexLocker(d ? &d->mutex : 0, url.d ? &url.d->mutex : 0);
    if (!d) return url.d ? QByteArray() < url.d->normalized() : false;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    if (!url.d) return d->normalized() < QByteArray();
    if (!QURL_HASFLAG(url.d->stateFlags, QUrlPrivate::Parsed)) url.d->parse();
    return d->normalized() < url.d->normalized();
}

/*!
    Returns true if this URL and the given \a url are equal;
    otherwise returns false.
*/
bool QUrl::operator ==(const QUrl &url) const
{
    if (!d) return url.isEmpty();
    if (!url.d) return isEmpty();
    QOrderedMutexLocker(&d->mutex, &url.d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    if (!QURL_HASFLAG(url.d->stateFlags, QUrlPrivate::Parsed)) url.d->parse();
    return d->normalized() == url.d->normalized();
}

/*!
    Returns true if this URL and the given \a url are not equal;
    otherwise returns false.
*/
bool QUrl::operator !=(const QUrl &url) const
{
    return !(*this == url);
}

/*!
    Assigns the specified \a url to this object.
*/
QUrl &QUrl::operator =(const QUrl &url)
{
    if (!d) {
        if (url.d) {
            url.d->ref.ref();
            d = url.d;
        }
    } else {
        if (url.d)
            qAtomicAssign(d, url.d);
        else
            clear();
    }
    return *this;
}

/*!
    Assigns the specified \a url to this object.
*/
QUrl &QUrl::operator =(const QString &url)
{
    if (url.isEmpty()) {
        clear();
    } else {
        QUrl tmp(url);
        if (!d) d = new QUrlPrivate;
        qAtomicAssign(d, tmp.d);
    }
    return *this;
}

/*!
    \fn void QUrl::swap(QUrl &other)
    \since 4.8

    Swaps URL \a other with this URL. This operation is very
    fast and never fails.
*/

/*! \internal

    Forces a detach.
*/
void QUrl::detach()
{
    if (!d) {
        d = new QUrlPrivate;
    } else {
        // Public method, so it must lock first.
        QMutexLocker lock(&d->mutex);
        detach(lock);
    }
}


/*! \internal

    Forces a detach. Unlocks \a locker once the detaching is done.

    It's ok to access private members afterwards, without lock, since
    after detaching we have our private copy of the d pointer, that
    no other QUrl instance can know about yet.
*/
void QUrl::detach(QMutexLocker &locker)
{
    Q_ASSERT(d); // we have a locker, so we have a d pointer
    // Ensure that we hold the mutex until after making our private copy,
    // so that another thread cannot deref + delete d meanwhile.
    // So this is a modified version of qAtomicDetach(d)
    if (d->ref == 1) {
        locker.unlock();
        return;
    }
    QUrlPrivate * x = d;
    d = new QUrlPrivate(*x);
    locker.unlock();
    if (!x->ref.deref())
        delete x;
}

/*!
    \internal
*/
bool QUrl::isDetached() const
{
    return !d || d->ref == 1;
}


/*!
    Returns a QUrl representation of \a localFile, interpreted as a local
    file. This function accepts paths separated by slashes as well as the
    native separator for this platform.

    This function also accepts paths with a doubled leading slash (or
    backslash) to indicate a remote file, as in
    "//servername/path/to/file.txt". Note that only certain platforms can
    actually open this file using QFile::open().

    \sa toLocalFile(), isLocalFile(), QDir::toNativeSeparators()
*/
QUrl QUrl::fromLocalFile(const QString &localFile)
{
    QUrl url;
    url.setScheme(QLatin1String("file"));
    QString deslashified = QDir::fromNativeSeparators(localFile);

    // magic for drives on windows
    if (deslashified.length() > 1 && deslashified.at(1) == QLatin1Char(':') && deslashified.at(0) != QLatin1Char('/')) {
        url.setPath(QLatin1Char('/') + deslashified);
    // magic for shared drive on windows
    } else if (deslashified.startsWith(QLatin1String("//"))) {
        int indexOfPath = deslashified.indexOf(QLatin1Char('/'), 2);
        url.setHost(deslashified.mid(2, indexOfPath - 2));
        if (indexOfPath > 2)
            url.setPath(deslashified.right(deslashified.length() - indexOfPath));
    } else {
        url.setPath(deslashified);
    }

    return url;
}

/*!
    Returns the path of this URL formatted as a local file path. The path
    returned will use forward slashes, even if it was originally created
    from one with backslashes.

    If this URL contains a non-empty hostname, it will be encoded in the
    returned value in the form found on SMB networks (for example,
    "//servername/path/to/file.txt").

    If this is a relative URL, in Qt 4.x this function returns the path to
    maintain backward compatability. This will change from 5.0 onwards. Then
    the path is returned only for URLs where the scheme is "file", and for
    all other URLs an empty string is returned.

    \sa fromLocalFile(), isLocalFile()
*/
QString QUrl::toLocalFile() const
{
    if (!d) return QString();

    // the call to isLocalFile() also ensures that we're parsed
    if (!isLocalFile() && !scheme().isEmpty())
        return QString();

    QString tmp;
    QString ourPath = path();

    QMutexLocker lock(&d->mutex); // for d->host

    // magic for shared drive on windows
    if (!d->host.isEmpty()) {
        tmp = QLatin1String("//") + d->host + (ourPath.length() > 0 && ourPath.at(0) != QLatin1Char('/')
                                               ? QLatin1Char('/') + ourPath :  ourPath);
    } else {
        tmp = ourPath;
        // magic for drives on windows
        if (ourPath.length() > 2 && ourPath.at(0) == QLatin1Char('/') && ourPath.at(2) == QLatin1Char(':'))
            tmp.remove(0, 1);
    }

    return tmp;
}

/*!
    \since 4.8
    Returns true if this URL is pointing to a local file path. A URL is a
    local file path if the scheme is "file".

    Note that this function considers URLs with hostnames to be local file
    paths, even if the eventual file path cannot be opened with
    QFile::open().

    \sa fromLocalFile(), toLocalFile()
*/
bool QUrl::isLocalFile() const
{
    if (!d) return false;
    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    if (d->scheme.compare(QLatin1String("file"), Qt::CaseInsensitive) != 0)
        return false;   // not file
    return true;
}

/*!
    Returns true if this URL is a parent of \a childUrl. \a childUrl is a child
    of this URL if the two URLs share the same scheme and authority,
    and this URL's path is a parent of the path of \a childUrl.
*/
bool QUrl::isParentOf(const QUrl &childUrl) const
{
    QString childPath = childUrl.path();

    if (!d)
        return ((childUrl.scheme().isEmpty())
            && (childUrl.authority().isEmpty())
            && childPath.length() > 0 && childPath.at(0) == QLatin1Char('/'));

    QMutexLocker lock(&d->mutex);
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    lock.unlock();

    QString ourPath = path();

    return ((childUrl.scheme().isEmpty() || scheme() == childUrl.scheme())
            && (childUrl.authority().isEmpty() || authority() == childUrl.authority())
            &&  childPath.startsWith(ourPath)
            && ((ourPath.endsWith(QLatin1Char('/')) && childPath.length() > ourPath.length())
                || (!ourPath.endsWith(QLatin1Char('/'))
                    && childPath.length() > ourPath.length() && childPath.at(ourPath.length()) == QLatin1Char('/'))));
}

/*!
    \fn void QUrl::setProtocol(const QString &s)

    Use setScheme() instead.
*/

/*!
    \fn void QUrl::setUser(const QString &s)

    Use setUserName() instead.
*/

/*!
    \fn bool QUrl::hasUser() const

    Use !userName().isEmpty() instead.
*/

/*!
    \fn bool QUrl::hasPassword() const

    Use !password().isEmpty() instead.
*/

/*!
    \fn bool QUrl::hasHost() const

    Use !host().isEmpty() instead.
*/

/*!
    \fn bool QUrl::hasPort() const

    Use port() != -1 instead.
*/

/*!
    \fn bool QUrl::hasPath() const

    Use !path().isEmpty() instead.
*/

/*!
    \fn void QUrl::setQuery(const QString &txt)

    Use setEncodedQuery() instead.
*/

/*!
    \fn void QUrl::setRef(const QString &txt)

    Use setFragment() instead.
*/

/*!
    \fn bool QUrl::hasRef() const

    Use !fragment().isEmpty() instead.
*/

/*!
    \fn void QUrl::addPath(const QString &p)

    Use setPath() instead.
*/

/*!
    \fn void QUrl::setFileName(const QString &txt)

    Use setPath() instead.
*/

/*!
    \fn void QUrl::decode(QString &url)

    Use fromPercentEncoding() instead.
*/

/*!
    \fn void QUrl::encode(QString &url)

    Use toPercentEncoding() instead.
*/

/*!
    \fn bool QUrl::cdUp()

    Use resolved("..") instead.

    \oldcode
        QUrl url("http://example.com/Developer/");
        url.cdUp();
    \newcode
        QUrl url("http://example.com/Developer/");
        url = url.resolved("..");
    \endcode
*/

/*!
    \fn bool QUrl::isRelativeUrl(const QString &url)

    Use isRelative() instead.
*/

/*!
    \fn void QUrl::reset()

    Use clear() instead.
*/

/*!
    \fn  QUrl::operator QString() const

    Use toString() instead.
*/

/*!
    \fn QString QUrl::protocol() const

    Use scheme() instead.
*/

/*!
    \fn QString QUrl::user() const

    Use userName() instead.
*/

/*!
    \fn QString QUrl::query() const

    Use encodedQuery() instead.
*/

/*!
    \fn QString QUrl::ref() const

    Use fragment() instead.
*/

/*!
    \fn QString QUrl::fileName() const

    Use QFileInfo(path()).fileName() instead.
*/

/*!
    \fn QString QUrl::dirPath() const

    Use QFileInfo(path()).absolutePath() or QFileInfo(path()) instead.
*/

#ifdef QT3_SUPPORT
void QUrl::setFileName(const QString &txt)
{
    QFileInfo fileInfo(path());
    fileInfo.setFile(txt);
    setPath(fileInfo.filePath());
}

QString QUrl::fileName() const
{
    QFileInfo fileInfo(path());
    return fileInfo.fileName();
}

QString QUrl::dirPath() const
{
    QFileInfo fileInfo(path());
    if (fileInfo.isAbsolute()) {
        QString absPath = fileInfo.absolutePath();
#ifdef Q_OS_WIN
        if (absPath.size() > 1 && absPath.at(1) == QLatin1Char(':'))
            absPath = absPath.mid(2);
#endif
        return absPath;
    }
    return fileInfo.path();
}
#endif


#ifndef QT_NO_DATASTREAM
/*! \relates QUrl

    Writes url \a url to the stream \a out and returns a reference
    to the stream.

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/
QDataStream &operator<<(QDataStream &out, const QUrl &url)
{
    QByteArray u = url.toEncoded();
    out << u;
    return out;
}

/*! \relates QUrl

    Reads a url into \a url from the stream \a in and returns a
    reference to the stream.

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/
QDataStream &operator>>(QDataStream &in, QUrl &url)
{
    QByteArray u;
    in >> u;
    url = QUrl::fromEncoded(u);
    return in;
}
#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const QUrl &url)
{
    d.maybeSpace() << "QUrl(" << url.toString() << ')';
    return d.space();
}
#endif

/*!
    \since 4.2

    Returns a text string that explains why an URL is invalid in the case being;
    otherwise returns an empty string.
*/
QString QUrl::errorString() const
{
    if (!d)
        return QLatin1String(QT_TRANSLATE_NOOP(QUrl, "Invalid URL \"\": ")); // XXX not a good message, but the one an empty URL produces
    QMutexLocker lock(&d->mutex);
    return d->createErrorString();
}

/*!
    \typedef QUrl::DataPtr
    \internal
*/

/*!
    \fn DataPtr &QUrl::data_ptr()
    \internal
*/

// The following code has the following copyright:
/*
   Copyright (C) Research In Motion Limited 2009. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Research In Motion Limited nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY Research In Motion Limited ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Research In Motion Limited BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


/*!
    Returns a valid URL from a user supplied \a userInput string if one can be
    deducted. In the case that is not possible, an invalid QUrl() is returned.

    \since 4.6

    Most applications that can browse the web, allow the user to input a URL
    in the form of a plain string. This string can be manually typed into
    a location bar, obtained from the clipboard, or passed in via command
    line arguments.

    When the string is not already a valid URL, a best guess is performed,
    making various web related assumptions.

    In the case the string corresponds to a valid file path on the system,
    a file:// URL is constructed, using QUrl::fromLocalFile().

    If that is not the case, an attempt is made to turn the string into a
    http:// or ftp:// URL. The latter in the case the string starts with
    'ftp'. The result is then passed through QUrl's tolerant parser, and
    in the case or success, a valid QUrl is returned, or else a QUrl().

    \section1 Examples:

    \list
    \o qt.nokia.com becomes http://qt.nokia.com
    \o ftp.qt.nokia.com becomes ftp://ftp.qt.nokia.com
    \o hostname becomes http://hostname
    \o /home/user/test.html becomes file:///home/user/test.html
    \endlist
*/
QUrl QUrl::fromUserInput(const QString &userInput)
{
    QString trimmedString = userInput.trimmed();

    // Check first for files, since on Windows drive letters can be interpretted as schemes
    if (QDir::isAbsolutePath(trimmedString))
        return QUrl::fromLocalFile(trimmedString);

    QUrl url = QUrl::fromEncoded(trimmedString.toUtf8(), QUrl::TolerantMode);
    QUrl urlPrepended = QUrl::fromEncoded("http://" + trimmedString.toUtf8(), QUrl::TolerantMode);

    // Check the most common case of a valid url with scheme and host
    // We check if the port would be valid by adding the scheme to handle the case host:port
    // where the host would be interpretted as the scheme
    if (url.isValid()
        && !url.scheme().isEmpty()
        && (!url.host().isEmpty() || !url.path().isEmpty())
        && urlPrepended.port() == -1)
        return url;

    // Else, try the prepended one and adjust the scheme from the host name
    if (urlPrepended.isValid() && (!urlPrepended.host().isEmpty() || !urlPrepended.path().isEmpty()))
    {
        int dotIndex = trimmedString.indexOf(QLatin1Char('.'));
        const QString hostscheme = trimmedString.left(dotIndex).toLower();
        if (hostscheme == QLatin1String("ftp"))
            urlPrepended.setScheme(QLatin1String("ftp"));
        return urlPrepended;
    }

    return QUrl();
}
// end of BSD code

QT_END_NAMESPACE
