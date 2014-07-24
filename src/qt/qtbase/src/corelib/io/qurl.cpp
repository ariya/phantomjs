/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2012 Intel Corporation.
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
    \inmodule QtCore

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
    constructor by passing a QString. Otherwise, setUrl() can also
    be used.

    URLs can be represented in two forms: encoded or unencoded. The
    unencoded representation is suitable for showing to users, but
    the encoded representation is typically what you would send to
    a web server. For example, the unencoded URL
    "http://bühler.example.com/List of applicants.xml"
    would be sent to the server as
    "http://xn--bhler-kva.example.com/List%20of%20applicants.xml".

    A URL can also be constructed piece by piece by calling
    setScheme(), setUserName(), setPassword(), setHost(), setPort(),
    setPath(), setQuery() and setFragment(). Some convenience
    functions are also available: setAuthority() sets the user name,
    password, host and port. setUserInfo() sets the user name and
    password at once.

    Call isValid() to check if the URL is valid. This can be done at any point
    during the constructing of a URL. If isValid() returns \c false, you should
    clear() the URL before proceeding, or start over by parsing a new URL with
    setUrl().

    Constructing a query is particularly convenient through the use of the \l
    QUrlQuery class and its methods QUrlQuery::setQueryItems(),
    QUrlQuery::addQueryItem() and QUrlQuery::removeQueryItem(). Use
    QUrlQuery::setQueryDelimiters() to customize the delimiters used for
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
    servers, mail clients and so on. Both forms are technically correct
    and represent the same URL unambiguously -- in fact, passing either
    form to QUrl's constructor or to setUrl() will yield the same QUrl
    object.

    QUrl conforms to the URI specification from
    \l{RFC 3986} (Uniform Resource Identifier: Generic Syntax), and includes
    scheme extensions from \l{RFC 1738} (Uniform Resource Locators). Case
    folding rules in QUrl conform to \l{RFC 3491} (Nameprep: A Stringprep
    Profile for Internationalized Domain Names (IDN)). It is also compatible with the
    \l{http://freedesktop.org/wiki/Specifications/file-uri-spec/}{file URI specification}
    from freedesktop.org, provided that the locale encodes file names using
    UTF-8 (required by IDN).

    \section2 Error checking

    QUrl is capable of detecting many errors in URLs while parsing it or when
    components of the URL are set with individual setter methods (like
    setScheme(), setHost() or setPath()). If the parsing or setter function is
    successful, any previously recorded error conditions will be discarded.

    By default, QUrl setter methods operate in QUrl::TolerantMode, which means
    they accept some common mistakes and mis-representation of data. An
    alternate method of parsing is QUrl::StrictMode, which applies further
    checks. See QUrl::ParsingMode for a description of the difference of the
    parsing modes.

    QUrl only checks for conformance with the URL specification. It does not
    try to verify that high-level protocol URLs are in the format they are
    expected to be by handlers elsewhere. For example, the following URIs are
    all considered valid by QUrl, even if they do not make sense when used:

    \list
      \li "http:/filename.html"
      \li "mailto://example.com"
    \endlist

    When the parser encounters an error, it signals the event by making
    isValid() return false and toString() / toEncoded() return an empty string.
    If it is necessary to show the user the reason why the URL failed to parse,
    the error condition can be obtained from QUrl by calling errorString().
    Note that this message is highly technical and may not make sense to
    end-users.

    QUrl is capable of recording only one error condition. If more than one
    error is found, it is undefined which error is reported.

    \section2 Character Conversions

    Follow these rules to avoid erroneous character conversion when
    dealing with URLs and strings:

    \list
    \li When creating an QString to contain a URL from a QByteArray or a
       char*, always use QString::fromUtf8().
    \endlist
*/

/*!
    \enum QUrl::ParsingMode

    The parsing mode controls the way QUrl parses strings.

    \value TolerantMode QUrl will try to correct some common errors in URLs.
                        This mode is useful for parsing URLs coming from sources
                        not known to be strictly standards-conforming.

    \value StrictMode Only valid URLs are accepted. This mode is useful for
                      general URL validation.

    \value DecodedMode QUrl will interpret the URL component in the fully-decoded form,
                       where percent characters stand for themselves, not as the beginning
                       of a percent-encoded sequence. This mode is only valid for the
                       setters setting components of a URL; it is not permitted in
                       the QUrl constructor, in fromEncoded() or in setUrl().
                       For more information on this mode, see the documentation for
                       QUrl::FullyDecoded.

    In TolerantMode, the parser has the following behaviour:

    \list

    \li Spaces and "%20": unencoded space characters will be accepted and will
    be treated as equivalent to "%20".

    \li Single "%" characters: Any occurrences of a percent character "%" not
    followed by exactly two hexadecimal characters (e.g., "13% coverage.html")
    will be replaced by "%25". Note that one lone "%" character will trigger
    the correction mode for all percent characters.

    \li Reserved and unreserved characters: An encoded URL should only
    contain a few characters as literals; all other characters should
    be percent-encoded. In TolerantMode, these characters will be
    accepted if they are found in the URL:
            space / double-quote / "<" / ">" / "\" /
            "^" / "`" / "{" / "|" / "}"
    Those same characters can be decoded again by passing QUrl::DecodeReserved
    to toString() or toEncoded(). In the getters of individual components,
    those characters are often returned in decoded form.

    \endlist

    When in StrictMode, if a parsing error is found, isValid() will return \c
    false and errorString() will return a message describing the error.
    If more than one error is detected, it is undefined which error gets
    reported.

    Note that TolerantMode is not usually enough for parsing user input, which
    often contains more errors and expectations than the parser can deal with.
    When dealing with data coming directly from the user -- as opposed to data
    coming from data-transfer sources, such as other programs -- it is
    recommended to use fromUserInput().

    \sa fromUserInput(), setUrl(), toString(), toEncoded(), QUrl::FormattingOptions
*/

/*!
    \enum QUrl::UrlFormattingOption

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
    \value RemoveFilename The filename (i.e. everything after the last '/' in the path) is removed.
            The trailing '/' is kept, unless StripTrailingSlash is set.
            Only valid if RemovePath is not set.
    \value PreferLocalFile If the URL is a local file according to isLocalFile()
     and contains no query or fragment, a local file path is returned.
    \value StripTrailingSlash  The trailing slash is removed if one is present.
    \value NormalizePathSegments  Modifies the path to remove redundant directory separators,
             and to resolve "."s and ".."s (as far as possible).

    Note that the case folding rules in \l{RFC 3491}{Nameprep}, which QUrl
    conforms to, require host names to always be converted to lower case,
    regardless of the Qt::FormattingOptions used.

    The options from QUrl::ComponentFormattingOptions are also possible.

    \sa QUrl::ComponentFormattingOptions
*/

/*!
    \enum QUrl::ComponentFormattingOption
    \since 5.0

    The component formatting options define how the components of an URL will
    be formatted when written out as text. They can be combined with the
    options from QUrl::FormattingOptions when used in toString() and
    toEncoded().

    \value PrettyDecoded   The component is returned in a "pretty form", with
                           most percent-encoded characters decoded. The exact
                           behavior of PrettyDecoded varies from component to
                           component and may also change from Qt release to Qt
                           release. This is the default.

    \value EncodeSpaces    Leave space characters in their encoded form ("%20").

    \value EncodeUnicode   Leave non-US-ASCII characters encoded in their UTF-8
                           percent-encoded form (e.g., "%C3%A9" for the U+00E9
                           codepoint, LATIN SMALL LETTER E WITH ACUTE).

    \value EncodeDelimiters Leave certain delimiters in their encoded form, as
                            would appear in the URL when the full URL is
                            represented as text. The delimiters are affected
                            by this option change from component to component.
                            This flag has no effect in toString() or toEncoded().

    \value EncodeReserved  Leave US-ASCII characters not permitted in the URL by
                           the specification in their encoded form. This is the
                           default on toString() and toEncoded().

    \value DecodeReserved  Decode the US-ASCII characters that the URL specification
                           does not allow to appear in the URL. This is the
                           default on the getters of individual components.

    \value FullyEncoded    Leave all characters in their properly-encoded form,
                           as this component would appear as part of a URL. When
                           used with toString(), this produces a fully-compliant
                           URL in QString form, exactly equal to the result of
                           toEncoded()

    \value FullyDecoded    Attempt to decode as much as possible. For individual
                           components of the URL, this decodes every percent
                           encoding sequence, including control characters (U+0000
                           to U+001F) and UTF-8 sequences found in percent-encoded form.
                           Use of this mode may cause data loss, see below for more information.

    The values of EncodeReserved and DecodeReserved should not be used together
    in one call. The behavior is undefined if that happens. They are provided
    as separate values because the behavior of the "pretty mode" with regards
    to reserved characters is different on certain components and specially on
    the full URL.

    \section2 Full decoding

    The FullyDecoded mode is similar to the behavior of the functions returning
    QString in Qt 4.x, in that every character represents itself and never has
    any special meaning. This is true even for the percent character ('%'),
    which should be interpreted to mean a literal percent, not the beginning of
    a percent-encoded sequence. The same actual character, in all other
    decoding modes, is represented by the sequence "%25".

    Whenever re-applying data obtained with QUrl::FullyDecoded into a QUrl,
    care must be taken to use the QUrl::DecodedMode parameter to the setters
    (like setPath() and setUserName()). Failure to do so may cause
    re-interpretation of the percent character ('%') as the beginning of a
    percent-encoded sequence.

    This mode is quite useful when portions of a URL are used in a non-URL
    context. For example, to extract the username, password or file paths in an
    FTP client application, the FullyDecoded mode should be used.

    This mode should be used with care, since there are two conditions that
    cannot be reliably represented in the returned QString. They are:

    \list
      \li \b{Non-UTF-8 sequences:} URLs may contain sequences of
      percent-encoded characters that do not form valid UTF-8 sequences. Since
      URLs need to be decoded using UTF-8, any decoder failure will result in
      the QString containing one or more replacement characters where the
      sequence existed.

      \li \b{Encoded delimiters:} URLs are also allowed to make a distinction
      between a delimiter found in its literal form and its equivalent in
      percent-encoded form. This is most commonly found in the query, but is
      permitted in most parts of the URL.
    \endlist

    The following example illustrates the problem:

    \code
        QUrl original("http://example.com/?q=a%2B%3Db%26c");
        QUrl copy(original);
        copy.setQuery(copy.query(QUrl::FullyDecoded), QUrl::DecodedMode);

        qDebug() << original.toString();   // prints: http://example.com/?q=a%2B%3Db%26c
        qDebug() << copy.toString();       // prints: http://example.com/?q=a+=b&c
    \endcode

    If the two URLs were used via HTTP GET, the interpretation by the web
    server would probably be different. In the first case, it would interpret
    as one parameter, with a key of "q" and value "a+=b&c". In the second
    case, it would probably interpret as two parameters, one with a key of "q"
    and value "a =b", and the second with a key "c" and no value.

    \sa QUrl::FormattingOptions
*/

/*!
    \fn QUrl::QUrl(QUrl &&other)

    Move-constructs a QUrl instance, making it point at the same
    object that \a other was pointing to.

    \since 5.2
*/

/*!
    \fn QUrl &QUrl::operator=(QUrl &&other)

    Move-assigns \a other to this QUrl instance.

    \since 5.2
*/

#include "qurl.h"
#include "qurl_p.h"
#include "qplatformdefs.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qdebug.h"
#include "qhash.h"
#include "qdir.h"         // for QDir::fromNativeSeparators
#include "qtldurl_p.h"
#include "private/qipaddress_p.h"
#include "qurlquery.h"
#if defined(Q_OS_WINCE_WM)
#pragma optimize("g", off)
#endif

QT_BEGIN_NAMESPACE
extern QString qt_normalizePathSegments(const QString &name, bool allowUncPaths); // qdir.cpp

inline static bool isHex(char c)
{
    c |= 0x20;
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

static inline QString ftpScheme()
{
    return QStringLiteral("ftp");
}

static inline QString fileScheme()
{
    return QStringLiteral("file");
}

#ifdef Q_COMPILER_CLASS_ENUM
#  define colon_uchar   : uchar
#else
#  define colon_uchar
#endif

class QUrlPrivate
{
public:
    enum Section colon_uchar {
        Scheme = 0x01,
        UserName = 0x02,
        Password = 0x04,
        UserInfo = UserName | Password,
        Host = 0x08,
        Port = 0x10,
        Authority = UserInfo | Host | Port,
        Path = 0x20,
        Hierarchy = Authority | Path,
        Query = 0x40,
        Fragment = 0x80,
        FullUrl = 0xff
    };

    enum Flags colon_uchar {
        IsLocalFile = 0x01
    };

    enum ErrorCode {
        // the high byte of the error code matches the Section
        // the first item in each value must be the generic "Invalid xxx Error"
        InvalidSchemeError = Scheme << 8,

        InvalidUserNameError = UserName << 8,

        InvalidPasswordError = Password << 8,

        InvalidRegNameError = Host << 8,
        InvalidIPv4AddressError,
        InvalidIPv6AddressError,
        InvalidCharacterInIPv6Error,
        InvalidIPvFutureError,
        HostMissingEndBracket,

        InvalidPortError = Port << 8,
        PortEmptyError,

        InvalidPathError = Path << 8,

        InvalidQueryError = Query << 8,

        InvalidFragmentError = Fragment << 8,

        // the following two cases are only possible in combination
        // with presence/absence of the authority and scheme. See validityError().
        AuthorityPresentAndPathIsRelative = Authority << 8 | Path << 8 | 0x10000,
        RelativeUrlPathContainsColonBeforeSlash = Scheme << 8 | Authority << 8 | Path << 8 | 0x10000,

        NoError = 0
    };

    struct Error {
        QString source;
        ErrorCode code;
        int position;
    };

    QUrlPrivate();
    QUrlPrivate(const QUrlPrivate &copy);
    ~QUrlPrivate();

    void parse(const QString &url, QUrl::ParsingMode parsingMode);
    bool isEmpty() const
    { return sectionIsPresent == 0 && port == -1 && path.isEmpty(); }

    Error *cloneError() const;
    void clearError();
    void setError(ErrorCode errorCode, const QString &source, int supplement = -1);
    ErrorCode validityError(QString *source = 0, int *position = 0) const;
    bool validateComponent(Section section, const QString &input, int begin, int end);
    bool validateComponent(Section section, const QString &input)
    { return validateComponent(section, input, 0, uint(input.length())); }

    // no QString scheme() const;
    void appendAuthority(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const;
    void appendUserInfo(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const;
    void appendUserName(QString &appendTo, QUrl::FormattingOptions options) const;
    void appendPassword(QString &appendTo, QUrl::FormattingOptions options) const;
    void appendHost(QString &appendTo, QUrl::FormattingOptions options) const;
    void appendPath(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const;
    void appendQuery(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const;
    void appendFragment(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const;

    // the "end" parameters are like STL iterators: they point to one past the last valid element
    bool setScheme(const QString &value, int len, bool doSetError);
    void setAuthority(const QString &auth, int from, int end, QUrl::ParsingMode mode);
    void setUserInfo(const QString &userInfo, int from, int end);
    void setUserName(const QString &value, int from, int end);
    void setPassword(const QString &value, int from, int end);
    bool setHost(const QString &value, int from, int end, QUrl::ParsingMode mode);
    void setPath(const QString &value, int from, int end);
    void setQuery(const QString &value, int from, int end);
    void setFragment(const QString &value, int from, int end);

    inline bool hasScheme() const { return sectionIsPresent & Scheme; }
    inline bool hasAuthority() const { return sectionIsPresent & Authority; }
    inline bool hasUserInfo() const { return sectionIsPresent & UserInfo; }
    inline bool hasUserName() const { return sectionIsPresent & UserName; }
    inline bool hasPassword() const { return sectionIsPresent & Password; }
    inline bool hasHost() const { return sectionIsPresent & Host; }
    inline bool hasPort() const { return port != -1; }
    inline bool hasPath() const { return !path.isEmpty(); }
    inline bool hasQuery() const { return sectionIsPresent & Query; }
    inline bool hasFragment() const { return sectionIsPresent & Fragment; }

    inline bool isLocalFile() const { return flags & IsLocalFile; }

    QString mergePaths(const QString &relativePath) const;

    QAtomicInt ref;
    int port;

    QString scheme;
    QString userName;
    QString password;
    QString host;
    QString path;
    QString query;
    QString fragment;

    Error *error;

    // not used for:
    //  - Port (port == -1 means absence)
    //  - Path (there's no path delimiter, so we optimize its use out of existence)
    // Schemes are never supposed to be empty, but we keep the flag anyway
    uchar sectionIsPresent;
    uchar flags;

    // 32-bit: 2 bytes tail padding available
    // 64-bit: 6 bytes tail padding available
};
#undef colon_uchar

inline QUrlPrivate::QUrlPrivate()
    : ref(1), port(-1),
      error(0),
      sectionIsPresent(0),
      flags(0)
{
}

inline QUrlPrivate::QUrlPrivate(const QUrlPrivate &copy)
    : ref(1), port(copy.port),
      scheme(copy.scheme),
      userName(copy.userName),
      password(copy.password),
      host(copy.host),
      path(copy.path),
      query(copy.query),
      fragment(copy.fragment),
      error(copy.cloneError()),
      sectionIsPresent(copy.sectionIsPresent),
      flags(copy.flags)
{
}

inline QUrlPrivate::~QUrlPrivate()
{
    delete error;
}

inline QUrlPrivate::Error *QUrlPrivate::cloneError() const
{
    return error ? new Error(*error) : 0;
}

inline void QUrlPrivate::clearError()
{
    delete error;
    error = 0;
}

inline void QUrlPrivate::setError(ErrorCode errorCode, const QString &source, int supplement)
{
    if (error) {
        // don't overwrite an error set in a previous section during parsing
        return;
    }
    error = new Error;
    error->code = errorCode;
    error->source = source;
    error->position = supplement;
}

// From RFC 3986, Appendix A Collected ABNF for URI
//    URI           = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
//[...]
//    scheme        = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
//
//    authority     = [ userinfo "@" ] host [ ":" port ]
//    userinfo      = *( unreserved / pct-encoded / sub-delims / ":" )
//    host          = IP-literal / IPv4address / reg-name
//    port          = *DIGIT
//[...]
//    reg-name      = *( unreserved / pct-encoded / sub-delims )
//[..]
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
// the path component has a complex ABNF that basically boils down to
// slash-separated segments of "pchar"

// The above is the strict definition of the URL components and we mostly
// adhere to it, with few exceptions. QUrl obeys the following behavior:
//  - percent-encoding sequences always use uppercase HEXDIG;
//  - unreserved characters are *always* decoded, no exceptions;
//  - the space character and bytes with the high bit set are controlled by
//    the EncodeSpaces and EncodeUnicode bits;
//  - control characters, the percent sign itself, and bytes with the high
//    bit set that don't form valid UTF-8 sequences are always encoded,
//    except in FullyDecoded mode;
//  - sub-delims are always left alone, except in FullyDecoded mode;
//  - gen-delim change behavior depending on which section of the URL (or
//    the entire URL) we're looking at; see below;
//  - characters not mentioned above, like "<", and ">", are usually
//    decoded in individual sections of the URL, but encoded when the full
//    URL is put together (we can change on subjective definition of
//    "pretty").
//
// The behavior for the delimiters bears some explanation. The spec says in
// section 2.2:
//     URIs that differ in the replacement of a reserved character with its
//     corresponding percent-encoded octet are not equivalent.
// (note: QUrl API mistakenly uses the "reserved" term, so we will refer to
// them here as "delimiters").
//
// For that reason, we cannot encode delimiters found in decoded form and we
// cannot decode the ones found in encoded form if that would change the
// interpretation. Conversely, we *can* perform the transformation if it would
// not change the interpretation. From the last component of a URL to the first,
// here are the gen-delims we can unambiguously transform when the field is
// taken in isolation:
//  - fragment: none, since it's the last
//  - query: "#" is unambiguous
//  - path: "#" and "?" are unambiguous
//  - host: completely special but never ambiguous, see setHost() below.
//  - password: the "#", "?", "/", "[", "]" and "@" characters are unambiguous
//  - username: the "#", "?", "/", "[", "]", "@", and ":" characters are unambiguous
//  - scheme: doesn't accept any delimiter, see setScheme() below.
//
// Internally, QUrl stores each component in the format that corresponds to the
// default mode (PrettyDecoded). It deviates from the "strict" FullyEncoded
// mode in the following way:
//  - spaces are decoded
//  - valid UTF-8 sequences are decoded
//  - gen-delims that can be unambiguously transformed are decoded
//  - characters controlled by DecodeReserved are often decoded, though this behavior
//    can change depending on the subjective definition of "pretty"
//
// Note that the list of gen-delims that we can transform is different for the
// user info (user name + password) and the authority (user info + host +
// port).


// list the recoding table modifications to be used with the recodeFromUser and
// appendToUser functions, according to the rules above. Spaces and UTF-8
// sequences are handled outside the tables.

// the encodedXXX tables are run with the delimiters set to "leave" by default;
// the decodedXXX tables are run with the delimiters set to "decode" by default
// (except for the query, which doesn't use these functions)

#define decode(x) ushort(x)
#define leave(x)  ushort(0x100 | (x))
#define encode(x) ushort(0x200 | (x))

static const ushort userNameInIsolation[] = {
    decode(':'), // 0
    decode('@'), // 1
    decode(']'), // 2
    decode('['), // 3
    decode('/'), // 4
    decode('?'), // 5
    decode('#'), // 6

    decode('"'), // 7
    decode('<'),
    decode('>'),
    decode('^'),
    decode('\\'),
    decode('|'),
    decode('{'),
    decode('}'),
    0
};
static const ushort * const passwordInIsolation = userNameInIsolation + 1;
static const ushort * const pathInIsolation = userNameInIsolation + 5;
static const ushort * const queryInIsolation = userNameInIsolation + 6;
static const ushort * const fragmentInIsolation = userNameInIsolation + 7;

static const ushort userNameInUserInfo[] =  {
    encode(':'), // 0
    decode('@'), // 1
    decode(']'), // 2
    decode('['), // 3
    decode('/'), // 4
    decode('?'), // 5
    decode('#'), // 6

    decode('"'), // 7
    decode('<'),
    decode('>'),
    decode('^'),
    decode('\\'),
    decode('|'),
    decode('{'),
    decode('}'),
    0
};
static const ushort * const passwordInUserInfo = userNameInUserInfo + 1;

static const ushort userNameInAuthority[] = {
    encode(':'), // 0
    encode('@'), // 1
    encode(']'), // 2
    encode('['), // 3
    decode('/'), // 4
    decode('?'), // 5
    decode('#'), // 6

    decode('"'), // 7
    decode('<'),
    decode('>'),
    decode('^'),
    decode('\\'),
    decode('|'),
    decode('{'),
    decode('}'),
    0
};
static const ushort * const passwordInAuthority = userNameInAuthority + 1;

static const ushort userNameInUrl[] = {
    encode(':'), // 0
    encode('@'), // 1
    encode(']'), // 2
    encode('['), // 3
    encode('/'), // 4
    encode('?'), // 5
    encode('#'), // 6

    // no need to list encode(x) for the other characters
    0
};
static const ushort * const passwordInUrl = userNameInUrl + 1;
static const ushort * const pathInUrl = userNameInUrl + 5;
static const ushort * const queryInUrl = userNameInUrl + 6;
static const ushort * const fragmentInUrl = userNameInUrl + 6;

static inline void parseDecodedComponent(QString &data)
{
    data.replace(QLatin1Char('%'), QStringLiteral("%25"));
}

static inline QString
recodeFromUser(const QString &input, const ushort *actions, int from, int to)
{
    QString output;
    const QChar *begin = input.constData() + from;
    const QChar *end = input.constData() + to;
    if (qt_urlRecode(output, begin, end, 0, actions))
        return output;

    return input.mid(from, to - from);
}

// appendXXXX functions: copy from the internal form to the external, user form.
// the internal value is stored in its PrettyDecoded form, so that case is easy.
static inline void appendToUser(QString &appendTo, const QString &value, QUrl::FormattingOptions options,
                                const ushort *actions)
{
    if (options == QUrl::PrettyDecoded) {
        appendTo += value;
        return;
    }

    if (!qt_urlRecode(appendTo, value.constData(), value.constEnd(), options, actions))
        appendTo += value;
}

inline void QUrlPrivate::appendAuthority(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const
{
    if ((options & QUrl::RemoveUserInfo) != QUrl::RemoveUserInfo) {
        appendUserInfo(appendTo, options, appendingTo);

        // add '@' only if we added anything
        if (hasUserName() || (hasPassword() && (options & QUrl::RemovePassword) == 0))
            appendTo += QLatin1Char('@');
    }
    appendHost(appendTo, options);
    if (!(options & QUrl::RemovePort) && port != -1)
        appendTo += QLatin1Char(':') + QString::number(port);
}

inline void QUrlPrivate::appendUserInfo(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const
{
    if (Q_LIKELY(!hasUserInfo()))
        return;

    const ushort *userNameActions;
    const ushort *passwordActions;
    if (options & QUrl::EncodeDelimiters) {
        userNameActions = userNameInUrl;
        passwordActions = passwordInUrl;
    } else {
        switch (appendingTo) {
        case UserInfo:
            userNameActions = userNameInUserInfo;
            passwordActions = passwordInUserInfo;
            break;

        case Authority:
            userNameActions = userNameInAuthority;
            passwordActions = passwordInAuthority;
            break;

        case FullUrl:
            userNameActions = userNameInUrl;
            passwordActions = passwordInUrl;
            break;

        default:
            // can't happen
            Q_UNREACHABLE();
            break;
        }
    }

    if (!qt_urlRecode(appendTo, userName.constData(), userName.constEnd(), options, userNameActions))
        appendTo += userName;
    if (options & QUrl::RemovePassword || !hasPassword()) {
        return;
    } else {
        appendTo += QLatin1Char(':');
        if (!qt_urlRecode(appendTo, password.constData(), password.constEnd(), options, passwordActions))
            appendTo += password;
    }
}

inline void QUrlPrivate::appendUserName(QString &appendTo, QUrl::FormattingOptions options) const
{
    // only called from QUrl::userName()
    appendToUser(appendTo, userName, options,
                 options & QUrl::EncodeDelimiters ? userNameInUrl : userNameInIsolation);
}

inline void QUrlPrivate::appendPassword(QString &appendTo, QUrl::FormattingOptions options) const
{
    // only called from QUrl::password()
    appendToUser(appendTo, password, options,
                 options & QUrl::EncodeDelimiters ? passwordInUrl : passwordInIsolation);
}

inline void QUrlPrivate::appendPath(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const
{
    QString thePath = path;
    if (options & QUrl::NormalizePathSegments) {
        thePath = qt_normalizePathSegments(path, false);
    }
    if (options & QUrl::RemoveFilename) {
        const int slash = path.lastIndexOf(QLatin1Char('/'));
        if (slash == -1)
            return;
        thePath = path.left(slash+1);
    }
    // check if we need to remove trailing slashes
    if (options & QUrl::StripTrailingSlash) {
        while (thePath.length() > 1 && thePath.endsWith(QLatin1Char('/')))
            thePath.chop(1);
    }

    appendToUser(appendTo, thePath, options,
                 appendingTo == FullUrl || options & QUrl::EncodeDelimiters ? pathInUrl : pathInIsolation);

}

inline void QUrlPrivate::appendFragment(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const
{
    appendToUser(appendTo, fragment, options,
                 options & QUrl::EncodeDelimiters ? fragmentInUrl :
                 appendingTo == FullUrl ? 0 : fragmentInIsolation);
}

inline void QUrlPrivate::appendQuery(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const
{
    appendToUser(appendTo, query, options,
                 appendingTo == FullUrl || options & QUrl::EncodeDelimiters ? queryInUrl : queryInIsolation);
}

// setXXX functions

inline bool QUrlPrivate::setScheme(const QString &value, int len, bool doSetError)
{
    // schemes are strictly RFC-compliant:
    //    scheme        = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
    // we also lowercase the scheme

    // schemes in URLs are not allowed to be empty, but they can be in
    // "Relative URIs" which QUrl also supports. QUrl::setScheme does
    // not call us with len == 0, so this can only be from parse()
    scheme.clear();
    if (len == 0)
        return false;

    sectionIsPresent |= Scheme;

    // validate it:
    int needsLowercasing = -1;
    const ushort *p = reinterpret_cast<const ushort *>(value.constData());
    for (int i = 0; i < len; ++i) {
        if (p[i] >= 'a' && p[i] <= 'z')
            continue;
        if (p[i] >= 'A' && p[i] <= 'Z') {
            needsLowercasing = i;
            continue;
        }
        if (p[i] >= '0' && p[i] <= '9' && i > 0)
            continue;
        if (p[i] == '+' || p[i] == '-' || p[i] == '.')
            continue;

        // found something else
        // don't call setError needlessly:
        // if we've been called from parse(), it will try to recover
        if (doSetError)
            setError(InvalidSchemeError, value, i);
        return false;
    }

    scheme = value.left(len);

    if (needsLowercasing != -1) {
        // schemes are ASCII only, so we don't need the full Unicode toLower
        QChar *schemeData = scheme.data(); // force detaching here
        for (int i = needsLowercasing; i >= 0; --i) {
            ushort c = schemeData[i].unicode();
            if (c >= 'A' && c <= 'Z')
                schemeData[i] = c + 0x20;
        }
    }

    // did we set to the file protocol?
    if (scheme == fileScheme())
        flags |= IsLocalFile;
    else
        flags &= ~IsLocalFile;
    return true;
}

inline void QUrlPrivate::setAuthority(const QString &auth, int from, int end, QUrl::ParsingMode mode)
{
    sectionIsPresent &= ~Authority;
    sectionIsPresent |= Host;

    // we never actually _loop_
    while (from != end) {
        int userInfoIndex = auth.indexOf(QLatin1Char('@'), from);
        if (uint(userInfoIndex) < uint(end)) {
            setUserInfo(auth, from, userInfoIndex);
            if (mode == QUrl::StrictMode && !validateComponent(UserInfo, auth, from, userInfoIndex))
                break;
            from = userInfoIndex + 1;
        }

        int colonIndex = auth.lastIndexOf(QLatin1Char(':'), end - 1);
        if (colonIndex < from)
            colonIndex = -1;

        if (uint(colonIndex) < uint(end)) {
            if (auth.at(from).unicode() == '[') {
                // check if colonIndex isn't inside the "[...]" part
                int closingBracket = auth.indexOf(QLatin1Char(']'), from);
                if (uint(closingBracket) > uint(colonIndex))
                    colonIndex = -1;
            }
        }

        if (colonIndex == end - 1) {
            // found a colon but no digits after it
            setError(PortEmptyError, auth, colonIndex + 1);
        } else if (uint(colonIndex) < uint(end)) {
            unsigned long x = 0;
            for (int i = colonIndex + 1; i < end; ++i) {
                ushort c = auth.at(i).unicode();
                if (c >= '0' && c <= '9') {
                    x *= 10;
                    x += c - '0';
                } else {
                    x = ulong(-1); // x != ushort(x)
                    break;
                }
            }
            if (x == ushort(x)) {
                port = ushort(x);
            } else {
                setError(InvalidPortError, auth, colonIndex + 1);
                if (mode == QUrl::StrictMode)
                    break;
            }
        } else {
            port = -1;
        }

        setHost(auth, from, qMin<uint>(end, colonIndex), mode);
        if (mode == QUrl::StrictMode && !validateComponent(Host, auth, from, qMin<uint>(end, colonIndex))) {
            // clear host too
            sectionIsPresent &= ~Authority;
            break;
        }

        // success
        return;
    }
    // clear all sections but host
    sectionIsPresent &= ~Authority | Host;
    userName.clear();
    password.clear();
    host.clear();
    port = -1;
}

inline void QUrlPrivate::setUserInfo(const QString &userInfo, int from, int end)
{
    int delimIndex = userInfo.indexOf(QLatin1Char(':'), from);
    setUserName(userInfo, from, qMin<uint>(delimIndex, end));

    if (uint(delimIndex) >= uint(end)) {
        password.clear();
        sectionIsPresent &= ~Password;
    } else {
        setPassword(userInfo, delimIndex + 1, end);
    }
}

inline void QUrlPrivate::setUserName(const QString &value, int from, int end)
{
    sectionIsPresent |= UserName;
    userName = recodeFromUser(value, userNameInIsolation, from, end);
}

inline void QUrlPrivate::setPassword(const QString &value, int from, int end)
{
    sectionIsPresent |= Password;
    password = recodeFromUser(value, passwordInIsolation, from, end);
}

inline void QUrlPrivate::setPath(const QString &value, int from, int end)
{
    // sectionIsPresent |= Path; // not used, save some cycles
    path = recodeFromUser(value, pathInIsolation, from, end);
}

inline void QUrlPrivate::setFragment(const QString &value, int from, int end)
{
    sectionIsPresent |= Fragment;
    fragment = recodeFromUser(value, fragmentInIsolation, from, end);
}

inline void QUrlPrivate::setQuery(const QString &value, int from, int iend)
{
    sectionIsPresent |= Query;
    query = recodeFromUser(value, queryInIsolation, from, iend);
}

// Host handling
// The RFC says the host is:
//    host          = IP-literal / IPv4address / reg-name
//    IP-literal    = "[" ( IPv6address / IPvFuture  ) "]"
//    IPvFuture     = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
//  [a strict definition of IPv6Address and IPv4Address]
//     reg-name      = *( unreserved / pct-encoded / sub-delims )
//
// We deviate from the standard in all but IPvFuture. For IPvFuture we accept
// and store only exactly what the RFC says we should. No percent-encoding is
// permitted in this field, so Unicode characters and space aren't either.
//
// For IPv4 addresses, we accept broken addresses like inet_aton does (that is,
// less than three dots). However, we correct the address to the proper form
// and store the corrected address. After correction, we comply to the RFC and
// it's exclusively composed of unreserved characters.
//
// For IPv6 addresses, we accept addresses including trailing (embedded) IPv4
// addresses, the so-called v4-compat and v4-mapped addresses. We also store
// those addresses like that in the hostname field, which violates the spec.
// IPv6 hosts are stored with the square brackets in the QString. It also
// requires no transformation in any way.
//
// As for registered names, it's the other way around: we accept only valid
// hostnames as specified by STD 3 and IDNA. That means everything we accept is
// valid in the RFC definition above, but there are many valid reg-names
// according to the RFC that we do not accept in the name of security. Since we
// do accept IDNA, reg-names are subject to ACE encoding and decoding, which is
// specified by the DecodeUnicode flag. The hostname is stored in its Unicode form.

inline void QUrlPrivate::appendHost(QString &appendTo, QUrl::FormattingOptions options) const
{
    // EncodeUnicode is the only flag that matters
    if ((options & QUrl::FullyDecoded) == QUrl::FullyDecoded)
        options = 0;
    else
        options &= QUrl::EncodeUnicode;
    if (host.isEmpty())
        return;
    if (host.at(0).unicode() == '[') {
        // IPv6Address and IPvFuture address never require any transformation
        appendTo += host;
    } else {
        // this is either an IPv4Address or a reg-name
        // if it is a reg-name, it is already stored in Unicode form
        if (options == QUrl::EncodeUnicode)
            appendTo += qt_ACE_do(host, ToAceOnly, AllowLeadingDot);
        else
            appendTo += host;
    }
}

// the whole IPvFuture is passed and parsed here, including brackets;
// returns null if the parsing was successful, or the QChar of the first failure
static const QChar *parseIpFuture(QString &host, const QChar *begin, const QChar *end, QUrl::ParsingMode mode)
{
    //    IPvFuture     = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
    static const char acceptable[] =
            "!$&'()*+,;=" // sub-delims
            ":"           // ":"
            "-._~";       // unreserved

    // the brackets and the "v" have been checked
    const QChar *const origBegin = begin;
    if (begin[3].unicode() != '.')
        return &begin[3];
    if ((begin[2].unicode() >= 'A' && begin[2].unicode() <= 'F') ||
            (begin[2].unicode() >= 'a' && begin[2].unicode() <= 'f') ||
            (begin[2].unicode() >= '0' && begin[2].unicode() <= '9')) {
        // this is so unlikely that we'll just go down the slow path
        // decode the whole string, skipping the "[vH." and "]" which we already know to be there
        host += QString::fromRawData(begin, 4);

        // uppercase the version, if necessary
        if (begin[2].unicode() >= 'a')
            host[host.length() - 2] = begin[2].unicode() - 0x20;

        begin += 4;
        --end;

        QString decoded;
        if (mode == QUrl::TolerantMode && qt_urlRecode(decoded, begin, end, QUrl::FullyDecoded, 0)) {
            begin = decoded.constBegin();
            end = decoded.constEnd();
        }

        for ( ; begin != end; ++begin) {
            if (begin->unicode() >= 'A' && begin->unicode() <= 'Z')
                host += *begin;
            else if (begin->unicode() >= 'a' && begin->unicode() <= 'z')
                host += *begin;
            else if (begin->unicode() >= '0' && begin->unicode() <= '9')
                host += *begin;
            else if (begin->unicode() < 0x80 && strchr(acceptable, begin->unicode()) != 0)
                host += *begin;
            else
                return decoded.isEmpty() ? begin : &origBegin[2];
        }
        host += QLatin1Char(']');
        return 0;
    }
    return &origBegin[2];
}

// ONLY the IPv6 address is parsed here, WITHOUT the brackets
static const QChar *parseIp6(QString &host, const QChar *begin, const QChar *end, QUrl::ParsingMode mode)
{
    QIPAddressUtils::IPv6Address address;
    const QChar *ret = QIPAddressUtils::parseIp6(address, begin, end);
    if (ret) {
        // this struct is kept in automatic storage because it's only 4 bytes
        const ushort decodeColon[] = { decode(':'), 0 };

        // IPv6 failed parsing, check if it was a percent-encoded character in
        // the middle and try again
        QString decoded;
        if (mode == QUrl::TolerantMode && qt_urlRecode(decoded, begin, end, 0, decodeColon)) {
            // recurse
            // if the parsing fails again, the qt_urlRecode above will return 0
            ret = parseIp6(host, decoded.constBegin(), decoded.constEnd(), mode);

            // we can't return ret, otherwise it would be dangling
            return ret ? end : 0;
        }

        // no transformation, nothing to re-parse
        return ret;
    }

    host.reserve(host.size() + (end - begin));
    host += QLatin1Char('[');
    QIPAddressUtils::toString(host, address);
    host += QLatin1Char(']');
    return 0;
}

inline bool QUrlPrivate::setHost(const QString &value, int from, int iend, QUrl::ParsingMode mode)
{
    const QChar *begin = value.constData() + from;
    const QChar *end = value.constData() + iend;

    const int len = end - begin;
    host.clear();
    sectionIsPresent |= Host;
    if (len == 0)
        return true;

    if (begin[0].unicode() == '[') {
        // IPv6Address or IPvFuture
        // smallest IPv6 address is      "[::]"   (len = 4)
        // smallest IPvFuture address is "[v7.X]" (len = 6)
        if (end[-1].unicode() != ']') {
            setError(HostMissingEndBracket, value);
            return false;
        }

        if (len > 5 && begin[1].unicode() == 'v') {
            const QChar *c = parseIpFuture(host, begin, end, mode);
            if (c)
                setError(InvalidIPvFutureError, value, c - value.constData());
            return !c;
        } else if (begin[1].unicode() == 'v') {
            setError(InvalidIPvFutureError, value, from);
        }

        const QChar *c = parseIp6(host, begin + 1, end - 1, mode);
        if (!c)
            return true;

        if (c == end - 1)
            setError(InvalidIPv6AddressError, value, from);
        else
            setError(InvalidCharacterInIPv6Error, value, c - value.constData());
        return false;
    }

    // check if it's an IPv4 address
    QIPAddressUtils::IPv4Address ip4;
    if (QIPAddressUtils::parseIp4(ip4, begin, end)) {
        // yes, it was
        QIPAddressUtils::toString(host, ip4);
        return true;
    }

    // This is probably a reg-name.
    // But it can also be an encoded string that, when decoded becomes one
    // of the types above.
    //
    // Two types of encoding are possible:
    //  percent encoding (e.g., "%31%30%2E%30%2E%30%2E%31" -> "10.0.0.1")
    //  Unicode encoding (some non-ASCII characters case-fold to digits
    //                    when nameprepping is done)
    //
    // The qt_ACE_do function below applies nameprepping and the STD3 check.
    // That means a Unicode string may become an IPv4 address, but it cannot
    // produce a '[' or a '%'.

    // check for percent-encoding first
    QString s;
    if (mode == QUrl::TolerantMode && qt_urlRecode(s, begin, end, 0, 0)) {
        // something was decoded
        // anything encoded left?
        int pos = s.indexOf(QChar(0x25)); // '%'
        if (pos != -1) {
            setError(InvalidRegNameError, s, pos);
            return false;
        }

        // recurse
        return setHost(s, 0, s.length(), QUrl::StrictMode);
    }

    s = qt_ACE_do(QString::fromRawData(begin, len), NormalizeAce, ForbidLeadingDot);
    if (s.isEmpty()) {
        setError(InvalidRegNameError, value);
        return false;
    }

    // check IPv4 again
    if (QIPAddressUtils::parseIp4(ip4, s.constBegin(), s.constEnd())) {
        QIPAddressUtils::toString(host, ip4);
    } else {
        host = s;
    }
    return true;
}

inline void QUrlPrivate::parse(const QString &url, QUrl::ParsingMode parsingMode)
{
    //   URI-reference = URI / relative-ref
    //   URI           = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
    //   relative-ref  = relative-part [ "?" query ] [ "#" fragment ]
    //   hier-part     = "//" authority path-abempty
    //                 / other path types
    //   relative-part = "//" authority path-abempty
    //                 /  other path types here

    sectionIsPresent = 0;
    flags = 0;
    clearError();

    // find the important delimiters
    int colon = -1;
    int question = -1;
    int hash = -1;
    const int len = url.length();
    const QChar *const begin = url.constData();
    const ushort *const data = reinterpret_cast<const ushort *>(begin);

    for (int i = 0; i < len; ++i) {
        uint uc = data[i];
        if (uc == '#' && hash == -1) {
            hash = i;

            // nothing more to be found
            break;
        }

        if (question == -1) {
            if (uc == ':' && colon == -1)
                colon = i;
            else if (uc == '?')
                question = i;
        }
    }

    // check if we have a scheme
    int hierStart;
    if (colon != -1 && setScheme(url, colon, /* don't set error */ false)) {
        hierStart = colon + 1;
    } else {
        // recover from a failed scheme: it might not have been a scheme at all
        scheme.clear();
        sectionIsPresent = 0;
        hierStart = 0;
    }

    int pathStart;
    int hierEnd = qMin<uint>(qMin<uint>(question, hash), len);
    if (hierEnd - hierStart >= 2 && data[hierStart] == '/' && data[hierStart + 1] == '/') {
        // we have an authority, it ends at the first slash after these
        int authorityEnd = hierEnd;
        for (int i = hierStart + 2; i < authorityEnd ; ++i) {
            if (data[i] == '/') {
                authorityEnd = i;
                break;
            }
        }

        setAuthority(url, hierStart + 2, authorityEnd, parsingMode);

        // even if we failed to set the authority properly, let's try to recover
        pathStart = authorityEnd;
        setPath(url, pathStart, hierEnd);
    } else {
        userName.clear();
        password.clear();
        host.clear();
        port = -1;
        pathStart = hierStart;

        if (hierStart < hierEnd)
            setPath(url, hierStart, hierEnd);
        else
            path.clear();
    }

    if (uint(question) < uint(hash))
        setQuery(url, question + 1, qMin<uint>(hash, len));

    if (hash != -1)
        setFragment(url, hash + 1, len);

    if (error || parsingMode == QUrl::TolerantMode)
        return;

    // The parsing so far was partially tolerant of errors, except for the
    // scheme parser (which is always strict) and the authority (which was
    // executed in strict mode).
    // If we haven't found any errors so far, continue the strict-mode parsing
    // from the path component onwards.

    if (!validateComponent(Path, url, pathStart, hierEnd))
        return;
    if (uint(question) < uint(hash) && !validateComponent(Query, url, question + 1, qMin<uint>(hash, len)))
        return;
    if (hash != -1)
        validateComponent(Fragment, url, hash + 1, len);
}

/*
    From http://www.ietf.org/rfc/rfc3986.txt, 5.2.3: Merge paths

    Returns a merge of the current path with the relative path passed
    as argument.

    Note: \a relativePath is relative (does not start with '/').
*/
inline QString QUrlPrivate::mergePaths(const QString &relativePath) const
{
    // If the base URI has a defined authority component and an empty
    // path, then return a string consisting of "/" concatenated with
    // the reference's path; otherwise,
    if (!host.isEmpty() && path.isEmpty())
        return QLatin1Char('/') + relativePath;

    // Return a string consisting of the reference's path component
    // appended to all but the last segment of the base URI's path
    // (i.e., excluding any characters after the right-most "/" in the
    // base URI path, or excluding the entire base URI path if it does
    // not contain any "/" characters).
    QString newPath;
    if (!path.contains(QLatin1Char('/')))
        newPath = relativePath;
    else
        newPath = path.leftRef(path.lastIndexOf(QLatin1Char('/')) + 1) + relativePath;

    return newPath;
}

/*
    From http://www.ietf.org/rfc/rfc3986.txt, 5.2.4: Remove dot segments

    Removes unnecessary ../ and ./ from the path. Used for normalizing
    the URL.
*/
static void removeDotsFromPath(QString *path)
{
    // The input buffer is initialized with the now-appended path
    // components and the output buffer is initialized to the empty
    // string.
    QChar *out = path->data();
    const QChar *in = out;
    const QChar *end = out + path->size();

    // If the input buffer consists only of
    // "." or "..", then remove that from the input
    // buffer;
    if (path->size() == 1 && in[0].unicode() == '.')
        ++in;
    else if (path->size() == 2 && in[0].unicode() == '.' && in[1].unicode() == '.')
        in += 2;
    // While the input buffer is not empty, loop:
    while (in < end) {

        // otherwise, if the input buffer begins with a prefix of "../" or "./",
        // then remove that prefix from the input buffer;
        if (path->size() >= 2 && in[0].unicode() == '.' && in[1].unicode() == '/')
            in += 2;
        else if (path->size() >= 3 && in[0].unicode() == '.'
                 && in[1].unicode() == '.' && in[2].unicode() == '/')
            in += 3;

        // otherwise, if the input buffer begins with a prefix of
        // "/./" or "/.", where "." is a complete path segment,
        // then replace that prefix with "/" in the input buffer;
        if (in <= end - 3 && in[0].unicode() == '/' && in[1].unicode() == '.'
                && in[2].unicode() == '/') {
            in += 2;
            continue;
        } else if (in == end - 2 && in[0].unicode() == '/' && in[1].unicode() == '.') {
            *out++ = QLatin1Char('/');
            in += 2;
            break;
        }

        // otherwise, if the input buffer begins with a prefix
        // of "/../" or "/..", where ".." is a complete path
        // segment, then replace that prefix with "/" in the
        // input buffer and remove the last //segment and its
        // preceding "/" (if any) from the output buffer;
        if (in <= end - 4 && in[0].unicode() == '/' && in[1].unicode() == '.'
                && in[2].unicode() == '.' && in[3].unicode() == '/') {
            while (out > path->constData() && (--out)->unicode() != '/')
                ;
            if (out == path->constData() && out->unicode() != '/')
                ++in;
            in += 3;
            continue;
        } else if (in == end - 3 && in[0].unicode() == '/' && in[1].unicode() == '.'
                   && in[2].unicode() == '.') {
            while (out > path->constData() && (--out)->unicode() != '/')
                ;
            if (out->unicode() == '/')
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
        while (in < end && in->unicode() != '/')
            *out++ = *in++;
    }
    path->truncate(out - path->constData());
}

inline QUrlPrivate::ErrorCode QUrlPrivate::validityError(QString *source, int *position) const
{
    Q_ASSERT(!source == !position);
    if (error) {
        if (source) {
            *source = error->source;
            *position = error->position;
        }
        return error->code;
    }

    // There are two more cases of invalid URLs that QUrl recognizes and they
    // are only possible with constructed URLs (setXXX methods), not with
    // parsing. Therefore, they are tested here.
    //
    // The two cases are a non-empty path that doesn't start with a slash and:
    //  - with an authority
    //  - without an authority, without scheme but the path with a colon before
    //    the first slash
    // Those cases are considered invalid because toString() would produce a URL
    // that wouldn't be parsed back to the same QUrl.

    if (path.isEmpty() || path.at(0) == QLatin1Char('/'))
        return NoError;
    if (sectionIsPresent & QUrlPrivate::Host) {
        if (source) {
            *source = path;
            *position = 0;
        }
        return AuthorityPresentAndPathIsRelative;
    }
    if (sectionIsPresent & QUrlPrivate::Scheme)
        return NoError;

    // check for a path of "text:text/"
    for (int i = 0; i < path.length(); ++i) {
        ushort c = path.at(i).unicode();
        if (c == '/') {
            // found the slash before the colon
            return NoError;
        }
        if (c == ':') {
            // found the colon before the slash, it's invalid
            if (source) {
                *source = path;
                *position = i;
            }
            return RelativeUrlPathContainsColonBeforeSlash;
        }
    }
    return NoError;
}

bool QUrlPrivate::validateComponent(QUrlPrivate::Section section, const QString &input,
                                    int begin, int end)
{
    // What we need to look out for, that the regular parser tolerates:
    //  - percent signs not followed by two hex digits
    //  - forbidden characters, which should always appear encoded
    //    '"' / '<' / '>' / '\' / '^' / '`' / '{' / '|' / '}' / BKSP
    //    control characters
    //  - delimiters not allowed in certain positions
    //    . scheme: parser is already strict
    //    . user info: gen-delims except ":" disallowed ("/" / "?" / "#" / "[" / "]" / "@")
    //    . host: parser is stricter than the standard
    //    . port: parser is stricter than the standard
    //    . path: all delimiters allowed
    //    . fragment: all delimiters allowed
    //    . query: all delimiters allowed
    static const char forbidden[] = "\"<>\\^`{|}\x7F";
    static const char forbiddenUserInfo[] = ":/?#[]@";

    Q_ASSERT(section != Authority && section != Hierarchy && section != FullUrl);

    const ushort *const data = reinterpret_cast<const ushort *>(input.constData());
    for (uint i = uint(begin); i < uint(end); ++i) {
        uint uc = data[i];
        if (uc >= 0x80)
            continue;

        bool error = false;
        if ((uc == '%' && (uint(end) < i + 2 || !isHex(data[i + 1]) || !isHex(data[i + 2])))
                || uc <= 0x20 || strchr(forbidden, uc)) {
            // found an error
            error = true;
        } else if (section & UserInfo) {
            if (section == UserInfo && strchr(forbiddenUserInfo + 1, uc))
                error = true;
            else if (section != UserInfo && strchr(forbiddenUserInfo, uc))
                error = true;
        }

        if (!error)
            continue;

        ErrorCode errorCode = ErrorCode(int(section) << 8);
        if (section == UserInfo) {
            // is it the user name or the password?
            errorCode = InvalidUserNameError;
            for (uint j = uint(begin); j < i; ++j)
                if (data[j] == ':') {
                    errorCode = InvalidPasswordError;
                    break;
                }
        }

        setError(errorCode, input, i);
        return false;
    }

    // no errors
    return true;
}

#if 0
inline void QUrlPrivate::validate() const
{
    QUrlPrivate *that = (QUrlPrivate *)this;
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
    } else if (scheme == ftpScheme() || scheme == httpScheme()) {
        if (host.isEmpty() && !(path.isEmpty() && encodedPath.isEmpty())) {
            that->isValid = false;
            that->errorInfo.setParams(0, QT_TRANSLATE_NOOP(QUrl, "the host is empty, but not the path"),
                                      0, 0);
        }
    }
}
#endif

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
    Constructs a URL by parsing \a url. QUrl will automatically percent encode
    all characters that are not allowed in a URL and decode the percent-encoded
    sequences that represent an unreserved character (letters, digits, hyphens,
    undercores, dots and tildes). All other characters are left in their
    original forms.

    Parses the \a url using the parser mode \a parsingMode. In TolerantMode
    (the default), QUrl will correct certain mistakes, notably the presence of
    a percent character ('%') not followed by two hexadecimal digits, and it
    will accept any character in any position. In StrictMode, encoding mistakes
    will not be tolerated and QUrl will also check that certain forbidden
    characters are not present in unencoded form. If an error is detected in
    StrictMode, isValid() will return false. The parsing mode DecodedMode is not
    permitted in this context.

    Example:

    \snippet code/src_corelib_io_qurl.cpp 0

    To construct a URL from an encoded string, you can also use fromEncoded():

    \snippet code/src_corelib_io_qurl.cpp 1

    Both functions are equivalent and, in Qt 5, both functions accept encoded
    data. Usually, the choice of the QUrl constructor or setUrl() versus
    fromEncoded() will depend on the source data: the constructor and setUrl()
    take a QString, whereas fromEncoded takes a QByteArray.

    \sa setUrl(), fromEncoded(), TolerantMode
*/
QUrl::QUrl(const QString &url, ParsingMode parsingMode) : d(0)
{
    setUrl(url, parsingMode);
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
    Returns \c true if the URL is non-empty and valid; otherwise returns \c false.

    The URL is run through a conformance test. Every part of the URL
    must conform to the standard encoding rules of the URI standard
    for the URL to be reported as valid.

    \snippet code/src_corelib_io_qurl.cpp 2
*/
bool QUrl::isValid() const
{
    if (isEmpty()) {
        // also catches d == 0
        return false;
    }
    return d->validityError() == QUrlPrivate::NoError;
}

/*!
    Returns \c true if the URL has no data; otherwise returns \c false.

    \sa clear()
*/
bool QUrl::isEmpty() const
{
    if (!d) return true;
    return d->isEmpty();
}

/*!
    Resets the content of the QUrl. After calling this function, the
    QUrl is equal to one that has been constructed with the default
    empty constructor.

    \sa isEmpty()
*/
void QUrl::clear()
{
    if (d && !d->ref.deref())
        delete d;
    d = 0;
}

/*!
    Parses \a url and sets this object to that value. QUrl will automatically
    percent encode all characters that are not allowed in a URL and decode the
    percent-encoded sequences that represent an unreserved character (letters,
    digits, hyphens, undercores, dots and tildes). All other characters are
    left in their original forms.

    Parses the \a url using the parser mode \a parsingMode. In TolerantMode
    (the default), QUrl will correct certain mistakes, notably the presence of
    a percent character ('%') not followed by two hexadecimal digits, and it
    will accept any character in any position. In StrictMode, encoding mistakes
    will not be tolerated and QUrl will also check that certain forbidden
    characters are not present in unencoded form. If an error is detected in
    StrictMode, isValid() will return false. The parsing mode DecodedMode is
    not permitted in this context and will produce a run-time warning.

    \sa url(), toString()
*/
void QUrl::setUrl(const QString &url, ParsingMode parsingMode)
{
    if (parsingMode == DecodedMode) {
        qWarning("QUrl: QUrl::DecodedMode is not permitted when parsing a full URL");
    } else {
        detach();
        d->parse(url, parsingMode);
    }
}

/*!
    \fn void QUrl::setEncodedUrl(const QByteArray &encodedUrl, ParsingMode parsingMode)
    \deprecated
    Constructs a URL by parsing the contents of \a encodedUrl.

    \a encodedUrl is assumed to be a URL string in percent encoded
    form, containing only ASCII characters.

    The parsing mode \a parsingMode is used for parsing \a encodedUrl.

    \obsolete Use setUrl(QString::fromUtf8(encodedUrl), parsingMode)

    \sa setUrl()
*/

/*!
    Sets the scheme of the URL to \a scheme. As a scheme can only
    contain ASCII characters, no conversion or decoding is done on the
    input. It must also start with an ASCII letter.

    The scheme describes the type (or protocol) of the URL. It's
    represented by one or more ASCII characters at the start the URL.

    A scheme is strictly \l {http://www.ietf.org/rfc/rfc3986.txt} {RFC 3986}-compliant:
        \tt {scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )}

    The following example shows a URL where the scheme is "ftp":

    \image qurl-authority2.png

    To set the scheme, the following call is used:
    \code
        QUrl url;
        url.setScheme("ftp");
    \endcode

    The scheme can also be empty, in which case the URL is interpreted
    as relative.

    \sa scheme(), isRelative()
*/
void QUrl::setScheme(const QString &scheme)
{
    detach();
    d->clearError();
    if (scheme.isEmpty()) {
        // schemes are not allowed to be empty
        d->sectionIsPresent &= ~QUrlPrivate::Scheme;
        d->flags &= ~QUrlPrivate::IsLocalFile;
        d->scheme.clear();
    } else {
        d->setScheme(scheme, scheme.length(), /* do set error */ true);
    }
}

/*!
    Returns the scheme of the URL. If an empty string is returned,
    this means the scheme is undefined and the URL is then relative.

    The scheme can only contain US-ASCII letters or digits, which means it
    cannot contain any character that would otherwise require encoding.
    Additionally, schemes are always returned in lowercase form.

    \sa setScheme(), isRelative()
*/
QString QUrl::scheme() const
{
    if (!d) return QString();

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

    \image qurl-authority.png

    The \a authority data is interpreted according to \a mode: in StrictMode,
    any '%' characters must be followed by exactly two hexadecimal characters
    and some characters (including space) are not allowed in undecoded form. In
    TolerantMode (the default), all characters are accepted in undecoded form
    and the tolerant parser will correct stray '%' not followed by two hex
    characters.

    This function does not allow \a mode to be QUrl::DecodedMode. To set fully
    decoded data, call setUserName(), setPassword(), setHost() and setPort()
    individually.

    \sa setUserInfo(), setHost(), setPort()
*/
void QUrl::setAuthority(const QString &authority, ParsingMode mode)
{
    detach();
    d->clearError();

    if (mode == DecodedMode) {
        qWarning("QUrl::setAuthority(): QUrl::DecodedMode is not permitted in this function");
        return;
    }

    d->setAuthority(authority, 0, authority.length(), mode);
    if (authority.isNull()) {
        // QUrlPrivate::setAuthority cleared almost everything
        // but it leaves the Host bit set
        d->sectionIsPresent &= ~QUrlPrivate::Authority;
    }
}

/*!
    Returns the authority of the URL if it is defined; otherwise
    an empty string is returned.

    This function returns an unambiguous value, which may contain that
    characters still percent-encoded, plus some control sequences not
    representable in decoded form in QString.

    The \a options argument controls how to format the user info component. The
    value of QUrl::FullyDecoded is not permitted in this function. If you need
    to obtain fully decoded data, call userName(), password(), host() and
    port() individually.

    \sa setAuthority(), userInfo(), userName(), password(), host(), port()
*/
QString QUrl::authority(ComponentFormattingOptions options) const
{
    if (!d) return QString();

    if (options == QUrl::FullyDecoded) {
        qWarning("QUrl::authority(): QUrl::FullyDecoded is not permitted in this function");
        return QString();
    }

    QString result;
    d->appendAuthority(result, options, QUrlPrivate::Authority);
    return result;
}

/*!
    Sets the user info of the URL to \a userInfo. The user info is an
    optional part of the authority of the URL, as described in
    setAuthority().

    The user info consists of a user name and optionally a password,
    separated by a ':'. If the password is empty, the colon must be
    omitted. The following example shows a valid user info string:

    \image qurl-authority3.png

    The \a userInfo data is interpreted according to \a mode: in StrictMode,
    any '%' characters must be followed by exactly two hexadecimal characters
    and some characters (including space) are not allowed in undecoded form. In
    TolerantMode (the default), all characters are accepted in undecoded form
    and the tolerant parser will correct stray '%' not followed by two hex
    characters.

    This function does not allow \a mode to be QUrl::DecodedMode. To set fully
    decoded data, call setUserName() and setPassword() individually.

    \sa userInfo(), setUserName(), setPassword(), setAuthority()
*/
void QUrl::setUserInfo(const QString &userInfo, ParsingMode mode)
{
    detach();
    d->clearError();
    QString trimmed = userInfo.trimmed();
    if (mode == DecodedMode) {
        qWarning("QUrl::setUserInfo(): QUrl::DecodedMode is not permitted in this function");
        return;
    }

    d->setUserInfo(trimmed, 0, trimmed.length());
    if (userInfo.isNull()) {
        // QUrlPrivate::setUserInfo cleared almost everything
        // but it leaves the UserName bit set
        d->sectionIsPresent &= ~QUrlPrivate::UserInfo;
    } else if (mode == StrictMode && !d->validateComponent(QUrlPrivate::UserInfo, userInfo)) {
        d->sectionIsPresent &= ~QUrlPrivate::UserInfo;
        d->userName.clear();
        d->password.clear();
    }
}

/*!
    Returns the user info of the URL, or an empty string if the user
    info is undefined.

    This function returns an unambiguous value, which may contain that
    characters still percent-encoded, plus some control sequences not
    representable in decoded form in QString.

    The \a options argument controls how to format the user info component. The
    value of QUrl::FullyDecoded is not permitted in this function. If you need
    to obtain fully decoded data, call userName() and password() individually.

    \sa setUserInfo(), userName(), password(), authority()
*/
QString QUrl::userInfo(ComponentFormattingOptions options) const
{
    if (!d) return QString();

    if (options == QUrl::FullyDecoded) {
        qWarning("QUrl::userInfo(): QUrl::FullyDecoded is not permitted in this function");
        return QString();
    }

    QString result;
    d->appendUserInfo(result, options, QUrlPrivate::UserInfo);
    return result;
}

/*!
    Sets the URL's user name to \a userName. The \a userName is part
    of the user info element in the authority of the URL, as described
    in setUserInfo().

    The \a userName data is interpreted according to \a mode: in StrictMode,
    any '%' characters must be followed by exactly two hexadecimal characters
    and some characters (including space) are not allowed in undecoded form. In
    TolerantMode (the default), all characters are accepted in undecoded form
    and the tolerant parser will correct stray '%' not followed by two hex
    characters. In DecodedMode, '%' stand for themselves and encoded characters
    are not possible.

    QUrl::DecodedMode should be used when setting the user name from a data
    source which is not a URL, such as a password dialog shown to the user or
    with a user name obtained by calling userName() with the QUrl::FullyDecoded
    formatting option.

    \sa userName(), setUserInfo()
*/
void QUrl::setUserName(const QString &userName, ParsingMode mode)
{
    detach();
    d->clearError();

    QString data = userName;
    if (mode == DecodedMode) {
        parseDecodedComponent(data);
        mode = TolerantMode;
    }

    d->setUserName(data, 0, data.length());
    if (userName.isNull())
        d->sectionIsPresent &= ~QUrlPrivate::UserName;
    else if (mode == StrictMode && !d->validateComponent(QUrlPrivate::UserName, userName))
        d->userName.clear();
}

/*!
    Returns the user name of the URL if it is defined; otherwise
    an empty string is returned.

    The \a options argument controls how to format the user name component. All
    values produce an unambiguous result. With QUrl::FullyDecoded, all
    percent-encoded sequences are decoded; otherwise, the returned value may
    contain some percent-encoded sequences for some control sequences not
    representable in decoded form in QString.

    Note that QUrl::FullyDecoded may cause data loss if those non-representable
    sequences are present. It is recommended to use that value when the result
    will be used in a non-URL context, such as setting in QAuthenticator or
    negotiating a login.

    \sa setUserName(), userInfo()
*/
QString QUrl::userName(ComponentFormattingOptions options) const
{
    if (!d) return QString();

    QString result;
    d->appendUserName(result, options);
    return result;
}

/*!
    \fn void QUrl::setEncodedUserName(const QByteArray &userName)
    \deprecated
    \since 4.4

    Sets the URL's user name to the percent-encoded \a userName. The \a
    userName is part of the user info element in the authority of the
    URL, as described in setUserInfo().

    \obsolete Use setUserName(QString::fromUtf8(userName))

    \sa setUserName(), encodedUserName(), setUserInfo()
*/

/*!
    \fn QByteArray QUrl::encodedUserName() const
    \deprecated
    \since 4.4

    Returns the user name of the URL if it is defined; otherwise
    an empty string is returned. The returned value will have its
    non-ASCII and other control characters percent-encoded, as in
    toEncoded().

    \obsolete Use userName(QUrl::FullyEncoded).toLatin1()

    \sa setEncodedUserName()
*/

/*!
    Sets the URL's password to \a password. The \a password is part of
    the user info element in the authority of the URL, as described in
    setUserInfo().

    The \a password data is interpreted according to \a mode: in StrictMode,
    any '%' characters must be followed by exactly two hexadecimal characters
    and some characters (including space) are not allowed in undecoded form. In
    TolerantMode, all characters are accepted in undecoded form and the
    tolerant parser will correct stray '%' not followed by two hex characters.
    In DecodedMode, '%' stand for themselves and encoded characters are not
    possible.

    QUrl::DecodedMode should be used when setting the password from a data
    source which is not a URL, such as a password dialog shown to the user or
    with a password obtained by calling password() with the QUrl::FullyDecoded
    formatting option.

    \sa password(), setUserInfo()
*/
void QUrl::setPassword(const QString &password, ParsingMode mode)
{
    detach();
    d->clearError();

    QString data = password;
    if (mode == DecodedMode) {
        parseDecodedComponent(data);
        mode = TolerantMode;
    }

    d->setPassword(data, 0, data.length());
    if (password.isNull())
        d->sectionIsPresent &= ~QUrlPrivate::Password;
    else if (mode == StrictMode && !d->validateComponent(QUrlPrivate::Password, password))
        d->password.clear();
}

/*!
    Returns the password of the URL if it is defined; otherwise
    an empty string is returned.

    The \a options argument controls how to format the user name component. All
    values produce an unambiguous result. With QUrl::FullyDecoded, all
    percent-encoded sequences are decoded; otherwise, the returned value may
    contain some percent-encoded sequences for some control sequences not
    representable in decoded form in QString.

    Note that QUrl::FullyDecoded may cause data loss if those non-representable
    sequences are present. It is recommended to use that value when the result
    will be used in a non-URL context, such as setting in QAuthenticator or
    negotiating a login.

    \sa setPassword()
*/
QString QUrl::password(ComponentFormattingOptions options) const
{
    if (!d) return QString();

    QString result;
    d->appendPassword(result, options);
    return result;
}

/*!
    \fn void QUrl::setEncodedPassword(const QByteArray &password)
    \deprecated
    \since 4.4

    Sets the URL's password to the percent-encoded \a password. The \a
    password is part of the user info element in the authority of the
    URL, as described in setUserInfo().

    \obsolete Use setPassword(QString::fromUtf8(password));

    \sa setPassword(), encodedPassword(), setUserInfo()
*/

/*!
    \fn QByteArray QUrl::encodedPassword() const
    \deprecated
    \since 4.4

    Returns the password of the URL if it is defined; otherwise an
    empty string is returned. The returned value will have its
    non-ASCII and other control characters percent-encoded, as in
    toEncoded().

    \obsolete Use password(QUrl::FullyEncoded).toLatin1()

    \sa setEncodedPassword(), toEncoded()
*/

/*!
    Sets the host of the URL to \a host. The host is part of the
    authority.

    The \a host data is interpreted according to \a mode: in StrictMode,
    any '%' characters must be followed by exactly two hexadecimal characters
    and some characters (including space) are not allowed in undecoded form. In
    TolerantMode, all characters are accepted in undecoded form and the
    tolerant parser will correct stray '%' not followed by two hex characters.
    In DecodedMode, '%' stand for themselves and encoded characters are not
    possible.

    Note that, in all cases, the result of the parsing must be a valid hostname
    according to STD 3 rules, as modified by the Internationalized Resource
    Identifiers specification (RFC 3987). Invalid hostnames are not permitted
    and will cause isValid() to become false.

    \sa host(), setAuthority()
*/
void QUrl::setHost(const QString &host, ParsingMode mode)
{
    detach();
    d->clearError();

    QString data = host;
    if (mode == DecodedMode) {
        parseDecodedComponent(data);
        mode = TolerantMode;
    }

    if (d->setHost(data, 0, data.length(), mode)) {
        if (host.isNull())
            d->sectionIsPresent &= ~QUrlPrivate::Host;
    } else if (!data.startsWith(QLatin1Char('['))) {
        // setHost failed, it might be IPv6 or IPvFuture in need of bracketing
        Q_ASSERT(d->error);

        data.prepend(QLatin1Char('['));
        data.append(QLatin1Char(']'));
        if (!d->setHost(data, 0, data.length(), mode)) {
            // failed again
            if (data.contains(QLatin1Char(':'))) {
                // source data contains ':', so it's an IPv6 error
                d->error->code = QUrlPrivate::InvalidIPv6AddressError;
            }
        } else {
            // succeeded
            d->clearError();
        }
    }
}

/*!
    Returns the host of the URL if it is defined; otherwise
    an empty string is returned.

    The \a options argument controls how the hostname will be formatted. The
    QUrl::EncodeUnicode option will cause this function to return the hostname
    in the ASCII-Compatible Encoding (ACE) form, which is suitable for use in
    channels that are not 8-bit clean or that require the legacy hostname (such
    as DNS requests or in HTTP request headers). If that flag is not present,
    this function returns the International Domain Name (IDN) in Unicode form,
    according to the list of permissible top-level domains (see
    idnWhiteList()).

    All other flags are ignored. Host names cannot contain control or percent
    characters, so the returned value can be considered fully decoded.

    \sa setHost(), idnWhitelist(), setIdnWhitelist(), authority()
*/
QString QUrl::host(ComponentFormattingOptions options) const
{
    if (!d) return QString();

    QString result;
    d->appendHost(result, options);
    if (result.startsWith(QLatin1Char('[')))
        return result.mid(1, result.length() - 2);
    return result;
}

/*!
    \fn void QUrl::setEncodedHost(const QByteArray &host)
    \deprecated
    \since 4.4

    Sets the URL's host to the ACE- or percent-encoded \a host. The \a
    host is part of the user info element in the authority of the
    URL, as described in setAuthority().

    \obsolete Use setHost(QString::fromUtf8(host)).

    \sa setHost(), encodedHost(), setAuthority(), fromAce()
*/

/*!
    \fn QByteArray QUrl::encodedHost() const
    \deprecated
    \since 4.4

    Returns the host part of the URL if it is defined; otherwise
    an empty string is returned.

    Note: encodedHost() does not return percent-encoded hostnames. Instead,
    the ACE-encoded (bare ASCII in Punycode encoding) form will be
    returned for any non-ASCII hostname.

    This function is equivalent to calling QUrl::toAce() on the return
    value of host().

    \obsolete Use host(QUrl::FullyEncoded).toLatin1() or toAce(host()).

    \sa setEncodedHost()
*/

/*!
    Sets the port of the URL to \a port. The port is part of the
    authority of the URL, as described in setAuthority().

    \a port must be between 0 and 65535 inclusive. Setting the
    port to -1 indicates that the port is unspecified.
*/
void QUrl::setPort(int port)
{
    detach();
    d->clearError();

    if (port < -1 || port > 65535) {
        port = -1;
        d->setError(QUrlPrivate::InvalidPortError, QString::number(port), 0);
    }

    d->port = port;
}

/*!
    \since 4.1

    Returns the port of the URL, or \a defaultPort if the port is
    unspecified.

    Example:

    \snippet code/src_corelib_io_qurl.cpp 3
*/
int QUrl::port(int defaultPort) const
{
    if (!d) return defaultPort;
    return d->port == -1 ? defaultPort : d->port;
}

/*!
    Sets the path of the URL to \a path. The path is the part of the
    URL that comes after the authority but before the query string.

    \image qurl-ftppath.png

    For non-hierarchical schemes, the path will be everything
    following the scheme declaration, as in the following example:

    \image qurl-mailtopath.png

    The \a path data is interpreted according to \a mode: in StrictMode,
    any '%' characters must be followed by exactly two hexadecimal characters
    and some characters (including space) are not allowed in undecoded form. In
    TolerantMode (the default), all characters are accepted in undecoded form and the
    tolerant parser will correct stray '%' not followed by two hex characters.
    In DecodedMode, '%' stand for themselves and encoded characters are not
    possible.

    QUrl::DecodedMode should be used when setting the path from a data source
    which is not a URL, such as a dialog shown to the user or with a path
    obtained by calling path() with the QUrl::FullyDecoded formatting option.

    \sa path()
*/
void QUrl::setPath(const QString &path, ParsingMode mode)
{
    detach();
    d->clearError();

    QString data = path;
    if (mode == DecodedMode) {
        parseDecodedComponent(data);
        mode = TolerantMode;
    }

    data = qt_normalizePathSegments(data, false);
    d->setPath(data, 0, data.length());

    // optimized out, since there is no path delimiter
//    if (path.isNull())
//        d->sectionIsPresent &= ~QUrlPrivate::Path;
//    else
    if (mode == StrictMode && !d->validateComponent(QUrlPrivate::Path, path))
        d->path.clear();
}

/*!
    Returns the path of the URL.

    The \a options argument controls how to format the path component. All
    values produce an unambiguous result. With QUrl::FullyDecoded, all
    percent-encoded sequences are decoded; otherwise, the returned value may
    contain some percent-encoded sequences for some control sequences not
    representable in decoded form in QString.

    Note that QUrl::FullyDecoded may cause data loss if those non-representable
    sequences are present. It is recommended to use that value when the result
    will be used in a non-URL context, such as sending to an FTP server.

    \sa setPath()
*/
QString QUrl::path(ComponentFormattingOptions options) const
{
    if (!d) return QString();

    QString result;
    d->appendPath(result, options, QUrlPrivate::Path);
    return result;
}

/*!
    \fn void QUrl::setEncodedPath(const QByteArray &path)
    \deprecated
    \since 4.4

    Sets the URL's path to the percent-encoded \a path.  The path is
    the part of the URL that comes after the authority but before the
    query string.

    \image qurl-ftppath.png

    For non-hierarchical schemes, the path will be everything
    following the scheme declaration, as in the following example:

    \image qurl-mailtopath.png

    \obsolete Use setPath(QString::fromUtf8(path)).

    \sa setPath(), encodedPath(), setUserInfo()
*/

/*!
    \fn QByteArray QUrl::encodedPath() const
    \deprecated
    \since 4.4

    Returns the path of the URL if it is defined; otherwise an
    empty string is returned. The returned value will have its
    non-ASCII and other control characters percent-encoded, as in
    toEncoded().

    \obsolete Use path(QUrl::FullyEncoded).toLatin1().

    \sa setEncodedPath(), toEncoded()
*/

/*!
    \since 5.2

    Returns the name of the file, excluding the directory path.

    Note that, if this QUrl object is given a path ending in a slash, the name of the file is considered empty.

    If the path doesn't contain any slash, it is fully returned as the fileName.

    Example:

    \snippet code/src_corelib_io_qurl.cpp 7

    The \a options argument controls how to format the file name component. All
    values produce an unambiguous result. With QUrl::FullyDecoded, all
    percent-encoded sequences are decoded; otherwise, the returned value may
    contain some percent-encoded sequences for some control sequences not
    representable in decoded form in QString.

    \sa path()
*/
QString QUrl::fileName(ComponentFormattingOptions options) const
{
    const QString ourPath = path(options);
    const int slash = ourPath.lastIndexOf(QLatin1Char('/'));
    if (slash == -1)
        return ourPath;
    return ourPath.mid(slash + 1);
}

/*!
    \since 4.2

    Returns \c true if this URL contains a Query (i.e., if ? was seen on it).

    \sa setQuery(), query(), hasFragment()
*/
bool QUrl::hasQuery() const
{
    if (!d) return false;
    return d->hasQuery();
}

/*!
    Sets the query string of the URL to \a query.

    This function is useful if you need to pass a query string that
    does not fit into the key-value pattern, or that uses a different
    scheme for encoding special characters than what is suggested by
    QUrl.

    Passing a value of QString() to \a query (a null QString) unsets
    the query completely. However, passing a value of QString("")
    will set the query to an empty value, as if the original URL
    had a lone "?".

    The \a query data is interpreted according to \a mode: in StrictMode,
    any '%' characters must be followed by exactly two hexadecimal characters
    and some characters (including space) are not allowed in undecoded form. In
    TolerantMode, all characters are accepted in undecoded form and the
    tolerant parser will correct stray '%' not followed by two hex characters.
    In DecodedMode, '%' stand for themselves and encoded characters are not
    possible.

    Query strings often contain percent-encoded sequences, so use of
    DecodedMode is discouraged. One special sequence to be aware of is that of
    the plus character ('+'). QUrl does not convert spaces to plus characters,
    even though HTML forms posted by web browsers do. In order to represent an
    actual plus character in a query, the sequence "%2B" is usually used. This
    function will leave "%2B" sequences untouched in TolerantMode or
    StrictMode.

    \sa query(), hasQuery()
*/
void QUrl::setQuery(const QString &query, ParsingMode mode)
{
    detach();
    d->clearError();

    QString data = query;
    if (mode == DecodedMode) {
        parseDecodedComponent(data);
        mode = TolerantMode;
    }

    d->setQuery(data, 0, data.length());
    if (query.isNull())
        d->sectionIsPresent &= ~QUrlPrivate::Query;
    else if (mode == StrictMode && !d->validateComponent(QUrlPrivate::Query, query))
        d->query.clear();
}

/*!
    \fn void QUrl::setEncodedQuery(const QByteArray &query)
    \deprecated

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

    \obsolete Use setQuery, which has the same null / empty behavior.

    \sa encodedQuery(), hasQuery()
*/

/*!
    \overload
    \since 5.0
    Sets the query string of the URL to \a query.

    This function reconstructs the query string from the QUrlQuery object and
    sets on this QUrl object. This function does not have parsing parameters
    because the QUrlQuery contains data that is already parsed.

    \sa query(), hasQuery()
*/
void QUrl::setQuery(const QUrlQuery &query)
{
    detach();
    d->clearError();

    // we know the data is in the right format
    d->query = query.toString();
    if (query.isEmpty())
        d->sectionIsPresent &= ~QUrlPrivate::Query;
    else
        d->sectionIsPresent |= QUrlPrivate::Query;
}

/*!
    \fn void QUrl::setQueryItems(const QList<QPair<QString, QString> > &query)
    \deprecated

    Sets the query string of the URL to an encoded version of \a
    query. The contents of \a query are converted to a string
    internally, each pair delimited by the character returned by
    pairDelimiter(), and the key and value are delimited by
    valueDelimiter().

    \note This method does not encode spaces (ASCII 0x20) as plus (+) signs,
    like HTML forms do. If you need that kind of encoding, you must encode
    the value yourself and use QUrl::setEncodedQueryItems.

    \obsolete Use QUrlQuery and setQuery().

    \sa queryItems(), setEncodedQueryItems()
*/

/*!
    \fn void QUrl::setEncodedQueryItems(const QList<QPair<QByteArray, QByteArray> > &query)
    \deprecated
    \since 4.4

    Sets the query string of the URL to the encoded version of \a
    query. The contents of \a query are converted to a string
    internally, each pair delimited by the character returned by
    pairDelimiter(), and the key and value are delimited by
    valueDelimiter().

    \obsolete Use QUrlQuery and setQuery().

    \sa encodedQueryItems(), setQueryItems()
*/

/*!
    \fn void QUrl::addQueryItem(const QString &key, const QString &value)
    \deprecated

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

    \obsolete Use QUrlQuery and setQuery().

    \sa addEncodedQueryItem()
*/

/*!
    \fn void QUrl::addEncodedQueryItem(const QByteArray &key, const QByteArray &value)
    \deprecated
    \since 4.4

    Inserts the pair \a key = \a value into the query string of the
    URL.

    \obsolete Use QUrlQuery and setQuery().

    \sa addQueryItem()
*/

/*!
    \fn QList<QPair<QString, QString> > QUrl::queryItems() const
    \deprecated

    Returns the query string of the URL, as a map of keys and values.

    \note This method does not decode spaces plus (+) signs as spaces (ASCII
    0x20), like HTML forms do. If you need that kind of decoding, you must
    use QUrl::encodedQueryItems and decode the data yourself.

    \obsolete Use QUrlQuery.

    \sa setQueryItems(), setEncodedQuery()
*/

/*!
    \fn QList<QPair<QByteArray, QByteArray> > QUrl::encodedQueryItems() const
    \deprecated
    \since 4.4

    Returns the query string of the URL, as a map of encoded keys and values.

    \obsolete Use QUrlQuery.

    \sa setEncodedQueryItems(), setQueryItems(), setEncodedQuery()
*/

/*!
    \fn bool QUrl::hasQueryItem(const QString &key) const
    \deprecated

    Returns \c true if there is a query string pair whose key is equal
    to \a key from the URL.

    \obsolete Use QUrlQuery.

    \sa hasEncodedQueryItem()
*/

/*!
    \fn bool QUrl::hasEncodedQueryItem(const QByteArray &key) const
    \deprecated
    \since 4.4

    Returns \c true if there is a query string pair whose key is equal
    to \a key from the URL.

    \obsolete Use QUrlQuery.

    \sa hasQueryItem()
*/

/*!
    \fn QString QUrl::queryItemValue(const QString &key) const
    \deprecated

    Returns the first query string value whose key is equal to \a key
    from the URL.

    \note This method does not decode spaces plus (+) signs as spaces (ASCII
    0x20), like HTML forms do. If you need that kind of decoding, you must
    use QUrl::encodedQueryItemValue and decode the data yourself.

    \obsolete Use QUrlQuery.

    \sa allQueryItemValues()
*/

/*!
    \fn QByteArray QUrl::encodedQueryItemValue(const QByteArray &key) const
    \deprecated
    \since 4.4

    Returns the first query string value whose key is equal to \a key
    from the URL.

    \obsolete Use QUrlQuery.

    \sa queryItemValue(), allQueryItemValues()
*/

/*!
    \fn QStringList QUrl::allQueryItemValues(const QString &key) const
    \deprecated

    Returns the a list of query string values whose key is equal to
    \a key from the URL.

    \note This method does not decode spaces plus (+) signs as spaces (ASCII
    0x20), like HTML forms do. If you need that kind of decoding, you must
    use QUrl::allEncodedQueryItemValues and decode the data yourself.

    \obsolete Use QUrlQuery.

    \sa queryItemValue()
*/

/*!
    \fn QList<QByteArray> QUrl::allEncodedQueryItemValues(const QByteArray &key) const
    \deprecated
    \since 4.4

    Returns the a list of query string values whose key is equal to
    \a key from the URL.

    \obsolete Use QUrlQuery.

    \sa allQueryItemValues(), queryItemValue(), encodedQueryItemValue()
*/

/*!
    \fn void QUrl::removeQueryItem(const QString &key)
    \deprecated

    Removes the first query string pair whose key is equal to \a key
    from the URL.

    \obsolete Use QUrlQuery.

    \sa removeAllQueryItems()
*/

/*!
    \fn void QUrl::removeEncodedQueryItem(const QByteArray &key)
    \deprecated
    \since 4.4

    Removes the first query string pair whose key is equal to \a key
    from the URL.

    \obsolete Use QUrlQuery.

    \sa removeQueryItem(), removeAllQueryItems()
*/

/*!
    \fn void QUrl::removeAllQueryItems(const QString &key)
    \deprecated

    Removes all the query string pairs whose key is equal to \a key
    from the URL.

    \obsolete Use QUrlQuery.

   \sa removeQueryItem()
*/

/*!
    \fn void QUrl::removeAllEncodedQueryItems(const QByteArray &key)
    \deprecated
    \since 4.4

    Removes all the query string pairs whose key is equal to \a key
    from the URL.

    \obsolete Use QUrlQuery.

   \sa removeQueryItem()
*/

/*!
    \fn QByteArray QUrl::encodedQuery() const
    \deprecated

    Returns the query string of the URL in percent encoded form.

    \obsolete Use query(QUrl::FullyEncoded).toLatin1()

    \sa setEncodedQuery(), query()
*/

/*!
    Returns the query string of the URL if there's a query string, or an empty
    result if not. To determine if the parsed URL contained a query string, use
    hasQuery().

    The \a options argument controls how to format the query component. All
    values produce an unambiguous result. With QUrl::FullyDecoded, all
    percent-encoded sequences are decoded; otherwise, the returned value may
    contain some percent-encoded sequences for some control sequences not
    representable in decoded form in QString.

    Note that use of QUrl::FullyDecoded in queries is discouraged, as queries
    often contain data that is supposed to remain percent-encoded, including
    the use of the "%2B" sequence to represent a plus character ('+').

    \sa setQuery(), hasQuery()
*/
QString QUrl::query(ComponentFormattingOptions options) const
{
    if (!d) return QString();

    QString result;
    d->appendQuery(result, options, QUrlPrivate::Query);
    if (d->hasQuery() && result.isNull())
        result.detach();
    return result;
}

/*!
    Sets the fragment of the URL to \a fragment. The fragment is the
    last part of the URL, represented by a '#' followed by a string of
    characters. It is typically used in HTTP for referring to a
    certain link or point on a page:

    \image qurl-fragment.png

    The fragment is sometimes also referred to as the URL "reference".

    Passing an argument of QString() (a null QString) will unset the fragment.
    Passing an argument of QString("") (an empty but not null QString)
    will set the fragment to an empty string (as if the original URL
    had a lone "#").

    The \a fragment data is interpreted according to \a mode: in StrictMode,
    any '%' characters must be followed by exactly two hexadecimal characters
    and some characters (including space) are not allowed in undecoded form. In
    TolerantMode, all characters are accepted in undecoded form and the
    tolerant parser will correct stray '%' not followed by two hex characters.
    In DecodedMode, '%' stand for themselves and encoded characters are not
    possible.

    QUrl::DecodedMode should be used when setting the fragment from a data
    source which is not a URL or with a fragment obtained by calling
    fragment() with the QUrl::FullyDecoded formatting option.

    \sa fragment(), hasFragment()
*/
void QUrl::setFragment(const QString &fragment, ParsingMode mode)
{
    detach();
    d->clearError();

    QString data = fragment;
    if (mode == DecodedMode) {
        parseDecodedComponent(data);
        mode = TolerantMode;
    }

    d->setFragment(data, 0, data.length());
    if (fragment.isNull())
        d->sectionIsPresent &= ~QUrlPrivate::Fragment;
    else if (mode == StrictMode && !d->validateComponent(QUrlPrivate::Fragment, fragment))
        d->fragment.clear();
}

/*!
    Returns the fragment of the URL. To determine if the parsed URL contained a
    fragment, use hasFragment().

    The \a options argument controls how to format the fragment component. All
    values produce an unambiguous result. With QUrl::FullyDecoded, all
    percent-encoded sequences are decoded; otherwise, the returned value may
    contain some percent-encoded sequences for some control sequences not
    representable in decoded form in QString.

    Note that QUrl::FullyDecoded may cause data loss if those non-representable
    sequences are present. It is recommended to use that value when the result
    will be used in a non-URL context.

    \sa setFragment(), hasFragment()
*/
QString QUrl::fragment(ComponentFormattingOptions options) const
{
    if (!d) return QString();

    QString result;
    d->appendFragment(result, options, QUrlPrivate::Fragment);
    if (d->hasFragment() && result.isNull())
        result.detach();
    return result;
}

/*!
    \fn void QUrl::setEncodedFragment(const QByteArray &fragment)
    \deprecated
    \since 4.4

    Sets the URL's fragment to the percent-encoded \a fragment. The fragment is the
    last part of the URL, represented by a '#' followed by a string of
    characters. It is typically used in HTTP for referring to a
    certain link or point on a page:

    \image qurl-fragment.png

    The fragment is sometimes also referred to as the URL "reference".

    Passing an argument of QByteArray() (a null QByteArray) will unset
    the fragment.  Passing an argument of QByteArray("") (an empty but
    not null QByteArray) will set the fragment to an empty string (as
    if the original URL had a lone "#").

    \obsolete Use setFragment(), which has the same behavior of null / empty.

    \sa setFragment(), encodedFragment()
*/

/*!
    \fn QByteArray QUrl::encodedFragment() const
    \deprecated
    \since 4.4

    Returns the fragment of the URL if it is defined; otherwise an
    empty string is returned. The returned value will have its
    non-ASCII and other control characters percent-encoded, as in
    toEncoded().

    \obsolete Use query(QUrl::FullyEncoded).toLatin1().

    \sa setEncodedFragment(), toEncoded()
*/

/*!
    \since 4.2

    Returns \c true if this URL contains a fragment (i.e., if # was seen on it).

    \sa fragment(), setFragment()
*/
bool QUrl::hasFragment() const
{
    if (!d) return false;
    return d->hasFragment();
}

/*!
    \since 4.8

    Returns the TLD (Top-Level Domain) of the URL, (e.g. .co.uk, .net).
    Note that the return value is prefixed with a '.' unless the
    URL does not contain a valid TLD, in which case the function returns
    an empty string.

    If \a options includes EncodeUnicode, the returned string will be in
    ASCII Compatible Encoding.
*/
QString QUrl::topLevelDomain(ComponentFormattingOptions options) const
{
    QString tld = qTopLevelDomain(host());
    if (options & EncodeUnicode) {
        return qt_ACE_do(tld, ToAceOnly, AllowLeadingDot);
    }
    return tld;
}

/*!
    Returns the result of the merge of this URL with \a relative. This
    URL is used as a base to convert \a relative to an absolute URL.

    If \a relative is not a relative URL, this function will return \a
    relative directly. Otherwise, the paths of the two URLs are
    merged, and the new URL returned has the scheme and authority of
    the base URL, but with the merged path, as in the following
    example:

    \snippet code/src_corelib_io_qurl.cpp 5

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

    QUrl t;
    // be non strict and allow scheme in relative url
    if (!relative.d->scheme.isEmpty() && relative.d->scheme != d->scheme) {
        t = relative;
        t.detach();
    } else {
        if (relative.d->hasAuthority()) {
            t = relative;
            t.detach();
        } else {
            t.d = new QUrlPrivate;

            // copy the authority
            t.d->userName = d->userName;
            t.d->password = d->password;
            t.d->host = d->host;
            t.d->port = d->port;
            t.d->sectionIsPresent = d->sectionIsPresent & QUrlPrivate::Authority;

            if (relative.d->path.isEmpty()) {
                t.d->path = d->path;
                if (relative.d->hasQuery()) {
                    t.d->query = relative.d->query;
                    t.d->sectionIsPresent |= QUrlPrivate::Query;
                } else if (d->hasQuery()) {
                    t.d->query = d->query;
                    t.d->sectionIsPresent |= QUrlPrivate::Query;
                }
            } else {
                t.d->path = relative.d->path.startsWith(QLatin1Char('/'))
                            ? relative.d->path
                            : d->mergePaths(relative.d->path);
                if (relative.d->hasQuery()) {
                    t.d->query = relative.d->query;
                    t.d->sectionIsPresent |= QUrlPrivate::Query;
                }
            }
        }
        t.d->scheme = d->scheme;
        if (d->hasScheme())
            t.d->sectionIsPresent |= QUrlPrivate::Scheme;
        else
            t.d->sectionIsPresent &= ~QUrlPrivate::Scheme;
        t.d->flags |= d->flags & QUrlPrivate::IsLocalFile;
    }
    t.d->fragment = relative.d->fragment;
    if (relative.d->hasFragment())
        t.d->sectionIsPresent |= QUrlPrivate::Fragment;
    else
        t.d->sectionIsPresent &= ~QUrlPrivate::Fragment;

    removeDotsFromPath(&t.d->path);

#if defined(QURL_DEBUG)
    qDebug("QUrl(\"%s\").resolved(\"%s\") = \"%s\"",
           qPrintable(url()),
           qPrintable(relative.url()),
           qPrintable(t.url()));
#endif
    return t;
}

/*!
    Returns \c true if the URL is relative; otherwise returns \c false. A URL is
    relative reference if its scheme is undefined; this function is therefore
    equivalent to calling scheme().isEmpty().

    Relative references are defined in RFC 3986 section 4.2.
*/
bool QUrl::isRelative() const
{
    if (!d) return true;
    return !d->hasScheme();
}

/*!
    Returns a string representation of the URL. The output can be customized by
    passing flags with \a options. The option QUrl::FullyDecoded is not
    permitted in this function since it would generate ambiguous data.

    The resulting QString can be passed back to a QUrl later on.

    Synonym for toString(options).

    \sa FormattingOptions, toEncoded(), toString()
*/
QString QUrl::url(FormattingOptions options) const
{
    return toString(options);
}

/*!
    Returns a string representation of the URL. The output can be customized by
    passing flags with \a options. The option QUrl::FullyDecoded is not
    permitted in this function since it would generate ambiguous data.

    The default formatting option is \l{QUrl::FormattingOptions}{PrettyDecoded}.

    \sa FormattingOptions, url(), setUrl()
*/
QString QUrl::toString(FormattingOptions options) const
{
    if (!isValid()) {
        // also catches isEmpty()
        return QString();
    }
    if (options == QUrl::FullyDecoded) {
        qWarning("QUrl: QUrl::FullyDecoded is not permitted when reconstructing the full URL");
        options = QUrl::PrettyDecoded;
    }

    // return just the path if:
    //  - QUrl::PreferLocalFile is passed
    //  - QUrl::RemovePath isn't passed (rather stupid if the user did...)
    //  - there's no query or fragment to return
    //    that is, either they aren't present, or we're removing them
    //  - it's a local file
    if (options.testFlag(QUrl::PreferLocalFile) && !options.testFlag(QUrl::RemovePath)
            && (!d->hasQuery() || options.testFlag(QUrl::RemoveQuery))
            && (!d->hasFragment() || options.testFlag(QUrl::RemoveFragment))
            && isLocalFile()) {
        return path(options);
    }

    QString url;

    // for the full URL, we consider that the reserved characters are prettier if encoded
    if (options & DecodeReserved)
        options &= ~EncodeReserved;
    else
        options |= EncodeReserved;

    if (!(options & QUrl::RemoveScheme) && d->hasScheme())
        url += d->scheme + QLatin1Char(':');

    bool pathIsAbsolute = d->path.startsWith(QLatin1Char('/'));
    if (!((options & QUrl::RemoveAuthority) == QUrl::RemoveAuthority) && d->hasAuthority()) {
        url += QLatin1String("//");
        d->appendAuthority(url, options, QUrlPrivate::FullUrl);
    } else if (isLocalFile() && pathIsAbsolute) {
        // Comply with the XDG file URI spec, which requires triple slashes.
        url += QLatin1String("//");
    }

    if (!(options & QUrl::RemovePath))
        d->appendPath(url, options, QUrlPrivate::FullUrl);

    if (!(options & QUrl::RemoveQuery) && d->hasQuery()) {
        url += QLatin1Char('?');
        d->appendQuery(url, options, QUrlPrivate::FullUrl);
    }
    if (!(options & QUrl::RemoveFragment) && d->hasFragment()) {
        url += QLatin1Char('#');
        d->appendFragment(url, options, QUrlPrivate::FullUrl);
    }

    return url;
}

/*!
    \since 5.0

    Returns a human-displayable string representation of the URL.
    The output can be customized by passing flags with \a options.
    The option RemovePassword is always enabled, since passwords
    should never be shown back to users.

    With the default options, the resulting QString can be passed back
    to a QUrl later on, but any password that was present initially will
    be lost.

    \sa FormattingOptions, toEncoded(), toString()
*/

QString QUrl::toDisplayString(FormattingOptions options) const
{
    return toString(options | RemovePassword);
}

/*!
    \since 5.2

    Returns an adjusted version of the URL.
    The output can be customized by passing flags with \a options.

    The encoding options from QUrl::ComponentFormattingOption don't make
    much sense for this method, nor does QUrl::PreferLocalFile.

    This is always equivalent to QUrl(url.toString(options)).

    \sa FormattingOptions, toEncoded(), toString()
*/
QUrl QUrl::adjusted(QUrl::FormattingOptions options) const
{
    if (!isValid()) {
        // also catches isEmpty()
        return QUrl();
    }
    QUrl that = *this;
    if (options & RemoveScheme)
        that.setScheme(QString());
    if ((options & RemoveAuthority) == RemoveAuthority) {
        that.setAuthority(QString());
    } else {
        if ((options & RemoveUserInfo) == RemoveUserInfo)
            that.setUserInfo(QString());
        else if (options & RemovePassword)
            that.setPassword(QString());
        if (options & RemovePort)
            that.setPort(-1);
    }
    if (options & RemoveQuery)
        that.setQuery(QString());
    if (options & RemoveFragment)
        that.setFragment(QString());
    if (options & RemovePath) {
        that.setPath(QString());
    } else if (options & (StripTrailingSlash | RemoveFilename | NormalizePathSegments)) {
        that.detach();
        QString path;
        d->appendPath(path, options | FullyEncoded, QUrlPrivate::Path);
        that.d->setPath(path, 0, path.length());
    }
    return that;
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
    options &= ~(FullyDecoded | FullyEncoded);
    QString stringForm = toString(options | FullyEncoded);
    return stringForm.toLatin1();
}

/*!
    \fn QUrl QUrl::fromEncoded(const QByteArray &input, ParsingMode parsingMode)

    Parses \a input and returns the corresponding QUrl. \a input is
    assumed to be in encoded form, containing only ASCII characters.

    Parses the URL using \a parsingMode. See setUrl() for more information on
    this parameter. QUrl::DecodedMode is not permitted in this context.

    \sa toEncoded(), setUrl()
*/
QUrl QUrl::fromEncoded(const QByteArray &input, ParsingMode mode)
{
    return QUrl(QString::fromUtf8(input.constData(), input.size()), mode);
}

/*!
    Returns a decoded copy of \a input. \a input is first decoded from
    percent encoding, then converted from UTF-8 to unicode.
*/
QString QUrl::fromPercentEncoding(const QByteArray &input)
{
    QByteArray ba = QByteArray::fromPercentEncoding(input);
    return QString::fromUtf8(ba, ba.size());
}

/*!
    Returns an encoded copy of \a input. \a input is first converted
    to UTF-8, and all ASCII-characters that are not in the unreserved group
    are percent encoded. To prevent characters from being percent encoded
    pass them to \a exclude. To force characters to be percent encoded pass
    them to \a include.

    Unreserved is defined as:
       \tt {ALPHA / DIGIT / "-" / "." / "_" / "~"}

    \snippet code/src_corelib_io_qurl.cpp 6
*/
QByteArray QUrl::toPercentEncoding(const QString &input, const QByteArray &exclude, const QByteArray &include)
{
    return input.toUtf8().toPercentEncoding(exclude, include);
}

/*! \fn QUrl QUrl::fromCFURL(CFURLRef url)
    \since 5.2

    Constructs a QUrl containing a copy of the CFURL \a url.
*/

/*! \fn CFURLRef QUrl::toCFURL() const
    \since 5.2

    Creates a CFURL from a QUrl. The caller owns the CFURL and is
    responsible for releasing it.
*/

/*!
    \fn QUrl QUrl::fromNSURL(const NSURL *url)
    \since 5.2

    Constructs a QUrl containing a copy of the NSURL \a url.
*/

/*!
    \fn NSURL* QUrl::toNSURL() const
    \since 5.2

    Creates a NSURL from a QUrl. The NSURL is autoreleased.
*/

/*!
    \internal
    \since 5.0
    Used in the setEncodedXXX compatibility functions. Converts \a ba to
    QString form.
*/
QString QUrl::fromEncodedComponent_helper(const QByteArray &ba)
{
    return qt_urlRecodeByteArray(ba);
}

/*!
    \fn QByteArray QUrl::toPunycode(const QString &uc)
    \obsolete
    Returns a \a uc in Punycode encoding.

    Punycode is a Unicode encoding used for internationalized domain
    names, as defined in RFC3492. If you want to convert a domain name from
    Unicode to its ASCII-compatible representation, use toAce().
*/

/*!
    \fn QString QUrl::fromPunycode(const QByteArray &pc)
    \obsolete
    Returns the Punycode decoded representation of \a pc.

    Punycode is a Unicode encoding used for internationalized domain
    names, as defined in RFC3492. If you want to convert a domain from
    its ASCII-compatible encoding to the Unicode representation, use
    fromAce().
*/

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
    return qt_ACE_do(QString::fromLatin1(domain), NormalizeAce, ForbidLeadingDot /*FIXME: make configurable*/);
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

    This function returns an empty QByteArray if \a domain is not a valid
    hostname. Note, in particular, that IPv6 literals are not valid domain
    names.
*/
QByteArray QUrl::toAce(const QString &domain)
{
    QString result = qt_ACE_do(domain, ToAceOnly, ForbidLeadingDot /*FIXME: make configurable*/);
    return result.toLatin1();
}

/*!
    \internal

    Returns \c true if this URL is "less than" the given \a url. This
    provides a means of ordering URLs.
*/
bool QUrl::operator <(const QUrl &url) const
{
    if (!d || !url.d) {
        bool thisIsEmpty = !d || d->isEmpty();
        bool thatIsEmpty = !url.d || url.d->isEmpty();

        // sort an empty URL first
        return thisIsEmpty && !thatIsEmpty;
    }

    int cmp;
    cmp = d->scheme.compare(url.d->scheme);
    if (cmp != 0)
        return cmp < 0;

    cmp = d->userName.compare(url.d->userName);
    if (cmp != 0)
        return cmp < 0;

    cmp = d->password.compare(url.d->password);
    if (cmp != 0)
        return cmp < 0;

    cmp = d->host.compare(url.d->host);
    if (cmp != 0)
        return cmp < 0;

    if (d->port != url.d->port)
        return d->port < url.d->port;

    cmp = d->path.compare(url.d->path);
    if (cmp != 0)
        return cmp < 0;

    if (d->hasQuery() != url.d->hasQuery())
        return url.d->hasQuery();

    cmp = d->query.compare(url.d->query);
    if (cmp != 0)
        return cmp < 0;

    if (d->hasFragment() != url.d->hasFragment())
        return url.d->hasFragment();

    cmp = d->fragment.compare(url.d->fragment);
    return cmp < 0;
}

/*!
    Returns \c true if this URL and the given \a url are equal;
    otherwise returns \c false.
*/
bool QUrl::operator ==(const QUrl &url) const
{
    if (!d && !url.d)
        return true;
    if (!d)
        return url.d->isEmpty();
    if (!url.d)
        return d->isEmpty();

    // First, compare which sections are present, since it speeds up the
    // processing considerably. We just have to ignore the host-is-present flag
    // for local files (the "file" protocol), due to the requirements of the
    // XDG file URI specification.
    int mask = QUrlPrivate::FullUrl;
    if (isLocalFile())
        mask &= ~QUrlPrivate::Host;
    return (d->sectionIsPresent & mask) == (url.d->sectionIsPresent & mask) &&
            d->scheme == url.d->scheme &&
            d->userName == url.d->userName &&
            d->password == url.d->password &&
            d->host == url.d->host &&
            d->port == url.d->port &&
            d->path == url.d->path &&
            d->query == url.d->query &&
            d->fragment == url.d->fragment;
}

/*!
    \since 5.2

    Returns \c true if this URL and the given \a url are equal after
    applying \a options to both; otherwise returns \c false.

    This is equivalent to calling adjusted(options) on both URLs
    and comparing the resulting urls, but faster.

*/
bool QUrl::matches(const QUrl &url, FormattingOptions options) const
{
    if (!d && !url.d)
        return true;
    if (!d)
        return url.d->isEmpty();
    if (!url.d)
        return d->isEmpty();

    // First, compare which sections are present, since it speeds up the
    // processing considerably. We just have to ignore the host-is-present flag
    // for local files (the "file" protocol), due to the requirements of the
    // XDG file URI specification.
    int mask = QUrlPrivate::FullUrl;
    if (isLocalFile())
        mask &= ~QUrlPrivate::Host;

    if (options & QUrl::RemoveScheme)
        mask &= ~QUrlPrivate::Scheme;
    else if (d->scheme != url.d->scheme)
        return false;

    if (options & QUrl::RemovePassword)
        mask &= ~QUrlPrivate::Password;
    else if (d->password != url.d->password)
        return false;

    if (options & QUrl::RemoveUserInfo)
        mask &= ~QUrlPrivate::UserName;
    else if (d->userName != url.d->userName)
        return false;

    if (options & QUrl::RemovePort)
        mask &= ~QUrlPrivate::Port;
    else if (d->port != url.d->port)
        return false;

    if (options & QUrl::RemoveAuthority)
        mask &= ~QUrlPrivate::Host;
    else if (d->host != url.d->host)
        return false;

    if (options & QUrl::RemoveQuery)
        mask &= ~QUrlPrivate::Query;
    else if (d->query != url.d->query)
        return false;

    if (options & QUrl::RemoveFragment)
        mask &= ~QUrlPrivate::Fragment;
    else if (d->fragment != url.d->fragment)
        return false;

    if ((d->sectionIsPresent & mask) != (url.d->sectionIsPresent & mask))
        return false;

    // Compare paths, after applying path-related options
    QString path1;
    d->appendPath(path1, options, QUrlPrivate::Path);
    QString path2;
    url.d->appendPath(path2, options, QUrlPrivate::Path);
    return path1 == path2;
}

/*!
    Returns \c true if this URL and the given \a url are not equal;
    otherwise returns \c false.
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
        detach();
        d->parse(url, TolerantMode);
    }
    return *this;
}

/*!
    \fn void QUrl::swap(QUrl &other)
    \since 4.8

    Swaps URL \a other with this URL. This operation is very
    fast and never fails.
*/

/*!
    \internal

    Forces a detach.
*/
void QUrl::detach()
{
    if (!d)
        d = new QUrlPrivate;
    else
        qAtomicDetach(d);
}

/*!
    \internal
*/
bool QUrl::isDetached() const
{
    return !d || d->ref.load() == 1;
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
    url.setScheme(fileScheme());
    QString deslashified = QDir::fromNativeSeparators(localFile);

    // magic for drives on windows
    if (deslashified.length() > 1 && deslashified.at(1) == QLatin1Char(':') && deslashified.at(0) != QLatin1Char('/')) {
        deslashified.prepend(QLatin1Char('/'));
    } else if (deslashified.startsWith(QLatin1String("//"))) {
        // magic for shared drive on windows
        int indexOfPath = deslashified.indexOf(QLatin1Char('/'), 2);
        url.setHost(deslashified.mid(2, indexOfPath - 2));
        if (indexOfPath > 2)
            deslashified = deslashified.right(deslashified.length() - indexOfPath);
        else
            deslashified.clear();
    }

    url.setPath(deslashified, DecodedMode);
    return url;
}

/*!
    Returns the path of this URL formatted as a local file path. The path
    returned will use forward slashes, even if it was originally created
    from one with backslashes.

    If this URL contains a non-empty hostname, it will be encoded in the
    returned value in the form found on SMB networks (for example,
    "//servername/path/to/file.txt").

    Note: if the path component of this URL contains a non-UTF-8 binary
    sequence (such as %80), the behaviour of this function is undefined.

    \sa fromLocalFile(), isLocalFile()
*/
QString QUrl::toLocalFile() const
{
    // the call to isLocalFile() also ensures that we're parsed
    if (!isLocalFile())
        return QString();

    QString tmp;
    QString ourPath = path(QUrl::FullyDecoded);

    // magic for shared drive on windows
    if (!d->host.isEmpty()) {
        tmp = QStringLiteral("//") + host() + (ourPath.length() > 0 && ourPath.at(0) != QLatin1Char('/')
                                               ? QLatin1Char('/') + ourPath :  ourPath);
    } else {
        tmp = ourPath;
#ifdef Q_OS_WIN
        // magic for drives on windows
        if (ourPath.length() > 2 && ourPath.at(0) == QLatin1Char('/') && ourPath.at(2) == QLatin1Char(':'))
            tmp.remove(0, 1);
#endif
    }
    return tmp;
}

/*!
    \since 4.8
    Returns \c true if this URL is pointing to a local file path. A URL is a
    local file path if the scheme is "file".

    Note that this function considers URLs with hostnames to be local file
    paths, even if the eventual file path cannot be opened with
    QFile::open().

    \sa fromLocalFile(), toLocalFile()
*/
bool QUrl::isLocalFile() const
{
    return d && d->isLocalFile();
}

/*!
    Returns \c true if this URL is a parent of \a childUrl. \a childUrl is a child
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

    QString ourPath = path();

    return ((childUrl.scheme().isEmpty() || d->scheme == childUrl.scheme())
            && (childUrl.authority().isEmpty() || authority() == childUrl.authority())
            &&  childPath.startsWith(ourPath)
            && ((ourPath.endsWith(QLatin1Char('/')) && childPath.length() > ourPath.length())
                || (!ourPath.endsWith(QLatin1Char('/'))
                    && childPath.length() > ourPath.length() && childPath.at(ourPath.length()) == QLatin1Char('/'))));
}


#ifndef QT_NO_DATASTREAM
/*! \relates QUrl

    Writes url \a url to the stream \a out and returns a reference
    to the stream.

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/
QDataStream &operator<<(QDataStream &out, const QUrl &url)
{
    QByteArray u;
    if (url.isValid())
        u = url.toEncoded();
    out << u;
    return out;
}

/*! \relates QUrl

    Reads a url into \a url from the stream \a in and returns a
    reference to the stream.

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/
QDataStream &operator>>(QDataStream &in, QUrl &url)
{
    QByteArray u;
    in >> u;
    url.setUrl(QString::fromLatin1(u));
    return in;
}
#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const QUrl &url)
{
    d.maybeSpace() << "QUrl(" << url.toDisplayString() << ')';
    return d.space();
}
#endif

static QString errorMessage(QUrlPrivate::ErrorCode errorCode, const QString &errorSource, int errorPosition)
{
    QChar c = uint(errorPosition) < uint(errorSource.length()) ?
                errorSource.at(errorPosition) : QChar(QChar::Null);

    switch (errorCode) {
    case QUrlPrivate::NoError:
        Q_ASSERT_X(false, "QUrl::errorString",
                   "Impossible: QUrl::errorString should have treated this condition");
        Q_UNREACHABLE();
        return QString();

    case QUrlPrivate::InvalidSchemeError: {
        QString msg = QStringLiteral("Invalid scheme (character '%1' not permitted)");
        return msg.arg(c);
    }

    case QUrlPrivate::InvalidUserNameError:
        return QString(QStringLiteral("Invalid user name (character '%1' not permitted)"))
                .arg(c);

    case QUrlPrivate::InvalidPasswordError:
        return QString(QStringLiteral("Invalid password (character '%1' not permitted)"))
                .arg(c);

    case QUrlPrivate::InvalidRegNameError:
        if (errorPosition != -1)
            return QString(QStringLiteral("Invalid hostname (character '%1' not permitted)"))
                    .arg(c);
        else
            return QStringLiteral("Invalid hostname (contains invalid characters)");
    case QUrlPrivate::InvalidIPv4AddressError:
        return QString(); // doesn't happen yet
    case QUrlPrivate::InvalidIPv6AddressError:
        return QStringLiteral("Invalid IPv6 address");
    case QUrlPrivate::InvalidCharacterInIPv6Error:
        return QStringLiteral("Invalid IPv6 address (character '%1' not permitted)").arg(c);
    case QUrlPrivate::InvalidIPvFutureError:
        return QStringLiteral("Invalid IPvFuture address (character '%1' not permitted)").arg(c);
    case QUrlPrivate::HostMissingEndBracket:
        return QStringLiteral("Expected ']' to match '[' in hostname");

    case QUrlPrivate::InvalidPortError:
        return QStringLiteral("Invalid port or port number out of range");
    case QUrlPrivate::PortEmptyError:
        return QStringLiteral("Port field was empty");

    case QUrlPrivate::InvalidPathError:
        return QString(QStringLiteral("Invalid path (character '%1' not permitted)"))
                .arg(c);

    case QUrlPrivate::InvalidQueryError:
        return QString(QStringLiteral("Invalid query (character '%1' not permitted)"))
                .arg(c);

    case QUrlPrivate::InvalidFragmentError:
        return QString(QStringLiteral("Invalid fragment (character '%1' not permitted)"))
                .arg(c);

    case QUrlPrivate::AuthorityPresentAndPathIsRelative:
        return QStringLiteral("Path component is relative and authority is present");
    case QUrlPrivate::RelativeUrlPathContainsColonBeforeSlash:
        return QStringLiteral("Relative URL's path component contains ':' before any '/'");
    }

    Q_ASSERT_X(false, "QUrl::errorString", "Cannot happen, unknown error");
    Q_UNREACHABLE();
    return QString();
}

static inline void appendComponentIfPresent(QString &msg, bool present, const char *componentName,
                                            const QString &component)
{
    if (present) {
        msg += QLatin1String(componentName);
        msg += QLatin1Char('"');
        msg += component;
        msg += QLatin1String("\",");
    }
}

/*!
    \since 4.2

    Returns an error message if the last operation that modified this QUrl
    object ran into a parsing error. If no error was detected, this function
    returns an empty string and isValid() returns \c true.

    The error message returned by this function is technical in nature and may
    not be understood by end users. It is mostly useful to developers trying to
    understand why QUrl will not accept some input.

    \sa QUrl::ParsingMode
*/
QString QUrl::errorString() const
{
    if (!d)
        return QString();

    QString errorSource;
    int errorPosition = 0;
    QUrlPrivate::ErrorCode errorCode = d->validityError(&errorSource, &errorPosition);
    if (errorCode == QUrlPrivate::NoError)
        return QString();

    QString msg = errorMessage(errorCode, errorSource, errorPosition);
    msg += QLatin1String("; source was \"");
    msg += errorSource;
    msg += QLatin1String("\";");
    appendComponentIfPresent(msg, d->sectionIsPresent & QUrlPrivate::Scheme,
                             " scheme = ", d->scheme);
    appendComponentIfPresent(msg, d->sectionIsPresent & QUrlPrivate::UserInfo,
                             " userinfo = ", userInfo());
    appendComponentIfPresent(msg, d->sectionIsPresent & QUrlPrivate::Host,
                             " host = ", d->host);
    appendComponentIfPresent(msg, d->port != -1,
                             " port = ", QString::number(d->port));
    appendComponentIfPresent(msg, !d->path.isEmpty(),
                             " path = ", d->path);
    appendComponentIfPresent(msg, d->sectionIsPresent & QUrlPrivate::Query,
                             " query = ", d->query);
    appendComponentIfPresent(msg, d->sectionIsPresent & QUrlPrivate::Fragment,
                             " fragment = ", d->fragment);
    if (msg.endsWith(QLatin1Char(',')))
        msg.chop(1);
    return msg;
}

/*!
    \since 5.1

    Converts a list of \a urls into a list of QStrings, using toString(\a options).
*/
QStringList QUrl::toStringList(const QList<QUrl> &urls, FormattingOptions options)
{
    QStringList lst;
    lst.reserve(urls.size());
    foreach (const QUrl &url, urls)
        lst.append(url.toString(options));
    return lst;

}

/*!
    \since 5.1

    Converts a list of strings representing \a urls into a list of urls, using QUrl(str, \a mode).
    Note that this means all strings must be urls, not for instance local paths.
*/
QList<QUrl> QUrl::fromStringList(const QStringList &urls, ParsingMode mode)
{
    QList<QUrl> lst;
    lst.reserve(urls.size());
    foreach (const QString &str, urls) {
        lst.append(QUrl(str, mode));
    }
    return lst;
}

/*!
    \typedef QUrl::DataPtr
    \internal
*/

/*!
    \fn DataPtr &QUrl::data_ptr()
    \internal
*/

/*!
    Returns the hash value for the \a url. If specified, \a seed is used to
    initialize the hash.

    \relates QHash
    \since 5.0
*/
uint qHash(const QUrl &url, uint seed) Q_DECL_NOTHROW
{
    if (!url.d)
        return qHash(-1, seed); // the hash of an unset port (-1)

    return qHash(url.d->scheme) ^
            qHash(url.d->userName) ^
            qHash(url.d->password) ^
            qHash(url.d->host) ^
            qHash(url.d->port, seed) ^
            qHash(url.d->path) ^
            qHash(url.d->query) ^
            qHash(url.d->fragment);
}

static QUrl adjustFtpPath(QUrl url)
{
    if (url.scheme() == ftpScheme()) {
        QString path = url.path(QUrl::PrettyDecoded);
        if (path.startsWith(QLatin1String("//")))
            url.setPath(QLatin1String("/%2F") + path.midRef(2), QUrl::TolerantMode);
    }
    return url;
}


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
    \li qt-project.org becomes http://qt-project.org
    \li ftp.qt-project.org becomes ftp://ftp.qt-project.org
    \li hostname becomes http://hostname
    \li /home/user/test.html becomes file:///home/user/test.html
    \endlist
*/
QUrl QUrl::fromUserInput(const QString &userInput)
{
    QString trimmedString = userInput.trimmed();

    // Check first for files, since on Windows drive letters can be interpretted as schemes
    if (QDir::isAbsolutePath(trimmedString))
        return QUrl::fromLocalFile(trimmedString);

    QUrl url = QUrl(trimmedString, QUrl::TolerantMode);
    QUrl urlPrepended = QUrl(QStringLiteral("http://") + trimmedString, QUrl::TolerantMode);

    // Check the most common case of a valid url with a scheme
    // We check if the port would be valid by adding the scheme to handle the case host:port
    // where the host would be interpretted as the scheme
    if (url.isValid()
        && !url.scheme().isEmpty()
        && urlPrepended.port() == -1)
        return adjustFtpPath(url);

    // Else, try the prepended one and adjust the scheme from the host name
    if (urlPrepended.isValid() && (!urlPrepended.host().isEmpty() || !urlPrepended.path().isEmpty()))
    {
        int dotIndex = trimmedString.indexOf(QLatin1Char('.'));
        const QString hostscheme = trimmedString.left(dotIndex).toLower();
        if (hostscheme == ftpScheme())
            urlPrepended.setScheme(ftpScheme());
        return adjustFtpPath(urlPrepended);
    }

    return QUrl();
}
// end of BSD code

QT_END_NAMESPACE
