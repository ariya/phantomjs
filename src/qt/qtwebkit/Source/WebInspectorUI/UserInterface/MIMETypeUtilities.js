/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.fileExtensionForURL = function(url)
{
    var lastPathComponent = parseURL(url).lastPathComponent;
    if (!lastPathComponent)
        return "";

    var index = lastPathComponent.indexOf(".");
    if (index === -1)
        return "";

    return lastPathComponent.substr(index + 1);
};

WebInspector.mimeTypeForFileExtension = function(extension)
{
    const extensionToMIMEType = {
        // Document types.
        "html": "text/html",
        "xhtml": "application/xhtml+xml",
        "xml": "text/xml",

        // Script types.
        "js": "text/javascript",
        "json": "application/json",
        "clj": "text/x-clojure",
        "coffee": "text/x-coffeescript",
        "ls": "text/x-livescript",
        "ts": "text/typescript",

        // Stylesheet types.
        "css": "text/css",
        "less": "text/x-less",
        "sass": "text/x-sass",
        "scss": "text/x-scss",

        // Image types.
        "bmp": "image/bmp",
        "gif": "image/gif",
        "jpeg": "image/jpeg",
        "jpg": "image/jpeg",
        "pdf": "application/pdf",
        "png": "image/png",
        "tif": "image/tiff",
        "tiff": "image/tiff",

        // Font types and Media types are ignored for now.

        // Miscellaneous types.
        "svg": "image/svg+xml",
        "txt": "text/plain",
        "xsl": "text/xsl"
    };

    return extensionToMIMEType[extension] || null;
};
