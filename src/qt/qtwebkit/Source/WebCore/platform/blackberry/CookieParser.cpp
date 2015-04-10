/*
 * Copyright (C) 2009 Julien Chaffraix <jchaffraix@pleyo.com>
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */ 

#include "config.h"
#include "CookieParser.h"

#include "Logging.h"
#include "ParsedCookie.h"
#include <network/DomainTools.h>
#include <wtf/CurrentTime.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

#define LOG_ERROR_AND_RETURN(format, ...) \
    { \
        LOG_ERROR(format, ## __VA_ARGS__); \
        return 0; \
    }

static inline bool isCookieHeaderSeparator(UChar c)
{
    return (c == '\r' || c =='\n');
}

static inline bool isLightweightSpace(UChar c)
{
    return (c == ' ' || c == '\t');
}

CookieParser::CookieParser(const KURL& defaultCookieURL)
    : m_defaultCookieURL(defaultCookieURL)
{
    m_defaultCookieHost = defaultCookieURL.host();
    m_defaultDomainIsIPAddress = false;
    BlackBerry::Platform::String hostDomainCanonical = BlackBerry::Platform::getCanonicalIPFormat(m_defaultCookieHost);
    if (!hostDomainCanonical.empty()) {
        m_defaultCookieHost = hostDomainCanonical;
        m_defaultDomainIsIPAddress = true;
    }
}

CookieParser::~CookieParser()
{
}

Vector<RefPtr<ParsedCookie> > CookieParser::parse(const String& cookies)
{
    unsigned cookieStart, cookieEnd = 0;
    double curTime = currentTime();
    Vector<RefPtr<ParsedCookie>, 4> parsedCookies;

    unsigned cookiesLength = cookies.length();
    if (!cookiesLength) // Code below doesn't handle this case
        return parsedCookies;

    // Iterate over the header to parse all the cookies.
    while (cookieEnd <= cookiesLength) {
        cookieStart = cookieEnd;
        
        // Find a cookie separator.
        while (cookieEnd <= cookiesLength && !isCookieHeaderSeparator(cookies[cookieEnd]))
            cookieEnd++;

        // Detect an empty cookie and go to the next one.
        if (cookieStart == cookieEnd) {
            ++cookieEnd;
            continue;
        }

        if (cookieEnd < cookiesLength && isCookieHeaderSeparator(cookies[cookieEnd]))
            ++cookieEnd;

        RefPtr<ParsedCookie> cookie = parseOneCookie(cookies, cookieStart, cookieEnd - 1, curTime);
        if (cookie)
            parsedCookies.append(cookie);
    }
    return parsedCookies;
}

PassRefPtr<ParsedCookie> CookieParser::parseOneCookie(const String& cookie)
{
    return parseOneCookie(cookie, 0, cookie.length() - 1, currentTime());
}

// The cookie String passed into this method will only contian the name value pairs as well as other related cookie
// attributes such as max-age and domain. Set-Cookie should never be part of this string.
PassRefPtr<ParsedCookie> CookieParser::parseOneCookie(const String& cookie, unsigned start, unsigned end, double curTime)
{
    RefPtr<ParsedCookie> res = ParsedCookie::create(curTime);

    if (!res)
        LOG_ERROR_AND_RETURN("Out of memory");

    res->setProtocol(m_defaultCookieURL.protocol());

    // Parse [NAME "="] VALUE
    unsigned tokenEnd = start; // Token end contains the position of the '=' or the end of a token
    unsigned pairEnd = start; // Pair end contains always the position of the ';'

    // Find the first ';' which is not double-quoted and the '=' (if they exist).
    bool foundEqual = false;
    while (pairEnd < end && cookie[pairEnd] != ';') {
        if (cookie[pairEnd] == '=') {
            if (tokenEnd == start) {
                tokenEnd = pairEnd;
                foundEqual = true;
            }
        } else if (cookie[pairEnd] == '"') {
            size_t secondQuotePosition = cookie.find('"', pairEnd + 1);
            if (secondQuotePosition != notFound && secondQuotePosition <= end) {
                pairEnd = secondQuotePosition + 1;
                continue;
            }
        }
        pairEnd++;
    }

    unsigned tokenStart = start;

    bool hasName = false; // This is a hack to avoid changing too much in this brutally brittle code.
    if (tokenEnd != start) {
        // There is a '=' so parse the NAME
        unsigned nameEnd = tokenEnd;

        // The tokenEnd is the position of the '=' so the nameEnd is one less
        nameEnd--;

        // Remove lightweight spaces.
        while (nameEnd && isLightweightSpace(cookie[nameEnd]))
            nameEnd--;

        while (tokenStart < nameEnd && isLightweightSpace(cookie[tokenStart]))
            tokenStart++;

        if (nameEnd + 1 <= tokenStart)
            LOG_ERROR_AND_RETURN("Empty name. Rejecting the cookie");

        String name = cookie.substring(tokenStart, nameEnd + 1 - start);
        res->setName(name);
        hasName = true;
    }

    // Now parse the VALUE
    tokenStart = tokenEnd + 1;
    if (!hasName)
        --tokenStart;

    // Skip lightweight spaces in our token
    while (tokenStart < pairEnd && isLightweightSpace(cookie[tokenStart]))
        tokenStart++;

    tokenEnd = pairEnd;
    while (tokenEnd > tokenStart && isLightweightSpace(cookie[tokenEnd - 1]))
        tokenEnd--;

    String value;
    if (tokenEnd == tokenStart) {
        // Firefox accepts empty value so we will do the same
        value = String();
    } else
        value = cookie.substring(tokenStart, tokenEnd - tokenStart);

    if (hasName)
        res->setValue(value);
    else if (foundEqual)
        return 0;
    else
        res->setName(value); // No NAME=VALUE, only NAME

    while (pairEnd < end) {
        // Switch to the next pair as pairEnd is on the ';' and fast-forward any lightweight spaces.
        pairEnd++;
        while (pairEnd < end && isLightweightSpace(cookie[pairEnd]))
            pairEnd++;

        tokenStart = pairEnd;
        tokenEnd = tokenStart; // initialize token end to catch first '='

        while (pairEnd < end && cookie[pairEnd] != ';') {
            if (tokenEnd == tokenStart && cookie[pairEnd] == '=')
                tokenEnd = pairEnd;
            pairEnd++;
        }

        // FIXME : should we skip lightweight spaces here ?

        unsigned length = tokenEnd - tokenStart;
        unsigned tokenStartSvg = tokenStart;

        String parsedValue;
        if (tokenStart != tokenEnd) {
            // There is an equal sign so remove lightweight spaces in VALUE
            tokenStart = tokenEnd + 1;
            while (tokenStart < pairEnd && isLightweightSpace(cookie[tokenStart]))
                tokenStart++;

            tokenEnd = pairEnd;
            while (tokenEnd > tokenStart && isLightweightSpace(cookie[tokenEnd - 1]))
                tokenEnd--;

            parsedValue = cookie.substring(tokenStart, tokenEnd - tokenStart);
        } else {
            // If the parsedValue is empty, initialise it in case we need it
            parsedValue = String();
            // Handle a token without value.
            length = pairEnd - tokenStart;
        }

        // Detect which "cookie-av" is parsed
        // Look at the first char then parse the whole for performance issue
        switch (cookie[tokenStartSvg]) {
        case 'P':
        case 'p' : {
            if (length >= 4 && ((cookie.find("ath", tokenStartSvg + 1, false) - tokenStartSvg) == 1)) {
                // We need the path to be decoded to match those returned from KURL::path().
                // The path attribute may or may not include percent-encoded characters. Fortunately
                // if there are no percent-encoded characters, decoding the url is a no-op.
                res->setPath(decodeURLEscapeSequences(parsedValue));

                // We have to disable the following check because sites like Facebook and
                // Gmail currently do not follow the spec.
#if 0
                // Check if path attribute is a prefix of the request URI.
                if (!m_defaultCookieURL.path().startsWith(res->path()))
                    LOG_ERROR_AND_RETURN("Invalid cookie attribute %s (path): it does not math the URL", cookie.ascii().data());
#endif
            } else
                LOG_ERROR("Invalid cookie attribute %s (path)", cookie.ascii().data());
            break;
        }

        case 'D':
        case 'd' : {
            if (length >= 6 && ((cookie.find("omain", tokenStartSvg + 1, false) - tokenStartSvg) == 1)) {
                if (parsedValue.length() > 1 && parsedValue[0] == '"' && parsedValue[parsedValue.length() - 1] == '"')
                    parsedValue = parsedValue.substring(1, parsedValue.length() - 2);

                // Check if the domain contains an embedded dot.
                size_t dotPosition = parsedValue.find(".", 1);
                if (dotPosition == notFound || dotPosition == parsedValue.length())
                    LOG_ERROR_AND_RETURN("Invalid cookie attribute %s (domain): it does not contain an embedded dot", cookie.ascii().data());

                // If the domain does not start with a dot, add one for security checks and to distinguish it from host-only domains
                // For example: ab.c.com dose not domain match b.c.com;
                StringBuilder parsedValueBuilder;
                if (parsedValue[0] != '.')
                    parsedValueBuilder.appendLiteral(".");
                parsedValueBuilder.append(parsedValue);
                String realDomain = parsedValueBuilder.toString();

                StringBuilder defaultHostBuilder;
                defaultHostBuilder.appendLiteral(".");
                defaultHostBuilder.append(m_defaultCookieHost);
                String defaultHost = defaultHostBuilder.toString();

                // Try to return an canonical ip address if the domain is an ip

                bool isIPAddress = false;
                // We only check if the current domain is an IP address when the default domain is an IP address
                // We know if the default domain is not an IP address and the current domain is, it won't suffix match
                // If it is an IP Address, we should treat it only if it matches the host exactly
                // We determine the canonical IP format before comparing because IPv6 could be represented in multiple formats
                if (m_defaultDomainIsIPAddress) {
                    String realDomainCanonical = BlackBerry::Platform::getCanonicalIPFormat(realDomain);
                    if (realDomainCanonical.isEmpty() || realDomainCanonical != defaultHost)
                        LOG_ERROR_AND_RETURN("Invalid cookie attribute %s (domain): domain is IP but does not match host's IP", cookie.ascii().data());
                    realDomain = realDomainCanonical;
                    isIPAddress = true;
                } else {
                    // The request host should domain match the Domain attribute.
                    // Domain string starts with a dot, so a.b.com should domain match .a.b.com.
                    // add a "." at beginning of host name, because it can handle many cases such as
                    // a.b.com matches b.com, a.b.com matches .B.com and a.b.com matches .A.b.Com
                    // and so on.
                    if (!defaultHost.endsWith(realDomain, false))
                        LOG_ERROR_AND_RETURN("Invalid cookie attribute %s (domain): it does not domain match the host", cookie.ascii().data());

                    // We should check for an embedded dot in the portion of string in the host not in the domain
                    // but to match firefox behaviour we do not.

                    // Check whether the domain is a top level domain, if it is throw it out
                    // http://publicsuffix.org/list/
                    if (BlackBerry::Platform::isTopLevelDomain(realDomain))
                        LOG_ERROR_AND_RETURN("Invalid cookie attribute %s (domain): it did not pass the top level domain check", cookie.ascii().data());
                }
                res->setDomain(realDomain, isIPAddress);
            } else
                LOG_ERROR("Invalid cookie attribute %s (domain)", cookie.ascii().data());
            break;
        }

        case 'E' :
        case 'e' : {
            if (length >= 7 && ((cookie.find("xpires", tokenStartSvg + 1, false) - tokenStartSvg) == 1))
                res->setExpiry(parsedValue);
            else
                LOG_ERROR("Invalid cookie attribute %s (expires)", cookie.ascii().data());
            break;
        }

        case 'M' :
        case 'm' : {
            if (length >= 7 && ((cookie.find("ax-age", tokenStartSvg + 1, false) - tokenStartSvg) == 1))
                res->setMaxAge(parsedValue);
            else
                LOG_ERROR("Invalid cookie attribute %s (max-age)", cookie.ascii().data());
            break;
        }

        case 'C' :
        case 'c' : {
            if (length >= 7 && ((cookie.find("omment", tokenStartSvg + 1, false) - tokenStartSvg) == 1))
                // We do not have room for the comment part (and so do Mozilla) so just log the comment.
                LOG(Network, "Comment %s for ParsedCookie : %s\n", parsedValue.ascii().data(), cookie.ascii().data());
            else
                LOG_ERROR("Invalid cookie attribute %s (comment)", cookie.ascii().data());
            break;
        }

        case 'V' :
        case 'v' : {
            if (length >= 7 && ((cookie.find("ersion", tokenStartSvg + 1, false) - tokenStartSvg) == 1)) {
                // Although the out-of-dated Cookie Spec(RFC2965, http://tools.ietf.org/html/rfc2965) defined
                // the value of version can only contain DIGIT, some random sites, e.g. https://devforums.apple.com
                // would use double quotation marks to quote the digit. So we need to get rid of them for compliance.
                if (parsedValue.length() > 1 && parsedValue[0] == '"' && parsedValue[parsedValue.length() - 1] == '"')
                    parsedValue = parsedValue.substring(1, parsedValue.length() - 2);

                if (parsedValue.toInt() != 1)
                    LOG_ERROR_AND_RETURN("ParsedCookie version %d not supported (only support version=1)", parsedValue.toInt());
            } else
                LOG_ERROR("Invalid cookie attribute %s (version)", cookie.ascii().data());
            break;
        }

        case 'S' :
        case 's' : {
            // Secure is a standalone token ("Secure;")
            if (length >= 6 && ((cookie.find("ecure", tokenStartSvg + 1, false) - tokenStartSvg) == 1))
                res->setSecureFlag(true);
            else
                LOG_ERROR("Invalid cookie attribute %s (secure)", cookie.ascii().data());
            break;
        }
        case 'H':
        case 'h': {
            // HttpOnly is a standalone token ("HttpOnly;")
            if (length >= 8 && ((cookie.find("ttpOnly", tokenStartSvg + 1, false) - tokenStartSvg) == 1))
                res->setIsHttpOnly(true);
            else
                LOG_ERROR("Invalid cookie attribute %s (HttpOnly)", cookie.ascii().data());
            break;
        }

        default : {
            // If length == 0, we should be at the end of the cookie (case : ";\r") so ignore it
            if (length)
                LOG_ERROR("Invalid token for cookie %s", cookie.ascii().data());
        }
        }
    }

    // Check if the cookie is valid with respect to the size limit.
    if (!res->isUnderSizeLimit())
        LOG_ERROR_AND_RETURN("ParsedCookie %s is above the 4kb in length : REJECTED", cookie.ascii().data());

    // If some pair was not provided, during parsing then apply some default value
    // the rest has been done in the constructor.

    // If no domain was provided, set it to the host
    if (!res->domain())
        res->setDomain(m_defaultCookieHost, m_defaultDomainIsIPAddress);

    // According to the Cookie Specificaiton (RFC6265, section 4.1.2.4 and 5.2.4, http://tools.ietf.org/html/rfc6265),
    // If no path was provided or the first character of the path value is not '/', set it to the host's path
    //
    // REFERENCE
    // 4.1.2.4. The Path Attribute
    //
    // The scope of each cookie is limited to a set of paths, controlled by
    // the Path attribute. If the server omits the Path attribute, the user
    // agent will use the "directory" of the request-uri's path component as
    // the default value. (See Section 5.1.4 for more details.)
    // ...........
    // 5.2.4. The Path Attribute
    //
    // If the attribute-name case-insensitively matches the string "Path",
    // the user agent MUST process the cookie-av as follows.
    //
    // If the attribute-value is empty or if the first character of the
    // attribute-value is not %x2F ("/"):
    //
    // Let cookie-path be the default-path.
    //
    // Otherwise:
    //
    // Let cookie-path be the attribute-value.
    //
    // Append an attribute to the cookie-attribute-list with an attribute-
    // name of Path and an attribute-value of cookie-path.
    if (!res->path() || !res->path().length() || !res->path().startsWith("/", false)) {
        String path = m_defaultCookieURL.string().substring(m_defaultCookieURL.pathStart(), m_defaultCookieURL.pathAfterLastSlash() - m_defaultCookieURL.pathStart() - 1);
        if (path.isEmpty())
            path = "/";
        // Since this is reading the raw url string, it could contain percent-encoded sequences. We
        // want it to be comparable to the return value of url.path(), which is not percent-encoded,
        // so we must remove the escape sequences.
        res->setPath(decodeURLEscapeSequences(path));
    }

    return res;
}

} // namespace WebCore
