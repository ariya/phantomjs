/*
 * Copyright (C) 2004, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009, 2011 Google Inc. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(GOOGLEURL)
#include "KURL.h"

#ifndef NDEBUG
#include <stdio.h>
#endif

#include <algorithm>

#include "NotImplemented.h"
#include "TextEncoding.h"
#include <wtf/HashMap.h>
#include <wtf/Vector.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>

#include <googleurl/src/url_util.h>

using WTF::isASCIILower;
using WTF::toASCIILower;
using std::binary_search;

namespace WebCore {

static const int maximumValidPortNumber = 0xFFFE;
static const int invalidPortNumber = 0xFFFF;

// Wraps WebCore's text encoding in a character set converter for the
// canonicalizer.
class KURLCharsetConverter : public url_canon::CharsetConverter {
public:
    // The encoding parameter may be 0, but in this case the object must not be called.
    KURLCharsetConverter(const TextEncoding* encoding)
        : m_encoding(encoding)
    {
    }

    virtual void ConvertFromUTF16(const url_parse::UTF16Char* input, int inputLength,
                                  url_canon::CanonOutput* output)
    {
        CString encoded = m_encoding->encode(input, inputLength, URLEncodedEntitiesForUnencodables);
        output->Append(encoded.data(), static_cast<int>(encoded.length()));
    }

private:
    const TextEncoding* m_encoding;
};

// Note that this function must be named differently than the one in KURL.cpp
// since our unit tests evilly include both files, and their local definition
// will be ambiguous.
static inline void assertProtocolIsGood(const char* protocol)
{
#ifndef NDEBUG
    const char* p = protocol;
    while (*p) {
        ASSERT(*p > ' ' && *p < 0x7F && !(*p >= 'A' && *p <= 'Z'));
        ++p;
    }
#endif
}

// Returns the characters for the given string, or a pointer to a static empty
// string if the input string is null. This will always ensure we have a non-
// null character pointer since ReplaceComponents has special meaning for null.
static inline const url_parse::UTF16Char* CharactersOrEmpty(const String& str)
{
    static const url_parse::UTF16Char zero = 0;
    return str.characters() ?
           reinterpret_cast<const url_parse::UTF16Char*>(str.characters()) :
           &zero;
}

static inline bool isUnicodeEncoding(const TextEncoding* encoding)
{
    return encoding->encodingForFormSubmission() == UTF8Encoding();
}

static bool lowerCaseEqualsASCII(const char* begin, const char* end, const char* str)
{
    while (begin != end && *str) {
        ASSERT(toASCIILower(*str) == *str);
        if (toASCIILower(*begin++) != *str++)
            return false;
    }

    // Both strings are equal (ignoring case) if and only if all of the characters were equal,
    // and the end of both has been reached.
    return begin == end && !*str;
}

static inline bool isSchemeFirstChar(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline bool isSchemeChar(char c)
{
    return isSchemeFirstChar(c) || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '*';
}

bool isValidProtocol(const String& protocol)
{
    // NOTE This is a copy of the function in KURL.cpp.
    // RFC3986: ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
    if (protocol.isEmpty())
        return false;
    if (!isSchemeFirstChar(protocol[0]))
        return false;
    unsigned protocolLength = protocol.length();
    for (unsigned i = 1; i < protocolLength; i++) {
        if (!isSchemeChar(protocol[i]))
            return false;
    }
    return true;
}


// KURLGooglePrivate -----------------------------------------------------------

KURLGooglePrivate::KURLGooglePrivate()
    : m_isValid(false)
    , m_protocolIsInHTTPFamily(false)
    , m_utf8IsASCII(true)
    , m_stringIsValid(false)
{
}

KURLGooglePrivate::KURLGooglePrivate(const url_parse::Parsed& parsed, bool isValid)
    : m_isValid(isValid)
    , m_protocolIsInHTTPFamily(false)
    , m_parsed(parsed)
    , m_utf8IsASCII(true)
    , m_stringIsValid(false)
{
}

KURLGooglePrivate::KURLGooglePrivate(WTF::HashTableDeletedValueType)
    : m_string(WTF::HashTableDeletedValue)
{
}
 
// Setters for the data. Using the ASCII version when you know the
// data is ASCII will be slightly more efficient. The UTF-8 version
// will always be correct if the caller is unsure.
void KURLGooglePrivate::setUtf8(const CString& str)
{
    const char* data = str.data();
    unsigned dataLength = str.length();

    // The m_utf8IsASCII must always be correct since the DeprecatedString
    // getter must create it with the proper constructor. This test can be
    // removed when DeprecatedString is gone, but it still might be a
    // performance win.
    m_utf8IsASCII = true;
    for (unsigned i = 0; i < dataLength; i++) {
        if (static_cast<unsigned char>(data[i]) >= 0x80) {
            m_utf8IsASCII = false;
            break;
        }
    }

    m_utf8 = str;
    m_stringIsValid = false;
    initProtocolIsInHTTPFamily();
}

void KURLGooglePrivate::setAscii(const CString& str)
{
    m_utf8 = str;
    m_utf8IsASCII = true;
    m_stringIsValid = false;
    initProtocolIsInHTTPFamily();
}

void KURLGooglePrivate::init(const KURL& base,
                             const String& relative,
                             const TextEncoding* queryEncoding)
{
    init(base, relative.characters(), relative.length(), queryEncoding);
}

template <typename CHAR>
void KURLGooglePrivate::init(const KURL& base, const CHAR* rel, int relLength,
                             const TextEncoding* queryEncoding)
{
    // As a performance optimization, we do not use the charset converter
    // if encoding is UTF-8 or other Unicode encodings. Note that this is
    // per HTML5 2.5.3 (resolving URL). The URL canonicalizer will be more
    // efficient with no charset converter object because it can do UTF-8
    // internally with no extra copies.

    // We feel free to make the charset converter object every time since it's
    // just a wrapper around a reference.
    KURLCharsetConverter charsetConverterObject(queryEncoding);
    KURLCharsetConverter* charsetConverter =
        (!queryEncoding || isUnicodeEncoding(queryEncoding)) ? 0 :
        &charsetConverterObject;

    url_canon::RawCanonOutputT<char> output;
    const CString& baseStr = base.m_url.utf8String();
    m_isValid = url_util::ResolveRelative(baseStr.data(), baseStr.length(),
                                          base.m_url.m_parsed, rel, relLength,
                                          charsetConverter,
                                          &output, &m_parsed);

    // See FIXME in KURLGooglePrivate in the header. If canonicalization has not
    // changed the string, we can avoid an extra allocation by using assignment.
    //
    // When KURL encounters an error such that the URL is invalid and empty
    // (for example, resolving a relative URL on a non-hierarchical base), it
    // will produce an isNull URL, and calling setUtf8 will produce an empty
    // non-null URL. This is unlikely to affect anything, but we preserve this
    // just in case.
    if (m_isValid || output.length()) {
        // Without ref, the whole url is guaranteed to be ASCII-only.
        if (m_parsed.ref.is_nonempty())
            setUtf8(CString(output.data(), output.length()));
        else
            setAscii(CString(output.data(), output.length()));
    } else {
        // WebCore expects resolved URLs to be empty rather than null.
        setUtf8(CString("", 0));
    }
}

void KURLGooglePrivate::initProtocolIsInHTTPFamily()
{
    if (!m_isValid) {
        m_protocolIsInHTTPFamily = false;
        return;
    }

    const char* scheme = m_utf8.data() + m_parsed.scheme.begin;
    if (m_parsed.scheme.len == 4)
        m_protocolIsInHTTPFamily = lowerCaseEqualsASCII(scheme, scheme + 4, "http");
    else if (m_parsed.scheme.len == 5)
        m_protocolIsInHTTPFamily = lowerCaseEqualsASCII(scheme, scheme + 5, "https");
    else
        m_protocolIsInHTTPFamily = false;
}

void KURLGooglePrivate::copyTo(KURLGooglePrivate* dest) const
{
    dest->m_isValid = m_isValid;
    dest->m_protocolIsInHTTPFamily = m_protocolIsInHTTPFamily;
    dest->m_parsed = m_parsed;

    // Don't copy the 16-bit string since that will be regenerated as needed.
    dest->m_utf8 = CString(m_utf8.data(), m_utf8.length());
    dest->m_utf8IsASCII = m_utf8IsASCII;
    dest->m_stringIsValid = false;
}

String KURLGooglePrivate::componentString(const url_parse::Component& comp) const
{
    if (!m_isValid || comp.len <= 0) {
        // KURL returns a null string if the URL is itself a null string, and an
        // empty string for other nonexistent entities.
        if (utf8String().isNull())
            return String();
        return String("", 0);
    }
    // begin and len are in terms of bytes which do not match
    // if string() is UTF-16 and input contains non-ASCII characters.
    // However, the only part in urlString that can contain non-ASCII
    // characters is 'ref' at the end of the string. In that case,
    // begin will always match the actual value and len (in terms of
    // byte) will be longer than what's needed by 'mid'. However, mid
    // truncates len to avoid go past the end of a string so that we can
    // get away withtout doing anything here.
    return string().substring(comp.begin, comp.len);
}

void KURLGooglePrivate::replaceComponents(const Replacements& replacements)
{
    url_canon::RawCanonOutputT<char> output;
    url_parse::Parsed newParsed;

    m_isValid = url_util::ReplaceComponents(utf8String().data(),
                                            utf8String().length(), m_parsed, replacements, 0, &output, &newParsed);

    m_parsed = newParsed;
    if (m_parsed.ref.is_nonempty())
        setUtf8(CString(output.data(), output.length()));
    else
        setAscii(CString(output.data(), output.length()));
}

const String& KURLGooglePrivate::string() const
{
    if (!m_stringIsValid) {
        // Handle the null case separately. Otherwise, constructing
        // the string like we do below would generate the empty string,
        // not the null string.
        if (m_utf8.isNull())
            m_string = String();
        else if (m_utf8IsASCII)
            m_string = String(m_utf8.data(), m_utf8.length());
        else
            m_string = String::fromUTF8(m_utf8.data(), m_utf8.length());
        m_stringIsValid = true;
    }
    return m_string;
}

// KURL ------------------------------------------------------------------------

// Creates with null-terminated string input representing an absolute URL.
// WebCore generally calls this only with hardcoded strings, so the input is
// ASCII. We treat it as UTF-8 just in case.
KURL::KURL(ParsedURLStringTag, const char *url)
{
    // FIXME The Mac code checks for beginning with a slash and converts it to
    // file: URL. We will want to add this as well once we can compile on a
    // system like that.
    m_url.init(KURL(), url, strlen(url), 0);

    // The one-argument constructors should never generate a null string.
    // This is a funny quirk of KURL.cpp (probably a bug) which we preserve.
    if (m_url.utf8String().isNull())
        m_url.setAscii(CString("", 0));
}

// Initializes with a string representing an absolute URL. No encoding
// information is specified. This generally happens when a KURL is converted
// to a string and then converted back. In this case, the URL is already
// canonical and in proper escaped form so needs no encoding. We treat it as
// UTF-8 just in case.
KURL::KURL(ParsedURLStringTag, const String& url)
{
    if (!url.isNull())
        m_url.init(KURL(), url, 0);
    else {
        // WebCore expects us to preserve the nullness of strings when this
        // constructor is used. In all other cases, it expects a non-null
        // empty string, which is what init() will create.
        m_url.m_isValid = false;
        m_url.m_protocolIsInHTTPFamily = false;
    }
}

// Constructs a new URL given a base URL and a possibly relative input URL.
// This assumes UTF-8 encoding.
KURL::KURL(const KURL& base, const String& relative)
{
    m_url.init(base, relative, 0);
}

// Constructs a new URL given a base URL and a possibly relative input URL.
// Any query portion of the relative URL will be encoded in the given encoding.
KURL::KURL(const KURL& base,
           const String& relative,
           const TextEncoding& encoding)
{
    m_url.init(base, relative, &encoding.encodingForFormSubmission());
}

KURL::KURL(const CString& canonicalSpec,
           const url_parse::Parsed& parsed, bool isValid)
    : m_url(parsed, isValid)
{
    // We know the reference fragment is the only part that can be UTF-8, so
    // we know it's ASCII when there is no ref.
    if (parsed.ref.is_nonempty())
        m_url.setUtf8(canonicalSpec);
    else
        m_url.setAscii(canonicalSpec);
}

#if USE(CF)
KURL::KURL(CFURLRef)
{
    notImplemented();
    invalidate();
}

CFURLRef KURL::createCFURL() const
{
    notImplemented();
    return 0;
}
#endif

KURL KURL::copy() const
{
    KURL result = *this;
    m_url.copyTo(&result.m_url);
    return result;
}

bool KURL::isNull() const
{
    return m_url.utf8String().isNull();
}

bool KURL::isEmpty() const
{
    return !m_url.utf8String().length();
}

bool KURL::isValid() const
{
    return m_url.m_isValid;
}

bool KURL::hasPort() const
{
    return hostEnd() < pathStart();
}

bool KURL::protocolIsInHTTPFamily() const
{
    return m_url.m_protocolIsInHTTPFamily;
}

bool KURL::hasPath() const
{
    // Note that http://www.google.com/" has a path, the path is "/". This can
    // return false only for invalid or nonstandard URLs.
    return m_url.m_parsed.path.len >= 0;
}

// We handle "parameters" separated by a semicolon, while KURL.cpp does not,
// which can lead to different results in some cases.
String KURL::lastPathComponent() const
{
    // When the output ends in a slash, WebCore has different expectations than
    // the GoogleURL library. For "/foo/bar/" the library will return the empty
    // string, but WebCore wants "bar".
    url_parse::Component path = m_url.m_parsed.path;
    if (path.len > 0 && m_url.utf8String().data()[path.end() - 1] == '/')
        path.len--;

    url_parse::Component file;
    url_parse::ExtractFileName(m_url.utf8String().data(), path, &file);

    // Bug: https://bugs.webkit.org/show_bug.cgi?id=21015 this function returns
    // a null string when the path is empty, which we duplicate here.
    if (!file.is_nonempty())
        return String();
    return m_url.componentString(file);
}

String KURL::protocol() const
{
    return m_url.componentString(m_url.m_parsed.scheme);
}

String KURL::host() const
{
    // Note: KURL.cpp unescapes here.
    return m_url.componentString(m_url.m_parsed.host);
}

// Returns 0 when there is no port.
//
// We treat URL's with out-of-range port numbers as invalid URLs, and they will
// be rejected by the canonicalizer. KURL.cpp will allow them in parsing, but
// return invalidPortNumber from this port() function, so we mirror that behavior here.
unsigned short KURL::port() const
{
    if (!m_url.m_isValid || m_url.m_parsed.port.len <= 0)
        return 0;
    int port = url_parse::ParsePort(m_url.utf8String().data(), m_url.m_parsed.port);
    ASSERT(port != url_parse::PORT_UNSPECIFIED); // Checked port.len <= 0 before.

    if (port == url_parse::PORT_INVALID || port > maximumValidPortNumber) // Mimic KURL::port()
        port = invalidPortNumber;

    return static_cast<unsigned short>(port);
}

// Returns the empty string if there is no password.
String KURL::pass() const
{
    // Bug: https://bugs.webkit.org/show_bug.cgi?id=21015 this function returns
    // a null string when the password is empty, which we duplicate here.
    if (!m_url.m_parsed.password.is_nonempty())
        return String();

    // Note: KURL.cpp unescapes here.
    return m_url.componentString(m_url.m_parsed.password);
}

// Returns the empty string if there is no username.
String KURL::user() const
{
    // Note: KURL.cpp unescapes here.
    return m_url.componentString(m_url.m_parsed.username);
}

String KURL::fragmentIdentifier() const
{
    // Empty but present refs ("foo.com/bar#") should result in the empty
    // string, which m_url.componentString will produce. Nonexistent refs
    // should be the null string.
    if (!m_url.m_parsed.ref.is_valid())
        return String();

    // Note: KURL.cpp unescapes here.
    return m_url.componentString(m_url.m_parsed.ref);
}

bool KURL::hasFragmentIdentifier() const
{
    // Note: KURL.cpp unescapes here.
    // FIXME determine if KURL.cpp agrees about an empty ref
    return m_url.m_parsed.ref.len >= 0;
}

void KURL::copyParsedQueryTo(ParsedURLParameters& parameters) const
{
    String query = m_url.componentString(m_url.m_parsed.query);
    const UChar* pos = query.characters();
    const UChar* end = query.characters() + query.length();
    while (pos < end) {
        const UChar* parameterStart = pos;
        while (pos < end && *pos != '&')
            ++pos;
        const UChar* parameterEnd = pos;
        if (pos < end) {
            ASSERT(*pos == '&');
            ++pos;
        }
        if (parameterStart == parameterEnd)
            continue;
        const UChar* nameStart = parameterStart;
        const UChar* equalSign = parameterStart;
        while (equalSign < parameterEnd && *equalSign != '=')
            ++equalSign;
        if (equalSign == nameStart)
            continue;
        String name(nameStart, equalSign - nameStart);
        String value = equalSign == parameterEnd ? String() : String(equalSign + 1, parameterEnd - equalSign - 1);
        parameters.set(name, value);
    }
}

String KURL::baseAsString() const
{
    // FIXME: There is probably a more efficient way to do this?
    return string().left(pathAfterLastSlash());
}

String KURL::query() const
{
    if (m_url.m_parsed.query.len >= 0)
        return m_url.componentString(m_url.m_parsed.query);

    // Bug: https://bugs.webkit.org/show_bug.cgi?id=21015 this function returns
    // an empty string when the query is empty rather than a null (not sure
    // which is right).
    // Returns a null if the query is not specified, instead of empty.
    if (m_url.m_parsed.query.is_valid())
        return String("", 0);
    return String();
}

String KURL::path() const
{
    // Note: KURL.cpp unescapes here.
    return m_url.componentString(m_url.m_parsed.path);
}

bool KURL::setProtocol(const String& protocol)
{
    // Firefox and IE remove everything after the first ':'.
    int separatorPosition = protocol.find(':');
    String newProtocol = protocol.substring(0, separatorPosition);

    // If KURL is given an invalid scheme, it returns failure without modifying
    // the URL at all. This is in contrast to most other setters which modify
    // the URL and set "m_isValid."
    url_canon::RawCanonOutputT<char> canonProtocol;
    url_parse::Component protocolComponent;
    if (!url_canon::CanonicalizeScheme(newProtocol.characters(),
                                       url_parse::Component(0, newProtocol.length()),
                                       &canonProtocol, &protocolComponent)
        || !protocolComponent.is_nonempty())
        return false;

    KURLGooglePrivate::Replacements replacements;
    replacements.SetScheme(CharactersOrEmpty(newProtocol),
                           url_parse::Component(0, newProtocol.length()));
    m_url.replaceComponents(replacements);

    // isValid could be false but we still return true here. This is because
    // WebCore or JS scripts can build up a URL by setting individual
    // components, and a JS exception is based on the return value of this
    // function. We want to throw the exception and stop the script only when
    // its trying to set a bad protocol, and not when it maybe just hasn't
    // finished building up its final scheme.
    return true;
}

void KURL::setHost(const String& host)
{
    KURLGooglePrivate::Replacements replacements;
    replacements.SetHost(CharactersOrEmpty(host),
                         url_parse::Component(0, host.length()));
    m_url.replaceComponents(replacements);
}

void KURL::setHostAndPort(const String& s)
{
    String host = s;
    String port;
    int hostEnd = s.find(":");
    if (hostEnd != -1) {
        host = s.left(hostEnd);
        port = s.substring(hostEnd + 1);
    }

    KURLGooglePrivate::Replacements replacements;
    // Host can't be removed, so we always set.
    replacements.SetHost(CharactersOrEmpty(host),
                         url_parse::Component(0, host.length()));

    if (port.isEmpty())  // Port may be removed, so we support clearing.
        replacements.ClearPort();
    else
        replacements.SetPort(CharactersOrEmpty(port), url_parse::Component(0, port.length()));
    m_url.replaceComponents(replacements);
}

void KURL::removePort()
{
    if (hasPort()) {
        String urlWithoutPort = m_url.string().left(hostEnd()) + m_url.string().substring(pathStart());
        m_url.setUtf8(urlWithoutPort.utf8());
    }
}

void KURL::setPort(unsigned short i)
{
    KURLGooglePrivate::Replacements replacements;
    String portStr;
    if (i) {
        portStr = String::number(i);
        replacements.SetPort(
            reinterpret_cast<const url_parse::UTF16Char*>(portStr.characters()),
            url_parse::Component(0, portStr.length()));

    } else {
        // Clear any existing port when it is set to 0.
        replacements.ClearPort();
    }
    m_url.replaceComponents(replacements);
}

void KURL::setUser(const String& user)
{
    // This function is commonly called to clear the username, which we
    // normally don't have, so we optimize this case.
    if (user.isEmpty() && !m_url.m_parsed.username.is_valid())
        return;

    // The canonicalizer will clear any usernames that are empty, so we
    // don't have to explicitly call ClearUsername() here.
    KURLGooglePrivate::Replacements replacements;
    replacements.SetUsername(CharactersOrEmpty(user),
                             url_parse::Component(0, user.length()));
    m_url.replaceComponents(replacements);
}

void KURL::setPass(const String& pass)
{
    // This function is commonly called to clear the password, which we
    // normally don't have, so we optimize this case.
    if (pass.isEmpty() && !m_url.m_parsed.password.is_valid())
        return;

    // The canonicalizer will clear any passwords that are empty, so we
    // don't have to explicitly call ClearUsername() here.
    KURLGooglePrivate::Replacements replacements;
    replacements.SetPassword(CharactersOrEmpty(pass),
                             url_parse::Component(0, pass.length()));
    m_url.replaceComponents(replacements);
}

void KURL::setFragmentIdentifier(const String& s)
{
    // This function is commonly called to clear the ref, which we
    // normally don't have, so we optimize this case.
    if (s.isNull() && !m_url.m_parsed.ref.is_valid())
        return;

    KURLGooglePrivate::Replacements replacements;
    if (s.isNull())
        replacements.ClearRef();
    else
        replacements.SetRef(CharactersOrEmpty(s), url_parse::Component(0, s.length()));
    m_url.replaceComponents(replacements);
}

void KURL::removeFragmentIdentifier()
{
    KURLGooglePrivate::Replacements replacements;
    replacements.ClearRef();
    m_url.replaceComponents(replacements);
}

void KURL::setQuery(const String& query)
{
    KURLGooglePrivate::Replacements replacements;
    if (query.isNull()) {
        // KURL.cpp sets to null to clear any query.
        replacements.ClearQuery();
    } else if (query.length() > 0 && query[0] == '?') {
        // WebCore expects the query string to begin with a question mark, but
        // GoogleURL doesn't. So we trim off the question mark when setting.
        replacements.SetQuery(CharactersOrEmpty(query),
                              url_parse::Component(1, query.length() - 1));
    } else {
        // When set with the empty string or something that doesn't begin with
        // a question mark, KURL.cpp will add a question mark for you. The only
        // way this isn't compatible is if you call this function with an empty
        // string. KURL.cpp will leave a '?' with nothing following it in the
        // URL, whereas we'll clear it.
        // FIXME We should eliminate this difference.
        replacements.SetQuery(CharactersOrEmpty(query),
                              url_parse::Component(0, query.length()));
    }
    m_url.replaceComponents(replacements);
}

void KURL::setPath(const String& path)
{
    // Empty paths will be canonicalized to "/", so we don't have to worry
    // about calling ClearPath().
    KURLGooglePrivate::Replacements replacements;
    replacements.SetPath(CharactersOrEmpty(path),
                         url_parse::Component(0, path.length()));
    m_url.replaceComponents(replacements);
}

// On Mac, this just seems to return the same URL, but with "/foo/bar" for
// file: URLs instead of file:///foo/bar. We don't bother with any of this,
// at least for now.
String KURL::prettyURL() const
{
    if (!m_url.m_isValid)
        return String();
    return m_url.string();
}

String decodeURLEscapeSequences(const String& str)
{
    return decodeURLEscapeSequences(str, UTF8Encoding());
}

// In KURL.cpp's implementation, this is called by every component getter.
// It will unescape every character, including '\0'. This is scary, and may
// cause security holes. We never call this function for components, and
// just return the ASCII versions instead.
//
// This function is also used to decode javascript: URLs and as a general
// purpose unescaping function.
//
// FIXME These should be merged to the KURL.cpp implementation.
String decodeURLEscapeSequences(const String& str, const TextEncoding& encoding)
{
    // FIXME We can probably use KURL.cpp's version of this function
    // without modification. However, I'm concerned about
    // https://bugs.webkit.org/show_bug.cgi?id=20559 so am keeping this old
    // custom code for now. Using their version will also fix the bug that
    // we ignore the encoding.
    //
    // FIXME b/1350291: This does not get called very often. We just convert
    // first to 8-bit UTF-8, then unescape, then back to 16-bit. This kind of
    // sucks, and we don't use the encoding properly, which will make some
    // obscure anchor navigations fail.
    CString cstr = str.utf8();

    const char* input = cstr.data();
    int inputLength = cstr.length();

    url_canon::RawCanonOutputT<url_parse::UTF16Char> unescaped;

    url_util::DecodeURLEscapeSequences(input, inputLength, &unescaped);

    return String(reinterpret_cast<UChar*>(unescaped.data()),
                  unescaped.length());
}

bool KURL::protocolIs(const char* protocol) const
{
    assertProtocolIsGood(protocol);

    // JavaScript URLs are "valid" and should be executed even if KURL decides they are invalid.
    // The free function protocolIsJavaScript() should be used instead.
    // FIXME: Chromium code needs to be fixed for this assert to be enabled. ASSERT(strcmp(protocol, "javascript"));

    if (m_url.m_parsed.scheme.len <= 0)
        return !protocol;
    return lowerCaseEqualsASCII(
        m_url.utf8String().data() + m_url.m_parsed.scheme.begin,
        m_url.utf8String().data() + m_url.m_parsed.scheme.end(),
        protocol);
}

// This is called to escape a URL string. It is only used externally when
// constructing mailto: links to set the query section. Since our query setter
// will automatically do the correct escaping, this function does not have to
// do any work.
//
// There is a possibility that a future caller may use this function in other
// ways, and may expect to get a valid URL string. The dangerous thing we want
// to protect against here is accidentally getting '\0' characters in a string
// that is not supposed to have them. Therefore, we escape these characters.
String encodeWithURLEscapeSequences(const String& notEncodedString)
{
    CString utf8 = UTF8Encoding().encode(
        reinterpret_cast<const UChar*>(notEncodedString.characters()),
        notEncodedString.length(),
        URLEncodedEntitiesForUnencodables);
    const char* input = utf8.data();
    int inputLength = utf8.length();

    Vector<char, 2048> buffer;
    for (int i = 0; i < inputLength; i++) {
        if (!input[i])
            buffer.append("%00", 3);
        else
            buffer.append(input[i]);
    }
    return String(buffer.data(), buffer.size());
}

bool KURL::isHierarchical() const
{
    if (!m_url.m_parsed.scheme.is_nonempty())
        return false;
    return url_util::IsStandard(
        &m_url.utf8String().data()[m_url.m_parsed.scheme.begin],
        m_url.m_parsed.scheme);
}

#ifndef NDEBUG
void KURL::print() const
{
    printf("%s\n", m_url.utf8String().data());
}
#endif

void KURL::invalidate()
{
    // This is only called from the constructor so resetting the (automatically
    // initialized) string and parsed structure would be a waste of time.
    m_url.m_isValid = false;
    m_url.m_protocolIsInHTTPFamily = false;
}

// Equal up to reference fragments, if any.
bool equalIgnoringFragmentIdentifier(const KURL& a, const KURL& b)
{
    // Compute the length of each URL without its ref. Note that the reference
    // begin (if it exists) points to the character *after* the '#', so we need
    // to subtract one.
    int aLength = a.m_url.utf8String().length();
    if (a.m_url.m_parsed.ref.len >= 0)
        aLength = a.m_url.m_parsed.ref.begin - 1;

    int bLength = b.m_url.utf8String().length();
    if (b.m_url.m_parsed.ref.len >= 0)
        bLength = b.m_url.m_parsed.ref.begin - 1;

    return aLength == bLength
        && !strncmp(a.m_url.utf8String().data(), b.m_url.utf8String().data(), aLength);
}

unsigned KURL::hostStart() const
{
    return m_url.m_parsed.CountCharactersBefore(url_parse::Parsed::HOST, false);
}

unsigned KURL::hostEnd() const
{
    return m_url.m_parsed.CountCharactersBefore(url_parse::Parsed::PORT, true);
}

unsigned KURL::pathStart() const
{
    return m_url.m_parsed.CountCharactersBefore(url_parse::Parsed::PATH, false);
}

unsigned KURL::pathEnd() const
{
    return m_url.m_parsed.CountCharactersBefore(url_parse::Parsed::QUERY, true);
}

unsigned KURL::pathAfterLastSlash() const
{
    // When there's no path, ask for what would be the beginning of it.
    if (!m_url.m_parsed.path.is_valid())
        return m_url.m_parsed.CountCharactersBefore(url_parse::Parsed::PATH, false);

    url_parse::Component filename;
    url_parse::ExtractFileName(m_url.utf8String().data(), m_url.m_parsed.path,
                               &filename);
    return filename.begin;
}

bool protocolIs(const String& url, const char* protocol)
{
    // Do the comparison without making a new string object.
    assertProtocolIsGood(protocol);

    // Check the scheme like GURL does.
    return url_util::FindAndCompareScheme(url.characters(), url.length(), 
        protocol, 0);
}

inline bool KURL::protocolIs(const String& string, const char* protocol)
{
    return WebCore::protocolIs(string, protocol);
}

bool protocolHostAndPortAreEqual(const KURL& a, const KURL& b)
{
    if (a.parsed().scheme.end() != b.parsed().scheme.end())
        return false;

    int hostStartA = a.hostStart();
    int hostLengthA = a.hostEnd() - hostStartA;
    int hostStartB = b.hostStart();
    int hostLengthB = b.hostEnd() - b.hostStart();
    if (hostLengthA != hostLengthB)
        return false;

    // Check the scheme
    for (int i = 0; i < a.parsed().scheme.end(); ++i)
        if (a.string()[i] != b.string()[i])
            return false;
    
    // And the host
    for (int i = 0; i < hostLengthA; ++i)
        if (a.string()[hostStartA + i] != b.string()[hostStartB + i])
            return false;
    
    if (a.port() != b.port())
        return false;

    return true;
}

} // namespace WebCore

#endif // USE(GOOGLEURL)
