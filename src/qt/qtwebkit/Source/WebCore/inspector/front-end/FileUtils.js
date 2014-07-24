/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

/**
 * @interface
 */
WebInspector.OutputStreamDelegate = function()
{
}

WebInspector.OutputStreamDelegate.prototype = {
    onTransferStarted: function() { },

    onTransferFinished: function() { },

    /**
     * @param {WebInspector.ChunkedReader} reader
     */
    onChunkTransferred: function(reader) { },

    /**
     * @param {WebInspector.ChunkedReader} reader
     */
    onError: function(reader, event) { },
}

/**
 * @interface
 */
WebInspector.OutputStream = function()
{
}

WebInspector.OutputStream.prototype = {
    /**
     * @param {string} data
     * @param {function(WebInspector.OutputStream)=} callback
     */
    write: function(data, callback) { },

    close: function() { }
}

/**
 * @interface
 */
WebInspector.ChunkedReader = function()
{
}

WebInspector.ChunkedReader.prototype = {
    /**
     * @return {number}
     */
    fileSize: function() { },

    /**
     * @return {number}
     */
    loadedSize: function() { },

    /**
     * @return {string}
     */
    fileName: function() { },

    cancel: function() { }
}

/**
 * @constructor
 * @implements {WebInspector.ChunkedReader}
 * @param {!File} file
 * @param {number} chunkSize
 * @param {!WebInspector.OutputStreamDelegate} delegate
 */
WebInspector.ChunkedFileReader = function(file, chunkSize, delegate)
{
    this._file = file;
    this._fileSize = file.size;
    this._loadedSize = 0;
    this._chunkSize = chunkSize;
    this._delegate = delegate;
    this._isCanceled = false;
}

WebInspector.ChunkedFileReader.prototype = {
    /**
     * @param {!WebInspector.OutputStream} output
     */
    start: function(output)
    {
        this._output = output;

        this._reader = new FileReader();
        this._reader.onload = this._onChunkLoaded.bind(this);
        this._reader.onerror = this._delegate.onError.bind(this._delegate, this);
        this._delegate.onTransferStarted();
        this._loadChunk();
    },

    cancel: function()
    {
        this._isCanceled = true;
    },

    /**
     * @return {number}
     */
    loadedSize: function()
    {
        return this._loadedSize;
    },

    /**
     * @return {number}
     */
    fileSize: function()
    {
        return this._fileSize;
    },

    /**
     * @return {string}
     */
    fileName: function()
    {
        return this._file.name;
    },

    /**
     * @param {Event} event
     */
    _onChunkLoaded: function(event)
    {
        if (this._isCanceled)
            return;

        if (event.target.readyState !== FileReader.DONE)
            return;

        var data = event.target.result;
        this._loadedSize += data.length;

        this._output.write(data);
        if (this._isCanceled)
            return;
        this._delegate.onChunkTransferred(this);

        if (this._loadedSize === this._fileSize) {
            this._file = null;
            this._reader = null;
            this._output.close();
            this._delegate.onTransferFinished();
            return;
        }

        this._loadChunk();
    },

    _loadChunk: function()
    {
        var chunkStart = this._loadedSize;
        var chunkEnd = Math.min(this._fileSize, chunkStart + this._chunkSize)
        var nextPart = this._file.slice(chunkStart, chunkEnd);
        this._reader.readAsText(nextPart);
    }
}

/**
 * @constructor
 * @implements {WebInspector.ChunkedReader}
 * @param {string} url
 * @param {!WebInspector.OutputStreamDelegate} delegate
 */
WebInspector.ChunkedXHRReader = function(url, delegate)
{
    this._url = url;
    this._delegate = delegate;
    this._fileSize = 0;
    this._loadedSize = 0;
    this._isCanceled = false;
}

WebInspector.ChunkedXHRReader.prototype = {
    /**
     * @param {!WebInspector.OutputStream} output
     */
    start: function(output)
    {
        this._output = output;

        this._xhr = new XMLHttpRequest();
        this._xhr.open("GET", this._url, true);
        this._xhr.onload = this._onLoad.bind(this);
        this._xhr.onprogress = this._onProgress.bind(this);
        this._xhr.onerror = this._delegate.onError.bind(this._delegate, this);
        this._xhr.send(null);

        this._delegate.onTransferStarted();
    },

    cancel: function()
    {
        this._isCanceled = true;
        this._xhr.abort();
    },

    /**
     * @return {number}
     */
    loadedSize: function()
    {
        return this._loadedSize;
    },

    /**
     * @return {number}
     */
    fileSize: function()
    {
        return this._fileSize;
    },

    /**
     * @return {string}
     */
    fileName: function()
    {
        return this._url;
    },

    /**
     * @param {Event} event
     */
    _onProgress: function(event)
    {
        if (this._isCanceled)
            return;

        if (event.lengthComputable)
            this._fileSize = event.total;

        var data = this._xhr.responseText.substring(this._loadedSize);
        if (!data.length)
            return;

        this._loadedSize += data.length;
        this._output.write(data);
        if (this._isCanceled)
            return;
        this._delegate.onChunkTransferred(this);
    },

    /**
     * @param {Event} event
     */
    _onLoad: function(event)
    {
        this._onProgress(event);

        if (this._isCanceled)
            return;

        this._output.close();
        this._delegate.onTransferFinished();
    }
}

/**
 * @param {function(!File)} callback
 * @return {Node}
 */
WebInspector.createFileSelectorElement = function(callback) {
    var fileSelectorElement = document.createElement("input");
    fileSelectorElement.type = "file";
    fileSelectorElement.setAttribute("tabindex", -1);
    fileSelectorElement.style.zIndex = -1;
    fileSelectorElement.style.position = "absolute";
    fileSelectorElement.onchange = function(event) {
        callback(fileSelectorElement.files[0]);
    };
    return fileSelectorElement;
}

/**
 * @param {string} source
 * @param {number=} startIndex
 * @param {number=} lastIndex
 */
WebInspector.findBalancedCurlyBrackets = function(source, startIndex, lastIndex) {
    lastIndex = lastIndex || source.length;
    startIndex = startIndex || 0;
    var counter = 0;
    var inString = false;

    for (var index = startIndex; index < lastIndex; ++index) {
        var character = source[index];
        if (inString) {
            if (character === "\\")
                ++index;
            else if (character === "\"")
                inString = false;
        } else {
            if (character === "\"")
                inString = true;
            else if (character === "{")
                ++counter;
            else if (character === "}") {
                if (--counter === 0)
                    return index + 1;
            }
        }
    }
    return -1;
}

/**
 * @constructor
 * @implements {WebInspector.OutputStream}
 */
WebInspector.FileOutputStream = function()
{
}

WebInspector.FileOutputStream.prototype = {
    /**
     * @param {string} fileName
     * @param {function(WebInspector.FileOutputStream, string=)} callback
     */
    open: function(fileName, callback)
    {
        this._closed = false;
        this._writeCallbacks = [];
        this._fileName = fileName;
        function callbackWrapper()
        {
            WebInspector.fileManager.removeEventListener(WebInspector.FileManager.EventTypes.SavedURL, callbackWrapper, this);
            WebInspector.fileManager.addEventListener(WebInspector.FileManager.EventTypes.AppendedToURL, this._onAppendDone, this);
            callback(this);
        }
        WebInspector.fileManager.addEventListener(WebInspector.FileManager.EventTypes.SavedURL, callbackWrapper, this);
        WebInspector.fileManager.save(this._fileName, "", true);
    },

    /**
     * @param {string} data
     * @param {function(WebInspector.OutputStream)=} callback
     */
    write: function(data, callback)
    {
        this._writeCallbacks.push(callback);
        WebInspector.fileManager.append(this._fileName, data);
    },

    close: function()
    {
        this._closed = true;
        if (this._writeCallbacks.length)
            return;
        WebInspector.fileManager.removeEventListener(WebInspector.FileManager.EventTypes.AppendedToURL, this._onAppendDone, this);
        WebInspector.fileManager.close(this._fileName);
    },

    /**
     * @param {Event} event
     */
    _onAppendDone: function(event)
    {
        if (event.data !== this._fileName)
            return;
        if (!this._writeCallbacks.length) {
            if (this._closed) {
                WebInspector.fileManager.removeEventListener(WebInspector.FileManager.EventTypes.AppendedToURL, this._onAppendDone, this);
                WebInspector.fileManager.close(this._fileName);
            }
            return;
        }
        var callback = this._writeCallbacks.shift();
        if (callback)
            callback(this);
    }
}
