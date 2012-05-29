/* Based on nsURLParsers.cc from Mozilla
 * -------------------------------------
 * Copyright (C) 1998 Netscape Communications Corporation.
 *
 * Other contributors:
 *   Darin Fisher (original author)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.
 */

#ifndef URLParser_h
#define URLParser_h

#include "URLComponent.h"
#include "URLSegments.h"

namespace WTF {

template<typename CHAR>
class URLParser {
public:
    enum SpecialPort {
        UnspecifiedPort = -1,
        InvalidPort = -2,
    };

    // This handles everything that may be an authority terminator, including
    // backslash. For special backslash handling see parseAfterScheme.
    static bool isPossibleAuthorityTerminator(CHAR ch)
    {
        return isURLSlash(ch) || ch == '?' || ch == '#' || ch == ';';
    }

    // Given an already-identified auth section, breaks it into its constituent
    // parts. The port number will be parsed and the resulting integer will be
    // filled into the given *port variable, or -1 if there is no port number
    // or it is invalid.
    static void parseAuthority(const CHAR* spec, const URLComponent& auth, URLComponent& username, URLComponent& password, URLComponent& host, URLComponent& port)
    {
        // FIXME: add ASSERT(auth.isValid()); // We should always get an authority.
        if (!auth.length()) {
            username.reset();
            password.reset();
            host.reset();
            port.reset();
            return;
        }

        // Search backwards for @, which is the separator between the user info
        // and the server info.  RFC 3986 forbids @ from occuring in auth, but
        // someone might include it in a password unescaped.
        int i = auth.begin() + auth.length() - 1;
        while (i > auth.begin() && spec[i] != '@')
            --i;

        if (spec[i] == '@') {
            // Found user info: <user-info>@<server-info>
            parseUserInfo(spec, URLComponent(auth.begin(), i - auth.begin()), username, password);
            parseServerInfo(spec, URLComponent::fromRange(i + 1, auth.begin() + auth.length()), host, port);
        } else {
            // No user info, everything is server info.
            username.reset();
            password.reset();
            parseServerInfo(spec, auth, host, port);
        }
    }

    static bool extractScheme(const CHAR* spec, int specLength, URLComponent& scheme)
    {
        // Skip leading whitespace and control characters.
        int begin = 0;
        while (begin < specLength && shouldTrimFromURL(spec[begin]))
            begin++;
        if (begin == specLength)
            return false; // Input is empty or all whitespace.

        // Find the first colon character.
        for (int i = begin; i < specLength; i++) {
            if (spec[i] == ':') {
                scheme = URLComponent::fromRange(begin, i);
                return true;
            }
        }
        return false; // No colon found: no scheme
    }

    // Fills in all members of the URLSegments structure (except for the
    // scheme) for standard URLs.
    //
    // |spec| is the full spec being parsed, of length |specLength|.
    // |afterScheme| is the character immediately following the scheme (after
    // the colon) where we'll begin parsing.
    static void parseAfterScheme(const CHAR* spec, int specLength, int afterScheme, URLSegments& parsed)
    {
        int numberOfSlashes = consecutiveSlashes(spec, afterScheme, specLength);
        int afterSlashes = afterScheme + numberOfSlashes;

        // First split into two main parts, the authority (username, password,
        // host, and port) and the full path (path, query, and reference).
        URLComponent authority;
        URLComponent fullPath;

        // Found "//<some data>", looks like an authority section. Treat
        // everything from there to the next slash (or end of spec) to be the
        // authority. Note that we ignore the number of slashes and treat it as
        // the authority.
        int authEnd = nextAuthorityTerminator(spec, afterSlashes, specLength);
        authority = URLComponent(afterSlashes, authEnd - afterSlashes);

        if (authEnd == specLength) // No beginning of path found.
            fullPath = URLComponent();
        else // Everything starting from the slash to the end is the path.
            fullPath = URLComponent(authEnd, specLength - authEnd);

        // Now parse those two sub-parts.
        parseAuthority(spec, authority, parsed.username, parsed.password, parsed.host, parsed.port);
        parsePath(spec, fullPath, parsed.path, parsed.query, parsed.fragment);
    }

    // The main parsing function for standard URLs. Standard URLs have a scheme,
    // host, path, etc.
    static void parseStandardURL(const CHAR* spec, int specLength, URLSegments& parsed)
    {
        // FIXME: add ASSERT(specLength >= 0);

        // Strip leading & trailing spaces and control characters.
        int begin = 0;
        trimURL(spec, begin, specLength);

        int afterScheme;
        if (extractScheme(spec, specLength, parsed.scheme))
            afterScheme = parsed.scheme.end() + 1; // Skip past the colon.
        else {
            // Say there's no scheme when there is a colon. We could also say
            // that everything is the scheme. Both would produce an invalid
            // URL, but this way seems less wrong in more cases.
            parsed.scheme.reset();
            afterScheme = begin;
        }
        parseAfterScheme(spec, specLength, afterScheme, parsed);
    }

    static void parsePath(const CHAR* spec, const URLComponent& path, URLComponent& filepath, URLComponent& query, URLComponent& fragment)
    {
        // path = [/]<segment1>/<segment2>/<...>/<segmentN>;<param>?<query>#<fragment>

        // Special case when there is no path.
        if (!path.isValid()) {
            filepath.reset();
            query.reset();
            fragment.reset();
            return;
        }
        // FIXME: add ASSERT(path.length() > 0); // We should never have 0 length paths.

        // Search for first occurrence of either ? or #.
        int pathEnd = path.begin() + path.length();

        int querySeparator = -1; // Index of the '?'
        int refSeparator = -1; // Index of the '#'
        for (int i = path.begin(); i < pathEnd; i++) {
            switch (spec[i]) {
            case '?':
                if (querySeparator < 0)
                    querySeparator = i;
                break;
            case '#':
                refSeparator = i;
                i = pathEnd; // Break out of the loop.
                break;
            default:
                break;
            }
        }

        // Markers pointing to the character after each of these corresponding
        // components. The code below works from the end back to the beginning,
        // and will update these indices as it finds components that exist.
        int fileEnd, queryEnd;

        // Fragment: from the # to the end of the path.
        if (refSeparator >= 0) {
            fileEnd = refSeparator;
            queryEnd = refSeparator;
            fragment = URLComponent::fromRange(refSeparator + 1, pathEnd);
        } else {
            fileEnd = pathEnd;
            queryEnd = pathEnd;
            fragment.reset();
        }

        // Query fragment: everything from the ? to the next boundary (either
        // the end of the path or the fragment fragment).
        if (querySeparator >= 0) {
            fileEnd = querySeparator;
            query = URLComponent::fromRange(querySeparator + 1, queryEnd);
        } else
            query.reset();

        // File path: treat an empty file path as no file path.
        if (fileEnd != path.begin())
            filepath = URLComponent::fromRange(path.begin(), fileEnd);
        else
            filepath.reset();
    }

    // Initializes a path URL which is merely a scheme followed by a path.
    // Examples include "about:foo" and "javascript:alert('bar');"
    static void parsePathURL(const CHAR* spec, int specLength, URLSegments& parsed)
    {
        // Get the non-path and non-scheme parts of the URL out of the way, we
        // never use them.
        parsed.username.reset();
        parsed.password.reset();
        parsed.host.reset();
        parsed.port.reset();
        parsed.query.reset();
        parsed.fragment.reset();

        // Strip leading & trailing spaces and control characters.
        // FIXME: Perhaps this is unnecessary?
        int begin = 0;
        trimURL(spec, begin, specLength);

        // Handle empty specs or ones that contain only whitespace or control
        // chars.
        if (begin == specLength) {
            parsed.scheme.reset();
            parsed.path.reset();
            return;
        }

        // Extract the scheme, with the path being everything following. We also
        // handle the case where there is no scheme.
        if (extractScheme(&spec[begin], specLength - begin, parsed.scheme)) {
            // Offset the results since we gave extractScheme a substring.
            parsed.scheme.setBegin(parsed.scheme.begin() + begin);

            // For compatibility with the standard URL parser, we treat no path
            // as -1, rather than having a length of 0 (we normally wouldn't
            // care so much for these non-standard URLs).
            if (parsed.scheme.end() == specLength - 1)
                parsed.path.reset();
            else
                parsed.path = URLComponent::fromRange(parsed.scheme.end() + 1, specLength);
        } else {
            // No scheme found, just path.
            parsed.scheme.reset();
            parsed.path = URLComponent::fromRange(begin, specLength);
        }
    }

    static void parseMailtoURL(const CHAR* spec, int specLength, URLSegments& parsed)
    {
        // FIXME: add ASSERT(specLength >= 0);

        // Get the non-path and non-scheme parts of the URL out of the way, we
        // never use them.
        parsed.username.reset();
        parsed.password.reset();
        parsed.host.reset();
        parsed.port.reset();
        parsed.fragment.reset();
        parsed.query.reset(); // May use this; reset for convenience.

        // Strip leading & trailing spaces and control characters.
        int begin = 0;
        trimURL(spec, begin, specLength);

        // Handle empty specs or ones that contain only whitespace or control
        // chars.
        if (begin == specLength) {
            parsed.scheme.reset();
            parsed.path.reset();
            return;
        }

        int pathBegin = -1;
        int pathEnd = -1;

        // Extract the scheme, with the path being everything following. We also
        // handle the case where there is no scheme.
        if (extractScheme(&spec[begin], specLength - begin, parsed.scheme)) {
            // Offset the results since we gave extractScheme a substring.
            parsed.scheme.setBegin(parsed.scheme.begin() + begin);

            if (parsed.scheme.end() != specLength - 1) {
                pathBegin = parsed.scheme.end() + 1;
                pathEnd = specLength;
            }
        } else {
            // No scheme found, just path.
            parsed.scheme.reset();
            pathBegin = begin;
            pathEnd = specLength;
        }

        // Split [pathBegin, pathEnd) into a path + query.
        for (int i = pathBegin; i < pathEnd; ++i) {
            if (spec[i] == '?') {
                parsed.query = URLComponent::fromRange(i + 1, pathEnd);
                pathEnd = i;
                break;
            }
        }

        // For compatibility with the standard URL parser, treat no path as
        // -1, rather than having a length of 0
        if (pathBegin == pathEnd)
            parsed.path.reset();
        else
            parsed.path = URLComponent::fromRange(pathBegin, pathEnd);
    }

    static int parsePort(const CHAR* spec, const URLComponent& component)
    {
        // Easy success case when there is no port.
        const int maxDigits = 5;
        if (component.isEmptyOrInvalid())
            return UnspecifiedPort;

        URLComponent nonZeroDigits(component.end(), 0);
        for (int i = 0; i < component.length(); ++i) {
            if (spec[component.begin() + i] != '0') {
                nonZeroDigits = URLComponent::fromRange(component.begin() + i, component.end());
                break;
            }
        }
        if (!nonZeroDigits.length())
            return 0; // All digits were 0.

        if (nonZeroDigits.length() > maxDigits)
            return InvalidPort;

        int port = 0;
        for (int i = 0; i < nonZeroDigits.length(); ++i) {
            CHAR ch = spec[nonZeroDigits.begin() + i];
            if (!isPortDigit(ch))
                return InvalidPort;
            port *= 10;
            port += static_cast<char>(ch) - '0';
        }
        if (port > 65535)
            return InvalidPort;
        return port;
    }

    static void extractFileName(const CHAR* spec, const URLComponent& path, URLComponent& fileName)
    {
        // Handle empty paths: they have no file names.
        if (path.isEmptyOrInvalid()) {
            fileName.reset();
            return;
        }

        // Search backwards for a parameter, which is a normally unused field
        // in a URL delimited by a semicolon. We parse the parameter as part of
        // the path, but here, we don't want to count it. The last semicolon is
        // the parameter.
        int fileEnd = path.end();
        for (int i = path.end() - 1; i > path.begin(); --i) {
            if (spec[i] == ';') {
                fileEnd = i;
                break;
            }
        }

        // Now search backwards from the filename end to the previous slash
        // to find the beginning of the filename.
        for (int i = fileEnd - 1; i >= path.begin(); --i) {
            if (isURLSlash(spec[i])) {
                // File name is everything following this character to the end
                fileName = URLComponent::fromRange(i + 1, fileEnd);
                return;
            }
        }

        // No slash found, this means the input was degenerate (generally paths
        // will start with a slash). Let's call everything the file name.
        fileName = URLComponent::fromRange(path.begin(), fileEnd);
    }

    static bool extractQueryKeyValue(const CHAR* spec, URLComponent& query, URLComponent& key, URLComponent& value)
    {
        if (query.isEmptyOrInvalid())
            return false;

        int start = query.begin();
        int current = start;
        int end = query.end();

        // We assume the beginning of the input is the beginning of the "key"
        // and we skip to the end of it.
        key.setBegin(current);
        while (current < end && spec[current] != '&' && spec[current] != '=')
            ++current;
        key.setLength(current - key.begin());

        // Skip the separator after the key (if any).
        if (current < end && spec[current] == '=')
            ++current;

        // Find the value part.
        value.setBegin(current);
        while (current < end && spec[current] != '&')
            ++current;
        value.setLength(current - value.begin());

        // Finally skip the next separator if any
        if (current < end && spec[current] == '&')
            ++current;

        // Save the new query
        query = URLComponent::fromRange(current, end);
        return true;
    }

// FIXME: This should be protected or private.
public:
    // We treat slashes and backslashes the same for IE compatibility.
    static inline bool isURLSlash(CHAR ch)
    {
        return ch == '/' || ch == '\\';
    }

    // Returns true if we should trim this character from the URL because it is
    // a space or a control character.
    static inline bool shouldTrimFromURL(CHAR ch)
    {
        return ch <= ' ';
    }

    // Given an already-initialized begin index and end index (the index after
    // the last CHAR in spec), this shrinks the range to eliminate
    // "should-be-trimmed" characters.
    static inline void trimURL(const CHAR* spec, int& begin, int& end)
    {
        // Strip leading whitespace and control characters.
        while (begin < end && shouldTrimFromURL(spec[begin]))
            ++begin;

        // Strip trailing whitespace and control characters. We need the >i
        // test for when the input string is all blanks; we don't want to back
        // past the input.
        while (end > begin && shouldTrimFromURL(spec[end - 1]))
            --end;
    }

    // Counts the number of consecutive slashes starting at the given offset
    // in the given string of the given length.
    static inline int consecutiveSlashes(const CHAR *string, int beginOffset, int stringLength)
    {
        int count = 0;
        while (beginOffset + count < stringLength && isURLSlash(string[beginOffset + count]))
            ++count;
        return count;
    }

private:
    // URLParser cannot be constructed.
    URLParser();

    // Returns true if the given character is a valid digit to use in a port.
    static inline bool isPortDigit(CHAR ch)
    {
        return ch >= '0' && ch <= '9';
    }

    // Returns the offset of the next authority terminator in the input starting
    // from startOffset. If no terminator is found, the return value will be equal
    // to specLength.
    static int nextAuthorityTerminator(const CHAR* spec, int startOffset, int specLength)
    {
        for (int i = startOffset; i < specLength; i++) {
            if (isPossibleAuthorityTerminator(spec[i]))
                return i;
        }
        return specLength; // Not found.
    }

    static void parseUserInfo(const CHAR* spec, const URLComponent& user, URLComponent& username, URLComponent& password)
    {
        // Find the first colon in the user section, which separates the
        // username and password.
        int colonOffset = 0;
        while (colonOffset < user.length() && spec[user.begin() + colonOffset] != ':')
            ++colonOffset;

        if (colonOffset < user.length()) {
            // Found separator: <username>:<password>
            username = URLComponent(user.begin(), colonOffset);
            password = URLComponent::fromRange(user.begin() + colonOffset + 1, user.begin() + user.length());
        } else {
            // No separator, treat everything as the username
            username = user;
            password = URLComponent();
        }
    }

    static void parseServerInfo(const CHAR* spec, const URLComponent& serverInfo, URLComponent& host, URLComponent& port)
    {
        if (!serverInfo.length()) {
            // No server info, host name is empty.
            host.reset();
            port.reset();
            return;
        }

        // If the host starts with a left-bracket, assume the entire host is an
        // IPv6 literal.  Otherwise, assume none of the host is an IPv6 literal.
        // This assumption will be overridden if we find a right-bracket.
        //
        // Our IPv6 address canonicalization code requires both brackets to
        // exist, but the ability to locate an incomplete address can still be
        // useful.
        int ipv6Terminator = spec[serverInfo.begin()] == '[' ? serverInfo.end() : -1;
        int colon = -1;

        // Find the last right-bracket, and the last colon.
        for (int i = serverInfo.begin(); i < serverInfo.end(); i++) {
            switch (spec[i]) {
            case ']':
                ipv6Terminator = i;
                break;
            case ':':
                colon = i;
                break;
            default:
                break;
            }
        }

        if (colon > ipv6Terminator) {
            // Found a port number: <hostname>:<port>
            host = URLComponent::fromRange(serverInfo.begin(), colon);
            if (!host.length())
                host.reset();
            port = URLComponent::fromRange(colon + 1, serverInfo.end());
        } else {
            // No port: <hostname>
            host = serverInfo;
            port.reset();
        }
    }
};

} // namespace WTF

#endif // URLParser_h
