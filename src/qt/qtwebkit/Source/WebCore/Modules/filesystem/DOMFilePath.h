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

#ifndef DOMFilePath_h
#define DOMFilePath_h

#if ENABLE(FILE_SYSTEM)

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

// DOMFileSystem path utilities. All methods in this class are static.
class DOMFilePath {
public:
    static const char separator;
    static const char root[];

    // Returns the name part from the given path.
    static String getName(const String& path);

    // Returns the parent directory path of the given path.
    static String getDirectory(const String& path);

    // Checks if a given path is a parent of mayBeChild. This method assumes given paths are absolute and do not have extra references to a parent (i.e. "../").
    static bool isParentOf(const String& path, const String& mayBeChild);

    // Appends the separator at the end of the path if it's not there already.
    static String ensureDirectoryPath(const String& path);

    // Returns a new path by appending a separator and the supplied path component to the path.
    static String append(const String& path, const String& component);

    static bool isAbsolute(const String& path)
    {
        return path.startsWith(DOMFilePath::root);
    }

    static bool endsWithSeparator(const String& path)
    {
        return path[path.length() - 1] == DOMFilePath::separator;
    }

    // Evaluates all "../" and "./" segments. Note that "/../" expands to "/", so you can't ever refer to anything above the root directory.
    static String removeExtraParentReferences(const String& path);

    // Checks if the given path follows the FileSystem API naming restrictions.
    static bool isValidPath(const String& path);

    // Checks if the given name follows the FileSystem API naming restrictions.
    static bool isValidName(const String& name);

private:
    DOMFilePath() { }
};

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)

#endif // DOMFilePath_h
