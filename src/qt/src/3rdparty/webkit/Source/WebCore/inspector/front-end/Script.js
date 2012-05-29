/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.Script = function(sourceID, sourceURL, startLine, startColumn, endLine, endColumn, errorLine, errorMessage, isContentScript)
{
    this.sourceID = sourceID;
    this.sourceURL = sourceURL;
    this.lineOffset = startLine;
    this.columnOffset = startColumn;
    this.endLine = endLine;
    this.endColumn = endColumn;
    this.errorLine = errorLine;
    this.errorMessage = errorMessage;
    this.isContentScript = isContentScript;
}

WebInspector.Script.prototype = {
    requestSource: function(callback)
    {
        if (this._source) {
            callback(this._source);
            return;
        }

        function didGetScriptSource(error, source)
        {
            this._source = source;
            callback(this._source);
        }
        DebuggerAgent.getScriptSource(this.sourceID, didGetScriptSource.bind(this));
    },

    editSource: function(newSource, callback)
    {
        function didEditScriptSource(error, callFrames)
        {
            if (!error)
                this._source = newSource;
            callback(error, callFrames);
        }
        DebuggerAgent.editScriptSource(this.sourceID, newSource, didEditScriptSource.bind(this));
    }
}
