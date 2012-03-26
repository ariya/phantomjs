/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "KURL.h"
#include "LinkHash.h"
#include "PlatformString.h"
#include <wtf/text/AtomicString.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/StringImpl.h>

namespace WebCore {

static inline size_t findSlashDotDotSlash(const UChar* characters, size_t length, size_t position)
{
    if (length < 4)
        return notFound;
    size_t loopLimit = length - 3;
    for (size_t i = position; i < loopLimit; ++i) {
        if (characters[i] == '/' && characters[i + 1] == '.' && characters[i + 2] == '.' && characters[i + 3] == '/')
            return i;
    }
    return notFound;
}

static inline size_t findSlashSlash(const UChar* characters, size_t length, size_t position)
{
    if (length < 2)
        return notFound;
    size_t loopLimit = length - 1;
    for (size_t i = position; i < loopLimit; ++i) {
        if (characters[i] == '/' && characters[i + 1] == '/')
            return i;
    }
    return notFound;
}

static inline size_t findSlashDotSlash(const UChar* characters, size_t length, size_t position)
{
    if (length < 3)
        return notFound;
    size_t loopLimit = length - 2;
    for (size_t i = position; i < loopLimit; ++i) {
        if (characters[i] == '/' && characters[i + 1] == '.' && characters[i + 2] == '/')
            return i;
    }
    return notFound;
}

static inline bool containsColonSlashSlash(const UChar* characters, unsigned length)
{
    if (length < 3)
        return false;
    unsigned loopLimit = length - 2;
    for (unsigned i = 0; i < loopLimit; ++i) {
        if (characters[i] == ':' && characters[i + 1] == '/' && characters[i + 2] == '/')
            return true;
    }
    return false;
}

static inline void squeezeOutNullCharacters(Vector<UChar, 512>& string)
{
    size_t size = string.size();
    size_t i = 0;
    for (i = 0; i < size; ++i) {
        if (!string[i])
            break;
    }
    if (i == size)
        return;
    size_t j = i;
    for (++i; i < size; ++i) {
        if (UChar character = string[i])
            string[j++] = character;
    }
    ASSERT(j < size);
    string.shrink(j);
}

static void cleanSlashDotDotSlashes(Vector<UChar, 512>& path, size_t firstSlash)
{
    size_t slash = firstSlash;
    do {
        size_t previousSlash = slash ? reverseFind(path.data(), path.size(), '/', slash - 1) : notFound;
        // Don't remove the host, i.e. http://foo.org/../foo.html
        if (previousSlash == notFound || (previousSlash > 3 && path[previousSlash - 2] == ':' && path[previousSlash - 1] == '/')) {
            path[slash] = 0;
            path[slash + 1] = 0;
            path[slash + 2] = 0;
        } else {
            for (size_t i = previousSlash; i < slash + 3; ++i)
                path[i] = 0;
        }
        slash += 3;
    } while ((slash = findSlashDotDotSlash(path.data(), path.size(), slash)) != notFound);
    squeezeOutNullCharacters(path);
}

static void mergeDoubleSlashes(Vector<UChar, 512>& path, size_t firstSlash)
{
    size_t refPos = find(path.data(), path.size(), '#');
    if (!refPos || refPos == notFound)
        refPos = path.size();

    size_t slash = firstSlash;
    while (slash < refPos) {
        if (!slash || path[slash - 1] != ':')
            path[slash++] = 0;
        else
            slash += 2;
        if ((slash = findSlashSlash(path.data(), path.size(), slash)) == notFound)
            break;
    }
    squeezeOutNullCharacters(path);
}

static void cleanSlashDotSlashes(Vector<UChar, 512>& path, size_t firstSlash)
{
    size_t slash = firstSlash;
    do {
        path[slash] = 0;
        path[slash + 1] = 0;
        slash += 2;
    } while ((slash = findSlashDotSlash(path.data(), path.size(), slash)) != notFound);
    squeezeOutNullCharacters(path);
}

static inline void cleanPath(Vector<UChar, 512>& path)
{
    // FIXME: Should not do this in the query or anchor part of the URL.
    size_t firstSlash = findSlashDotDotSlash(path.data(), path.size(), 0);
    if (firstSlash != notFound)
        cleanSlashDotDotSlashes(path, firstSlash);

    // FIXME: Should not do this in the query part.
    firstSlash = findSlashSlash(path.data(), path.size(), 0);
    if (firstSlash != notFound)
        mergeDoubleSlashes(path, firstSlash);

    // FIXME: Should not do this in the query or anchor part.
    firstSlash = findSlashDotSlash(path.data(), path.size(), 0);
    if (firstSlash != notFound)
        cleanSlashDotSlashes(path, firstSlash);
}

static inline bool matchLetter(UChar c, UChar lowercaseLetter)
{
    return (c | 0x20) == lowercaseLetter;
}

static inline bool needsTrailingSlash(const UChar* characters, unsigned length)
{
    if (length < 6)
        return false;
    if (!matchLetter(characters[0], 'h')
            || !matchLetter(characters[1], 't')
            || !matchLetter(characters[2], 't')
            || !matchLetter(characters[3], 'p'))
        return false;
    if (!(characters[4] == ':'
            || (matchLetter(characters[4], 's') && characters[5] == ':')))
        return false;

    unsigned pos = characters[4] == ':' ? 5 : 6;

    // Skip initial two slashes if present.
    if (pos + 1 < length && characters[pos] == '/' && characters[pos + 1] == '/')
        pos += 2;

    // Find next slash.
    while (pos < length && characters[pos] != '/')
        ++pos;

    return pos == length;
}

static ALWAYS_INLINE LinkHash visitedLinkHashInline(const UChar* url, unsigned length)
{
    return AlreadyHashed::avoidDeletedValue(StringHasher::computeHash(url, length));
}

LinkHash visitedLinkHash(const UChar* url, unsigned length)
{
    return visitedLinkHashInline(url, length);
}

static ALWAYS_INLINE void visitedURLInline(const KURL& base, const AtomicString& attributeURL, Vector<UChar, 512>& buffer)
{
    if (attributeURL.isNull())
        return;

    const UChar* characters = attributeURL.characters();
    unsigned length = attributeURL.length();

    // This is a poor man's completeURL. Faster with less memory allocation.
    // FIXME: It's missing a lot of what completeURL does and a lot of what KURL does.
    // For example, it does not handle international domain names properly.

    // FIXME: It is wrong that we do not do further processing on strings that have "://" in them:
    //    1) The "://" could be in the query or anchor.
    //    2) The URL's path could have a "/./" or a "/../" or a "//" sequence in it.

    // FIXME: needsTrailingSlash does not properly return true for a URL that has no path, but does
    // have a query or anchor.

    bool hasColonSlashSlash = containsColonSlashSlash(characters, length);

    if (hasColonSlashSlash && !needsTrailingSlash(characters, length)) {
        buffer.append(attributeURL.characters(), attributeURL.length());
        return;
    }


    if (hasColonSlashSlash) {
        // FIXME: This is incorrect for URLs that have a query or anchor; the "/" needs to go at the
        // end of the path, *before* the query or anchor.
        buffer.append(characters, length);
        buffer.append('/');
        return;
    }

    if (!length)
        buffer.append(base.string().characters(), base.string().length());
    else {
        switch (characters[0]) {
            case '/':
                buffer.append(base.string().characters(), base.pathStart());
                break;
            case '#':
                buffer.append(base.string().characters(), base.pathEnd());
                break;
            default:
                buffer.append(base.string().characters(), base.pathAfterLastSlash());
                break;
        }
    }
    buffer.append(characters, length);
    cleanPath(buffer);
    if (needsTrailingSlash(buffer.data(), buffer.size())) {
        // FIXME: This is incorrect for URLs that have a query or anchor; the "/" needs to go at the
        // end of the path, *before* the query or anchor.
        buffer.append('/');
    }

    return;
}

void visitedURL(const KURL& base, const AtomicString& attributeURL, Vector<UChar, 512>& buffer)
{
    return visitedURLInline(base, attributeURL, buffer);
}

LinkHash visitedLinkHash(const KURL& base, const AtomicString& attributeURL)
{
    Vector<UChar, 512> url;
    visitedURLInline(base, attributeURL, url);
    if (url.isEmpty())
        return 0;

    return visitedLinkHashInline(url.data(), url.size());
}

}  // namespace WebCore
