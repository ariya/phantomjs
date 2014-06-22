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

WebInspector.IssueMessage = function(source, level, text, url, lineNumber, parameters)
{
    WebInspector.Object.call(this);

    this._level = level;
    this._text = text;

    // FIXME <http://webkit.org/b/76404>: Remove the string equality checks for undefined
    // once we don't get that value anymore from WebCore.

    // FIXME: If the URL is undefined, get the URL from the stacktrace.
    if (url && url !== "undefined")
        this._url = url;

    if (typeof lineNumber === "number" && lineNumber >= 0)
        this._lineNumber = lineNumber;

    if (parameters && parameters !== "undefined") {
        this._parameters = [];
        for (var i = 0; i < parameters.length; ++i) {
            if (parameters[i] instanceof WebInspector.RemoteObject) {
                this._parameters.push(parameters[i]);
                continue;
            }

            if (typeof parameters[i] === "object")
                this._parameters.push(WebInspector.RemoteObject.fromPayload(parameters[i]));
            else
                this._parameters.push(WebInspector.RemoteObject.fromPrimitiveValue(parameters[i]));
        }
    }

    this._formatTextIfNecessary();

    switch (source) {
    case "javascript":
        // FIXME: It would be nice if we had this information (the specific type of JavaScript error)
        // as part of the data passed from WebCore, instead of having to determine it ourselves.
        var prefixRegex = /^([^:]+): (?:DOM Exception \d+: )?/;
        var match = prefixRegex.exec(this._text);
        if (match && match[1] in WebInspector.IssueMessage.Type._prefixTypeMap) {
            this._type = WebInspector.IssueMessage.Type._prefixTypeMap[match[1]];
            this._text = this._text.substring(match[0].length);
        } else
            this._type = WebInspector.IssueMessage.Type.OtherIssue;
        break;

    case "html":
    case "css":
    case "wml":
    case "xml":
        this._type = WebInspector.IssueMessage.Type.PageIssue;
        break;

    case "network":
        this._type = WebInspector.IssueMessage.Type.NetworkIssue;
        break;

    case "console-api":
    case "other":
        this._type = WebInspector.IssueMessage.Type.OtherIssue;
        break;

    default:
        console.error("Unknown issue source:", source);
        this._type = WebInspector.IssueMessage.Type.OtherIssue;
    }
}

WebInspector.IssueMessage.Level = {
    Error: "error",
    Warning: "warning"
};

WebInspector.IssueMessage.Type = {
    SemanticIssue: "issue-message-type-semantic-issue",
    RangeIssue: "issue-message-type-range-issue",
    ReferenceIssue: "issue-message-type-reference-issue",
    TypeIssue: "issue-message-type-type-issue",
    PageIssue: "issue-message-type-page-issue",
    NetworkIssue: "issue-message-type-network-issue",
    SecurityIssue: "issue-message-type-security-issue",
    OtherIssue: "issue-message-type-other-issue"
};

WebInspector.IssueMessage.Type._prefixTypeMap = {
    "SyntaxError": WebInspector.IssueMessage.Type.SemanticIssue,
    "URIError": WebInspector.IssueMessage.Type.SemanticIssue,
    "EvalError": WebInspector.IssueMessage.Type.SemanticIssue,
    "INVALID_CHARACTER_ERR": WebInspector.IssueMessage.Type.SemanticIssue,
    "SYNTAX_ERR": WebInspector.IssueMessage.Type.SemanticIssue,

    "RangeError": WebInspector.IssueMessage.Type.RangeIssue,
    "INDEX_SIZE_ERR": WebInspector.IssueMessage.Type.RangeIssue,
    "DOMSTRING_SIZE_ERR": WebInspector.IssueMessage.Type.RangeIssue,

    "ReferenceError": WebInspector.IssueMessage.Type.ReferenceIssue,
    "HIERARCHY_REQUEST_ERR": WebInspector.IssueMessage.Type.ReferenceIssue,
    "INVALID_STATE_ERR": WebInspector.IssueMessage.Type.ReferenceIssue,
    "NOT_FOUND_ERR": WebInspector.IssueMessage.Type.ReferenceIssue,
    "WRONG_DOCUMENT_ERR": WebInspector.IssueMessage.Type.ReferenceIssue,

    "TypeError": WebInspector.IssueMessage.Type.TypeIssue,
    "INVALID_NODE_TYPE_ERR": WebInspector.IssueMessage.Type.TypeIssue,
    "TYPE_MISMATCH_ERR": WebInspector.IssueMessage.Type.TypeIssue,

    "SECURITY_ERR": WebInspector.IssueMessage.Type.SecurityIssue,

    "NETWORK_ERR": WebInspector.IssueMessage.Type.NetworkIssue,

    "ABORT_ERR": WebInspector.IssueMessage.Type.OtherIssue,
    "DATA_CLONE_ERR": WebInspector.IssueMessage.Type.OtherIssue,
    "INUSE_ATTRIBUTE_ERR": WebInspector.IssueMessage.Type.OtherIssue,
    "INVALID_ACCESS_ERR": WebInspector.IssueMessage.Type.OtherIssue,
    "INVALID_MODIFICATION_ERR": WebInspector.IssueMessage.Type.OtherIssue,
    "NAMESPACE_ERR": WebInspector.IssueMessage.Type.OtherIssue,
    "NOT_SUPPORTED_ERR": WebInspector.IssueMessage.Type.OtherIssue,
    "NO_DATA_ALLOWED_ERR": WebInspector.IssueMessage.Type.OtherIssue,
    "NO_MODIFICATION_ALLOWED_ERR": WebInspector.IssueMessage.Type.OtherIssue,
    "QUOTA_EXCEEDED_ERR": WebInspector.IssueMessage.Type.OtherIssue,
    "TIMEOUT_ERR": WebInspector.IssueMessage.Type.OtherIssue,
    "URL_MISMATCH_ERR": WebInspector.IssueMessage.Type.OtherIssue,
    "VALIDATION_ERR": WebInspector.IssueMessage.Type.OtherIssue
};

WebInspector.IssueMessage.Type.displayName = function(type)
{
    switch(type) {
    case WebInspector.IssueMessage.Type.SemanticIssue:
        return WebInspector.UIString("Semantic Issue");
    case WebInspector.IssueMessage.Type.RangeIssue:
        return WebInspector.UIString("Range Issue");
    case WebInspector.IssueMessage.Type.ReferenceIssue:
        return WebInspector.UIString("Reference Issue");
    case WebInspector.IssueMessage.Type.TypeIssue:
        return WebInspector.UIString("Type Issue");
    case WebInspector.IssueMessage.Type.PageIssue:
        return WebInspector.UIString("Page Issue");
    case WebInspector.IssueMessage.Type.NetworkIssue:
        return WebInspector.UIString("Network Issue");
    case WebInspector.IssueMessage.Type.SecurityIssue:
        return WebInspector.UIString("Security Issue");
    case WebInspector.IssueMessage.Type.OtherIssue:
        return WebInspector.UIString("Other Issue");
    default:
        console.error("Unknown issue message type:", type);
        return WebInspector.UIString("Other Issue");
    }
};

WebInspector.IssueMessage.prototype = {
    constructor: WebInspector.IssueMessage,

    get type()
    {
        return this._type;
    },

    get level()
    {
        return this._level;
    },

    get text()
    {
        return this._text;
    },

    get url()
    {
        return this._url;
    },

    get lineNumber()
    {
        return this._lineNumber;
    },

    // Private

    _formatTextIfNecessary: function()
    {
        if (!this._parameters)
            return;
        
        if (WebInspector.RemoteObject.type(this._parameters[0]) !== "string")
            return;

        function valueFormatter(obj)
        {
            return obj.description;
        }
        
        var formatters = {};
        formatters.o = valueFormatter;
        formatters.s = valueFormatter;
        formatters.f = valueFormatter;
        formatters.i = valueFormatter;
        formatters.d = valueFormatter;
        
        function append(a, b)
        {
            a += b;
            return a;
        }

        var result = String.format(this._parameters[0].description, this._parameters.slice(1), formatters, "", append);
        var resultText = result.formattedResult;
        
        for (var i = 0; i < result.unusedSubstitutions.length; ++i)
            resultText += " " + result.unusedSubstitutions[i].description;
        
        this._text = resultText;
    }
};

WebInspector.IssueMessage.prototype.__proto__ = WebInspector.Object.prototype;
