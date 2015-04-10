/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SecurityOrigin.h"

#include "BlobURL.h"
#include "FileSystem.h"
#include "KURL.h"
#include "SchemeRegistry.h"
#include "SecurityPolicy.h"
#include "ThreadableBlobRegistry.h"
#include <wtf/MainThread.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

const int InvalidPort = 0;
const int MaxAllowedPort = 65535;

static bool schemeRequiresHost(const KURL& url)
{
    // We expect URLs with these schemes to have authority components. If the
    // URL lacks an authority component, we get concerned and mark the origin
    // as unique.
    return url.protocolIsInHTTPFamily() || url.protocolIs("ftp");
}

bool SecurityOrigin::shouldUseInnerURL(const KURL& url)
{
#if ENABLE(BLOB)
    // FIXME: Blob URLs don't have inner URLs. Their form is "blob:<inner-origin>/<UUID>", so treating the part after "blob:" as a URL is incorrect.
    if (url.protocolIs("blob"))
        return true;
#endif
#if ENABLE(FILE_SYSTEM)
    if (url.protocolIs("filesystem"))
        return true;
#endif
    UNUSED_PARAM(url);
    return false;
}

// In general, extracting the inner URL varies by scheme. It just so happens
// that all the URL schemes we currently support that use inner URLs for their
// security origin can be parsed using this algorithm.
KURL SecurityOrigin::extractInnerURL(const KURL& url)
{
    if (url.innerURL())
        return *url.innerURL();
    // FIXME: Update this callsite to use the innerURL member function when
    // we finish implementing it.
    return KURL(ParsedURLString, decodeURLEscapeSequences(url.path()));
}

static PassRefPtr<SecurityOrigin> getCachedOrigin(const KURL& url)
{
#if ENABLE(BLOB)
    if (url.protocolIs("blob"))
        return ThreadableBlobRegistry::getCachedOrigin(url);
#else
    UNUSED_PARAM(url);
#endif
    return 0;
}

static bool shouldTreatAsUniqueOrigin(const KURL& url)
{
    if (!url.isValid())
        return true;

    // FIXME: Do we need to unwrap the URL further?
    KURL innerURL = SecurityOrigin::shouldUseInnerURL(url) ? SecurityOrigin::extractInnerURL(url) : url;

    // FIXME: Check whether innerURL is valid.

    // For edge case URLs that were probably misparsed, make sure that the origin is unique.
    // This is an additional safety net against bugs in KURL parsing, and for network back-ends that parse URLs differently,
    // and could misinterpret another component for hostname.
    if (schemeRequiresHost(innerURL) && innerURL.host().isEmpty())
        return true;

    // SchemeRegistry needs a lower case protocol because it uses HashMaps
    // that assume the scheme has already been canonicalized.
    String protocol = innerURL.protocol().lower();

    if (SchemeRegistry::shouldTreatURLSchemeAsNoAccess(protocol))
        return true;

    // This is the common case.
    return false;
}

SecurityOrigin::SecurityOrigin(const KURL& url)
    : m_protocol(url.protocol().isNull() ? "" : url.protocol().lower())
    , m_host(url.host().isNull() ? "" : url.host().lower())
    , m_port(url.port())
    , m_isUnique(false)
    , m_universalAccess(false)
    , m_domainWasSetInDOM(false)
    , m_storageBlockingPolicy(AllowAllStorage)
    , m_enforceFilePathSeparation(false)
    , m_needsDatabaseIdentifierQuirkForFiles(false)
{
    // document.domain starts as m_host, but can be set by the DOM.
    m_domain = m_host;

    if (isDefaultPortForProtocol(m_port, m_protocol))
        m_port = InvalidPort;

    // By default, only local SecurityOrigins can load local resources.
    m_canLoadLocalResources = isLocal();

    if (m_canLoadLocalResources)
        m_filePath = url.path(); // In case enforceFilePathSeparation() is called.
}

SecurityOrigin::SecurityOrigin()
    : m_protocol("")
    , m_host("")
    , m_domain("")
    , m_port(InvalidPort)
    , m_isUnique(true)
    , m_universalAccess(false)
    , m_domainWasSetInDOM(false)
    , m_canLoadLocalResources(false)
    , m_storageBlockingPolicy(AllowAllStorage)
    , m_enforceFilePathSeparation(false)
    , m_needsDatabaseIdentifierQuirkForFiles(false)
{
}

SecurityOrigin::SecurityOrigin(const SecurityOrigin* other)
    : m_protocol(other->m_protocol.isolatedCopy())
    , m_host(other->m_host.isolatedCopy())
    , m_domain(other->m_domain.isolatedCopy())
    , m_filePath(other->m_filePath.isolatedCopy())
    , m_port(other->m_port)
    , m_isUnique(other->m_isUnique)
    , m_universalAccess(other->m_universalAccess)
    , m_domainWasSetInDOM(other->m_domainWasSetInDOM)
    , m_canLoadLocalResources(other->m_canLoadLocalResources)
    , m_storageBlockingPolicy(other->m_storageBlockingPolicy)
    , m_enforceFilePathSeparation(other->m_enforceFilePathSeparation)
    , m_needsDatabaseIdentifierQuirkForFiles(other->m_needsDatabaseIdentifierQuirkForFiles)
{
}

PassRefPtr<SecurityOrigin> SecurityOrigin::create(const KURL& url)
{
    RefPtr<SecurityOrigin> cachedOrigin = getCachedOrigin(url);
    if (cachedOrigin.get())
        return cachedOrigin;

    if (shouldTreatAsUniqueOrigin(url)) {
        RefPtr<SecurityOrigin> origin = adoptRef(new SecurityOrigin());

        if (url.protocolIs("file")) {
            // Unfortunately, we can't represent all unique origins exactly
            // the same way because we need to produce a quirky database
            // identifier for file URLs due to persistent storage in some
            // embedders of WebKit.
            origin->m_needsDatabaseIdentifierQuirkForFiles = true;
        }

        return origin.release();
    }

    if (shouldUseInnerURL(url))
        return adoptRef(new SecurityOrigin(extractInnerURL(url)));

    return adoptRef(new SecurityOrigin(url));
}

PassRefPtr<SecurityOrigin> SecurityOrigin::createUnique()
{
    RefPtr<SecurityOrigin> origin = adoptRef(new SecurityOrigin());
    ASSERT(origin->isUnique());
    return origin.release();
}

PassRefPtr<SecurityOrigin> SecurityOrigin::isolatedCopy() const
{
    return adoptRef(new SecurityOrigin(this));
}

void SecurityOrigin::setDomainFromDOM(const String& newDomain)
{
    m_domainWasSetInDOM = true;
    m_domain = newDomain.lower();
}

bool SecurityOrigin::isSecure(const KURL& url)
{
    // Invalid URLs are secure, as are URLs which have a secure protocol.
    if (!url.isValid() || SchemeRegistry::shouldTreatURLSchemeAsSecure(url.protocol()))
        return true;

    // URLs that wrap inner URLs are secure if those inner URLs are secure.
    if (shouldUseInnerURL(url) && SchemeRegistry::shouldTreatURLSchemeAsSecure(extractInnerURL(url).protocol()))
        return true;

    return false;
}

bool SecurityOrigin::canAccess(const SecurityOrigin* other) const
{
    if (m_universalAccess)
        return true;

    if (this == other)
        return true;

    if (isUnique() || other->isUnique())
        return false;

    // Here are two cases where we should permit access:
    //
    // 1) Neither document has set document.domain. In this case, we insist
    //    that the scheme, host, and port of the URLs match.
    //
    // 2) Both documents have set document.domain. In this case, we insist
    //    that the documents have set document.domain to the same value and
    //    that the scheme of the URLs match.
    //
    // This matches the behavior of Firefox 2 and Internet Explorer 6.
    //
    // Internet Explorer 7 and Opera 9 are more strict in that they require
    // the port numbers to match when both pages have document.domain set.
    //
    // FIXME: Evaluate whether we can tighten this policy to require matched
    //        port numbers.
    //
    // Opera 9 allows access when only one page has set document.domain, but
    // this is a security vulnerability.

    bool canAccess = false;
    if (m_protocol == other->m_protocol) {
        if (!m_domainWasSetInDOM && !other->m_domainWasSetInDOM) {
            if (m_host == other->m_host && m_port == other->m_port)
                canAccess = true;
        } else if (m_domainWasSetInDOM && other->m_domainWasSetInDOM) {
            if (m_domain == other->m_domain)
                canAccess = true;
        }
    }

    if (canAccess && isLocal())
       canAccess = passesFileCheck(other);

    return canAccess;
}

bool SecurityOrigin::passesFileCheck(const SecurityOrigin* other) const
{
    ASSERT(isLocal() && other->isLocal());

    if (!m_enforceFilePathSeparation && !other->m_enforceFilePathSeparation)
        return true;

    return (m_filePath == other->m_filePath);
}

bool SecurityOrigin::canRequest(const KURL& url) const
{
    if (m_universalAccess)
        return true;

    if (getCachedOrigin(url) == this)
        return true;

    if (isUnique())
        return false;

    RefPtr<SecurityOrigin> targetOrigin = SecurityOrigin::create(url);

    if (targetOrigin->isUnique())
        return false;

    // We call isSameSchemeHostPort here instead of canAccess because we want
    // to ignore document.domain effects.
    if (isSameSchemeHostPort(targetOrigin.get()))
        return true;

    if (SecurityPolicy::isAccessWhiteListed(this, targetOrigin.get()))
        return true;

    return false;
}

bool SecurityOrigin::taintsCanvas(const KURL& url) const
{
    if (canRequest(url))
        return false;

    // This function exists because we treat data URLs as having a unique origin,
    // contrary to the current (9/19/2009) draft of the HTML5 specification.
    // We still want to let folks paint data URLs onto untainted canvases, so
    // we special case data URLs below. If we change to match HTML5 w.r.t.
    // data URL security, then we can remove this function in favor of
    // !canRequest.
    if (url.protocolIsData())
        return false;

    return true;
}

bool SecurityOrigin::canReceiveDragData(const SecurityOrigin* dragInitiator) const
{
    if (this == dragInitiator)
        return true;

    return canAccess(dragInitiator);  
}

// This is a hack to allow keep navigation to http/https feeds working. To remove this
// we need to introduce new API akin to registerURLSchemeAsLocal, that registers a
// protocols navigation policy.
// feed(|s|search): is considered a 'nesting' scheme by embedders that support it, so it can be
// local or remote depending on what is nested. Currently we just check if we are nesting
// http or https, otherwise we ignore the nesting for the purpose of a security check. We need
// a facility for registering nesting schemes, and some generalized logic for them.
// This function should be removed as an outcome of https://bugs.webkit.org/show_bug.cgi?id=69196
static bool isFeedWithNestedProtocolInHTTPFamily(const KURL& url)
{
    const String& urlString = url.string();
    if (!urlString.startsWith("feed", false))
        return false;

    return urlString.startsWith("feed://", false) 
        || urlString.startsWith("feed:http:", false) || urlString.startsWith("feed:https:", false)
        || urlString.startsWith("feeds:http:", false) || urlString.startsWith("feeds:https:", false)
        || urlString.startsWith("feedsearch:http:", false) || urlString.startsWith("feedsearch:https:", false);
}

bool SecurityOrigin::canDisplay(const KURL& url) const
{
    if (m_universalAccess)
        return true;

    String protocol = url.protocol().lower();

    if (isFeedWithNestedProtocolInHTTPFamily(url))
        return true;

    if (SchemeRegistry::canDisplayOnlyIfCanRequest(protocol))
        return canRequest(url);

    if (SchemeRegistry::shouldTreatURLSchemeAsDisplayIsolated(protocol))
        return m_protocol == protocol || SecurityPolicy::isAccessToURLWhiteListed(this, url);

    if (SecurityPolicy::restrictAccessToLocal() && SchemeRegistry::shouldTreatURLSchemeAsLocal(protocol))
        return canLoadLocalResources() || SecurityPolicy::isAccessToURLWhiteListed(this, url);

    return true;
}

bool SecurityOrigin::canAccessStorage(const SecurityOrigin* topOrigin, ShouldAllowFromThirdParty shouldAllowFromThirdParty) const
{
    if (isUnique())
        return false;

    if (m_storageBlockingPolicy == BlockAllStorage)
        return false;

    // FIXME: This check should be replaced with an ASSERT once we can guarantee that topOrigin is not null.
    if (!topOrigin)
        return true;

    if (topOrigin->m_storageBlockingPolicy == BlockAllStorage)
        return false;

    if (shouldAllowFromThirdParty == AlwaysAllowFromThirdParty)
        return true;

    if ((m_storageBlockingPolicy == BlockThirdPartyStorage || topOrigin->m_storageBlockingPolicy == BlockThirdPartyStorage) && topOrigin->isThirdParty(this))
        return false;

    return true;
}

SecurityOrigin::Policy SecurityOrigin::canShowNotifications() const
{
    if (m_universalAccess)
        return AlwaysAllow;
    if (isUnique())
        return AlwaysDeny;
    return Ask;
}

bool SecurityOrigin::isThirdParty(const SecurityOrigin* child) const
{
    if (child->m_universalAccess)
        return false;

    if (this == child)
        return false;

    if (isUnique() || child->isUnique())
        return true;

    return !isSameSchemeHostPort(child);
}

void SecurityOrigin::grantLoadLocalResources()
{
    // Granting privileges to some, but not all, documents in a SecurityOrigin
    // is a security hazard because the documents without the privilege can
    // obtain the privilege by injecting script into the documents that have
    // been granted the privilege.
    m_canLoadLocalResources = true;
}

void SecurityOrigin::grantUniversalAccess()
{
    m_universalAccess = true;
}

#if ENABLE(CACHE_PARTITIONING)
String SecurityOrigin::cachePartition() const
{
    if (m_storageBlockingPolicy != BlockThirdPartyStorage)
        return String();

    if (m_protocol != "http" && m_protocol != "https")
        return String();

    return host();
}
#endif

void SecurityOrigin::enforceFilePathSeparation()
{
    ASSERT(isLocal());
    m_enforceFilePathSeparation = true;
}

bool SecurityOrigin::isLocal() const
{
    return SchemeRegistry::shouldTreatURLSchemeAsLocal(m_protocol);
}

String SecurityOrigin::toString() const
{
    if (isUnique())
        return "null";
    if (m_protocol == "file" && m_enforceFilePathSeparation)
        return "null";
    return toRawString();
}

String SecurityOrigin::toRawString() const
{
    if (m_protocol == "file")
        return "file://";

    StringBuilder result;
    result.reserveCapacity(m_protocol.length() + m_host.length() + 10);
    result.append(m_protocol);
    result.append("://");
    result.append(m_host);

    if (m_port) {
        result.append(':');
        result.appendNumber(m_port);
    }

    return result.toString();
}

PassRefPtr<SecurityOrigin> SecurityOrigin::createFromString(const String& originString)
{
    return SecurityOrigin::create(KURL(KURL(), originString));
}

static const char separatorCharacter = '_';

PassRefPtr<SecurityOrigin> SecurityOrigin::createFromDatabaseIdentifier(const String& databaseIdentifier)
{ 
    // Make sure there's a first separator
    size_t separator1 = databaseIdentifier.find(separatorCharacter);
    if (separator1 == notFound)
        return create(KURL());
        
    // Make sure there's a second separator
    size_t separator2 = databaseIdentifier.reverseFind(separatorCharacter);
    if (separator2 == notFound)
        return create(KURL());
        
    // Ensure there were at least 2 separator characters. Some hostnames on intranets have
    // underscores in them, so we'll assume that any additional underscores are part of the host.
    if (separator1 == separator2)
        return create(KURL());
        
    // Make sure the port section is a valid port number or doesn't exist
    bool portOkay;
    int port = databaseIdentifier.right(databaseIdentifier.length() - separator2 - 1).toInt(&portOkay);
    bool portAbsent = (separator2 == databaseIdentifier.length() - 1);
    if (!(portOkay || portAbsent))
        return create(KURL());
    
    if (port < 0 || port > MaxAllowedPort)
        return create(KURL());
        
    // Split out the 3 sections of data
    String protocol = databaseIdentifier.substring(0, separator1);
    String host = databaseIdentifier.substring(separator1 + 1, separator2 - separator1 - 1);
    
    host = decodeURLEscapeSequences(host);
    return create(KURL(KURL(), protocol + "://" + host + ":" + String::number(port) + "/"));
}

PassRefPtr<SecurityOrigin> SecurityOrigin::create(const String& protocol, const String& host, int port)
{
    if (port < 0 || port > MaxAllowedPort)
        return createUnique();
    String decodedHost = decodeURLEscapeSequences(host);
    return create(KURL(KURL(), protocol + "://" + host + ":" + String::number(port) + "/"));
}

String SecurityOrigin::databaseIdentifier() const 
{
    // Historically, we've used the following (somewhat non-sensical) string
    // for the databaseIdentifier of local files. We used to compute this
    // string because of a bug in how we handled the scheme for file URLs.
    // Now that we've fixed that bug, we still need to produce this string
    // to avoid breaking existing persistent state.
    if (m_needsDatabaseIdentifierQuirkForFiles)
        return "file__0";

    StringBuilder stringBuilder;
    stringBuilder.append(m_protocol);
    stringBuilder.append(separatorCharacter);
    stringBuilder.append(encodeForFileName(m_host));
    stringBuilder.append(separatorCharacter);
    stringBuilder.appendNumber(m_port);

    return stringBuilder.toString();
}

bool SecurityOrigin::equal(const SecurityOrigin* other) const 
{
    if (other == this)
        return true;
    
    if (!isSameSchemeHostPort(other))
        return false;

    if (m_domainWasSetInDOM != other->m_domainWasSetInDOM)
        return false;

    if (m_domainWasSetInDOM && m_domain != other->m_domain)
        return false;

    return true;
}

bool SecurityOrigin::isSameSchemeHostPort(const SecurityOrigin* other) const 
{
    if (m_host != other->m_host)
        return false;

    if (m_protocol != other->m_protocol)
        return false;

    if (m_port != other->m_port)
        return false;

    if (isLocal() && !passesFileCheck(other))
        return false;

    return true;
}

String SecurityOrigin::urlWithUniqueSecurityOrigin()
{
    ASSERT(isMainThread());
    DEFINE_STATIC_LOCAL(const String, uniqueSecurityOriginURL, (ASCIILiteral("data:,")));
    return uniqueSecurityOriginURL;
}

} // namespace WebCore
