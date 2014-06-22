/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "DOMFilePath.h"

#if ENABLE(FILE_SYSTEM)

#include <wtf/Vector.h>
#include <wtf/text/CString.h>

namespace WebCore {

const char DOMFilePath::separator = '/';
const char DOMFilePath::root[] = "/";

String DOMFilePath::append(const String& base, const String& components)
{
    return ensureDirectoryPath(base) + components;
}

String DOMFilePath::ensureDirectoryPath(const String& path)
{
    if (!DOMFilePath::endsWithSeparator(path)) {
        String newPath = path;
        newPath.append(DOMFilePath::separator);
        return newPath;
    }
    return path;
}

String DOMFilePath::getName(const String& path)
{
    int index = path.reverseFind(DOMFilePath::separator);
    if (index != -1)
        return path.substring(index + 1);
    return path;
}

String DOMFilePath::getDirectory(const String& path)
{
    int index = path.reverseFind(DOMFilePath::separator);
    if (!index)
        return DOMFilePath::root;
    if (index != -1)
        return path.substring(0, index);
    return ".";
}

bool DOMFilePath::isParentOf(const String& parent, const String& mayBeChild)
{
    ASSERT(DOMFilePath::isAbsolute(parent));
    ASSERT(DOMFilePath::isAbsolute(mayBeChild));
    if (parent == DOMFilePath::root && mayBeChild != DOMFilePath::root)
        return true;
    if (parent.length() >= mayBeChild.length() || !mayBeChild.startsWith(parent, false))
        return false;
    if (mayBeChild[parent.length()] != DOMFilePath::separator)
        return false;
    return true;
}

String DOMFilePath::removeExtraParentReferences(const String& path)
{
    ASSERT(DOMFilePath::isAbsolute(path));
    Vector<String> components;
    Vector<String> canonicalized;
    path.split(DOMFilePath::separator, components);
    for (size_t i = 0; i < components.size(); ++i) {
        if (components[i] == ".")
            continue;
        if (components[i] == "..") {
            if (canonicalized.size() > 0)
                canonicalized.removeLast();
            continue;
        }
        canonicalized.append(components[i]);
    }
    if (canonicalized.isEmpty())
        return DOMFilePath::root;
    String result;
    for (size_t i = 0; i < canonicalized.size(); ++i) {
        result.append(DOMFilePath::separator);
        result.append(canonicalized[i]);
    }
    return result;
}

bool DOMFilePath::isValidPath(const String& path)
{
    if (path.isEmpty() || path == DOMFilePath::root)
        return true;

    // Embedded NULs are not allowed.
    if (path.find(static_cast<UChar>(0)) != WTF::notFound)
        return false;

    // While not [yet] restricted by the spec, '\\' complicates implementation for Chromium.
    if (path.find('\\') != WTF::notFound)
        return false;

    // This method is only called on fully-evaluated absolute paths. Any sign of ".." or "." is likely an attempt to break out of the sandbox.
    Vector<String> components;
    path.split(DOMFilePath::separator, components);
    for (size_t i = 0; i < components.size(); ++i) {
        if (components[i] == ".")
            return false;
        if (components[i] == "..")
            return false;
    }
    return true;
}

bool DOMFilePath::isValidName(const String& name)
{
    if (name.isEmpty())
        return true;
    // '/' is not allowed in name.
    if (name.contains('/'))
        return false;
    return isValidPath(name);
}

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)
